#include "AstrawildBuildingActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPowerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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

    BuildingId = FGuid::NewGuid();
}

void AAstrawildBuildingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildBuildingActor, bIsSwitchedOn);
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
    return true;
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
    CurrentHealth = Data.CurrentHealth;
    StoredCharge = Data.StoredCharge;
    bIsSwitchedOn = Data.bIsSwitchedOn;
    OwnerPlayerId = Data.OwnerPlayerId;

    const UAstrawildBuildingDefinition* Def = GetBuildingDefinition();
    if (Def)
    {
        MaxHealth = FMath::Max(1.0f, Def->MaxHealth);
        InitializeFromDefinition(Def, OwnerPlayerId);
    }
    return true;
}
