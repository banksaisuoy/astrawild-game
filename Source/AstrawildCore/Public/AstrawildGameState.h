#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AstrawildTypes.h"
#include "AstrawildGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEndingTriggered, EAstrawildEndingState, Ending, EAstrawildEndingState, OldEnding);

/**
 * Replicated world state — single source of truth for time-of-day, day number and weather
 * (directive §13/§12/§28). Server-authoritative: only the server (via subsystems) writes;
 * clients read for rendering, audio and UI.
 *
 * Final Run (FR-6): also owns the Act 3 ending state — a one-way, replicated,
 * save-persistent (schema v5) world verdict. None = the story is still in play.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AAstrawildGameState();

    /** Minutes since midnight, 0..1439. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Time", ReplicatedUsing=OnRep_TimeOfDayMinutes)
    int32 TimeOfDayMinutes = 8 * 60;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Time", Replicated)
    int32 DayNumber = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Weather", ReplicatedUsing=OnRep_WeatherState)
    EAstrawildWeatherState WeatherState = EAstrawildWeatherState::Clear;

    /** World generation seed — replicated so client-side procedural logic matches. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World", Replicated)
    int32 WorldSeed = 1337;

    /**
     * LCP-2: true once the server has FINALIZED the world seed (bootstrapper
     * BeginPlay or save-load restore). Clients must not build their deterministic
     * cosmetic world copy before this flips — the default 1337 could otherwise
     * race a save-restored seed and build the wrong world locally.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World", Replicated)
    bool bWorldSeedSynced = false;

    // --- LCP-5: LAN co-op shared-state mirrors ---

    /**
     * LCP-5: replicated snapshot of the HOST-authoritative research pool (RP +
     * unlocked tech ids). GameInstance subsystems do NOT replicate — clients
     * import this into their local research subsystem on OnRep so every screen
     * reads the shared truth. Written server-side after each research mutation.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Research", ReplicatedUsing=OnRep_ResearchMirror)
    FAstrawildResearchSaveData ResearchMirror;

    /** LCP-5: client-side import of the replicated research snapshot (local subsystem mirror). */
    UFUNCTION()
    void OnRep_ResearchMirror();

    // --- Final Run (FR-6): Act 3 ending state ---

    /** One-way ending verdict (save schema v5). None = story in play. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Ending", ReplicatedUsing=OnRep_EndingState)
    EAstrawildEndingState EndingState = EAstrawildEndingState::None;

    /** True once any ending has been chosen — post-game free roam is live. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Ending", Replicated)
    bool bPostGameActive = false;

    /** Fired once when the ending is first set (HUD banner, weather pin, saves hook here). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World|Ending")
    FAstrawildEndingTriggered OnEndingTriggered;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetTimeOfDayNormalized() const;

    /** Hour in 0..23 (e.g. 14.5 = 14:30). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetTimeOfDayHours() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    bool IsNight() const;

    /** 0 = dawn, 0.5 = noon, 1 = midnight phases mapped to sun curve. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetSunCycleAlpha() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    FText GetTimeOfDayText() const;

    /** Final Run (FR-6): true once any ending has been chosen (post-game free roam). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Ending")
    bool IsPostGameActive() const { return bPostGameActive; }

    /** Final Run (FR-6): human-readable ending banner text (HUD reads this). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Ending")
    FText GetEndingBannerText() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server-side setters (called by subsystems only). */
    void SetTimeOfDayMinutes(int32 InMinutes);
    void AdvanceDay();
    void SetWeatherState(EAstrawildWeatherState InState);
    void SetWorldSeed(int32 InSeed);

    /**
     * Final Run (FR-6): trigger the ending (server only, one-way — a set ending
     * never changes back or switches sides). "The Dawn That Stays" additionally
     * pins the weather to Clear forever (the EndingBreak); "The Storm That Sleeps"
     * keeps the living sky. Both unlock post-game free roam.
     */
    void SetEndingState(EAstrawildEndingState InState);

protected:
    UFUNCTION()
    void OnRep_TimeOfDayMinutes();

    UFUNCTION()
    void OnRep_WeatherState();

    UFUNCTION()
    void OnRep_EndingState();
};
