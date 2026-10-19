#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h"
#include "AstrawildHudWidget.generated.h"

class AAstrawildEchoBossCharacter;
class UCanvasPanel;
class UProgressBar;
class UTextBlock;

/**
 * Pure-C++ HUD (directive §29): builds its entire widget tree in NativeConstruct —
 * zero UMG assets needed. Reads replicated state each refresh. A future CommonUI/UMG
 * screen can subclass or replace it through PlayerController.HudWidgetClass.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /**
     * Show a transient notification line (craft results, captures, quest beats).
     * CGC-3: the optional Cue parameter differentiates the FEW semantic sites
     * (research unlock / craft outcome / warning beats / cancellations) — the
     * DEFAULT stays the generic Confirm blip so the other ~60 call sites are
     * untouched. Fail-closed: a missing cue keeps the toast visible.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|HUD")
    void PushNotification(const FText& Message, EAstrawildToastCue Cue = EAstrawildToastCue::Confirm);

    /**
     * CGC-3: the toast-cue map — PURE STATIC so test 137 pins every enum
     * value's path (and the DCP-5 pin test re-asserts it) without an engine
     * world. Every path resolves against the flat-imported Content/Audio
     * packages; unknown enum values fail closed to the Confirm blip.
     */
    static const TCHAR* GetToastCuePath(const EAstrawildToastCue Cue);

    // --- CGC-2 (B): pure guidance formatting (world-free, test-pinned) ---

    /**
     * 8-wind compass text (N/NE/E/SE/S/SW/W/NW) for a bearing in DEGREES
     * CLOCKWISE FROM NORTH. Wrap math: fmod into [0,360) first (negative
     * bearings — e.g. atan2 results — wrap through the +360 correction),
     * then round(yaw/45) % 8 picks the wind so each sector center is exact
     * and the sector edges (±22.5°) fall to the COUNTERclockwise neighbor.
     */
    static FString FormatBearingText(const float YawToTargetDeg);

    /**
     * Planar distance text in meters: "%dm" rounded, clamped >= 0, capped at
     * the far display ceiling "999m+" (any farther target reads the same —
     * the line guides, it never pretends to survey).
     */
    static FString FormatDistanceText(const float DistanceCm);

protected:
    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> HungerBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> ThirstBar;

    UPROPERTY()
    TObjectPtr<UTextBlock> TimeText;

    UPROPERTY()
    TObjectPtr<UTextBlock> WeatherText;

    /** Audit C-2: research pool readout (top-center, under weather). */
    UPROPERTY()
    TObjectPtr<UTextBlock> ResearchText;

    /** LCP-6: persistent session-mode line — SINGLE PLAYER / LAN HOST / LAN CLIENT (PART 6). */
    UPROPERTY()
    TObjectPtr<UTextBlock> SessionModeText;

    /** Audit C-6: build-mode readout (center, above the prompt). */
    UPROPERTY()
    TObjectPtr<UTextBlock> BuildText;

    UPROPERTY()
    TObjectPtr<UTextBlock> QuestText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PromptText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CaptureText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CommandText;

    /** GDP: party ability [T] + player smart-cast [Y] readiness readout. */
    UPROPERTY()
    TObjectPtr<UTextBlock> AbilityText;

    /** Wave 3: equipped weapon + shield readout (right-bottom). */
    UPROPERTY()
    TObjectPtr<UTextBlock> EquipmentText;

    UPROPERTY()
    TObjectPtr<UTextBlock> NotificationText;

    /** Batch 7 — Shattered Vale: zone banner (title + flavor line under the world info). */
    UPROPERTY()
    TObjectPtr<UTextBlock> ZoneBannerText;

    UPROPERTY()
    TObjectPtr<UTextBlock> ZoneSubText;

    // --- Final production run: boss bar + scanner/drone readout ---

    /** PHASE 14: boss encounter health bar (top-center, hidden while no boss lives). */
    UPROPERTY()
    TObjectPtr<UProgressBar> BossHealthBar;

    UPROPERTY()
    TObjectPtr<UTextBlock> BossText;

    /** Final Run (FR-6): persistent ending verdict banner (post-game overlay). */
    UPROPERTY()
    TObjectPtr<UTextBlock> EndingBannerText;

    /** Scanner + drone companion status line (under the capture chance). */
    UPROPERTY()
    TObjectPtr<UTextBlock> ScanText;

    /** Production V2: active world-event banner (top-center under the zone line). */
    TObjectPtr<UTextBlock> WorldEventText;

    /** Production V2: power-grid readout (generation/draw/stored). */
    TObjectPtr<UTextBlock> PowerText;

    /** Production V2: weapon + ammo line (combat readability, Master Plan §8). */
    UPROPERTY()
    TObjectPtr<UTextBlock> WeaponText;

    /** Sci-Fi reticle at screen center. */
    UPROPERTY()
    TObjectPtr<UTextBlock> CrosshairText;

private:
    float RefreshAccumulator = 0.0f;
    float NotificationRemaining = 0.0f;

    /** Cached boss actor for the encounter bar (weak — defeat/destroy clears it). */
    TWeakObjectPtr<AAstrawildEchoBossCharacter> CachedBoss;

    /** Zone banner state (client-local lookup via the pure static zone table). */
    EAstrawildZone CurrentZone = EAstrawildZone::None;
    TArray<EAstrawildZone> LocallyDiscoveredZones;
    float ZoneSweepAccumulator = 0.3f;

    void BuildWidgetTree();
    void RefreshState();
    void RefreshZoneBanner();

    /**
     * CGC-2 (B): one guidance line for the FIRST incomplete objective of the
     * active quest ("\u2192 Lumewisp ~86m SW"), resolved from live world
     * targets: POI -> zone center + offset (the map-screen idiom), species ->
     * nearest live wild Echo (dormancy-aware fallback text), portal -> the
     * matching dungeon portal, collect -> the nearest matching resource node.
     * Fallbacks stay honest ("search the wilds"); non-spatial objective types
     * return an empty line. Computed at RefreshState cadence only.
     */
    FString ResolveObjectiveGuidance(const TArray<FAstrawildQuestObjective>& Objectives,
        const FVector& PlayerLocation) const;

    class AAstrawildPlayerCharacter* GetAstrawildPawn() const;
};
