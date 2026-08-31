#include "AstrawildBuildingActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildPowerSubsystem.h"
#include "AstrawildResearchSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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
}

void AAstrawildBuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsSwitchedOn);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsPowered);
    DOREPLIFETIME(AAstrawildBuildingActor, CurrentHealth);
    DOREPLIFETIME(AAstrawildBuildingActor, StoredCharge);
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
        default:
            VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 1.0f));
            break;
        }
    }

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
    // Final production run: Research Desk interaction opens the research TREE screen —
    // the player picks the branch (the old auto-buy-cheapest behavior removed player
    // agency entirely). Runs wherever the interacting player's controller is local;
    // the unlock itself remains server-authoritative through TryUnlockTech.
    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    UWorld* World = GetWorld();
    if (!Def || Def->Category != EAstrawildBuildingCategory::Research || !World)
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        PC->ToggleResearchScreen();
    }
}

float AAstrawildBuildingActor::GetHealthFraction() const
{
    return FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
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
    return Data;
}

bool AAstrawildBuildingActor::FromSaveData(const FAstrawildBuildingSaveData& Data)
{
    if (!Data.BuildingId.IsValid() || Data.DefinitionId.IsNone())
    {
        return false;
    }

    BuildingId = Data.BuildingId;
    DefinitionId = Data.DefinitionId;
    SetActorTransform(Data.Transform);
    StoredCharge = Data.StoredCharge;
    bIsSwitchedOn = Data.bIsSwitchedOn;
    // Batch 2 — Item C: restore hint power state — the PowerSubsystem's ResolveGridNow()
    // (called by SaveSubsystem::LoadWorld right after the building spawn loop) will
    // overwrite this with the freshly-resolved value on the same frame.
    bIsPowered = Data.bIsPowered;
    OwnerPlayerId = Data.OwnerPlayerId;

    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (Def)
    {
        MaxHealth = FMath::Max(1.0f, Def->MaxHealth);
        InitializeFromDefinition(Def, OwnerPlayerId);
        // Audit H-5: apply the saved health AFTER initialization — InitializeFromDefinition
        // resets CurrentHealth to MaxHealth, which used to heal every damaged building on load.
        CurrentHealth = FMath::Clamp(Data.CurrentHealth, 1.0f, MaxHealth);
    }
    else
    {
        CurrentHealth = FMath::Max(1.0f, Data.CurrentHealth);
    }
    UpdateVisualPowerState();
    return true;
}
