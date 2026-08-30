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
    UAstrawildHudWidget();

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Show a transient notification line (craft results, captures, quest beats). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|HUD")
    void PushNotification(const FText& Message);

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

    /** Scanner + drone companion status line (under the capture chance). */
    UPROPERTY()
    TObjectPtr<UTextBlock> ScanText;

    /** Production V2: active world-event banner (top-center under the zone line). */
    TObjectPtr<UTextBlock> WorldEventText;

    /** Production V2: power-grid readout (generation/draw/stored). */
    TObjectPtr<UTextBlock> PowerText;

    /** Production V2: weapon + ammo line (combat readability, Master Plan §8). */
    TObjectPtr<UTextBlock> WeaponText;

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

    class AAstrawildPlayerCharacter* GetAstrawildPawn() const;
};
