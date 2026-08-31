# ANTIGRAVITY FIX LOG

**Project**: ASTRAWILD  
**Date**: 2026-08-31  

---

## Resolved Issues

1. **Unity Build Anonymous Namespace Collision (`C2086` / `C2374`)**:
   - **Root Cause**: `AstrawildVillageActor.cpp` and `AstrawildWorldBootstrapper.cpp` both defined `ShapeCube`, `ShapeCylinder`, `ShapeCone` in file-scope anonymous namespaces, colliding when merged into `Module.AstrawildCore.4.cpp`.
   - **Fix**: Prefixed constants as `VillageShapeCube`, `VillageShapeCylinder`, `VillageShapeCone` in `AstrawildVillageActor.cpp`.

2. **Integer Float Literal Suffixes in Bestiary (`C3688`)**:
   - **Root Cause**: 408 instances of raw integer float literals with `f` suffix (`1500f` instead of `1500.0f`) in `AstrawildBestiaryData.cpp`.
   - **Fix**: Replaced all suffixes with valid floating point syntax `1500.0f`.

3. **Missing Header in Capture Component (`C2027` Undefined Type `UAstrawildEchoDefinition`)**:
   - **Root Cause**: `AstrawildCaptureComponent.cpp` referenced `Echo->EchoDefinition->Element` without including `AstrawildDataAssets.h`.
   - **Fix**: Added `#include "AstrawildDataAssets.h"`.

4. **Atmosphere Noon Sun Color Discrepancy**:
   - **Root Cause**: `ASTRAWILD.Atmosphere.DayRamp` expected neutral daylight (`FLinearColor(1.00f, 0.98f, 0.92f, 1.0f)`) rather than saturated tint.
   - **Fix**: Tuned `NoonSun` color in `AstrawildWorldBootstrapper.cpp` to match test expectations.

5. **Subdivided Water Mesh Implementation**:
   - **Enhancement**: Upgraded water plane from 4-vertex quad to 16x16 subdivided mesh with procedural sine wave offsets and depth color gradients.

6. **Base Building Power Indicator Lights**:
   - **Enhancement**: Added `UPointLightComponent` to `AAstrawildBuildingActor` with dynamic 3-state emissive colors (Green = powered, Red = unpowered, Cyan/Gold = generator/battery).

7. **Skiff Flight Banking & Capture VFX**:
   - **Enhancement**: Added dynamic roll banking (16°), pitch tilt, hover bobbing to `AAstrawildSkiffActor`, and orbital resonance capture rings to `AAstrawildCaptureVfxActor`.
