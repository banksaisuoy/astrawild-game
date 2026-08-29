# ASTRAWILD — Combat System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 3 sync — weapon attack power + shield block integration)
**Primary sources:** `AstrawildCombatComponent.h/.cpp`, `AstrawildSurvivalComponent.cpp`,
`AstrawildEchoCharacter.cpp` (ApplyElementalDamage), `AstrawildPlayerCharacter.cpp` (input wiring)

Third-person action combat: light/heavy attacks, dodge with invulnerability frames, block, stamina gating,
and an elemental damage pipeline. All damage resolution is **server-side**; the client sends intent through
reliable `Server_` RPCs.

---

## 1. Tunables (defaults from `UAstrawildCombatComponent`)

| Group | Property | Default |
|---|---|---|
| Attack | `LightAttackDamage` | 25 |
| | `LightAttackCooldown` | 0.45 s |
| | `HeavyAttackDamage` | 60 |
| | `HeavyAttackCooldown` | 1.3 s |
| | `HeavyAttackStaminaCost` | 25 |
| | `AttackRange` | 320 cm |
| | `AttackElement` | `Ash` (equipment override PLANNED) |
| Dodge | `DodgeCooldown` | 0.9 s |
| | `DodgeStaminaCost` | 22 |
| | `DodgeInvulnerabilitySeconds` | 0.4 s |
| | `DodgeImpulseStrength` | 900 (LaunchCharacter impulse) |
| Block | `UnarmedBlockMitigation` | 0.45 (45 %) — renamed in wave 3 (was `BlockMitigation` 0.65); an equipped shield replaces it |
| | `BlockSpeedMultiplier` | 0.45 (movement ×0.45 while blocking) |

Player-character legacy mirror (`AAstrawildPlayerCharacter`): `AttackDamage 25`, `AttackDistance 280`,
`AttackCooldownSeconds 0.45` — retained from v1, superseded in practice by the component's combat path.

---

## 2. Attacks

### 2.1 Flow (client → server)

```
Input (LMB / F)
  └─ PlayerCharacter::Attack / HeavyAttack
      └─ CombatComponent::RequestLightAttack / RequestHeavyAttack     [client]
          ├─ CanAttack(bHeavy)?  (alive + cooldown elapsed)
          └─ ServerLightAttack / ServerHeavyAttack  (Server, Reliable)
                └─ ExecuteAttack(bHeavy)                              [server]
```

`ExecuteAttack`:
1. Re-checks alive + cooldown (never trust the client).
2. **Heavy only**: consumes 25 stamina (`Survival->TryConsumeStamina`); fails if exhausted. Light attacks
   are stamina-free.
3. Records attack time (per-type cooldown clocks).
4. **Hit resolution sweep**: `SweepMultiByChannel` on `ECC_Pawn`, sphere radius **90 cm**, from the player
   location to `location + forward × AttackRange (320 cm)`, ignoring self.
5. For each hit: Echoes (skip defeated) take `ApplyElementalDamage(GetOutgoingAttackDamage(bHeavy),
   AttackElement)`; damage targets take `ApplyDamage`. Accumulates total damage into the
   `OnAttackExecuted(bHeavy, DamageDealt)` broadcast.

Damage numbers land **after** the target's own defense/elemental math, so the broadcast reports applied damage.

### 2.2 Elemental pipeline (target-side, `AAstrawildEchoCharacter::ApplyElementalDamage`)

```
Damage = RawDamage
if Element == target.WeaknessElement : Damage × 1.5        (weakness)
else if Element == target.Element     : Damage × (1 − ElementalResistance)   (default resist 0.2)
Damage = max(0, Damage − target.Defense)                    (flat defense)
Health −= Damage
```

- Defeat (`Health ≤ 0`) broadcasts `OnDefeated`, notifies the ecosystem (population `DefeatedCount`),
  publishes `Event.EchoDefeated` — or `Event.HostileDefeated` for hostile species — which drives the
  Dawn Guard quest.
- Physical attacks (element `None`) skip both elemental modifiers; only flat defense applies.
- Players currently have **no elemental resistance** (see Known Gaps).

### 2.3 Equipment integration (wave 3)

`ExecuteAttack` resolves outgoing damage through **`GetOutgoingAttackDamage(bHeavy)`**:

```
Outgoing = (bHeavy ? HeavyAttackDamage : LightAttackDamage) + Inventory->GetEquippedWeaponAttackPower()
```

- The equipped weapon's flat `AttackPower` (item definition) is added to **both** attack tiers.
- `UAstrawildCombatComponent::GetEquippedWeaponAttackPower()` proxies the inventory component
  (resolves `EquippedItemId` through the item registry; 0 when unarmed or the id is unknown).
- CODE_DEFAULT progression: **Dawnwood Club +6** (light 25→31, heavy 60→66), **Dawn Crystal Blade
  +14** (light 25→39, heavy 60→74).
- Weapon routing on equip: `InventoryComponent::EquipItem` puts items with `AttackPower > 0` in the
  weapon slot (`EquippedItemId`), items with `BlockMitigation > 0` in the shield slot
  (`EquippedShieldItemId`) — both replicated, both persisted in the save.
- Quick access: **X** (equip-best) equips the strongest owned weapon + shield; `AW.EquipItem <id>`
  cheats a specific item on; the HUD shows `Weapon: <name> (+N) | Shield: <name>`.

---

## 3. Dodge

```
Input (Q) ─► RequestDodge(direction)            [client: alive + 0.9 s cooldown gate]
          └─► ServerDodge(FVector_NetQuantizeNormal Direction)   [server]
                ├─ alive + cooldown re-check
                ├─ TryConsumeStamina(22)
                ├─ DodgeInvulnerabilityRemaining = 0.4 s ; bReplicatedDodgeTimer = 0.4
                ├─ OnDodgeStateChanged(true, 0.4) broadcast
                └─ LaunchCharacter(direction × 900, XY-only, false)
```

- **I-frames**: while `DodgeInvulnerabilityRemaining > 0`, `GetMitigatedIncomingDamage` returns **0** —
  incoming attacks miss entirely.
- Direction = last movement input vector (or actor forward). The server clamps to the XY plane.
- The component tick counts the invulnerability window down and broadcasts the end
  (`OnDodgeStateChanged(false, 0)`).
- `bReplicatedDodgeTimer` (float, replicated) exposes the window to clients for VFX/animation.

---

## 4. Block

```
Input (RMB hold) ─► RequestSetBlocking(true/false)   [client]
                 └─► ServerSetBlocking(bool)          [server]
                       ├─ force false when dead
                       └─ bIsBlocking (Replicated) + OnBlockingChanged broadcast
```

- **Mitigation is equipment-dependent (wave 3)**: `GetMitigatedIncomingDamage(Raw)` returns
  `Raw × (1 − GetEffectiveBlockMitigation())` while blocking.
  - Unarmed baseline: `UnarmedBlockMitigation = 0.45` → 55 % of raw damage passes.
  - With the **Stonehide Shield** equipped (`BlockMitigation 0.65`): the shield value **replaces** the
    unarmed baseline (never stacks with it) → 35 % passes. Shield values are clamped to 0..0.8.
- **Speed**: `PlayerCharacter::RefreshMovementSpeed` multiplies walk/sprint speed by
  `BlockSpeedMultiplier (0.45)` while blocking — unchanged.
- Block applies to all incoming AI melee damage routed through the combat component (both Echo-on-player
  paths in the AI controller use `GetMitigatedIncomingDamage`).

### Combined incoming-damage resolution

```
IsDodging?            ──► 0 damage (i-frames)
else IsBlocking?      ──► Raw × (1 − EffectiveBlockMitigation)   (0.55 unarmed / 0.35 with Stonehide Shield)
else                  ──► Raw
then SurvivalComponent::ApplyDamage  (god mode check → health −, OnStatsChanged, death at 0)
```

---

## 5. Server RPC Inventory (combat)

| RPC | Reliability | Payload | Server-side validation |
|---|---|---|---|
| `ServerLightAttack` | Reliable | — | alive, cooldown, sweep |
| `ServerHeavyAttack` | Reliable | — | alive, cooldown, stamina 25, sweep |
| `ServerDodge` | Reliable | `FVector_NetQuantizeNormal` direction | alive, cooldown, stamina 22, impulse |
| `ServerSetBlocking` | Reliable | `bool` | alive check, replicated flag |

Attack requests are client-gated by cooldown before sending (request-side prediction of *permission*, not
of the hit itself — all hit resolution remains server-side).

---

## 6. Survival Damage Entry

All player damage funnels through `UAstrawildSurvivalComponent::ApplyDamage(DamageAmount)`:

- Server-authority + alive + god-mode checks; returns applied damage.
- Broadcasts `OnStatsChanged(Health, Stamina)`; logs via `LogAstrawildCombat`.
- Death at 0 HP → `Die()` → `OnDied` → PlayerCharacter disables input and asks the GameMode for a respawn
  in 5 s (see `ASTRAWILD_SURVIVAL_SYSTEM.md` §6).

Fall-out-of-world: `FellOutOfWorld` applies 9999 damage server-side.

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| Target lock | NOT IMPLEMENTED |
| Ranged attacks / weapons | NOT IMPLEMENTED (melee sweep only) |
| Hit reactions / stagger | NOT IMPLEMENTED (damage numbers + AI flee state only) |
| Telegraphs / VFX / sound | NOT IMPLEMENTED (placeholder visuals only) |
| Player elemental resistances | NOT IMPLEMENTED |
| Weapon-driven `AttackElement` override | NOT IMPLEMENTED (component `AttackElement` stays `Ash`; only flat weapon damage is wired) |
| Combat UI (damage numbers) | NOT IMPLEMENTED (HUD shows bars + the wave 3 equipment readout only) |
