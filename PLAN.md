# ASTRAWILD Production Plan

## Goal

สร้าง Vertical Slice เกม third-person cooperative survival adventure ที่เล่นกับเพื่อนได้ในอนาคต โดยมี exploration, resource harvesting, combat, Echo capture, crafting, rest point และ save/load ผ่านแผนที่ทดลองขนาดเล็กก่อนขยายโลก

## Current truth

- Source of truth: GitHub `main` branch.
- Current verified tree has C++ core and documentation but no Unreal binary assets.
- The generated visual target is `Docs/visual_target_astrawild.png`; it is concept reference, not a final game asset.
- Unreal Compile and Playtest must be run on the target Windows machine by Antigravity.

## Milestones

### M0 — Repository and C++ gate

**Deliverable:** target project opens, `AstrawildCore` compiles, no generated folders are committed.

**Proof:** `ASTRAWILDEditor Win64 Development` build result recorded in `Docs/BUILD_STATUS.md`.

### M1 — Playable primitive slice

**Deliverable:** `L_Prototype.umap` with Player, Input, resource nodes, three Echo actors, rest point, damage target, Data Assets and minimal HUD.

**Proof:** Play-in-Editor test passes movement → interaction → harvest → inventory → damage → capture → craft → rest point → save/load.

### M2 — Gameplay readability

**Deliverable:** UI feedback, debug commands, placeholder materials, basic Niagara/audio feedback and clear landmark layout.

**Proof:** screenshots/video at Low and Medium scalability settings; no unreadable interaction or missing state.

### M3 — Asset replacement

**Deliverable:** licensed/original meshes, animation, materials, collision, LOD, VFX and audio replace primitive placeholders.

**Proof:** every asset has a row in `Docs/ThirdPartyLicenses.md` and the core-loop test still passes.

### M4 — Co-op foundation

**Deliverable:** authority boundaries for inventory, capture, damage, crafting and build; session flow for 1–4 players.

**Proof:** two-client test with replication and save policy documented.

## Risk slices in priority order

1. Compile and generated-header compatibility.
2. Enhanced Input and player Blueprint wiring.
3. Stable Data Asset IDs and content validation.
4. Capture/inventory/crafting transaction correctness.
5. Save/load and corruption fallback.
6. Primitive map readability and collision.
7. AI and multiplayer authority.
8. Asset import, animation, VFX and audio polish.

## Stop conditions

Do not expand the world, add dozens of Echo species, or begin dedicated server work until M1 passes. Do not call the project complete from a static audit. Complete means compile, map/Blueprint creation, core-loop Playtest and Save/Load all pass on the target machine.
