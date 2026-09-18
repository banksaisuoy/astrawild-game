#include "AstrawildBaseTerminalActor.h"

#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AAstrawildBaseTerminalActor::AAstrawildBaseTerminalActor()
{
    // Territory upkeep runs at a coarse cadence (level + decay, not per-frame).
    PrimaryActorTick.TickInterval = 5.0f;
}

void AAstrawildBaseTerminalActor::BeginPlay()
{
    Super::BeginPlay();

    RefreshBaseLevel();

    // Territory beacon: the indicator light carries the claim identity.
    if (PowerIndicatorLight)
    {
        PowerIndicatorLight->SetLightColor(FLinearColor(0.05f, 0.55f, 0.45f));
        PowerIndicatorLight->SetIntensity(6.0f);
    }
}

AAstrawildBaseTerminalActor* AAstrawildBaseTerminalActor::FindNearestTerminal(const UWorld* World, const FVector& Location)
{
    if (!World)
    {
        return nullptr;
    }

    AAstrawildBaseTerminalActor* Best = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (TActorIterator<AAstrawildBaseTerminalActor> It(const_cast<UWorld*>(World)); It; ++It)
    {
        AAstrawildBaseTerminalActor* Terminal = *It;
        if (!IsValid(Terminal) || Terminal->IsDestroyed())
        {
            continue;
        }
        const float DistanceSquared = FVector::DistSquared(Terminal->GetActorLocation(), Location);
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Best = Terminal;
        }
    }

    return Best;
}

bool AAstrawildBaseTerminalActor::IsInsideTerritory(const UWorld* World, const FVector& Location)
{
    const AAstrawildBaseTerminalActor* Terminal = FindNearestTerminal(World, Location);
    if (!Terminal)
    {
        return false;
    }
    return FVector::DistSquared(Terminal->GetActorLocation(), Location) <=
        TerritoryRadius * TerritoryRadius;
}

bool AAstrawildBaseTerminalActor::IsPlacementAllowed(const UWorld* World, const FVector& Location)
{
    if (!World)
    {
        return false;
    }

    // No terminal anywhere yet: open-world placement (early game flow) —
    // the nearest-terminal query returning null encodes "no claims exist".
    const AAstrawildBaseTerminalActor* Nearest = FindNearestTerminal(World, Location);
    if (!Nearest)
    {
        return true;
    }

    return FVector::DistSquared(Nearest->GetActorLocation(), Location) <=
        TerritoryRadius * TerritoryRadius;
}

int32 AAstrawildBaseTerminalActor::ComputeBaseLevel(int32 BuildingCountInRadius)
{
    if (BuildingCountInRadius >= 16)
    {
        return 3;
    }
    if (BuildingCountInRadius >= 8)
    {
        return 2;
    }
    return 1;
}

int32 AAstrawildBaseTerminalActor::GetGarrisonCapForLevel(int32 Level)
{
    switch (Level)
    {
    case 2:
        return 10;
    case 3:
        return 20;
    default:
        return 5;
    }
}

int32 AAstrawildBaseTerminalActor::CountBuildingsInRadius() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0;
    }

    int32 Count = 0;
    for (TActorIterator<AAstrawildBuildingActor> It(const_cast<UWorld*>(World)); It; ++It)
    {
        const AAstrawildBuildingActor* Building = *It;
        if (!IsValid(Building) || Building->IsDestroyed())
        {
            continue;
        }
        // Terminals themselves do not count toward their own level.
        if (Building->DefinitionId != TEXT("Building_BaseTerminal") &&
            FVector::DistSquared(Building->GetActorLocation(), GetActorLocation()) <=
            TerritoryRadius * TerritoryRadius)
        {
            ++Count;
        }
    }
    return Count;
}

void AAstrawildBaseTerminalActor::RefreshBaseLevel()
{
    const int32 NewLevel = ComputeBaseLevel(CountBuildingsInRadius());
    if (NewLevel != BaseLevel)
    {
        BaseLevel = NewLevel;
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Base Terminal %s reached level %d (cap %d)"),
            *BuildingId.ToString(), BaseLevel, GetGarrisonCap());
    }
}

void AAstrawildBaseTerminalActor::ApplyTerritoryDecay()
{
    const UWorld* World = GetWorld();
    if (!World || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Buildings standing outside EVERY territory slowly weather away —
    // the directive's "structures outside the claim decay" rule.
    for (TActorIterator<AAstrawildBuildingActor> It(World); It; ++It)
    {
        AAstrawildBuildingActor* Building = *It;
        if (!IsValid(Building) || Building->IsDestroyed())
        {
            continue;
        }
        if (Building == this)
        {
            continue;
        }
        if (IsInsideTerritory(World, Building->GetActorLocation()))
        {
            continue;
        }
        Building->ApplyBuildingDamage(OutOfTerritoryDecayPerMinute);
    }
}

void AAstrawildBaseTerminalActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    RefreshBaseLevel();

    DecayAccumulator += DeltaTime;
    if (DecayAccumulator >= 60.0f)
    {
        DecayAccumulator = 0.0f;
        ApplyTerritoryDecay();
    }
}

void AAstrawildBaseTerminalActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Garrison status: how many base-assigned echoes this player runs vs. cap.
    int32 GarrisonCount = 0;
    if (const UAstrawildEchoRosterSubsystem* Roster = World->GetSubsystem<UAstrawildEchoRosterSubsystem>())
    {
        // The roster owns every captured echo; base garrison counts the ones
        // currently commanded to WORK (the base-assigned pool).
        GarrisonCount = Roster->GetBaseGarrisonCount();
    }

    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        const int32 Buildings = CountBuildingsInRadius();
        const int32 NextLevelBuildings = BaseLevel == 1 ? 8 : 16;
        PC->Notify(FText::Format(
            NSLOCTEXT("ASTRAWILD", "BaseTerminalStatus",
                "Base Terminal — Level {0} | Garrison {1}/{2} | {3} building(s) in range (next level at {4})"),
            FText::AsNumber(BaseLevel),
            FText::AsNumber(GarrisonCount),
            FText::AsNumber(GetGarrisonCap()),
            FText::AsNumber(Buildings),
            FText::AsNumber(NextLevelBuildings)));
    }
}
