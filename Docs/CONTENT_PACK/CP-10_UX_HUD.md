# CP-10 — UX / HUD POLISH: Glassmorphism, Feedback & Screen Standards

**Goal:** a diegetic sci-fi frontier interface — glass panels, palette discipline,
and feedback on every action. The pure-C++ UMG screens (HUD, inventory, research,
crafting, shop, pause, **dialogue — NEW**) are functionally complete; this pack
restyles them and adds feedback layers. The C++ widgets stay the runtime base —
restyle via a shared UMG theming kit, not rewrites.

---

## 1. Visual language — "Dawn Glass"

| Token | Value |
|---|---|
| Panel fill | `#12151ACC` (76% glass) |
| Panel border | 1 px `#4ADCC8` at 40% (teal trim) |
| Headline text | `#F5F3EA` warm white |
| Body text | `#D9D4C4` |
| Accent (interactive) | teal `#A2F6EF` |
| Accent (warning) | Ember `#FFB87C` |
| Accent (danger) | `#F2555C` |
| Rarity chip colors | palette (CP-02 §4) |
| Radius | 12 px panels, 8 px chips |
| Panel scrim | world dim 55% (dialogue already live) |

Fonts: Roboto (engine) is the placeholder; production swaps to an open-license
geometric sans (2 weights). All screens: 60 fps interaction budget, no per-frame
text rebuilds (screens already rebuild on event).

## 2. HUD upgrades (the always-on layer)

| Element | Spec |
|---|---|
| Vitals cluster | hex-cell gauges HP (warm) / SP (teal) / Energy (Ember) with damage flash + slow-fill heal |
| Temperature readout | compact dual-band (cold/heat) matching the split-insulation stats |
| Weapon card | family icon + ammo + tier pips; reload state pulses Ember |
| World-event banner | top-slide, event-tinted (already live — restyle tokens only) |
| Crosshair | 4-tick; **hit marker** × on confirmed damage, element-tinted; **kill marker** ring; lock-on brackets (CP-05 §1) when missile cone acquires |
| Mini-radar | 12 m compass strip + N marker + discovered POI blips (type colors already C++-tinted) + scanner sweep ring (tier color) |
| Quest tracker | active objectives, proximity reveal for ReachLocation/DiscoverPOI |
| Roster chip (party) | up to 3 member mini-cards with HP bar + evolution-ready sparkle (`CanEvolve` → CP-02 §2) |

## 3. Screens (restyle + specific upgrades)

- **Inventory:** grid + rarity borders + tooltip cards (stats, element, tier); drag
  drop; weight bar with over-encumbrance warning.
- **Research tree:** hologram nodes (CP-07 §6) per branch color; prune to completed
  state on load.
- **Crafting:** recipe list + live "can craft" gating (H-11 guard already ensures
  output fits — surface its failure states as UI copy).
- **Shop:** wares with sell-back prices; balance chip.
- **Dialogue (NEW screen):** current pure-C++ panel adopts Dawn Glass tokens;
  speaker name amber, choice hover teal, gated choices never rendered (already live).
- **Pause:** resume/save/load/quit with autosave timestamp.

## 4. Feedback standards (every action answers)

| Action | Feedback |
|---|---|
| Harvest | item toast + node dim (C++ live) + pickup chirp |
| Capture success | full sequence (CP-05 §2) + roster add toast |
| Evolution | burst + toast + roster card refresh |
| Research unlock | node flare + one-shot cue |
| Quest objective | tracker check tick + cue; banner on complete |
| Night raid | banner + horn + radar red sweep |
| Death | screen desat + respawn flow (existing) |

## 5. Accessibility & input

- Full gamepad support (existing Enhanced Input mappings) — every new element focusable.
- Text minimum 14 pt body; color-coded info never relies on color alone (icons pair).
- Subtitles: dialogue line text always-on (already), voice toggle later.
- Hold-to-repeat on buy/craft buttons; 44 px touch targets for future touch QA.

## 6. Post-launch stretch

- Codex UI over the 214-species bestiary (data all exists — the journal subsystem
  already tracks discovery).
- Map screen (zone discovery data live).
- Photo mode; UI scale slider; rebind UI.

## Acceptance

- [ ] All 7 screens share Dawn Glass tokens — visual family test passes side-by-side.
- [ ] Every action in §4 answers within 100 ms (perceptible + non-blocking).
- [ ] HUD clutter ≤ 25% of screen at 16:9; nothing overlaps at 1280×720 minimum.
- [ ] Hit/kill markers fire exactly on C++ damage confirmation (never predicted).
- [ ] Gamepad-only playthrough of the core loop (menu→game→craft→fight→capture→save).
