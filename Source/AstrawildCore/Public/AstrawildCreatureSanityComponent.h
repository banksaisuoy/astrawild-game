#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildCreatureSanityComponent.generated.h"

/**
 * SCP Phase 9.2 — creature sanity, illness and healthcare (directive [3] Phase 9).
 *
 * Mounted on every Echo (captured instances drive the loop):
 *  - Sanity 0-100 drains while working or fighting, recovers at Creature Beds /
 *    Hot Springs and while resting at night.
 *  - Below 40 the echo enters the DEPRESSED band (work output x0.6).
 *  - Sustained low sanity risks illness: Ulcer (health drain), SprainedAnkle
 *    (speed x0.75), Slacker (work x0.3) — deterministic risk accumulation, no
 *    dice spikes.
 *  - Medicine (Cure Tonic, crafted at the Medicine Bench) clears illness and
 *    restores 30 sanity.
 *
 * Save additive: Sanity + IllnessId on FAstrawildEchoInstanceV2 (absent in
 * legacy saves = healthy defaults).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCreatureSanityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCreatureSanityComponent();

    /** Depressed band threshold (work multiplier kicks in below this). */
    static constexpr float DepressedThreshold = 40.0f;

    /** Sustained-risk band — illness can only be contracted below this. */
    static constexpr float IllnessThreshold = 25.0f;

    // --- State ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Sanity", Replicated)
    float Sanity = 100.0f;

    /** Active illness id (NAME_None = healthy). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Sanity", Replicated)
    FName IllnessId = NAME_None;

    // --- Queries (multipliers consumed by work + locomotion) ---

    /** Work output multiplier: Depressed x0.6, Slacker illness x0.3 on top. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Sanity")
    float GetWorkOutputMultiplier() const;

    /** Movement multiplier: SprainedAnkle x0.75. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Sanity")
    float GetSpeedMultiplier() const;

    /** Health drain per second while Ulcer is active. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Sanity")
    float GetHealthDrainPerSecond() const;

    /** True when the echo is ill (HUD icon + bench cure list). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Sanity")
    bool IsIll() const { return !IllnessId.IsNone(); }

    // --- Actions ---

    /** Direct sanity adjustment (clamped). Returns the applied delta. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Sanity")
    float AddSanity(float Amount);

    /** Cure the active illness (medicine path) — also restores 30 sanity. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Sanity")
    bool ApplyMedicine();

    // --- Static contracts (automation tests) ---

    /**
     * One sanity step: drains while working/fighting, recovers at beds, hot
     * springs and night rest. Pure function of the situation snapshot.
     */
    static float ComputeSanityDelta(float DeltaSeconds, bool bWorking, bool bInCombat,
        bool bNearBed, bool bNearHotSpring, bool bNightRest);

    /** Illness risk 0..1 after ExposureSeconds below the illness threshold. */
    static float ComputeIllnessRisk(float Sanity, float ExposureSeconds);

    /** Resolve an illness id from a 0..1 risk roll (deterministic bands). */
    static FName SelectIllness(float RiskRoll);

    /** Per-illness modifier table (public constants for tests + HUD). */
    static float GetIllnessWorkMultiplier(FName InIllnessId);
    static float GetIllnessSpeedMultiplier(FName InIllnessId);
    static float GetIllnessHealthDrain(FName InIllnessId);

    // --- Save integration (additive) ---

    void ExportForSave(float& OutSanity, FName& OutIllnessId) const;
    void ImportFromSave(float InSanity, FName InIllnessId);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** FCR-1-c (M-c6): damage-feed reset for the 20s combat window. */
    UFUNCTION()
    void HandleOwnerDamaged(class AAstrawildEchoCharacter* Echo, float NewHealth);

private:
    /** Seconds currently spent below the illness threshold (risk accumulator). */
    float LowSanityExposure = 0.0f;

    /** Cache of the last combat time on the owner echo (drain window). */
    float SecondsSinceCombat = 1e9f;

    /** Bed / hot-spring proximity cache (refreshed at a 1s cadence). */
    float ProximityAccumulator = 0.0f;
    bool bNearBed = false;
    bool bNearHotSpring = false;

    bool IsOwnerServerEcho() const;
    void RefreshComfortProximity();
    bool IsNight() const;
};
