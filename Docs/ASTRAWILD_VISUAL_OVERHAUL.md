# ASTRAWILD — Visual Overhaul (VO-1R) Specification

> **Status**: STATIC_VERIFIED at the VO-1R tip on `agent/visual-overhaul-v1`
> (base `5f86a47`). The runtime LOOK itself is ENGINE-UNVERIFIED until
> ENGINE-RUN-1 grades it in PIE per `ASTRAWILD_CINEMATIC_IDENTITY.md` §2.
> Owner: GLM. Session: 2026-10-19.

## 0. Why a rebuild (provenance)

The original VO-1 session (Palworld / Animal Crossing art direction, five
pillars, ~304MB LFS payload) was lost with its sandbox before its branch was
ever pushed — no bundle survived either. This document specifies the REBUILD
(VO-1R): the same five pillars, expressed entirely in the repo's zero-asset
procedural language (PMC geometry + post-process settings + light-rig bias)
so every layer survives compile, stays deterministic, and is covered by
world-free automation contracts. The lost binary payload (AI-generated
textures) was deliberately NOT recreated byte-for-byte; a real art pass can
layer on top through the existing ArtSource pipeline at any time without
touching this contract layer.

## 1. Art direction

Palworld / Animal Crossing: a friendly, saturated, bloom-soft world. Sat-up
color, warm sun, flat bright ambient, thin pastel fog, flower fields in every
meadow, chunkier cuter creatures, and "living air" (pollen by day, fireflies
by night). The VIS-001 charm spectrum (Cute / Cool / Strange) stays intact —
the overhaul AMPLIFIES the cute band and never touches cool/strange
silhouettes, so the bestiary keeps its authored contrast.

## 2. The five pillars

| # | Pillar | Implementation | Toggle |
|---|--------|----------------|--------|
| 1 | Toon post-process grade | `AstrawildVisualOverhaul::EvalToonGrade()` — applied by `AAstrawildVisualOverhaulDirector` onto a priority-10 unbound PP volume (sits above the bootstrapper's restrained priority-1 baseline) | `bEnableVisualOverhaul` |
| 2 | Anime atmosphere bias | `AAstrawildWorldBootstrapper::ApplyVisualOverhaulBias()` — applied AFTER the authored day/night ramp inside `UpdateAtmosphere` (single writer preserved: warmer sun, ×0.92 sun intensity, ×1.28 sky intensity, ×0.75 fog density, pastel fog color) | same |
| 3 | Flower fields | `FAstrawildDressingProfile::FlowerCount` (single source of truth) + `AAstrawildBiomeDressingActor::AppendFlower` (stem + leaf + 5-6 petal ring + golden center, dressing section 3) + `EvalPetalTint` deterministic per-flower palettes | same |
| 4 | Kawaii Echo amplification | `EvalKawaiiScales()` — cute band only: head ×1.12 (compounds on VIS-001's 1.18 → ≈1.32 total), eyes ×1.35, blush dots; cool/strange untouched | same |
| 5 | Ambient motes | `EvalMoteTransform` bounded sinusoidal orbits around the local player — golden pollen by day, teal fireflies by night; 48 motes (cap 96); dedicated servers skip the layer | same |

## 3. Toon grade values (contract)

| Field | Value | Rationale |
|---|---|---|
| BloomIntensity | 1.05 | soft anime glow (baseline was 0.6) |
| BloomThreshold | 0.85 | only petals/skies bloom |
| ColorSaturation | 1.22 uniform | painted pop |
| ColorContrast | 1.04 | gentle cel snap |
| ColorGamma | 0.92 | midtone lift (painted highlight) |
| VignetteIntensity | 0.18 | barely-there framing |
| AutoExposure Min/Max/Bias | 0.08 / 3.6 / 0.35 | night playable, noon unblown |

## 4. Flower budgets (12 zones, density-scaled at runtime)

| Zone | Budget | Palette family |
|---|---|---|
| Dawn Fields | 160 | coral / sunrise cream / blush rose |
| Verdant Reach | 150 | poppy / cornflower / marigold (rainbow meadow) |
| Glimmerwood | 90 | glow teal / mint / bioluminescent cyan |
| Tidebreaker Isles | 60 | coral / hot pink / sun yellow |
| Dusk Marsh | 55 | violet / lilac / teal mist |
| Frostveil Expanse | 45 | ice blue / white lavender / frost cyan |
| Ember Ridge | 35 | ember orange / magma red / gold |
| Sunscar Desert | 35 | magenta bloom / dry gold / dusty rose |
| Stormcrest Highlands | 30 | thistle / storm blue / silver |
| Azure Shallows | 30 | aqua / seafoam / sky |
| Hollow Approach | 20 | ash purple / pale bone / dusk violet |
| Pearlsea Reef | 14 | pearl pink / aqua pearl / shell peach |

Flowers share the grass slope tolerance and the water/exclusion gates; the
scatter is deterministic (same seed → same field on server and every LAN
client). Each flower picks a palette entry by `seed mod 3` with deterministic
value/saturation jitter, so a field never reads as one flat color.

## 5. Automation contracts (135 → 140)

| Test | Verifies |
|---|---|
| `ASTRAWILD.VisualOverhaul.ToonGradeContract` | grade bounds (saturated but never blown out) + bias bounds + `ApplyVisualOverhaulBias` end-to-end math on a synthetic sample |
| `ASTRAWILD.VisualOverhaul.FlowerFieldBudgets` | `EvalFlowerBudget` mirrors the profile table (single source of truth); meadows > 120; ≥ 8 zones bloom; all budgets ≥ 0 |
| `ASTRAWILD.VisualOverhaul.KawaiiScales` | neutral defaults are identity (1/1/0); amplified profile in range; global toggle roundtrip (save/restore) |
| `ASTRAWILD.VisualOverhaul.AmbientMoteMotion` | determinism, orbit ceiling (≤ 1400cm), smooth bounded drift (no teleport), invalid input fails closed, day-warm/night-cool tints |
| `ASTRAWILD.VisualOverhaul.PetalPaletteVariety` | ≥ 2 distinct tints per zone across seeds, luma visibility band, determinism, invalid zone fails safe to white |

## 6. Turning it off

One switch: `bEnableVisualOverhaul` on `AAstrawildWorldBootstrapper`
(default ON). Off restores the restrained Production V2 baseline grade and
atmosphere, drops the flower section and motes, and returns Echo bodies to
pure VIS-001 proportions. The global flag is seeded in `BeginPlay` BEFORE the
client early-return so every machine agrees.

## 7. Machine-day acceptance (ENGINE-RUN-1 addendum)

When ENGINE-RUN-1 runs, grade the overhaul in PIE alongside the existing
feel contract: sun reads warm-not-harsh at noon, fog stays legible at range,
Dawn Fields reads as a FLOWER FIELD from the camp, one cute-band Echo
(e.g. a Small Beast quadruped) shows chunky eyes + blush at gameplay camera
distance, and motes are visible around the player day and night. Any MISS →
tune the pure structs in `AstrawildVisualOverhaul.cpp` (single-file change,
contracts re-run in seconds).
