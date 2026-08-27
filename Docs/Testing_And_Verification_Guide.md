# ASTRAWILD Testing & Verification Guide

## 1. Acceptance Criteria Checklist
- [ ] **Player Character**: Responsive third-person controls (WASD, Mouse Look, Sprint [Shift], Jump [Space], Attack [LMB], Aim Capture [RMB]).
- [ ] **Harvesting**: Striking trees with Axe / rocks with Pick grants correct resources directly to inventory.
- [ ] **Combat**: Melee combo strikes register hitboxes; damage numbers & health bar updates correctly; elemental advantage delivers 1.75x damage.
- [ ] **Echo AI**: Wild Echoes wander peacefully until provoked or approached within aggro range; low HP triggers flee behavior or defensive skills.
- [ ] **Capture**: Throwing Astra Resonator rolls capture odds based on remaining target HP%; capture success puts Echo into player party and despawns wild actor.
- [ ] **Crafting**: Interacting with Crafting Bench displays valid recipes; crafting verifies ingredient counts and produces items.
- [ ] **Building**: Pressing Build Mode (B key) presents hologram preview; clicking places piece if unobstructed and resources exist.
- [ ] **Save & Load**: Executing Save persists player position, inventory, captured Echoes, and placed buildings; reload restores exact game state.

## 2. In-Game Debug Console Commands
- `Astrawild.GiveItem <ItemTag> <Amount>`: Grants specified items to player inventory.
- `Astrawild.SpawnEcho <SpeciesTag>`: Spawns an Echo species at player aim point.
- `Astrawild.Heal`: Fully restores player Health and Stamina.
- `Astrawild.SaveSlot <SlotName>`: Performs immediate synchronous/async save.
- `Astrawild.LoadSlot <SlotName>`: Reloads specified save slot.
- `Astrawild.ToggleDebugHUD`: Toggles on-screen combat, AI, and performance metrics.