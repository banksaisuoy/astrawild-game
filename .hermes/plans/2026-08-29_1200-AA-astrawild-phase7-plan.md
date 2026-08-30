# ASTRAWILD Phase 7-8 Plan — Authored Map + Blueprints

Plan saved at workspace: .hermes/plans/2026-08-29_1200-AA-astrawild-phase7-plan.md

Status: Phase 7-8 requires Editor GUI (binary .umap / .uasset creation) — cannot complete fully from CLI alone. Verified prerequisites present.

Evidence checklist (for reviewer):
- [ ] BP_Player.uasset in Editor (derived from AstrawildCharacter)
- [ ] BP_Echo_Pyrelite / Thornback / Aquavine (derived from EchoBase)
- [ ] BP_Alpha_Solarix (derived from AlphaEcho)
- [ ] 4-zone authored LV_DawnValley_Test.umap
- [ ] WBP_MasterHUD / Inventory / Crafting / EchoDex
- [ ] PIE smoke test: 20min loop (explore → harvest → combat → capture → craft → build → save/load)
- [ ] Build: compile + link pass (already verified)
- [ ] DataTables: 38/38 .uasset imported (already verified)

Open questions:
1. Skeletal mesh / AnimBP for Echoes — use existing SM_Echo_ static mesh or full rig?
2. Network PIE test — now or after map?
3. Binary asset tracking in Git?
4. Phase 8 packaging before or after authored map?
