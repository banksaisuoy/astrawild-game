# ASTRAWILD — Antigravity Engineering & Fix Log

**Date:** August 30, 2026  
**Agent:** Antigravity (Google DeepMind)  
**Target Environment:** Unreal Engine 5.8.2 | Visual Studio 2022 (MSVC 14.44) | .NET 10 | Windows 11 x64  
**Workspace:** `E:\AstrawildGame` | Output Package: `E:\Astrawild_Packaged`

---

## Summary of Fixes Applied

| Component / File | Issue Description | Surgical Fix Applied |
|---|---|---|
| `AstrawildShopWidget.cpp` & `.h` | Variable shadowing (`C4458`), invalid `UScrollBoxSlot::SetSize` float arg, invalid `TWeakObjectPtr` method calls, missing `WidgetTree` root references. | Renamed shadowed parameter names (`ParentShop`, `Padding`), used `FSlateChildSize(ESlateSizeRule::Fill)`, replaced `RootWidget` with `WidgetTree->RootWidget`, and checked raw pointers cleanly. |
| `AstrawildPlayerController.cpp` | Return type ternary mismatch in widget class getters; obsolete `SetPauserPlayerState` world call; invalid GameMode cheat assignment. | Replaced ternary mismatches with clean `TSubclassOf<UUserWidget>` returns, replaced pauser call with `SetPause(true/false)`, configured `CheatClass = UAstrawildCheatManager::StaticClass()` in PC constructor. |
| `AstrawildBuildingActor.h` | Missing `IsSwitchedOn()` declaration required by building subsystem. | Added `bool IsSwitchedOn() const { return bIsSwitchedOn; }` getter. |
| `AstrawildBuildingComponent.cpp` | `OverlapMultiByChannel` type mismatch using `TArray<FHitResult>` instead of `TArray<FOverlapResult>`. | Added `#include "Engine/OverlapResult.h"` and updated output container to `TArray<FOverlapResult>`. |
| `AstrawildProjectileActor.cpp` | `CollisionSphere->IgnoreActorWhenMoving` rejected `const AActor*`. | Removed `const` qualifier on owner pointer before passing to ignore list. |
| `AstrawildResearchScreenWidget.cpp` | Direct `RootWidget` access and bare `Anchors(...)` constructor. | Replaced with `WidgetTree->RootWidget` and `FAnchors(...)`. |
| `AstrawildItemRegistrySubsystem.cpp` | Type deduction mismatch when iterating `TMap<FName, TObjectPtr<T>>` into `TArray<T*>`. | Explicitly dereferenced and converted pointers during array accumulation. |
| `AstrawildPauseMenuWidget.cpp` | Direct `RootWidget` access, bare `Anchors`, slot variable shadowing. | Replaced with `WidgetTree->RootWidget`, `FAnchors`, and unique local slot identifiers. |
| `AstrawildPlayerCharacter.cpp` & `AstrawildEchoCharacter.cpp` | `UNavigationInvokerComponent::SetRadii` deprecated in UE5.8; missing Movement component headers. | Updated to `NavInvoker->SetGenerationRadii(...)` and added `#include "GameFramework/CharacterMovementComponent.h"`. |
| `AstrawildCombatComponent.h` | Logic typo in `IsDodging()` checking `DodgeInvulnerabilityRemaining < 0.0f`. | Corrected condition to `DodgeInvulnerabilityRemaining > 0.0f`. |
| `AstrawildAutomationTests.cpp` | Ambiguous `TestEqual` overload conversions for floating point literals. | Specified explicit `double` literals (`240000.0`, `160000.0`). |
| `AstrawildHudWidget.h` & `.cpp` | Parameterless constructor conflicted with `GENERATED_BODY()` default `FObjectInitializer` constructor. | Removed parameterless constructor, allowing UMG `GENERATED_BODY()` default constructor to handle instantiation. |
| `AstrawildInventoryScreenWidget.cpp` | Const-qualification cast issue on `GetPlayerCharacter()`; bare `Anchors`. | Fixed const-correctness and used `FAnchors`. |
| `AstrawildEchoAIController.h` & `.cpp` | `OnTargetPerceptionUpdated.AddUObject` failed on dynamic multicast delegate; missing `UFUNCTION()` on handler. | Added `UFUNCTION()` to `HandlePerception` and switched binding to `Perception->OnTargetPerceptionUpdated.AddDynamic(...)`. |
| `AstrawildUtilityDroneActor.cpp` | Variable name `Owner` shadowed `AActor::Owner`; invalid `IsValid()` on `TObjectPtr`. | Renamed local variable to `TargetPlayer` and verified raw pointer validity. |
| `AstrawildWorldBootstrapper.cpp` | Invalid header `#include "Engine/SkyAtmosphere.h"`; missing light component headers; `Sky->SetMobility` called on `ASkyLight` instead of `USkyLightComponent`. | Added `#include "Components/SkyAtmosphereComponent.h"`, `#include "Components/LightComponent.h"`, `#include "Components/PointLightComponent.h"`, `#include "Components/SkyLightComponent.h"`, and set mobility on `SkyComponent`. |
| `AstrawildCheatManager.cpp` | Missing `#include "AstrawildDataAssets.h"` and `#include "AstrawildInventoryComponent.h"`. | Added required headers and fixed `CaptureAll()` iteration. |
| `AstrawildTerrainTileActor.cpp` | `UProceduralMeshComponent::bUseAsyncCookCreation` renamed in UE5. | Updated property to `Mesh->bUseAsyncCooking = false;`. |
| `AstrawildNPCCharacter.cpp` | Attempted to load non-existent `/Engine/BasicShapes/Capsule.Capsule`. | Updated to `/Engine/BasicShapes/Cylinder.Cylinder`. |
| `CopyBuildToStagingDirectory.Automation.cs` (Engine Toolchain) | `ArgumentOutOfRangeException` in `OrderFile..ctor` when file name ends with `_` or matches split boundaries. | Added boundary check: `if (index != -1 && index + 1 < FileName.Length)`. |
| `UnrealPak` Engine Binary | Missing binary `UnrealPak.exe` needed for stage/pak/archive. | Built standalone `UnrealPak Win64 Development` via UnrealBuildTool. |
