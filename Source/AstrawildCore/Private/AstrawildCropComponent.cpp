#include "AstrawildCropComponent.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"

UAstrawildCropComponent::UAstrawildCropComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
}

bool UAstrawildCropComponent::IsAuthority() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    const UWorld* World = Owner->GetWorld();
    if (!World)
    {
        return true; // world-free test object — drive the same math locally.
    }
    return Owner->GetLocalRole() == ROLE_Authority;
}

bool UAstrawildCropComponent::IsRaining() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    const UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>();
    if (!Weather)
    {
        return false;
    }
    const EAstrawildWeatherState State = Weather->GetCurrentWeather();
    return State == EAstrawildWeatherState::Rain || State == EAstrawildWeatherState::HeavyRain ||
        State == EAstrawildWeatherState::Storm;
}

int32 UAstrawildCropComponent::GetCurrentMonth() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0;
    }
    const UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>();
    if (!Time)
    {
        return 0;
    }
    // 1 game day per 24 real minutes; a "month" is 7 game days, giving a full
    // four-season year across a long session (directive 4 seasons).
    return (Time->GetCurrentDay() / 7) % 4;
}

float UAstrawildCropComponent::ComputeGrowthStep(float DeltaSeconds, bool bWatered, bool bFertilized, float SeasonMultiplier)
{
    // Dry soil halves growth; compost doubles it; the season scales the base.
    // (Multiplier stacking is multiplicative, clamped to a sane band.)
    const float WaterMultiplier = bWatered ? 1.0f : 0.5f;
    const float FertilizerMultiplier = bFertilized ? 2.0f : 1.0f;
    const float SafeSeason = FMath::Clamp(SeasonMultiplier, 0.25f, 2.0f);
    const float Step = (DeltaSeconds / BaseGrowthSeconds) * WaterMultiplier * FertilizerMultiplier * SafeSeason;
    return FMath::Clamp(Step, 0.0f, 1.0f);
}

float UAstrawildCropComponent::ComputeSeasonMultiplier(int32 MonthIndex)
{
    // Four seasons: spring 1.25, summer 1.0, autumn 0.85, winter 0.5.
    switch (MonthIndex % 4)
    {
    case 0:
        return 1.25f;
    case 2:
        return 0.85f;
    case 3:
        return 0.5f;
    default:
        return 1.0f;
    }
}

EAstrawildCropState UAstrawildCropComponent::ResolveStateFromProgress(float Progress)
{
    if (Progress >= 1.0f)
    {
        return EAstrawildCropState::Mature;
    }
    if (Progress >= 0.66f)
    {
        return EAstrawildCropState::Young;
    }
    if (Progress >= 0.25f)
    {
        return EAstrawildCropState::Sprout;
    }
    if (Progress > 0.0f)
    {
        return EAstrawildCropState::Planted;
    }
    return EAstrawildCropState::Empty;
}

void UAstrawildCropComponent::RefreshWatering()
{
    // Rain refreshes a 90-second watering window; the player can also water by
    // using a Dew Flask while looking at the plot (interact branch).
    if (IsRaining())
    {
        WateredSecondsRemaining = 90.0f;
    }
    else
    {
        WateredSecondsRemaining = FMath::Max(0.0f, WateredSecondsRemaining - 1.0f);
    }
}

void UAstrawildCropComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsAuthority())
    {
        return;
    }

    RefreshWatering();

    if (CropState == EAstrawildCropState::Empty || CropState == EAstrawildCropState::Withered)
    {
        return;
    }

    if (CropState == EAstrawildCropState::Mature)
    {
        // Wither clock: harvest before the crop spoils on the stalk.
        MatureSeconds += DeltaTime;
        if (MatureSeconds >= WitherSeconds)
        {
            CropState = EAstrawildCropState::Withered;
            UE_LOG(LogAstrawildBuilding, Log, TEXT("Crop: %s withered on the stalk"), *SeedItemId.ToString());
        }
        return;
    }

    // Growing states advance.
    const float Step = ComputeGrowthStep(DeltaTime, WateredSecondsRemaining > 0.0f, bFertilized,
        ComputeSeasonMultiplier(GetCurrentMonth()));
    GrowthProgress = FMath::Clamp(GrowthProgress + Step, 0.0f, 1.0f);
    CropState = ResolveStateFromProgress(GrowthProgress);

    if (CropState == EAstrawildCropState::Mature)
    {
        MatureSeconds = 0.0f;
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Crop: %s matured (fertilized: %s)"),
            *SeedItemId.ToString(), bFertilized ? TEXT("yes") : TEXT("no"));
    }
}

bool UAstrawildCropComponent::PlantSeed(UAstrawildInventoryComponent* Inventory, FName SeedId)
{
    if (!Inventory || SeedId.IsNone())
    {
        return false;
    }
    if (CropState != EAstrawildCropState::Empty && CropState != EAstrawildCropState::Withered &&
        CropState != EAstrawildCropState::Harvested)
    {
        return false;
    }
    if (!Inventory->RemoveItem(SeedId, 1))
    {
        return false;
    }

    SeedItemId = SeedId;
    GrowthProgress = 0.0f;
    bFertilized = false;
    MatureSeconds = 0.0f;
    CropState = EAstrawildCropState::Planted;
    return true;
}

bool UAstrawildCropComponent::Harvest(UAstrawildInventoryComponent* Inventory, FName& OutItemId, int32& OutQuantity)
{
    OutItemId = NAME_None;
    OutQuantity = 0;

    if (!Inventory || CropState != EAstrawildCropState::Mature || SeedItemId.IsNone())
    {
        return false;
    }

    // Yield: 4x the seed spent (fertilized plots pay 6x — the compost loop
    // has a real payoff).
    const int32 Yield = bFertilized ? 6 : 4;
    if (!Inventory->AddItem(SeedItemId, Yield))
    {
        return false;
    }

    OutItemId = SeedItemId;
    OutQuantity = Yield;
    CropState = EAstrawildCropState::Harvested;
    GrowthProgress = 0.0f;
    bFertilized = false;
    MatureSeconds = 0.0f;
    return true;
}

FText UAstrawildCropComponent::GetPlotStatusText() const
{
    switch (CropState)
    {
    case EAstrawildCropState::Empty:
        return NSLOCTEXT("ASTRAWILD", "CropEmpty", "Empty plot — E plants a Glimmer Berry seed");
    case EAstrawildCropState::Planted:
    case EAstrawildCropState::Sprout:
    case EAstrawildCropState::Young:
        return NSLOCTEXT("ASTRAWILD", "CropGrowing", "Growing — rain or a Dew Flask waters it");
    case EAstrawildCropState::Mature:
        return NSLOCTEXT("ASTRAWILD", "CropMature", "Mature — E harvests");
    case EAstrawildCropState::Harvested:
        return NSLOCTEXT("ASTRAWILD", "CropHarvested", "Harvested — E replants");
    case EAstrawildCropState::Withered:
    default:
        return NSLOCTEXT("ASTRAWILD", "CropWithered", "Withered — E clears and replants");
    }
}

void UAstrawildCropComponent::ExportForSave(FName& OutSeedId, uint8& OutState, float& OutProgress, bool& OutFertilized) const
{
    OutSeedId = SeedItemId;
    OutState = static_cast<uint8>(CropState);
    OutProgress = FMath::Clamp(GrowthProgress, 0.0f, 1.0f);
    OutFertilized = bFertilized;
}

void UAstrawildCropComponent::ImportFromSave(FName InSeedId, uint8 InState, float InProgress, bool bInFertilized)
{
    SeedItemId = InSeedId;

    // Sanitize the state byte: only the seven ladder values survive.
    if (InState <= static_cast<uint8>(EAstrawildCropState::Withered))
    {
        CropState = static_cast<EAstrawildCropState>(InState);
    }
    else
    {
        CropState = EAstrawildCropState::Empty;
        SeedItemId = NAME_None;
    }

    GrowthProgress = FMath::IsFinite(InProgress) ? FMath::Clamp(InProgress, 0.0f, 1.0f) : 0.0f;
    bFertilized = bInFertilized;

    // An empty plot with a seed id (corrupt combo) drops the seed id.
    if (CropState == EAstrawildCropState::Empty)
    {
        SeedItemId = NAME_None;
        GrowthProgress = 0.0f;
        bFertilized = false;
    }

    MatureSeconds = 0.0f;
}
