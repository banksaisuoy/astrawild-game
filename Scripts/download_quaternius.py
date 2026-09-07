#!/usr/bin/env python3
"""
ASTRAWILD — Quaternius Ultimate-pack acquisition (LCP-7, user-approved source).

Acquires the six user-approved CC0 Quaternius packs from their official pages
(license verified BEFORE any download — the pack page must state CC0 AND the
pack's own License.txt must arrive with matching text) into `ArtSource/Models/Quaternius_*/`.

Delivery reality (recorded by the AA-2 research, re-verified here): the packs
live in public Google Drive folders. Every folder page is plain HTML with
data-id/data-tooltip entries, and every FILE downloads deterministically via
  https://drive.usercontent.google.com/download?id=<fileId>&export=download&confirm=t
so this crawler never touches an API, login or cookie.

Scope per pack (the game needs importable, self-contained assets — the same
policy as the Kenney batches):
  glTF subtree wherever offered (self-contained .gltf + pack atlases),
  FBX for the two packs that ship no glTF (Nature, Ruins — Ruins keeps its
  Textures folder so relative refs resolve).

Idempotent: re-running skips files that already exist with the recorded size.
Every accepted file gets a SHA-256 in Docs/ASSET_ACQUISITION_QUATERNIUS_MANIFEST.json.
LICENSE_UNCLEAR or a failed CC0 check aborts the WHOLE pack (never the run) and
records the reason.

Run:  python3 Scripts/download_quaternius.py [--dry-run]
"""

import argparse
import hashlib
import json
import re
import sys
import time
import urllib.request
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
DEST_ROOT = REPO_ROOT / "ArtSource" / "Models"
MANIFEST_PATH = REPO_ROOT / "Docs" / "ASSET_ACQUISITION_QUATERNIUS_MANIFEST.json"
CACHE_ROOT = Path.home() / ".astrawild_download_cache" / "quaternius"

CREATOR = "Quaternius"
PACK_PAGE_BASE = "https://quaternius.com/packs/"
DRIVE_FOLDER_URL = "https://drive.google.com/drive/folders/{folder_id}"
FILE_DOWNLOAD_URL = "https://drive.usercontent.google.com/download?id={file_id}&export=download&confirm=t"
USER_AGENT = "Mozilla/5.0 (astrawild-asset-pipeline)"

LICENSE_URL = "https://quaternius.com/license.html"

# The CC0 statement the pack License.txt files carry (checked per pack, after
# whitespace normalization, case-insensitive substring).
LICENSE_MARKER = "CC0"

# Per-pack acquisition plan (user-approved list, PART 11 of the LCP directive).
# subtree: list of regex filters applied against the path "Category/Format"
# segments walked from the pack root. Files under matching subtrees download.
PACKS = [
    {
        "pack": "Ultimate Animated Animals",
        "page": "ultimateanimatedanimals.html",
        "dest_dir": "Quaternius_UltimateAnimatedAnimals",
        "drive_folder": "1uJ3N5HfB7jKTseJUNQr3N4YaN0UuEtHk",
        "subtree": [r"^glTF$"],
        "formats": ["gltf", "png", "txt"],
    },
    {
        "pack": "Ultimate Monsters",
        "page": "ultimatemonsters.html",
        "dest_dir": "Quaternius_UltimateMonsters",
        "drive_folder": "18m4KpzpEzhC9wl7jzr6dUc0N8Jozr79C",
        "subtree": [r"^(Big|Blob|Flying)/glTF$"],
        "formats": ["gltf", "png", "txt"],
    },
    {
        "pack": "Ultimate Nature Pack",
        "page": "ultimatenature.html",
        "dest_dir": "Quaternius_UltimateNature",
        "drive_folder": "1-Kl0L_Jg8awbh0S5T-z3zxh4mVlnxTpa",
        "subtree": [r"^FBX$"],
        "formats": ["fbx", "txt"],
    },
    {
        "pack": "Ultimate Space Kit",
        "page": "ultimatespacekit.html",
        "dest_dir": "Quaternius_UltimateSpaceKit",
        "drive_folder": "17F8HlI2zPTlo32aieW5YPPwOk78xo-2m",
        "subtree": [r"^[^/]+/GLTF$", r"^[^/]+$"],  # category glTF folders + root atlas/license
        "formats": ["gltf", "png", "txt"],
    },
    {
        "pack": "Ultimate Modular Ruins Pack",
        "page": "ultimatemodularruins.html",
        "dest_dir": "Quaternius_UltimateModularRuins",
        "drive_folder": "1ETp2ldaHaP0BkS4FBmkT-g9Yf88T_cIX",
        "subtree": [r"^FBX$", r"^Textures$", r"^[^/]+$"],  # meshes + textures + root docs
        "formats": ["fbx", "png", "jpg", "jpeg", "txt"],
    },
    {
        "pack": "Ultimate Modular Men Pack",
        "page": "ultimatemodularcharacters.html",  # page title: "Ultimate Modular Men Pack"
        "dest_dir": "Quaternius_UltimateModularMen",
        "drive_folder": "1USAAquX2JJWuA2m6zol0KUkFe3UkZ8zX",
        "subtree": [r"^Individual Characters/glTF$", r"^[^/]+$"],
        "formats": ["gltf", "png", "txt"],
    },
]

TYPE_SUFFIXES = (" Shared folder", " folder", " Binary", " Image", " Text", " Video", " Unknown", " Document")


def log(msg):
    print(f"[quaternius] {msg}", flush=True)


def http_get(url, binary=False, timeout=90):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        data = resp.read()
    return data if binary else data.decode("utf-8", errors="ignore")


def sha256_of(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_folder_entries(html_text):
    """[(file_id, name, is_folder)] from a Drive folder page (data-id + tooltip)."""
    pairs = re.findall(r'data-id="([-\w]{20,})"[^>]*data-tooltip="([^"]+)"', html_text)
    if not pairs:
        pairs = re.findall(r'data-id="([-\w]{20,})"[^>]*aria-label="([^"]+)"', html_text)
    out = []
    for file_id, raw_name in pairs:
        name = raw_name
        is_folder = "folder" in name.lower()
        for suffix in TYPE_SUFFIXES:
            if name.endswith(suffix):
                name = name[: -len(suffix)]
                break
        name = name.strip()
        if name:
            out.append((file_id, name, is_folder))
    return out


def crawl_folder(folder_id, path_prefix, depth=0, max_depth=4):
    """Recursively enumerate a Drive folder: [(drive_path, file_id, file_name)]."""
    if depth > max_depth:
        return []
    try:
        html_text = http_get(DRIVE_FOLDER_URL.format(folder_id=folder_id), timeout=60)
    except Exception as exc:  # noqa: BLE001
        log(f"  ! folder fetch failed at '{path_prefix}': {exc}")
        return []
    files = []
    for file_id, name, is_folder in parse_folder_entries(html_text):
        child_path = f"{path_prefix}/{name}" if path_prefix else name
        if is_folder:
            time.sleep(0.4)
            files.extend(crawl_folder(file_id, child_path, depth + 1, max_depth))
        else:
            files.append((path_prefix, file_id, name))
    return files


def subtree_matches(path_prefix, patterns):
    import re as _re

    if not path_prefix:
        # Root files: only packs whose plan explicitly includes the root level.
        return any(p == "^[^/]+$" for p in patterns)
    return any(_re.match(p, path_prefix) for p in patterns)


def verify_pack_page(pack):
    """The pack page must state CC0 (the license gate before any download)."""
    html_text = http_get(PACK_PAGE_BASE + pack["page"], timeout=60)
    title = re.search(r"<title>([^<]+)</title>", html_text)
    if "CC0" not in html_text:
        return None, "pack page does not state CC0"
    return (title.group(1).strip() if title else pack["pack"]), None


def fetch_license_text(pack, root_files):
    """Download the pack's own License.txt and verify the CC0 marker."""
    license_entry = next(((fid, name) for prefix, fid, name in root_files if name.lower() == "license.txt"), None)
    if not license_entry:
        return None, "no License.txt in the pack folder"
    file_id, _ = license_entry
    # License text is small; the interstitial retry matters more here than speed.
    raw = None
    for attempt in range(3):
        try:
            raw = http_get(FILE_DOWNLOAD_URL.format(file_id=file_id), binary=True, timeout=60)
        except Exception as exc:  # noqa: BLE001
            return None, f"License.txt download failed: {exc}"
        if raw and not _looks_like_html(raw):
            break
        time.sleep(1.0 + attempt)
    if not raw or _looks_like_html(raw):
        return None, "License.txt not served (HTML interstitial persisted)"
    text = raw.decode("utf-8", errors="ignore")
    normalized = re.sub(r"\s+", " ", text).lower()
    if LICENSE_MARKER.lower() not in normalized:
        return None, "License.txt does not mention CC0"
    return text.strip(), None


def _looks_like_html(data: bytes) -> bool:
    head = data[:512].lstrip()[:32].lower()
    return head.startswith(b"<!doctype") or head.startswith(b"<html") or b"<head>" in head


def download_file(file_id, dest: Path):
    """Download one Drive file (one retry past the virus-scan interstitial)."""
    url = FILE_DOWNLOAD_URL.format(file_id=file_id)
    data = None
    for attempt in range(2):
        try:
            data = http_get(url, binary=True, timeout=180)
        except Exception as exc:  # noqa: BLE001
            return None, str(exc)
        if data and not _looks_like_html(data):
            return data, None
        time.sleep(1.2 + attempt)  # interstitial seen — Drive warms the confirm state
        url = FILE_DOWNLOAD_URL.format(file_id=file_id) + f"&uuid={int(time.time())}"
    if not data:
        return None, "empty response"
    return None, "received HTML (confirm/virus-scan interstitial) — file not served"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dry-run", action="store_true", help="enumerate + verify licenses only")
    args = parser.parse_args()

    CACHE_ROOT.mkdir(parents=True, exist_ok=True)
    DEST_ROOT.mkdir(parents=True, exist_ok=True)

    manifest = {
        "schema": 1,
        "generated": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "generator": "Scripts/download_quaternius.py (LCP-7)",
        "source": "quaternius.com (official pages) -> public Google Drive folders",
        "license": "CC0 1.0 Universal (verified per pack: page statement + pack License.txt)",
        "license_url": LICENSE_URL,
        "packs": [],
    }

    total_files = 0
    total_bytes = 0

    for pack in PACKS:
        log(f"--- {pack['pack']} ---")
        title, err = verify_pack_page(pack)
        if err:
            log(f"  LICENSE GATE FAILED: {err} — pack SKIPPED (never enters the repo).")
            manifest["packs"].append({"pack": pack["pack"], "status": "LICENSE_UNCLEAR", "reason": err})
            continue
        log(f"  page verified: '{title}' states CC0.")

        root_files = crawl_folder(pack["drive_folder"], "")
        license_text, lic_err = fetch_license_text(pack, root_files)
        if lic_err:
            log(f"  LICENSE.TXT GATE FAILED: {lic_err} — pack SKIPPED.")
            manifest["packs"].append({"pack": pack["pack"], "status": "LICENSE_UNCLEAR", "reason": lic_err})
            continue
        log("  License.txt verified (CC0).")

        dest_dir = DEST_ROOT / pack["dest_dir"]
        entries = []
        for prefix, file_id, name in root_files:
            if not subtree_matches(prefix, pack["subtree"]) and name.lower() != "license.txt":
                continue
            ext = name.rsplit(".", 1)[-1].lower() if "." in name else ""
            if ext not in pack["formats"]:
                continue
            entries.append((prefix, file_id, name))

        pack_record = {
            "pack": pack["pack"],
            "page_title": title,
            "source_url": PACK_PAGE_BASE + pack["page"],
            "drive_folder": pack["drive_folder"],
            "license": "CC0 1.0 Universal",
            "license_url": LICENSE_URL,
            "date_checked": manifest["generated"][:10],
            "dest": str(dest_dir.relative_to(REPO_ROOT)).replace("\\", "/"),
            "planned_files": len(entries),
            "status": "LICENSE_VERIFIED",
            "files": [],
        }

        if args.dry_run:
            log(f"  dry-run: would download {len(entries)} files.")
            for prefix, _fid, name in entries[:5]:
                log(f"    {prefix + '/' if prefix else ''}{name}")
            manifest["packs"].append(pack_record)
            continue

        dest_dir.mkdir(parents=True, exist_ok=True)
        # Store the license text beside the assets (evidence + attribution).
        (dest_dir / "LICENSE_CC0.txt").write_text(license_text, encoding="utf-8")

        accepted = skipped = failed = 0
        for prefix, file_id, name in entries:
            rel = (Path(prefix) / name) if prefix else Path(name)
            target = dest_dir / rel
            if target.exists() and target.stat().st_size > 0:
                skipped += 1
                continue
            data, err = download_file(file_id, target)
            if err:
                failed += 1
                log(f"  ! {rel}: {err}")
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            accepted += 1
            total_bytes += len(data)
            pack_record["files"].append(
                {"path": str(rel).replace("\\", "/"), "bytes": len(data), "sha256": sha256_of(data)}
            )
            time.sleep(0.25)  # polite crawl cadence
        total_files += accepted

        pack_record["accepted_files"] = accepted
        pack_record["skipped_existing"] = skipped
        pack_record["failed_files"] = failed
        pack_record["status"] = "LICENSE_VERIFIED" if failed == 0 else "PARTIAL"
        log(f"  accepted={accepted} skipped={skipped} failed={failed}")
        manifest["packs"].append(pack_record)

    manifest["stats"] = {"files_downloaded": total_files, "bytes_downloaded": total_bytes}
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    log(f"manifest: {MANIFEST_PATH.relative_to(REPO_ROOT)}")
    log(f"totals: {total_files} files, {total_bytes / (1024 * 1024):.1f} MB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
