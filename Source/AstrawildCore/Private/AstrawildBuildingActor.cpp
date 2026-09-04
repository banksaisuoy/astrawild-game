#include "AstrawildBuildingActor.h"

#include "AstrawildBaseTerminalActor.h"
#include "AstrawildCore.h"
#include "AstrawildCropComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDurabilityComponent.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildGeneticsLibrary.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildPowerSubsystem.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildTurretComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildBuildingActor::AAstrawildBuildingActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
    }

    PowerIndicatorLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PowerIndicatorLight"));
    if (PowerIndicatorLight)
    {
        PowerIndicatorLight->SetupAttachment(RootComponent);
        PowerIndicatorLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
        PowerIndicatorLight->SetAttenuationRadius(350.0f);
        PowerIndicatorLight->SetCastShadows(false);
        PowerIndicatorLight->SetIntensity(0.0f);
    }

    BuildingId = FGuid::NewGuid();

    // FR-9: the sliding door panel — attached to the root, only visible/colliding
    // for Door-category buildings (the root shrinks to a thin track for them so
    // the saved actor transform never moves when the door slides).
    DoorPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorPanel"));
    if (DoorPanel)
    {
        DoorPanel->SetupAttachment(VisualMesh);
        DoorPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DoorPanel->SetVisibility(false);
        DoorPanel->SetRelativeScale3D(FVector(1.4f, 0.15f, 1.6f));
    }
}

void AAstrawildBuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsSwitchedOn);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsPowered);
    DOREPLIFETIME(AAstrawildBuildingActor, CurrentHealth);
    DOREPLIFETIME(AAstrawildBuildingActor, StoredCharge);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsOpen);
}

void AAstrawildBuildingActor::BeginPlay()
{
    Super::BeginPlay();
    RegisterPower();
}

void AAstrawildBuildingActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
            {
                Power->UnregisterBuilding(this);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AAstrawildBuildingActor::RegisterPower()
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (!Def || Def->PowerRole == EAstrawildPowerRole::Consumer && Def->PowerDraw <= 0.0f)
    {
        return; // Pure structural pieces skip the grid.
    }

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
        {
            Power->RegisterBuilding(this);
        }
    }
}

const UAstrawildBuildingDefinition* AAstrawildBuildingActor::GetBuildingDefinition() const
{
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    return Registry ? Registry->FindBuilding(DefinitionId) : nullptr;
}

AAstrawildBuildingActor* AAstrawildBuildingActor::SpawnForDefinition(UWorld* World,
    const UAstrawildBuildingDefinition* Definition, const FVector& Location, const FRotator& Rotation)
{
    if (!World || !IsValid(Definition))
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // SCP Phase 9: the Base Terminal is the one specialized building — its
    // subclass carries territory/garrison logic. Placing and save-load both
    // route through here so the actor class can never diverge between paths.
    UClass* ActorClass = AAstrawildBuildingActor::StaticClass();
    if (Definition->DefinitionId == TEXT("Building_BaseTerminal"))
    {
        ActorClass = AAstrawildBaseTerminalActor::StaticClass();
    }

    return World->SpawnActor<AAstrawildBuildingActor>(ActorClass, Location, Rotation, Params);
}

bool AAstrawildBuildingActor::InitializeFromDefinition(const UAstrawildBuildingDefinition* Definition, const FName InOwnerPlayerId)
{
    if (!IsValid(Definition) || Definition->DefinitionId.IsNone())
    {
        return false;
    }

    DefinitionId = Definition->DefinitionId;
    OwnerPlayerId = InOwnerPlayerId;
    MaxHealth = FMath::Max(1.0f, Definition->MaxHealth);
    CurrentHealth = MaxHealth;

    // SCP Phase 8/11: specialized behaviors attach by definition (crop cycles
    // on farm plots, auto-turrets on defense pieces — plain buildings skip).
    if (DefinitionId == TEXT("Building_FarmPlot") && !CropComponent)
    {
        CropComponent = NewObject<UAstrawildCropComponent>(this);
        CropComponent->RegisterComponent();
    }
    if (Definition->Category == EAstrawildBuildingCategory::Defense && !TurretComponent)
    {
        TurretComponent = NewObject<UAstrawildTurretComponent>(this);
        TurretComponent->RegisterComponent();
    }

    // Structural scale per category for readable placeholder silhouettes.
    if (VisualMesh)
    {
        switch (Definition->Category)
        {
        case EAstrawildBuildingCategory::Foundation:
            VisualMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 0.2f));
            break;
        case EAstrawildBuildingCategory::Wall:
            VisualMesh->SetWorldScale3D(FVector(2.0f, 0.2f, 1.5f));
            break;
        case EAstrawildBuildingCategory::Power:
            VisualMesh->SetWorldScale3D(FVector(0.9f, 0.9f, 1.4f));
            break;
        // Final Run (FR-9): the four new construction pieces.
        case EAstrawildBuildingCategory::Floor:
            // Thin plank deck laid on foundations/walls.
            VisualMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 0.12f));
            break;
        case EAstrawildBuildingCategory::Roof:
            // Wide flat cap — reads as shelter from the top-down build view.
            VisualMesh->SetWorldScale3D(FVector(2.2f, 2.2f, 0.18f));
            break;
        case EAstrawildBuildingCategory::Door:
            // Root becomes the thin track; DoorPanel is the visible leaf.
            VisualMesh->SetWorldScale3D(FVector(1.5f, 0.12f, 0.12f));
            break;
        case EAstrawildBuildingCategory::Storage:
            // Chest-sized box — reads as a container, not a wall.
            VisualMesh->SetWorldScale3D(FVector(1.1f, 1.1f, 0.9f));
            break;
        default:
            VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 1.0f));
            break;
        }

        // FR-9: only doors carry the sliding panel; everything else hides it.
        if (DoorPanel)
        {
            const bool bIsDoor = Definition->Category == EAstrawildBuildingCategory::Door;
            DoorPanel->SetVisibility(bIsDoor);
            DoorPanel->SetCollisionEnabled(bIsDoor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
            DoorPanel->SetRelativeLocation(FVector::ZeroVector);
        }
    }

    // FR-9: doors start closed (the panel state applies on the next refresh).
    ApplyDoorVisualState();

    RegisterPower();
    UpdateVisualPowerState();
    return true;
}

void AAstrawildBuildingActor::UpdateVisualPowerState()
{
    if (!PowerIndicatorLight)
    {
        return;
    }

    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (!Def)
    {
        PowerIndicatorLight->SetIntensity(0.0f);
        return;
    }

    if (Def->Category == EAstrawildBuildingCategory::Power)
    {
        // Generator / Battery: glowing active cyan/gold core
        PowerIndicatorLight->SetLightColor(FLinearColor(0.2f, 0.9f, 0.85f));
        PowerIndicatorLight->SetIntensity(bIsSwitchedOn ? 2.5f : 0.0f);
    }
    else if (Def->PowerRole == EAstrawildPowerRole::Consumer && Def->PowerDraw > 0.0f)
    {
        // Consumer: Green = powered, Red = unpowered
        PowerIndicatorLight->SetLightColor(bIsPowered ? FLinearColor(0.2f, 1.0f, 0.3f) : FLinearColor(1.0f, 0.15f, 0.1f));
        PowerIndicatorLight->SetIntensity(1.8f);
    }
    else
    {
        PowerIndicatorLight->SetIntensity(0.0f);
    }
}

void AAstrawildBuildingActor::OnRep_IsPowered()
{
    UpdateVisualPowerState();
}

void AAstrawildBuildingActor::ApplyBuildingDamage(const float DamageAmount)
{
    if (GetLocalRole() != ROLE_Authority || DamageAmount <= 0.0f)
    {
        return;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
    OnBuildingDamaged.Broadcast(this);

    if (IsDestroyed())
    {
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Building %s destroyed."), *GetName());
        Destroy();
    }
}

void AAstrawildBuildingActor::SetSwitchedOn(const bool bOn)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        bIsSwitchedOn = bOn;
        UpdateVisualPowerState();
    }
}

FText AAstrawildBuildingActor::GetInteractionPrompt_Implementation() const
{
    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (!Def)
    {
        return FText::GetEmpty();
    }

    // Final Run (FR-9): door + storage crate prompts.
    if (Def->Category == EAstrawildBuildingCategory::Door)
    {
        return bIsOpen
            ? FText::FromString(TEXT("Close door [E]"))
            : FText::FromString(TEXT("Open door [E]"));
    }
    if (Def->Category == EAstrawildBuildingCategory::Storage)
    {
        const int32 StoredCount = StoredItems.Num();
        if (StoredCount <= 0)
        {
            return FText::FromString(TEXT("Storage crate (empty) — deposit [E]"));
        }
        return FText::FromString(FString::Printf(TEXT("Storage crate (%d/%d stacks) — deposit / take [E]"),
            StoredCount, StorageCapacity));
    }

    // Audit C-2: the Research Desk is the in-world research entry point.
    // Final production run: interacting now OPENS the research screen (player
    // agency over the tree) — the prompt still previews the cheapest unlock.
    if (Def->Category == EAstrawildBuildingCategory::Research)
    {
        if (UWorld* World = GetWorld())
        {
            if (World->GetGameInstance())
            {
                if (const UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
                {
                    int32 Cost = 0;
                    FText DisplayName;
                    if (Research->GetNextUnlockableTechId(Cost, DisplayName) != NAME_None)
                    {
                        return FText::FromString(FString::Printf(TEXT("Open research (%d RP, next: %s) [E]"),
                            Research->GetResearchPoints(), *DisplayName.ToString()));
                    }
                    return FText::FromString(FString::Printf(TEXT("Research Desk — %d RP (nothing unlockable) [E]"),
                        Research->GetResearchPoints()));
                }
            }
        }
    }

    return FText::FromString(FString::Printf(TEXT("%s"), *Def->DisplayName.ToString()));
}

void AAstrawildBuildingActor::Interact_Implementation(AActor* InteractingActor)
{
    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    UWorld* World = GetWorld();
    if (!Def || !World)
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    // Final Run (FR-9): door toggle (server-authoritative, one-way flip).
    if (Def->Category == EAstrawildBuildingCategory::Door)
    {
        if (GetLocalRole() == ROLE_Authority)
        {
            bIsOpen = !bIsOpen;
            ApplyDoorVisualState();
            UE_LOG(LogAstrawildBuilding, Log, TEXT("Door %s toggled %s."),
                *BuildingId.ToString(), bIsOpen ? TEXT("open") : TEXT("closed"));
        }
        return;
    }

    // Final Run (FR-9): storage crate transfer (deposit-first, withdraw when
    // nothing depositable remains or the crate is full).
    if (Def->Category == EAstrawildBuildingCategory::Storage)
    {
        const FText Result = TransferStorageStack(Player);
        if (AAstrawildPlayerController* CratePC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            CratePC->Notify(Result);
        }
        return;
    }

    // Final production run: Research Desk interaction opens the research TREE screen —
    // the player picks the branch (the old auto-buy-cheapest behavior removed player
    // agency entirely). Runs wherever the interacting player's controller is local;
    // the unlock itself remains server-authoritative through TryUnlockTech.
    if (Def->Category == EAstrawildBuildingCategory::Research)
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            PC->ToggleResearchScreen();
        }
        return;
    }

    // SCP Phase 10: breeding pen — the two highest-bond party echoes + a
    // breeding cake produce one Echo Egg (species follows the dominant parent).
    if (DefinitionId == TEXT("Building_BreedingPen") && Player->InventoryComponent &&
        GetLocalRole() == ROLE_Authority)
    {
        UAstrawildEchoRosterSubsystem* Roster = World ? World->GetSubsystem<UAstrawildEchoRosterSubsystem>() : nullptr;
        const TArray<AAstrawildEchoCharacter*> Party = Roster ? Roster->GetSpawnedParty() : TArray<AAstrawildEchoCharacter*>();

        FText Result = NSLOCTEXT("ASTRAWILD", "BreedNeedTwo", "Breeding Pen: needs two bonded Echoes and a Breeding Cake.");

        if (Party.Num() >= 2 && Player->InventoryComponent->HasItem(TEXT("Item_BreedingCake"), 1))
        {
            // The two highest-bond parents sire the egg.
            AAstrawildEchoCharacter* ParentA = nullptr;
            AAstrawildEchoCharacter* ParentB = nullptr;
            for (AAstrawildEchoCharacter* Echo : Party)
            {
                if (!Echo || !Echo->bCaptured)
                {
                    continue;
                }
                if (!ParentA || Echo->Bond > ParentA->Bond)
                {
                    ParentB = ParentA;
                    ParentA = Echo;
                }
                else if (!ParentB || Echo->Bond > ParentB->Bond)
                {
                    ParentB = Echo;
                }
            }

            if (ParentA && ParentB && Player->InventoryComponent->RemoveItem(TEXT("Item_BreedingCake"), 1))
            {
                Player->InventoryComponent->AddItemSilent(TEXT("Item_EchoEgg"), 1);
                Result = FText::Format(
                    NSLOCTEXT("ASTRAWILD", "BreedEgg", "An Echo Egg formed from {0} and {1}! Incubate it at the Egg Incubator."),
                    FText::FromName(ParentA->EchoDefinition ? ParentA->EchoDefinition->DefinitionId : NAME_None),
                    FText::FromName(ParentB->EchoDefinition ? ParentB->EchoDefinition->DefinitionId : NAME_None));
                UE_LOG(LogAstrawildBuilding, Log, TEXT("Breeding: egg produced (parents %s + %s)."),
                    *GetNameSafe(ParentA), *GetNameSafe(ParentB));
            }
        }

        if (AAstrawildPlayerController* BreedPC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            BreedPC->Notify(Result);
        }
        return;
    }

    // SCP Phase 10: egg incubator — consumes an Echo Egg and hatches a new
    // party member with rolled genetics (warm hatches near fire gain a level).
    if (DefinitionId == TEXT("Building_EggIncubator") && Player->InventoryComponent &&
        GetLocalRole() == ROLE_Authority)
    {
        UAstrawildEchoRosterSubsystem* Roster = World ? World->GetSubsystem<UAstrawildEchoRosterSubsystem>() : nullptr;
        FText Result = NSLOCTEXT("ASTRAWILD", "IncubatorNeedEgg", "Egg Incubator: bring an Echo Egg.");

        if (Player->InventoryComponent->HasItem(TEXT("Item_EchoEgg"), 1) && Roster)
        {
            // Warmth: a campfire or hearth within 800cm accelerates the hatch
            // (directive: temperature-driven speed) — warm hatch = +1 level.
            bool bWarmHatch = false;
            for (TActorIterator<AAstrawildBuildingActor> WarmIt(World); WarmIt; ++WarmIt)
            {
                const AAstrawildBuildingActor* HeatSource = *WarmIt;
                if (!IsValid(HeatSource) || (HeatSource->DefinitionId != TEXT("Building_Campfire") &&
                    HeatSource->DefinitionId != TEXT("Building_Heater")))
                {
                    continue;
                }
                if (FVector::DistSquared(HeatSource->GetActorLocation(), GetActorLocation()) <= 800.0f * 800.0f)
                {
                    bWarmHatch = true;
                    break;
                }
            }

            // Species: the dominant (highest-bond) party parent's line.
            AAstrawildEchoCharacter* Template = nullptr;
            for (AAstrawildEchoCharacter* Echo : Roster->GetSpawnedParty())
            {
                if (Echo && Echo->bCaptured && IsValid(Echo->EchoDefinition) &&
                    (!Template || Echo->Bond > Template->Bond))
                {
                    Template = Echo;
                }
            }

            if (Template && Player->InventoryComponent->RemoveItem(TEXT("Item_EchoEgg"), 1))
            {
                // Roll genetics from the template's traits (self-pairing keeps
                // the 70/30 inheritance rules meaningful).
                const FAstrawildGeneticsProfile Genetics = UAstrawildGeneticsLibrary::RollOffspring(
                    Template->InstanceTraits, Template->InstanceTraits, FMath::Rand());

                // Spawn the offspring from the parent's species.
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 200.0f;
                AAstrawildEchoCharacter* Offspring = World->SpawnActor<AAstrawildEchoCharacter>(
                    AAstrawildEchoCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
                if (Offspring && Offspring->InitializeFromDefinition(Template->EchoDefinition))
                {
                    Offspring->SetInstanceTraits(Genetics.Traits);
                    // FCR-1-d fix (H-d5): the rolled IVs now PERSIST on the offspring
                    // (previously rolled then dropped — the IV layer was dead code).
                    Offspring->InstanceIVs = Genetics.IVs;
                    Offspring->Capture(10.0f);
                    Offspring->OwnerPlayerId = Player->GetFName();
                    if (bWarmHatch)
                    {
                        Offspring->Level = 2;
                    }
                    Roster->AddToRoster(Offspring);

                    Result = FText::Format(
                        NSLOCTEXT("ASTRAWILD", "IncubatorHatch", "Hatched a {0}! Traits: {1}."),
                        FText::FromName(Template->EchoDefinition->DefinitionId),
                        FText::FromString(FString::JoinBy(Genetics.Traits, TEXT(", "),
                            [](const FName& Trait) { return Trait.ToString(); })));
                }
                else
                {
                    // Fail-closed: the egg is only consumed on a successful hatch.
                    Player->InventoryComponent->AddItemSilent(TEXT("Item_EchoEgg"), 1);
                    Result = NSLOCTEXT("ASTRAWILD", "IncubatorFail", "The egg trembled but did not hatch (spawn failed — egg kept).");
                }
            }
        }

        if (AAstrawildPlayerController* IncubPC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            IncubPC->Notify(Result);
        }
        return;
    }

    // SCP Phase 8: farm plots run their crop cycle interact — plant a berry
    // seed, water with a Dew Flask, fertilize with compost, harvest mature
    // stalks. Status flows to the player through the notify line.
    if (CropComponent && DefinitionId == TEXT("Building_FarmPlot") && Player->InventoryComponent)
    {
        if (GetLocalRole() == ROLE_Authority)
        {
            FText Result = CropComponent->GetPlotStatusText();

            const FName CropSeed = CropComponent->SeedItemId;
            const EAstrawildCropState State = CropComponent->CropState;

            if (State == EAstrawildCropState::Mature)
            {
                FName HarvestedId = NAME_None;
                int32 HarvestedQuantity = 0;
                if (CropComponent->Harvest(Player->InventoryComponent, HarvestedId, HarvestedQuantity))
                {
                    Result = FText::Format(
                        NSLOCTEXT("ASTRAWILD", "CropHarvested", "Harvested {0} x{1}."),
                        FText::FromName(HarvestedId), FText::AsNumber(HarvestedQuantity));
                }
            }
            else if (State == EAstrawildCropState::Empty || State == EAstrawildCropState::Withered ||
                State == EAstrawildCropState::Harvested)
            {
                static const FName SeedId = TEXT("Item_Berry");
                if (CropComponent->PlantSeed(Player->InventoryComponent, SeedId))
                {
                    Result = NSLOCTEXT("ASTRAWILD", "CropPlanted", "Planted a Glimmer Berry seed.");
                }
            }
            else if (Player->InventoryComponent->HasItem(TEXT("Item_Compost"), 1) && !CropComponent->bFertilized)
            {
                if (Player->InventoryComponent->RemoveItem(TEXT("Item_Compost"), 1))
                {
                    CropComponent->bFertilized = true;
                    Result = NSLOCTEXT("ASTRAWILD", "CropFertilized", "Composted — the plot grows twice as fast.");
                }
            }
            else if (Player->InventoryComponent->HasItem(TEXT("Item_WaterFlask"), 1))
            {
                if (Player->InventoryComponent->RemoveItem(TEXT("Item_WaterFlask"), 1))
                {
                    CropComponent->WaterPlot();
                    Result = NSLOCTEXT("ASTRAWILD", "CropWatered", "Watered the plot (90 seconds).");
                }
            }
            else if (!CropSeed.IsNone())
            {
                Result = CropComponent->GetPlotStatusText();
            }

            if (AAstrawildPlayerController* PlotPC = Cast<AAstrawildPlayerController>(Player->GetController()))
            {
                PlotPC->Notify(Result);
            }
        }
        return;
    }

    // Final-audit M-6: Workstation-category pieces (Workbench, Campfire, Sawmill,
    // Composter, FeedTrough...) used to be inert decorations — placed, powered,
    // then nothing on interact. They now open the crafting screen, the same
    // surface the camp CraftingStation uses (F-02), so every built station is a
    // real station. The craft itself stays server-authoritative per-recipe.
    if (Def->Category == EAstrawildBuildingCategory::Workstation)
    {
        // SCP Phase 9: the Medicine Bench cures ill party echoes FIRST — one
        // Cure Tonic per cured creature (crafting screen opens after).
        if (DefinitionId == TEXT("Building_MedicineBench") && Player->InventoryComponent &&
            GetLocalRole() == ROLE_Authority)
        {
            int32 CuredCount = 0;
            if (UAstrawildEchoRosterSubsystem* Roster = World ? World->GetSubsystem<UAstrawildEchoRosterSubsystem>() : nullptr)
            {
                for (AAstrawildEchoCharacter* Echo : Roster->GetSpawnedParty())
                {
                    if (!Echo || !Echo->SanityComponent || !Echo->SanityComponent->IsIll())
                    {
                        continue;
                    }
                    if (Player->InventoryComponent->HasItem(TEXT("Item_CureTonic"), 1) &&
                        Player->InventoryComponent->RemoveItem(TEXT("Item_CureTonic"), 1))
                    {
                        if (Echo->SanityComponent->ApplyMedicine())
                        {
                            ++CuredCount;
                        }
                    }
                }
            }
            if (AAstrawildPlayerController* MedPC = Cast<AAstrawildPlayerController>(Player->GetController()))
            {
                MedPC->Notify(FText::Format(
                    NSLOCTEXT("ASTRAWILD", "MedicineBenchResult", "Medicine bench: cured {0} creature(s)."),
                    FText::AsNumber(CuredCount)));
            }
        }

        // SCP Phase 12: the Repair Bench repairs every worn piece for bench-cost
        // materials FIRST (open the crafting screen after, same as any station).
        if (DefinitionId == TEXT("Building_RepairBench") && Player->DurabilityComponent &&
            GetLocalRole() == ROLE_Authority)
        {
            const TArray<FName> Worn = Player->DurabilityComponent->GetTrackedEquippedItemIds();
            int32 RepairedCount = 0;
            for (const FName ItemId : Worn)
            {
                const FName Path = Player->DurabilityComponent->RepairItem(ItemId, /*bAtRepairBench=*/true);
                if (!Path.IsNone())
                {
                    ++RepairedCount;
                }
            }
            if (AAstrawildPlayerController* BenchPC = Cast<AAstrawildPlayerController>(Player->GetController()))
            {
                BenchPC->Notify(FText::Format(
                    NSLOCTEXT("ASTRAWILD", "RepairBenchResult", "Repaired {0} piece(s) at the bench."),
                    FText::AsNumber(RepairedCount)));
            }
        }

        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            PC->ToggleCraftingScreen();
        }
        return;
    }
}

float AAstrawildBuildingActor::GetHealthFraction() const
{
    return FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
}

void AAstrawildBuildingActor::ApplyDoorVisualState()
{
    // FR-9: garage-style slide — the panel rises into the track when open.
    // The ACTOR transform never moves, so save/load placement stays exact.
    if (DoorPanel)
    {
        DoorPanel->SetRelativeLocation(bIsOpen ? FVector(0.0f, 0.0f, 140.0f) : FVector::ZeroVector);
        DoorPanel->SetCollisionEnabled(bIsOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    }
}

void AAstrawildBuildingActor::OnRep_IsOpen()
{
    ApplyDoorVisualState();
}

FText AAstrawildBuildingActor::TransferStorageStack(AAstrawildPlayerCharacter* Player)
{
    // FR-9 crate contract (server-side only — the caller routes through the
    // server interact path):
    //   1. deposit the first inventory stack that is NOT currently equipped;
    //   2. otherwise withdraw the first stored stack back to the player.
    // One stack per press — deliberate: transfers stay legible, and weight
    // limits apply naturally through AddItem's own overflow rules.
    if (GetLocalRole() != ROLE_Authority || !Player || !Player->InventoryComponent)
    {
        return FText::FromString(TEXT("The crate is locked."));
    }

    UAstrawildInventoryComponent* Inventory = Player->InventoryComponent;

    // --- Deposit: first non-equipped, valid stack. ---
    TArray<FAstrawildItemStack> Stacks = Inventory->GetItemStacks();
    for (int32 i = 0; i < Stacks.Num(); ++i)
    {
        const FAstrawildItemStack& Stack = Stacks[i];
        if (!Stack.IsValid())
        {
            continue;
        }
        const bool bEquipped = Stack.ItemId == Inventory->EquippedItemId || Stack.ItemId == Inventory->EquippedShieldItemId ||
            Stack.ItemId == Inventory->EquippedArmorItemId || Stack.ItemId == Inventory->EquippedHelmetItemId ||
            Stack.ItemId == Inventory->EquippedExosuitItemId || Stack.ItemId == Inventory->EquippedScannerItemId;
        if (bEquipped)
        {
            continue;
        }
        if (StoredItems.Num() >= StorageCapacity)
        {
            return FText::FromString(FString::Printf(TEXT("Crate full (%d/%d)."), StoredItems.Num(), StorageCapacity));
        }
        if (Inventory->RemoveItem(Stack.ItemId, Stack.Quantity))
        {
            StoredItems.Add(Stack);
            return FText::FromString(FString::Printf(TEXT("Stored: %s x%d"),
                *Stack.ItemId.ToString(), Stack.Quantity));
        }
    }

    // --- Withdraw: first stored stack. ---
    if (StoredItems.Num() > 0)
    {
        const FAstrawildItemStack Stack = StoredItems[0];
        if (Inventory->AddItem(Stack.ItemId, Stack.Quantity))
        {
            StoredItems.RemoveAt(0);
            return FText::FromString(FString::Printf(TEXT("Took: %s x%d"),
                *Stack.ItemId.ToString(), Stack.Quantity));
        }
        return FText::FromString(TEXT("Too heavy to carry — stash something first."));
    }

    return FText::FromString(TEXT("Nothing to store."));
}

FAstrawildBuildingSaveData AAstrawildBuildingActor::ToSaveData() const
{
    FAstrawildBuildingSaveData Data;
    Data.BuildingId = BuildingId;
    Data.DefinitionId = DefinitionId;
    Data.Transform = GetActorTransform();
    Data.CurrentHealth = CurrentHealth;
    Data.StoredCharge = StoredCharge;
    Data.bIsSwitchedOn = bIsSwitchedOn;
    // Batch 2 — Item C: capture last resolved power state for save-load continuity.
    // Falls back to bIsPowered (which defaults to false) if the power subsystem is gone.
    Data.bIsPowered = bIsPowered;
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
        {
            Data.bIsPowered = Power->IsBuildingPowered(this);
        }
    }
    Data.OwnerPlayerId = OwnerPlayerId;

    // FR-2 (Final Run redo): snapshot the construction cost so a future load whose
    // definition was removed from the registry can refund the player instead of
    // eating the material (pre-V4.1 saves deserialize NAME_None/0 and the load path
    // logs the loss instead of guessing).
    if (const UAstrawildBuildingDefinition* Def = GetBuildingDefinition())
    {
        Data.RefundItemId = Def->RequiredItemId;
        Data.RefundItemCount = FMath::Max(0, Def->RequiredItemCount);
    }

    // FR-9: door state + crate contents persist (additive v5 payload fields).
    Data.bIsOpen = bIsOpen;
    Data.StoredItems = StoredItems;

    // SCP Phase 8: crop lifecycle rides the building payload (additive).
    if (CropComponent)
    {
        CropComponent->ExportForSave(Data.CropSeedId, Data.CropState, Data.CropGrowth, Data.bCropFertilized);
    }
    return Data;
}

bool AAstrawildBuildingActor::FromSaveData(const FAstrawildBuildingSaveData& Data)
{
    if (!Data.BuildingId.IsValid() || Data.DefinitionId.IsNone())
    {
        return false;
    }

    // FR-2 (Final Run redo): fail-closed. A missing definition previously
    // "restored" anyway — an invincible ghost with fallback health, unknown scale
    // and no power identity that blocked the base forever. The caller (SaveSubsystem)
    // destroys the actor and refunds the material snapshot instead.
    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (!Def)
    {
        UE_LOG(LogAstrawildBuilding, Warning, TEXT("FromSaveData: definition %s missing from registry — failing closed."),
            *Data.DefinitionId.ToString());
        return false;
    }

    BuildingId = Data.BuildingId;
    DefinitionId = Data.DefinitionId;
    SetActorTransform(Data.Transform);
    StoredCharge = (FMath::IsNaN(Data.StoredCharge) || !FMath::IsFinite(Data.StoredCharge)) ? 0.0f : Data.StoredCharge;
    bIsSwitchedOn = Data.bIsSwitchedOn;
    // Batch 2 — Item C: restore hint power state — the PowerSubsystem's ResolveGridNow()
    // (called by SaveSubsystem::LoadWorld right after the building spawn loop) will
    // overwrite this with the freshly-resolved value on the same frame.
    bIsPowered = Data.bIsPowered;
    OwnerPlayerId = Data.OwnerPlayerId;

    MaxHealth = FMath::Max(1.0f, Def->MaxHealth);
    InitializeFromDefinition(Def, OwnerPlayerId);
    // Audit H-5: apply the saved health AFTER initialization — InitializeFromDefinition
    // resets CurrentHealth to MaxHealth, which used to heal every damaged building on load.
    // FR-2: NaN health (corrupt save) falls back to full instead of poisoning the bar.
    CurrentHealth = (FMath::IsNaN(Data.CurrentHealth) || !FMath::IsFinite(Data.CurrentHealth))
        ? MaxHealth
        : FMath::Clamp(Data.CurrentHealth, 1.0f, MaxHealth);

    // FR-9: restore door state (panel position + collision) and crate contents.
    // Invalid stored stacks (corrupt save) are dropped fail-closed — a crate never
    // mints items from garbage data.
    bIsOpen = Data.bIsOpen;
    StoredItems.Reset();
    for (const FAstrawildItemStack& Stack : Data.StoredItems)
    {
        if (Stack.IsValid())
        {
            StoredItems.Add(Stack);
        }
    }

    // SCP Phase 8: crop restore (InitializeFromDefinition above already created
    // the component for farm plots; pre-SCP saves deserialize as empty plots).
    if (CropComponent)
    {
        CropComponent->ImportFromSave(Data.CropSeedId, Data.CropState, Data.CropGrowth, Data.bCropFertilized);
    }
    ApplyDoorVisualState();

    UpdateVisualPowerState();
    return true;
}
