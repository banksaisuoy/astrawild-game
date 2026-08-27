# ASTRAWILD Combat & Capture Design Specification

## 1. Elemental Resonance Matrix
Elemental affinities produce multipliers during combat damage calculations:

| Attacker \ Defender | Neutral | Solar (Fire) | Torrent (Water) | Geo (Earth) |
| :--- | :--- | :--- | :--- | :--- |
| **Neutral** | 1.0x | 1.0x | 1.0x | 1.0x |
| **Solar** | 1.0x | 0.5x | 0.5x (Disadvantaged) | **1.75x (Super Effective)** |
| **Torrent** | 1.0x | **1.75x (Super Effective)** | 0.5x | 0.5x (Disadvantaged) |
| **Geo** | 1.0x | 0.5x (Disadvantaged) | **1.75x (Super Effective)** | 0.5x |

---

## 2. Authoritative Damage Formula
```cpp
FinalDamage = (BaseAttackPower * SkillMultiplier * (100.0f / (100.0f + TargetDefense))) * ElementalMultiplier * CriticalMultiplier + RandomVariation;
```
- **Armor Reduction**: Diminishing returns scaling via `100 / (100 + Defense)`.
- **Critical Hits**: 1.5x damage on critical strike (base crit rate 5%).

---

## 3. Capture Mechanic (Astra Resonator)
The player throws an **Astra Resonator** projectile towards a wild Echo. Upon impact:
1. Target must be in `Wild` or `Hostile` state (cannot capture already captured or boss-locked entities).
2. Capture Probability Formula:
```cpp
CaptureChance = (1.0f - (CurrentHP / MaxHP)) * ResonatorPower * SpeciesCaptureModifier * StatusConditionBonus;
CaptureChance = FMath::Clamp(CaptureChance, 0.05f, 0.98f);
```
3. The Resonator shakes up to 3 times (Roll 1: 33% threshold, Roll 2: 66% threshold, Roll 3: 100% threshold).
4. If successful:
   - Target despawns from wild world.
   - Instantiated as `FAstrawildCapturedEchoData` and added to Player's active party (up to 5) or sent to Reserve Storage.
   - Triggers capture celebration VFX/SFX and UI notification.
5. If failed:
   - Resonator shatters.
   - Echo breaks out and enters an enraged `Hostile` state with increased speed for 5s.