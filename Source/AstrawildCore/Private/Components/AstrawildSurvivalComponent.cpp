#include "Components/AstrawildSurvivalComponent.h"

#include "Math/UnrealMathUtility.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Engine/DataTable.h"

UAstrawildSurvivalComponent::UAstrawildSurvivalComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildSurvivalComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHunger = FMath::Clamp(CurrentHunger, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(CurrentThirst, 0.0f, MaxThirst);
    CurrentTemperature = ComfortableTemperature;
}

void UAstrawildSurvivalComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float HungerDelta = -(HungerDrainPerMinute / 60.0f) * DeltaTime;
    const float ThirstDelta = -(ThirstDrainPerMinute / 60.0f) * DeltaTime;
    CurrentHunger = FMath::Clamp(CurrentHunger + HungerDelta, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(CurrentThirst + ThirstDelta, 0.0f, MaxThirst);
    OnHungerChanged.Broadcast(CurrentHunger, MaxHunger, HungerDelta);
    OnThirstChanged.Broadcast(CurrentThirst, MaxThirst, ThirstDelta);

    WarningCooldownRemaining = FMath::Max(0.0f, WarningCooldownRemaining - DeltaTime);
    BroadcastWarnings();
    if (HasAuthorityForSurvival())
    {
        AdvanceFoodSystems(DeltaTime);
    }
}

bool UAstrawildSurvivalComponent::ConsumeFood(const float HungerRestored)
{
    if (HungerRestored <= 0.0f)
    {
        return false;
    }

    const float OldValue = CurrentHunger;
    CurrentHunger = FMath::Clamp(CurrentHunger + HungerRestored, 0.0f, MaxHunger);
    OnHungerChanged.Broadcast(CurrentHunger, MaxHunger, CurrentHunger - OldValue);
    return !FMath::IsNearlyEqual(OldValue, CurrentHunger);
}

bool UAstrawildSurvivalComponent::DrinkWater(const float ThirstRestored)
{
    if (ThirstRestored <= 0.0f)
    {
        return false;
    }

    const float OldValue = CurrentThirst;
    CurrentThirst = FMath::Clamp(CurrentThirst + ThirstRestored, 0.0f, MaxThirst);
    OnThirstChanged.Broadcast(CurrentThirst, MaxThirst, CurrentThirst - OldValue);
    return !FMath::IsNearlyEqual(OldValue, CurrentThirst);
}

void UAstrawildSurvivalComponent::SetTemperature(const float NewTemperature)
{
    const float OldValue = CurrentTemperature;
    CurrentTemperature = NewTemperature;
    OnTemperatureChanged.Broadcast(CurrentTemperature, ComfortableTemperature, CurrentTemperature - OldValue);
    BroadcastWarnings();
}

void UAstrawildSurvivalComponent::SetCarryWeight(const float NewWeight)
{
    CarryWeight = FMath::Max(0.0f, NewWeight);
    if (IsOverburdened())
    {
        OnSurvivalWarning.Broadcast(TEXT("Overburdened"));
    }
}

bool UAstrawildSurvivalComponent::RegisterFoodItem(const FAstrawildItemSlot& FoodSlot, const bool bRefrigerated)
{
    if (!HasAuthorityForSurvival() || !FoodSlot.IsValid())
    {
        return false;
    }
    const FAstrawildCookingRecipeRow* Definition = FindFoodDefinition(FoodSlot.ItemTag);
    if (!Definition)
    {
        return false;
    }
    FAstrawildTrackedFoodState State;
    State.ItemInstanceId = FoodSlot.InstanceId;
    State.FoodItemTag = FoodSlot.ItemTag;
    State.RemainingQuantity = FMath::Max(1, FoodSlot.Quantity);
    State.RemainingFreshnessSeconds = Definition->SpoilageDurationSeconds;
    State.bRefrigerated = bRefrigerated || bFoodStorageRefrigerated;
    TrackedFood.RemoveAll([&State](const FAstrawildTrackedFoodState& Existing)
    {
        return Existing.ItemInstanceId == State.ItemInstanceId;
    });
    TrackedFood.Add(State);
    return true;
}

void UAstrawildSurvivalComponent::SetFoodStorageRefrigerated(const bool bEnabled)
{
    if (!HasAuthorityForSurvival())
    {
        return;
    }
    bFoodStorageRefrigerated = bEnabled;
    for (FAstrawildTrackedFoodState& State : TrackedFood)
    {
        State.bRefrigerated = bEnabled;
    }
}

bool UAstrawildSurvivalComponent::ConsumeRegisteredFood(const FGuid& ItemInstanceId)
{
    if (!HasAuthorityForSurvival())
    {
        return false;
    }
    const int32 Index = TrackedFood.IndexOfByPredicate([&ItemInstanceId](const FAstrawildTrackedFoodState& State)
    {
        return State.ItemInstanceId == ItemInstanceId;
    });
    if (Index == INDEX_NONE)
    {
        return false;
    }
    const FGameplayTag FoodTag = TrackedFood[Index].FoodItemTag;
    const FAstrawildCookingRecipeRow* Definition = FindFoodDefinition(FoodTag);
    UAstrawildInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    if (!Definition || !Inventory || !Inventory->RemoveItem(FoodTag, 1) || !ConsumeFood(Definition->HungerRestored))
    {
        return false;
    }
    if (Definition->ThirstRestored > 0.0f)
    {
        DrinkWater(Definition->ThirstRestored);
    }
    if (Definition->BuffTag.IsValid() && Definition->BuffDurationSeconds > 0.0f)
    {
        FAstrawildActiveFoodBuff Buff;
        Buff.BuffTag = Definition->BuffTag;
        Buff.Magnitude = Definition->BuffMagnitude;
        Buff.RemainingDurationSeconds = Definition->BuffDurationSeconds;
        ActiveFoodBuffs.Add(Buff);
    }
    TrackedFood[Index].RemainingQuantity = FMath::Max(0, TrackedFood[Index].RemainingQuantity - 1);
    if (TrackedFood[Index].RemainingQuantity <= 0)
    {
        TrackedFood.RemoveAt(Index);
    }
    return true;
}

float UAstrawildSurvivalComponent::GetFoodFreshnessNormalized(const FGuid& ItemInstanceId) const
{
    for (const FAstrawildTrackedFoodState& State : TrackedFood)
    {
        if (State.ItemInstanceId == ItemInstanceId)
        {
            if (const FAstrawildCookingRecipeRow* Definition = FindFoodDefinition(State.FoodItemTag))
            {
                return Definition->SpoilageDurationSeconds > 0.0f ? FMath::Clamp(State.RemainingFreshnessSeconds / Definition->SpoilageDurationSeconds, 0.0f, 1.0f) : 0.0f;
            }
        }
    }
    return 0.0f;
}

float UAstrawildSurvivalComponent::GetActiveFoodBuffMagnitude(const FGameplayTag BuffTag) const
{
    float total = 0.0f;
    for (const FAstrawildActiveFoodBuff& Buff : ActiveFoodBuffs)
    {
        if (Buff.BuffTag == BuffTag && Buff.RemainingDurationSeconds > 0.0f)
        {
            total += Buff.Magnitude;
        }
    }
    return total;
}

void UAstrawildSurvivalComponent::AdvanceFoodSystems(const float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }
    for (int32 Index = ActiveFoodBuffs.Num() - 1; Index >= 0; --Index)
    {
        ActiveFoodBuffs[Index].RemainingDurationSeconds = FMath::Max(0.0f, ActiveFoodBuffs[Index].RemainingDurationSeconds - DeltaSeconds);
        if (ActiveFoodBuffs[Index].RemainingDurationSeconds <= 0.0f)
        {
            ActiveFoodBuffs.RemoveAt(Index);
        }
    }

    UAstrawildInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    for (int32 Index = TrackedFood.Num() - 1; Index >= 0; --Index)
    {
        FAstrawildTrackedFoodState& State = TrackedFood[Index];
        const FAstrawildCookingRecipeRow* Definition = FindFoodDefinition(State.FoodItemTag);
        if (!Definition)
        {
            TrackedFood.RemoveAt(Index);
            continue;
        }
        const float Rate = State.bRefrigerated ? FMath::Clamp(Definition->RefrigeratedSpoilageRate, 0.0f, 1.0f) : 1.0f;
        State.RemainingFreshnessSeconds = FMath::Max(0.0f, State.RemainingFreshnessSeconds - DeltaSeconds * Rate);
        if (State.RemainingFreshnessSeconds <= 0.0f)
        {
            if (Inventory)
            {
                Inventory->RemoveItem(State.FoodItemTag, State.RemainingQuantity);
                Inventory->AddItem(FGameplayTag::RequestGameplayTag(FName(TEXT("Item.Material.Fertilizer")), false), State.RemainingQuantity);
            }
            OnSurvivalWarning.Broadcast(TEXT("FoodDecayed"));
            TrackedFood.RemoveAt(Index);
        }
    }
}

const FAstrawildCookingRecipeRow* UAstrawildSurvivalComponent::FindCookingRecipe(const FGameplayTag& RecipeTag) const
{
    if (!CookingRecipeTable || !RecipeTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildCookingRecipeRow*> Rows;
    CookingRecipeTable->GetAllRows<FAstrawildCookingRecipeRow>(TEXT("AstrawildCookingLookup"), Rows);
    return Rows.FindByPredicate([RecipeTag](const FAstrawildCookingRecipeRow* Row)
    {
        return Row && Row->RecipeTag == RecipeTag;
    });
}

const FAstrawildCookingRecipeRow* UAstrawildSurvivalComponent::FindFoodDefinition(const FGameplayTag& FoodItemTag) const
{
    if (!CookingRecipeTable || !FoodItemTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildCookingRecipeRow*> Rows;
    CookingRecipeTable->GetAllRows<FAstrawildCookingRecipeRow>(TEXT("AstrawildFoodLookup"), Rows);
    return Rows.FindByPredicate([FoodItemTag](const FAstrawildCookingRecipeRow* Row)
    {
        return Row && Row->OutputItemTag == FoodItemTag;
    });
}

bool UAstrawildSurvivalComponent::HasAuthorityForSurvival() const
{
    return !GetOwner() || GetOwner()->HasAuthority();
}

float UAstrawildSurvivalComponent::GetHungerPercent() const
{
    return MaxHunger > 0.0f ? CurrentHunger / MaxHunger : 0.0f;
}

float UAstrawildSurvivalComponent::GetThirstPercent() const
{
    return MaxThirst > 0.0f ? CurrentThirst / MaxThirst : 0.0f;
}

float UAstrawildSurvivalComponent::GetTemperatureStress() const
{
    return TemperatureTolerance > 0.0f ? FMath::Clamp(FMath::Abs(CurrentTemperature - ComfortableTemperature) / TemperatureTolerance, 0.0f, 1.0f) : 1.0f;
}

bool UAstrawildSurvivalComponent::IsOverburdened() const
{
    return CarryWeight > CarryWeightCapacity;
}

void UAstrawildSurvivalComponent::ExportToProfile(FAstrawildPlayerProfile& OutProfile) const
{
    OutProfile.Hunger = CurrentHunger;
    OutProfile.Thirst = CurrentThirst;
    OutProfile.BodyTemperature = CurrentTemperature;
    OutProfile.CarryWeight = CarryWeight;
}

void UAstrawildSurvivalComponent::ImportFromProfile(const FAstrawildPlayerProfile& InProfile)
{
    CurrentHunger = FMath::Clamp(InProfile.Hunger, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(InProfile.Thirst, 0.0f, MaxThirst);
    CurrentTemperature = InProfile.BodyTemperature;
    CarryWeight = FMath::Max(0.0f, InProfile.CarryWeight);
}

void UAstrawildSurvivalComponent::BroadcastWarnings()
{
    if (WarningCooldownRemaining > 0.0f)
    {
        return;
    }

    if (GetHungerPercent() <= 0.15f)
    {
        OnSurvivalWarning.Broadcast(TEXT("LowHunger"));
        WarningCooldownRemaining = 5.0f;
    }
    else if (GetThirstPercent() <= 0.15f)
    {
        OnSurvivalWarning.Broadcast(TEXT("LowThirst"));
        WarningCooldownRemaining = 5.0f;
    }
    else if (GetTemperatureStress() >= 0.85f)
    {
        OnSurvivalWarning.Broadcast(TEXT("TemperatureStress"));
        WarningCooldownRemaining = 5.0f;
    }
}
