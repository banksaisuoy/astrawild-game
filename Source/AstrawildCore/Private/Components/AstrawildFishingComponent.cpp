#include "Components/AstrawildFishingComponent.h"

#include "Components/AstrawildInventoryComponent.h"
#include "Data/AstrawildFishingData.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAstrawildFishingComponent::UAstrawildFishingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildFishingComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAstrawildFishingComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (HasAuthorityForFishing() && bFishingActive && Tension >= 100.0f)
    {
        SetFishingResult(EAstrawildFishingResult::LineBroken);
    }
}

void UAstrawildFishingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildFishingComponent, bFishingActive);
    DOREPLIFETIME(UAstrawildFishingComponent, ActiveFishTag);
    DOREPLIFETIME(UAstrawildFishingComponent, ActiveCatchItemTag);
    DOREPLIFETIME(UAstrawildFishingComponent, ActiveFishSellPrice);
    DOREPLIFETIME(UAstrawildFishingComponent, Tension);
    DOREPLIFETIME(UAstrawildFishingComponent, ReelProgressNormalized);
    DOREPLIFETIME(UAstrawildFishingComponent, FishingResult);
}

bool UAstrawildFishingComponent::HasAuthorityForFishing() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor && OwnerActor->HasAuthority();
}

bool UAstrawildFishingComponent::StartFishing(const FGameplayTag& BaitTag)
{
    if (!HasAuthorityForFishing())
    {
        if (const AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
        {
            ServerStartFishing(BaitTag);
        }
        return false;
    }
    return StartFishingAuthority(BaitTag);
}

bool UAstrawildFishingComponent::StartFishingAuthority(const FGameplayTag& BaitTag)
{
    if (bFishingActive || !BaitTag.IsValid() || !FishDexTable)
    {
        return false;
    }

    UAstrawildInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    if (!Inventory || !Inventory->HasItem(BaitTag, 1))
    {
        SetFishingResult(EAstrawildFishingResult::Invalid);
        return false;
    }
    if (!SelectFishForContext(BaitTag))
    {
        SetFishingResult(EAstrawildFishingResult::Invalid);
        return false;
    }
    if (!Inventory->RemoveItem(BaitTag, 1))
    {
        SetFishingResult(EAstrawildFishingResult::Invalid);
        return false;
    }

    ActiveBaitTag = BaitTag;
    bFishingActive = true;
    Tension = 50.0f;
    ReelProgressNormalized = 0.0f;
    SetFishingResult(EAstrawildFishingResult::Active);
    return true;
}

bool UAstrawildFishingComponent::SelectFishForContext(const FGameplayTag& BaitTag)
{
    TArray<FAstrawildFishRow*> Candidates;
    FishDexTable->GetAllRows<FAstrawildFishRow>(TEXT("UAstrawildFishingComponent::SelectFishForContext"), Candidates);

    float TotalWeight = 0.0f;
    for (const FAstrawildFishRow* Row : Candidates)
    {
        if (!Row || Row->FishTag.IsValid() == false || Row->BaitTag != BaitTag)
        {
            continue;
        }
        if (FishingDepthMeters < Row->MinDepthMeters || FishingDepthMeters > Row->MaxDepthMeters)
        {
            continue;
        }
        if (Row->HabitatTag.IsValid() && FishingHabitatTag.IsValid() && Row->HabitatTag != FishingHabitatTag)
        {
            continue;
        }
        TotalWeight += FMath::Max(0.0f, Row->RarityWeight);
    }

    if (TotalWeight <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    float Selection = FMath::FRandRange(0.0f, TotalWeight);
    for (const FAstrawildFishRow* Row : Candidates)
    {
        if (!Row || Row->FishTag.IsValid() == false || Row->BaitTag != BaitTag)
        {
            continue;
        }
        if (FishingDepthMeters < Row->MinDepthMeters || FishingDepthMeters > Row->MaxDepthMeters)
        {
            continue;
        }
        if (Row->HabitatTag.IsValid() && FishingHabitatTag.IsValid() && Row->HabitatTag != FishingHabitatTag)
        {
            continue;
        }

        Selection -= FMath::Max(0.0f, Row->RarityWeight);
        if (Selection <= 0.0f)
        {
            ActiveFishTag = Row->FishTag;
            ActiveCatchItemTag = Row->CatchItemTag;
            ActiveFishRequiredReelSeconds = FMath::Max(0.1f, Row->RequiredReelSeconds);
            ActiveFishPullStrength = FMath::Max(0.1f, Row->PullStrength);
            ActiveFishSafeTensionMin = FMath::Clamp(Row->SafeTensionMin, 0.0f, 100.0f);
            ActiveFishSafeTensionMax = FMath::Clamp(Row->SafeTensionMax, ActiveFishSafeTensionMin, 100.0f);
            ActiveFishSellPrice = FMath::Max(1, Row->SellPrice);
            return true;
        }
    }
    return false;
}

bool UAstrawildFishingComponent::SetFishingContext(const float DepthMeters, const FGameplayTag& HabitatTag)
{
    if (!HasAuthorityForFishing())
    {
        return false;
    }
    FishingDepthMeters = FMath::Max(0.0f, DepthMeters);
    FishingHabitatTag = HabitatTag;
    return true;
}

bool UAstrawildFishingComponent::UpdateReelInput(const float ReelInput, const float DeltaSeconds)
{
    if (!HasAuthorityForFishing())
    {
        if (const AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
        {
            ServerUpdateReelInput(ReelInput, DeltaSeconds);
        }
        return false;
    }
    return UpdateReelAuthority(ReelInput, DeltaSeconds);
}

bool UAstrawildFishingComponent::UpdateReelAuthority(const float ReelInput, const float DeltaSeconds)
{
    if (!bFishingActive || DeltaSeconds <= 0.0f)
    {
        return false;
    }

    const float SafeDelta = FMath::Min(DeltaSeconds, 0.1f);
    const float Input = FMath::Clamp(ReelInput, -1.0f, 1.0f);
    const float Pull = ActiveFishPullStrength * 12.0f * SafeDelta;
    const float ReelTension = Input * 20.0f * SafeDelta;
    Tension = FMath::Clamp(Tension + Pull + ReelTension, 0.0f, 100.0f);

    if (Tension >= 100.0f)
    {
        SetFishingResult(EAstrawildFishingResult::LineBroken);
        return false;
    }

    if (Input > 0.0f && IsTensionSafe())
    {
        ReelProgressNormalized = FMath::Clamp(
            ReelProgressNormalized + (Input * SafeDelta / ActiveFishRequiredReelSeconds), 0.0f, 1.0f);
    }
    else if (Input <= 0.0f)
    {
        ReelProgressNormalized = FMath::Max(0.0f, ReelProgressNormalized - (0.05f * SafeDelta));
    }

    if (ReelProgressNormalized >= 1.0f)
    {
        return CommitCatchAuthority();
    }
    return true;
}

bool UAstrawildFishingComponent::IsTensionSafe() const
{
    return Tension >= ActiveFishSafeTensionMin && Tension <= ActiveFishSafeTensionMax;
}

bool UAstrawildFishingComponent::StopFishing(const bool bReleaseLine)
{
    if (!HasAuthorityForFishing())
    {
        if (const AActor* OwnerActor = GetOwner(); OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
        {
            ServerStopFishing(bReleaseLine);
        }
        return false;
    }
    return StopFishingAuthority(bReleaseLine);
}

bool UAstrawildFishingComponent::StopFishingAuthority(const bool bReleaseLine)
{
    if (!bFishingActive)
    {
        return false;
    }
    SetFishingResult(bReleaseLine ? EAstrawildFishingResult::Escaped : EAstrawildFishingResult::Invalid);
    return true;
}

bool UAstrawildFishingComponent::CommitCatchAuthority()
{
    if (!HasAuthorityForFishing() || !bFishingActive || !ActiveCatchItemTag.IsValid())
    {
        SetFishingResult(EAstrawildFishingResult::Invalid);
        return false;
    }

    UAstrawildInventoryComponent* Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    if (!Inventory || !Inventory->CanAddItem(ActiveCatchItemTag, 1) || !Inventory->AddItem(ActiveCatchItemTag, 1))
    {
        SetFishingResult(EAstrawildFishingResult::InventoryFull);
        return false;
    }

    const FGameplayTag CaughtFishTag = ActiveFishTag;
    const FGameplayTag CaughtItemTag = ActiveCatchItemTag;
    const int32 CaughtSellPrice = ActiveFishSellPrice;
    SetFishingResult(EAstrawildFishingResult::Caught);
    OnFishCaught.Broadcast(CaughtFishTag, CaughtItemTag, CaughtSellPrice);
    return true;
}

void UAstrawildFishingComponent::SetFishingResult(const EAstrawildFishingResult NewResult)
{
    FishingResult = NewResult;
    if (NewResult != EAstrawildFishingResult::Active)
    {
        bFishingActive = false;
    }
    OnFishingStateChanged.Broadcast(NewResult);
}

void UAstrawildFishingComponent::OnRepFishingState()
{
    OnFishingStateChanged.Broadcast(FishingResult);
    if (FishingResult == EAstrawildFishingResult::Caught)
    {
        OnFishCaught.Broadcast(ActiveFishTag, ActiveCatchItemTag, ActiveFishSellPrice);
    }
}

void UAstrawildFishingComponent::ServerStartFishing_Implementation(const FGameplayTag BaitTag)
{
    StartFishingAuthority(BaitTag);
}

void UAstrawildFishingComponent::ServerUpdateReelInput_Implementation(const float ReelInput, const float DeltaSeconds)
{
    UpdateReelAuthority(ReelInput, DeltaSeconds);
}

void UAstrawildFishingComponent::ServerStopFishing_Implementation(const bool bReleaseLine)
{
    StopFishingAuthority(bReleaseLine);
}
