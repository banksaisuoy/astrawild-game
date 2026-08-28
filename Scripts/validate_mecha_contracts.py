"""Validate the original ASTRAWILD exosuit integration contracts."""
from __future__ import annotations

import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"
FORBIDDEN_TERMS = ("Gundam", "Wing Zero", "TEK Mecha", "Buster Rifle", "Funnel Bits")


def rows(name: str) -> list[dict[str, str]]:
    with (SOURCE / name).open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def list_value(value: str) -> list[str]:
    value = (value or "").strip()
    if value.startswith("(") and value.endswith(")"):
        value = value[1:-1]
    return [item.strip() for item in value.split(",") if item.strip()]


def main() -> int:
    errors: list[str] = []
    frames = rows("DT_MechaFrames.csv")
    weapons = rows("DT_MechaWeapons.csv")
    evolutions = rows("DT_CyberneticEvolutions.csv")
    animations = rows("DT_MechaAnimationProfiles.csv")
    vfx = rows("DT_MechaVFX.csv")
    legacy_echoes = {row.get("SpeciesTag", "") for row in rows("DT_EchoDex.csv")}
    master_echoes = {row.get("SpeciesTag", "") for row in rows("DT_EchoDex_200.csv")}
    all_echoes = legacy_echoes | master_echoes
    mecha_data_header = (ROOT / "Source/AstrawildCore/Public/Data/AstrawildMechaData.h").read_text(encoding="utf-8", errors="replace")
    mecha_cpp = (ROOT / "Source/AstrawildCore/Private/Components/AstrawildMechaComponent.cpp").read_text(encoding="utf-8", errors="replace")
    cockpit_cpp = (ROOT / "Source/AstrawildCore/Private/UI/AstrawildCockpitWidget.cpp").read_text(encoding="utf-8", errors="replace")
    for struct_name in ("FAstrawildMechaWeaponRow", "FAstrawildMechaFrameRow", "FAstrawildCyberneticEvolutionRow"):
        if f"struct ASTRAWILDCORE_API {struct_name}" not in mecha_data_header:
            errors.append(f"{struct_name} is missing ASTRAWILDCORE_API export")
    for required_token in ("TargetLocation.ContainsNaN()", "LineTraceSingleByChannel", "ApplyPointDamage", "GetLifetimeReplicatedProps", "DOREPLIFETIME", "ServerFireHardpointWeapon_Implementation", "ServerEquipMechaFrame_Implementation", "OnRepMechaState"):
        if required_token not in mecha_cpp:
            errors.append(f"mecha runtime/network contract is missing {required_token}")
    for required_token in ("ReplicatedUsing = OnRepMechaState", "UFUNCTION(Server, Reliable)", "FVector_NetQuantize"):
        if required_token not in mecha_data_header and required_token not in (ROOT / "Source/AstrawildCore/Public/Components/AstrawildMechaComponent.h").read_text(encoding="utf-8", errors="replace"):
            errors.append(f"mecha header network contract is missing {required_token}")
    if "IsTargetLockAllowed" not in cockpit_cpp or "LineTraceSingleByChannel" not in cockpit_cpp:
        errors.append("cockpit target lock contract is missing actor/LOS validation")

    if len(frames) != 5:
        errors.append(f"expected 5 mecha frame rows, found {len(frames)}")
    if len(weapons) < 14:
        errors.append(f"expected at least 14 mecha weapon rows after default expansion, found {len(weapons)}")
    for table_name, table_rows, key in (("frames", frames, "FrameTag"), ("weapons", weapons, "WeaponTag"), ("vfx", vfx, "EffectTag")):
        values = [row.get(key, "") for row in table_rows]
        if any(not value for value in values):
            errors.append(f"{table_name} contains an empty {key}")
        if len(values) != len(set(values)):
            errors.append(f"{table_name} contains duplicate {key} values")

    weapon_tags = {row.get("WeaponTag", "") for row in weapons}
    for frame in frames:
        for tag in list_value(frame.get("DefaultWeaponTags", "")):
            if tag not in weapon_tags:
                errors.append(f"{frame.get('FrameTag')} references missing weapon tag {tag}")
    animation_frames = {row.get("FrameTag", "") for row in animations}
    frame_tags = {row.get("FrameTag", "") for row in frames}
    if animation_frames != frame_tags:
        errors.append(f"animation profiles must cover exactly all frame tags; profiles={sorted(animation_frames)}, frames={sorted(frame_tags)}")
    for row in evolutions:
        if row.get("BaseEchoTag", "") not in all_echoes:
            errors.append(f"cyber evolution {row.get('Name')} references missing base Echo {row.get('BaseEchoTag')}")
        if not row.get("ResultingMechaEchoTag", "").startswith("Echo."):
            errors.append(f"cyber evolution {row.get('Name')} has invalid result tag")
    for row in vfx:
        path = row.get("NiagaraSystemPath", "")
        if not path.startswith("/Game/Astrawild/VFX/"):
            errors.append(f"VFX {row.get('EffectTag')} must use an ASTRAWILD /Game path")
        if row.get("bLooping") not in {"true", "false"} or row.get("bFallbackToEmitter") not in {"true", "false"}:
            errors.append(f"VFX {row.get('EffectTag')} has invalid boolean fields")

    searched = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for path in (
            SOURCE / "DT_MechaFrames.csv",
            SOURCE / "DT_MechaWeapons.csv",
            SOURCE / "DT_CyberneticEvolutions.csv",
            SOURCE / "DT_MechaAnimationProfiles.csv",
            SOURCE / "DT_MechaVFX.csv",
            ROOT / "Docs/P5_ASTRA_EXOSUIT_SYSTEM_SPEC.md",
        )
    )
    for term in FORBIDDEN_TERMS:
        if re.search(re.escape(term), searched, flags=re.IGNORECASE):
            errors.append(f"originality guard found forbidden external-IP term: {term}")

    if errors:
        print("ASTRAWILD mecha contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"ASTRAWILD mecha contract validation passed ({len(frames)} frames, {len(weapons)} weapons, {len(animations)} animation profiles, {len(vfx)} VFX bindings, replicated input/state bridge).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
