"""Validate source-level guardrails for the ASTRAWILD compact vertical slice."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8", errors="replace")


def main() -> int:
    errors: list[str] = []

    source_checks = {
        "Source/AstrawildCore/Private/Components/AstrawildCaptureComponent.cpp": (
            "Resonator projectile spawn failed; refunded",
            "Inv->AddItem(ActualTag, 1)",
            "GetLifetimeReplicatedProps",
            "DOREPLIFETIME(UAstrawildCaptureComponent, ActiveParty)",
            "ServerThrowResonator_Implementation",
            "ServerSummonSelectedCompanion_Implementation",
            "ServerRecallActiveCompanion_Implementation",
            "ServerSelectNextPartySlot_Implementation",
            "ServerSelectPrevPartySlot_Implementation",
            "OnRepCaptureState",
            "Capture outcome must be resolved by the server.",
            "if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())",
        ),
        "Source/AstrawildCore/Private/Components/AstrawildBuildingComponent.cpp": (
            "Spawn failure must be transaction-safe",
            "Inv->AddItem(Ingredient.ItemTag, Ingredient.Quantity)",
            "OnBuildingFailed.Broadcast(LastPlacementErrorMessage)",
        ),
        "Source/AstrawildCore/Public/Components/AstrawildInventoryComponent.h": (
            "bool CanAddItem(const FGameplayTag& ItemTag, int32 Quantity) const",
        ),
        "Source/AstrawildCore/Private/Components/AstrawildInventoryComponent.cpp": (
            "bool UAstrawildInventoryComponent::CanAddItem",
            "constexpr int32 MaxStack = 99",
            "GetLifetimeReplicatedProps",
            "DOREPLIFETIME(UAstrawildInventoryComponent, Slots)",
            "ServerAddItem_Implementation",
            "ServerRemoveItem_Implementation",
            "ServerMoveOrSwapSlot_Implementation",
            "ServerSplitSlot_Implementation",
            "ServerClearInventory_Implementation",
            "OnRepSlots",
        ),
        "Source/AstrawildCore/Private/Environment/AstrawildHarvestableNode.cpp": (
            "inventory cannot accept the full yield",
            "!Inv->CanAddItem(PrimaryResourceTag, OutQuantityHarvested)",
        ),
        "Source/AstrawildCore/Public/AstrawildTypes.h": (
            "struct ASTRAWILDCORE_API FAstrawildFoodSaveState",
            "struct ASTRAWILDCORE_API FAstrawildFoodBuffSaveState",
            "TArray<FAstrawildFoodSaveState> TrackedFood",
        ),
        "Source/AstrawildCore/Private/Components/AstrawildSurvivalComponent.cpp": (
            "OutProfile.TrackedFood.Reset()",
            "OutProfile.ActiveFoodBuffs.Reset()",
            "bFoodStorageRefrigerated = InProfile.bFoodStorageRefrigerated",
            "ConsumeFood returns false when hunger is already full",
            "CurrentHunger >= MaxHunger - KINDA_SMALL_NUMBER",
            "!Inventory->HasItem(FoodTag, 1)",
            "Inventory->AddItem(FoodTag, 1)",
        ),
        "Source/AstrawildCore/Private/SaveSystem/AstrawildSaveGame.cpp": (
            "PlayerProfile.TrackedFood.RemoveAt(i)",
            "PlayerProfile.ActiveFoodBuffs.RemoveAt(i)",
        ),
        "Source/AstrawildCore/Public/Components/AstrawildQuestComponent.h": (
            "bool bAutoStartDependentQuests = true",
        ),
        "Source/AstrawildCore/Private/Components/AstrawildQuestComponent.cpp": (
            "bAutoStartDependentQuests && QuestTable",
            "Candidate->PrerequisiteQuestTag.ToString() == QuestId.ToString()",
            "StartQuest(DependentQuestId)",
        ),
        "Source/AstrawildCore/Public/Environment/AstrawildInteractableActor.h": (
            "EAstrawildQuestObjectiveType QuestObjectiveType",
            "float HungerRestoredOnInteract",
            "float ThirstRestoredOnInteract",
        ),
        "Source/AstrawildCore/Private/Environment/AstrawildInteractableActor.cpp": (
            "EAstrawildQuestObjectiveType::Collect",
            "Quest->AddProgressForTarget(QuestObjectiveType",
            "Survival->ConsumeFood",
            "Survival->DrinkWater",
        ),
        "Source/AstrawildCore/Private/Environment/AstrawildPrototypeArena.cpp": (
            '#include "Echoes/AstrawildAlphaEcho.h"',
            "if (bHasGeneratedArena)",
            "if (bAutoGenerateOnBeginPlay && HasAuthority())",
            "0x53000001u",
            "AAstrawildAlphaEcho* SolarixAlpha",
            "Building.StorageChest",
            "Location.AquavineSpring",
        ),
        "Source/AstrawildCore/Public/Environment/AstrawildPrototypeArena.h": (
            "bool bHasGeneratedArena = false",
        ),
        "Source/AstrawildCore/Private/SaveSystem/AstrawildSaveSubsystem.cpp": (
            "ExistingBuildings",
            "WorldSnapshot.PlacedBuildings",
            "SpawnActor<AAstrawildBuildingPiece>",
            "RestoredBuilding->LoadSaveData(BuildingData)",
        ),
        "Source/AstrawildCore/Private/Environment/AstrawildBuildingPiece.cpp": (
            "Loading an empty saved container must clear",
            "ContainerInventory->LoadInventorySlots(SaveData.ContainerInventory)",
        ),
        "Source/AstrawildCore/Private/World/AstrawildPowerGridSubsystem.cpp": (
            "GetWorld()->GetTimerManager().SetTimer",
            "batteryDischargeWatts",
            "float availableWatts = generation + batteryDischargeWatts",
        ),
        "Source/AstrawildCore/Private/Characters/AstrawildMountedWeaponBase.cpp": (
            "#include \"Echoes/AstrawildEchoBase.h\"",
            "RequiredPartnerSkillTag",
            "GetPartnerSkillTag()",
        ),
        "Source/AstrawildCore/Public/Framework/AstrawildGameMode.h": (
            "DT_Quests.DT_Quests",
            "DT_QuestObjectives.DT_QuestObjectives",
            "FName StartingQuestId = TEXT(\"Quest.Awakening\")",
        ),
        "Source/AstrawildCore/Private/Framework/AstrawildGameMode.cpp": (
            "void AAstrawildGameMode::PostLogin",
            "BootstrapPlayerQuest(NewPlayer)",
            "Character->Quest->StartQuest(StartingQuestId)",
        ),
        "Source/AstrawildCore/Public/Characters/AstrawildCharacter.h": (
            "bool bGrantDebugResonators = false",
            "int32 DebugResonatorQuantity = 5",
        ),
        "Source/AstrawildCore/Private/Characters/AstrawildCharacter.cpp": (
            "Resonators are intentionally",
            "if (bGrantDebugResonators && DebugResonatorQuantity > 0)",
        ),
        "Scripts/generate_gameplay_tag_registry.py": (
            "STRING_TAG_PATTERN",
            "BACKTICK_TAG_PATTERN",
            "Ability.BaseDamage",
        ),
    }

    source_checks.update({
        "Source/AstrawildCore/Public/World/AstrawildUnderwaterData.h": (
            "FAstrawildUnderwaterZoneRow",
            "MinDepthMeters",
            "MaxDepthMeters",
            "PressureDamagePerSecond",
            "OxygenDrainMultiplier",
            "HazardTags",
            "SpawnSpeciesTags",
        ),
        "Source/AstrawildCore/Public/World/AstrawildUnderwaterSubsystem.h": (
            "EAstrawildUnderwaterMovementMode",
            "PressureEmergency",
            "OxygenTankCapacitySeconds",
            "SurfaceOxygenRefillPerSecond",
            "EvaluateDiverState",
            "GetActiveZoneRow",
        ),
        "Source/AstrawildCore/Private/World/AstrawildUnderwaterSubsystem.cpp": (
            "IsAbyssalTrenchDepth",
            "CalculatePressureDamagePerSecond",
            "CalculateOxygenDrainPerSecond",
            "FMath::Min(SafeCapacity",
            "State.MovementMode = EAstrawildUnderwaterMovementMode::PressureEmergency",
            "FindRow<FAstrawildUnderwaterZoneRow>",
        ),
        "Source/AstrawildCore/Public/Components/AstrawildFishingComponent.h": (
            "EAstrawildFishingResult",
            "ServerStartFishing",
            "ServerUpdateReelInput",
            "ServerStopFishing",
            "UpdateReelInput",
            "IsTensionSafe",
        ),
        "Source/AstrawildCore/Private/Components/AstrawildFishingComponent.cpp": (
            "GetLifetimeReplicatedProps",
            "StartFishingAuthority",
            "SelectFishForContext",
            "Tension >= 100.0f",
            "InventoryFull",
            "Inventory->CanAddItem",
            "ServerStartFishing_Implementation",
            "ServerUpdateReelInput_Implementation",
            "ServerStopFishing_Implementation",
        ),
        "Source/AstrawildCore/Public/World/AstrawildRacingData.h": (
            "FAstrawildRaceCheckpoint",
            "FAstrawildRaceBoostPad",
            "FAstrawildRaceTrackDefinition",
            "FAstrawildRaceParticipantState",
        ),
        "Source/AstrawildCore/Public/World/AstrawildRacingSubsystem.h": (
            "RegisterTrack",
            "SubmitCheckpoint",
            "ActivateBoostPad",
            "GetParticipantSpeedMultiplier",
        ),
        "Source/AstrawildCore/Private/World/AstrawildRacingSubsystem.cpp": (
            "HasAuthorityForRacing",
            "CheckpointIndex != State->NextCheckpointIndex",
            "FVector::DistSquared(TrustedLocation",
            "State->CompletedLaps++",
            "State->bFinished = true",
            "OnRaceFinished.Broadcast",
            "MatchingPad",
            "State->BoostRemainingSeconds",
            "RemoveInvalidParticipants",
        ),
    })

    for relative, required_tokens in source_checks.items():
        path = ROOT / relative
        if not path.exists():
            errors.append(f"missing source file: {relative}")
            continue
        content = read(relative)
        for token in required_tokens:
            if token not in content:
                errors.append(f"{relative} is missing guard token: {token}")

    fish_path = ROOT / "Content/Astrawild/Data/Source/DT_FishDex.csv"
    with fish_path.open(encoding="utf-8-sig", newline="") as handle:
        fish_rows = list(csv.DictReader(handle))
    if len(fish_rows) != 30:
        errors.append(f"FishDex must contain exactly 30 rows; found {len(fish_rows)}")
    if len({row.get("FishTag", "") for row in fish_rows}) != len(fish_rows):
        errors.append("FishDex FishTag values must be unique")

    underwater_path = ROOT / "Content/Astrawild/Data/Source/DT_UnderwaterZones.csv"
    with underwater_path.open(encoding="utf-8-sig", newline="") as handle:
        underwater_rows = list(csv.DictReader(handle))
    abyssal = next((row for row in underwater_rows if row.get("Name") == "Underwater_AbyssalTrench"), None)
    if not abyssal:
        errors.append("Underwater DataTable must contain Underwater_AbyssalTrench")
    else:
        if abyssal.get("MinDepthMeters") != "100" or abyssal.get("MaxDepthMeters") != "1000":
            errors.append("Abyssal Trench depth contract must be 100-1000 meters")
        if abyssal.get("PressureDamagePerSecond") != "5.0" or abyssal.get("OxygenDrainMultiplier") != "1.5":
            errors.append("Abyssal Trench pressure/oxygen multipliers drifted")

    quest_path = ROOT / "Content/Astrawild/Data/Source/DT_QuestObjectives.csv"
    with quest_path.open(encoding="utf-8-sig", newline="") as handle:
        quest_rows = list(csv.DictReader(handle))
    campwater = next((row for row in quest_rows if row.get("Name") == "Objective_Campwater_Collect"), None)
    if not campwater or campwater.get("TargetTag") != "Item.Water":
        errors.append("Campwater collect objective must target the shared Item.Water tag")

    tag_registry = read("Config/DefaultGameplayTags.ini")
    for required_tag in ("Item.Water", "Location.AquavineSpring", "Echo.SolarixAlpha", "Biome.AbyssalTrench", "Hazard.Pressure"):
        if f'Tag="{required_tag}"' not in tag_registry:
            errors.append(f"gameplay tag registry is missing {required_tag}")
    for forbidden_false_positive in ("Ability.BaseDamage", "Ability.AbilityName", "Work.WorkEfficiencyMultiplier"):
        if f'Tag=\"{forbidden_false_positive}\"' in tag_registry:
            errors.append(f"gameplay tag registry contains field false positive {forbidden_false_positive}")

    map_spec = read("Docs/VERTICAL_SLICE_MAP_20MIN_SPEC.md")
    for phrase in (
        "Solarix Alpha",
        "Dawn Fiber",
        "Storage Chest",
        "20–30",
        "Acceptance checklist",
        "Runtime generation is disabled or guarded",
    ):
        if phrase not in map_spec:
            errors.append(f"map specification is missing required phrase: {phrase}")

    if errors:
        print("ASTRAWILD vertical-slice guard validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("ASTRAWILD vertical-slice guard validation passed (map bootstrap, interaction, save/build/capture rollback, power-grid, mounted-weapon, underwater, Fishing, and Racing guards).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
