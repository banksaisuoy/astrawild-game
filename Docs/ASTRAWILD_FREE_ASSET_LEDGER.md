# ASTRAWILD — FREE ASSET LEDGER (LCP-7, binding record)

**Policy** (user directive PART 10): only assets whose LICENSE permits this
personal 4-player LAN game. Default preference **CC0**; secondary: licenses
that clearly permit personal/non-commercial use with satisfiable attribution.
"Free" on a website is NEVER sufficient — every row below carries a verified
license + source + date checked.

**Scope rule** (PART 12/13): assets are acquired only when they materially
improve the game, and must fit the SCI-FI SURVIVAL FRONTIER identity (no asset
collage). Kenney content is SUPPORT material (UI/props/VFX/industrial
decoration) — the visual identity stays the native procedural + Tier-A set.

**Column contract** (PART 19): asset · source · license · license URL ·
file path · hash (file-level SHA-256 in the JSON manifests referenced per row)
· usage · attribution · status.

**Statuses**: `LICENSE_VERIFIED` (acquired, import-ready),
`LICENSE_VERIFIED / DELIVERY_PENDING` (license proven, delivery not yet
scripted), `APPROVED-NOT-REQUIRED` (approved source, current coverage
sufficient), `REJECTED` (with reason). Import/binding decisions belong to the
Antigravity one-time integration (IMPORT_READY ≠ UE5_VERIFIED).

---

## 1. Kenney packs (AA-1 / AA-2 batches — 15 packs, 3,678 files, 75.8 MB)

Source site: `https://kenney.nl` (official publisher pages, direct zip URLs).
License: **CC0 1.0 Universal** on every pack page AND in every archive's
`License.txt` (dual verification at acquisition time). License URL:
`https://kenney.nl/assets` (per-pack page states CC0) +
`https://creativecommons.org/publicdomain/zero/1.0/`.
Attribution: **none required** (CC0; credit given voluntarily in
`ASSETS_CREDITS.md` + `Docs/ThirdPartyLicenses.md`).
File-level SHA-256 hashes: `Docs/ASSET_ACQUISITION_MANIFEST.json`
(packs_accepted → files; ~3,678 records). Acquisition dates: 2026 (AA-1),
2026 (AA-2). Downloader: `Scripts/download_assets.py` (approved-URL allowlist,
hash dedupe, format validators, idempotent).

| # | Asset (pack) | File path (repo) | Usage (integration target) | Files | Status |
| :-- | :--- | :--- | :--- | :-- | :--- |
| 1 | Impact Sounds | `ArtSource/Audio/Kenney_ImpactSounds/` | Weapon/impact feedback layer (combat audio) | 130 (602 audio total across 3 packs) | LICENSE_VERIFIED |
| 2 | Interface Sounds | `ArtSource/Audio/Kenney_InterfaceSounds/` | UI clicks/confirmations (PART 18 UI feedback) | — | LICENSE_VERIFIED |
| 3 | Sci-fi Sounds | `ArtSource/Audio/Kenney_SciFiSounds/` | Tech/research/weapon sci-fi layer | — | LICENSE_VERIFIED |
| 4 | Nature Kit | `ArtSource/Models/Kenney_NatureKit/GLB/` | Biome dressing/farm/village/ruins props (PART 12 ENVIRONMENT) | 314 | LICENSE_VERIFIED |
| 5 | Space Kit | `ArtSource/Models/Kenney_SpaceKit/GLB/` | Dungeon/ancient-tech dressing; turret candidates | 107 | LICENSE_VERIFIED |
| 6 | Blaster Kit | `ArtSource/Models/Kenney_BlasterKit/GLB/` | CANDIDATE_REPLACEMENT weapon pool (Decision 06: compare-first protocol) | 40 | LICENSE_VERIFIED |
| 7 | Particle Pack | `ArtSource/Textures/Kenney_ParticlePack/` | P0 combat VFX sprites (hit/elemental/projectile) | 96 | LICENSE_VERIFIED |
| 8 | UI Pack: Sci-Fi | `ArtSource/Textures/Kenney_UIPackSciFi/` | P0 UI art (panels/buttons/icons ×6 families) + Future/Narrow TTF | 690+2 | LICENSE_VERIFIED |
| 9 | Survival Kit | `ArtSource/Models/Kenney_SurvivalKit/GLB/` | Camps/fires/crates/tools across all 12 zones (PART 12) | 80 | LICENSE_VERIFIED |
| 10 | City Kit (Industrial) | `ArtSource/Models/Kenney_CityKitIndustrial/GLB/` | Ember Ridge/Stormcrest industrial + research props | 38 | LICENSE_VERIFIED |
| 11 | Modular Space Kit | `ArtSource/Models/Kenney_ModularSpaceKit/GLB/` | Modular sci-fi dungeon tiles (PART 12 DUNGEON) | 41 | LICENSE_VERIFIED |
| 12 | Modular Dungeon Kit | `ArtSource/Models/Kenney_ModularDungeonKit/GLB/` | Stone/ancient modular dungeon tiles | 40 | LICENSE_VERIFIED |
| 13 | Animated Characters: Survivors | `ArtSource/Models/Kenney_AnimatedCharactersSurvivors/` | 1 medium humanoid + idle/run/jump FBX (retarget reference) | 4 | LICENSE_VERIFIED |
| 14 | Skyboxes | `ArtSource/Textures/Kenney_Skyboxes/` | Equirect skybox candidates (day/morning/night/alien/space) | 5 | LICENSE_VERIFIED |
| 15 | Crosshair Pack | `ArtSource/Textures/Kenney_CrosshairPack/` | HUD reticle replacement (4 styles × 2 resolutions) | 1,600 | LICENSE_VERIFIED |

Rejections (documented, never acquired): Sci-Fi RTS (2D), Digital Audio, UI
Audio, RPG Audio, Kenney 2D creature/character family (no 3D creature catalog
exists at Kenney), Quaternius QAL-licensed newer packs (QAL forbids
redistribution). OGA: viable future source (per-asset CC0 + direct downloads
verified) — no current need. See `Docs/ASSET_ACQUISITION_REPORT.md`.

---

## 2. Quaternius Ultimate packs (LCP-7 batch — the six user-approved packs)

Source: `https://quaternius.com/packs/<pack>.html` (official pages; every page
states CC0 with the site license link `https://quaternius.com/license.html`).
Per-pack license verification is DOUBLE: the pack page statement AND the pack's
own `License.txt` (fetched from the pack's public Google Drive folder and
checked for the CC0 1.0 Universal dedication before ANY file downloads).
Attribution: **none required** (CC0; "LowPoly Models by @Quaternius" is
voluntarily preserved in each pack folder's `LICENSE_CC0.txt`).
Delivery: public Google Drive folders, deterministically crawled + downloaded
file-by-file (no login, no API, no cookies) by
`Scripts/download_quaternius.py` (license gates first; idempotent; SHA-256 per
file). File-level hashes: `Docs/ASSET_ACQUISITION_QUATERNIUS_MANIFEST.json`.
Date checked: 2026 (LCP-7 session).

| # | Asset (pack) | File path (repo) | Usage (integration target) | Files | Status |
| :-- | :--- | :--- | :--- | :-- | :-- |
| 16 | Ultimate Animated Animals | `ArtSource/Models/Quaternius_UltimateAnimatedAnimals/glTF/` | 12 animated creatures (skinned, 13 animations each) — Tier-B/C wild Echo body candidates + real locomotion animation source (PART 12 ECHO land creatures) | 12 glTF | LICENSE_VERIFIED |
| 17 | Ultimate Monsters | `ArtSource/Models/Quaternius_UltimateMonsters/{Big,Blob,Flying}/glTF/` | Big/Blob/Flying monster archetypes + atlas — hostile/dungeon Echo candidates (PART 12 ECHO monsters) | ~51 glTF+atlas | LICENSE_VERIFIED |
| 18 | Ultimate Nature Pack | `ArtSource/Models/Quaternius_UltimateNature/FBX/` | 50 low-poly nature models (trees/rocks/plants) — biome dressing variety for the 12 zones (PART 12 ENVIRONMENT) | 50 FBX | LICENSE_VERIFIED |
| 19 | Ultimate Space Kit | `ArtSource/Models/Quaternius_UltimateSpaceKit/{Characters,Environment,Items,Vehicles}/GLTF/` | Sci-fi environment/items/vehicles — dungeon + industrial dressing (PART 12 sci-fi structures) | ~76 glTF+atlas | LICENSE_VERIFIED |
| 20 | Ultimate Modular Ruins Pack | `ArtSource/Models/Quaternius_UltimateModularRuins/{FBX,Textures}/` | Modular ruins architecture — dungeon variety + POI landmark dressing (PART 12 DUNGEON/ruins) | 50 FBX + textures | LICENSE_VERIFIED |
| 21 | Ultimate Modular Men Pack | `ArtSource/Models/Quaternius_UltimateModularMen/Individual Characters/glTF/` | Modular humanoid characters — NPC body upgrade candidates (villager silhouettes; compare-first like weapons) | 12 glTF | LICENSE_VERIFIED |

Name note: the directive's "Ultimate Modular Men" ships on quaternius.com at
URL `ultimatemodularcharacters.html` with the page title "Ultimate Modular Men
Pack" — same pack, URL/title mismatch recorded here.

---

## 3. Approved sources with no current acquisition

| Source | License basis | Decision | Reason |
| :--- | :--- | :--- | :--- |
| Poly Haven (models/textures/HDRI) | Site-wide CC0 (`https://polyhaven.com/license`) | APPROVED-NOT-REQUIRED | Realistic photographic style — P2 upgrade path only; the frontier identity is stylized/low-poly today (recorded since AA-1) |
| Freesound (audio) | CC0 filter exists (`https://freesound.org`) | APPROVED-NOT-REQUIRED | The 3 CC0 Kenney audio packs cover the PART 18 feedback matrix at current scope; per-asset license gate tooling noted for any future need |
| OpenGameArt | Per-asset CC0 (machine-parseable) | DEFERRED | Viable future source (research-verified); no current gap after the Kenney + Quaternius batches |
| Kenney (remaining catalog) | CC0 | AS-NEEDED | 15 packs acquired cover the matrix; new packs only with a matrix gap |

---

## 4. Binding rules (standing)

1. LICENSE_UNCLEAR **never** enters the repository — the Quaternius script
   aborts the whole pack (not just the file) on any license-gate failure.
2. Every future acquisition batch extends this ledger + a JSON manifest with
   file-level hashes; the two never disagree (manifest is generated, the ledger
   cites it).
3. Kenney/Quaternius content is SUPPORT material — the Sci-Fi Survival
   Frontier identity, the Tier-A creature set and the native procedural
   fallbacks stay authoritative (PART 13: no asset collage).
4. Import/binding/cook verification of everything above is ENGINE-side and
   belongs to the Antigravity one-time integration (HANDOFF §20/§20b/§20c) —
   LICENSE_VERIFIED ≠ UE5_VERIFIED.
