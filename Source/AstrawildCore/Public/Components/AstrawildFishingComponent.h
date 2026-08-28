#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AstrawildFishingComponent.generated.h"

class UDataTable;

UENUM(BlueprintType)
enum class EAstrawildFishingResult : uint8
{
    Idle,
    Active,
    Caught,
    Escaped,
    LineBroken,
    InventoryFull,
    Invalid
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildFishingStateChangedSignature, EAstrawildFishingResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAstrawildFishCaughtSignature, FGameplayTag, FishTag, FGameplayTag, CatchItemTag, int32, SellPrice);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildFishingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildFishingComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Fishing|Data")
    TObjectPtr<UDataTable> FishDexTable;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State")
    bool bFishingActive = false;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State")
    FGameplayTag ActiveFishTag;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State")
    FGameplayTag ActiveCatchItemTag;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State", meta=(ClampMin="1"))
    int32 ActiveFishSellPrice = 1;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Tension = 0.0f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ReelProgressNormalized = 0.0f;

    UPROPERTY(ReplicatedUsing=OnRepFishingState, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|State")
    EAstrawildFishingResult FishingResult = EAstrawildFishingResult::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|Context")
    float FishingDepthMeters = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Fishing|Context")
    FGameplayTag FishingHabitatTag;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Fishing|Events")
    FOnAstrawildFishingStateChangedSignature OnFishingStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Fishing|Events")
    FOnAstrawildFishCaughtSignature OnFishCaught;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Fishing")
    bool StartFishing(const FGameplayTag& BaitTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Fishing")
    bool UpdateReelInput(float ReelInput, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Fishing")
    bool StopFishing(bool bReleaseLine);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Fishing|Authority")
    bool SetFishingContext(float DepthMeters, const FGameplayTag& HabitatTag);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Fishing")
    bool IsFishing() const { return bFishingActive; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Fishing")
    float GetTensionNormalized() const { return FMath::Clamp(Tension / 100.0f, 0.0f, 1.0f); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Fishing")
    bool IsTensionSafe() const;

    UFUNCTION(Server, Reliable)
    void ServerStartFishing(FGameplayTag BaitTag);

    UFUNCTION(Server, Reliable)
    void ServerUpdateReelInput(float ReelInput, float DeltaSeconds);

    UFUNCTION(Server, Reliable)
    void ServerStopFishing(bool bReleaseLine);

private:
    FGameplayTag ActiveBaitTag;
    float ActiveFishRequiredReelSeconds = 0.0f;
    float ActiveFishPullStrength = 0.0f;
    float ActiveFishSafeTensionMin = 0.0f;
    float ActiveFishSafeTensionMax = 100.0f;

    bool HasAuthorityForFishing() const;
    bool StartFishingAuthority(const FGameplayTag& BaitTag);
    bool UpdateReelAuthority(float ReelInput, float DeltaSeconds);
    bool StopFishingAuthority(bool bReleaseLine);
    bool SelectFishForContext(const FGameplayTag& BaitTag);
    bool CommitCatchAuthority();
    void SetFishingResult(EAstrawildFishingResult NewResult);

    UFUNCTION()
    void OnRepFishingState();
};
