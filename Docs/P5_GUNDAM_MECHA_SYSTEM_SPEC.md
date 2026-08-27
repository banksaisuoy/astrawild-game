# ASTRAWILD: Gundam Exosuit & Cybernetic Echo Fusion Architecture Specification

**Module**: `AstrawildCore`  
**Primary Components**: `UAstrawildMechaComponent`, `UAstrawildCyberneticEvolutionComponent`  
**Data Tables**: `DT_MechaFrames.csv`, `DT_MechaWeapons.csv`, `DT_CyberneticEvolutions.csv`  
**Target Engine**: Unreal Engine 5.8  

---

## 1. Executive Concept & Dual-Mode Mechanics

ASTRAWILD introduces high-tier **Astra Mecha Frames & Cybernetic Bio-Beast Fusions** inspired by Gundam, Titanfall, and ARK TEK Tier:

```mermaid
graph TD
    Player[Player Character] -->|Equip Exosuit / Pilot Cockpit| MechaFrame[UAstrawildMechaComponent: Astra Striker / Wing Zero]
    Echo[Organic Echo: Pyrelite / Volthound] -->|Astra Core Evolution| CyberBeast[Cybernetic Echo: TEK Mecha Unit]
    
    Player + CyberBeast -->|Gundam Fusion Sync| FusionMode[Supersonic Flying Gundam Exosuit]
    
    FusionMode --> Flight[6-DOF Flight Thrusters: 3200-5800 cm/s Overboost]
    FusionMode --> Weapons[Hardpoint Arsenal: Mega Beam Rifle, Beam Sabers, Micro-Missiles, Chest Annihilator]
    FusionMode --> Defense[Aegis Energy Shield + Heat Dissipation Cycle]
```

### Dual-State Operation:
1. **Mode A (Companion Cyber-Beast)**:
   - Fights autonomously alongside the player as a heavy cybernetic war beast with energy cannons and rocket pods.
2. **Mode B (Gundam Pilot / Fusion Armor)**:
   - Player docks into the cockpit or fuses with the cyber-beast, gaining full 6-DOF supersonic flight, twin beam sabers, lock-on missile swarms, and high-energy laser buster cannons.

---

## 2. Hardpoint Weapons & Weapon Slots

| Hardpoint Slot | Weapon Name | Base Damage | Fire Rate | Special Mechanics |
| :--- | :--- | :--- | :--- | :--- |
| **PrimaryRightHand** | `Weapon.BeamRifle` | 240.0 | 4.0/s | High-velocity particle beam (14,000 cm/s). |
| **SecondaryLeftHand** | `Weapon.BeamSaber` | 450.0 | 2.0/s | High-heat plasma melee blade with 3-hit combo. |
| **ShoulderRight** | `Weapon.MicroMissiles` | 90.0 | 12.0/s | 32-tube multi-lock homing micro-missiles. |
| **PrimaryRightHand** | `Weapon.TwinBusterRifle`| 850.0 | 1.0/s | Continuous piercing mega laser beam. |
| **ShoulderLeft** | `Weapon.TwinRailgun` | 650.0 | 1.5/s | Electromagnetic hyper-velocity railgun (22,000 cm/s). |
| **RearThruster** | `Weapon.FunnelBits` | 180.0 | 8.0/s | All-range remote funnel drone bits with auto-tracking. |
| **ChestCore** | `Weapon.SolarAnnihilator`| 1800.0 | 0.3/s | Ultimate Core Cannon with screen-wide vaporizing beam. |

---

## 3. Flight Thrusters, Overboost & Heat Cycle

- **Energy Core**: High-capacity energy tank (1000 - 3000 Energy) powering thrusters and energy weapons.
- **Flight Cruise**: `MOVE_Flying` at 2600 - 3800 cm/s.
- **Supersonic Overboost**: Rapid acceleration to 4200 - 5800 cm/s with forward camera FOV warp and vapor trails.
- **Heat Dissipation**: Energy weapons generate heat (0-100%). Hitting 100% causes emergency shutdown until heat cools below 20%.