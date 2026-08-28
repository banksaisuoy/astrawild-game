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
    // Preflight all failure-prone conditions before consuming inventory. In particular,
    // ConsumeFood returns false when hunger is already full; removing first would lose food.
    if (!Definition || !Inventory || Definition->HungerRestored <= 0.0f ||
        CurrentHunger >= MaxHunger - KINDA_SMALL_NUMBER || !Inventory->HasItem(FoodTag, 1))
    {
        return false;
    }
    if (!Inventory->RemoveItem(FoodTag, 1))
    {
        return false;
    }
    if (!ConsumeFood(Definition->HungerRestored))
    {
        Inventory->AddItem(FoodTag, 1);
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
    FAstrawildCookingRecipeRow** Found = Rows.FindByPredicate([RecipeTag](const FAstrawildCookingRecipeRow* Row)
    {
        return Row && Row->RecipeTag == RecipeTag;
    });
    return Found ? *Found : nullptr;
}

const FAstrawildCookingRecipeRow* UAstrawildSurvivalComponent::FindFoodDefinition(const FGameplayTag& FoodItemTag) const
{
    if (!CookingRecipeTable || !FoodItemTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildCookingRecipeRow*> Rows;
    CookingRecipeTable->GetAllRows<FAstrawildCookingRecipeRow>(TEXT("AstrawildFoodLookup"), Rows);
    FAstrawildCookingRecipeRow** Found = Rows.FindByPredicate([FoodItemTag](const FAstrawildCookingRecipeRow* Row)
    {
        return Row && Row->OutputItemTag == FoodItemTag;
    });
    return Found ? *Found : nullptr;
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
    OutProfile.TrackedFood.Reset();
    for (const FAstrawildTrackedFoodState& State : TrackedFood)
    {
        FAstrawildFoodSaveState& SavedState = OutProfile.TrackedFood.AddDefaulted_GetRef();
        SavedState.ItemInstanceId = State.ItemInstanceId;
        SavedState.FoodItemTag = State.FoodItemTag;
        SavedState.RemainingQuantity = State.RemainingQuantity;
        SavedState.RemainingFreshnessSeconds = State.RemainingFreshnessSeconds;
        SavedState.bRefrigerated = State.bRefrigerated;
    }
    OutProfile.ActiveFoodBuffs.Reset();
    for (const FAstrawildActiveFoodBuff& Buff : ActiveFoodBuffs)
    {
        FAstrawildFoodBuffSaveState& SavedBuff = OutProfile.ActiveFoodBuffs.AddDefaulted_GetRef();
        SavedBuff.BuffTag = Buff.BuffTag;
        SavedBuff.Magnitude = Buff.Magnitude;
        SavedBuff.RemainingDurationSeconds = Buff.RemainingDurationSeconds;
    }
    OutProfile.bFoodStorageRefrigerated = bFoodStorageRefrigerated;
}

void UAstrawildSurvivalComponent::ImportFromProfile(const FAstrawildPlayerProfile& InProfile)
{
    CurrentHunger = FMath::Clamp(InProfile.Hunger, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(InProfile.Thirst, 0.0f, MaxThirst);
    CurrentTemperature = InProfile.BodyTemperature;
    CarryWeight = FMath::Max(0.0f, InProfile.CarryWeight);
    TrackedFood.Reset();
    for (const FAstrawildFoodSaveState& SavedState : InProfile.TrackedFood)
    {
        if (!SavedState.FoodItemTag.IsValid() || SavedState.RemainingQuantity <= 0)
        {
            continue;
        }
        FAstrawildTrackedFoodState& State = TrackedFood.AddDefaulted_GetRef();
        State.ItemInstanceId = SavedState.ItemInstanceId;
        State.FoodItemTag = SavedState.FoodItemTag;
        State.RemainingQuantity = FMath::Max(1, SavedState.RemainingQuantity);
        State.RemainingFreshnessSeconds = FMath::Max(0.0f, SavedState.RemainingFreshnessSeconds);
        State.bRefrigerated = SavedState.bRefrigerated;
    }
    ActiveFoodBuffs.Reset();
    for (const FAstrawildFoodBuffSaveState& SavedBuff : InProfile.ActiveFoodBuffs)
    {
        if (!SavedBuff.BuffTag.IsValid() || SavedBuff.RemainingDurationSeconds <= 0.0f)
        {
            continue;
        }
        FAstrawildActiveFoodBuff& Buff = ActiveFoodBuffs.AddDefaulted_GetRef();
        Buff.BuffTag = SavedBuff.BuffTag;
        Buff.Magnitude = SavedBuff.Magnitude;
        Buff.RemainingDurationSeconds = SavedBuff.RemainingDurationSeconds;
    }
    bFoodStorageRefrigerated = InProfile.bFoodStorageRefrigerated;
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
