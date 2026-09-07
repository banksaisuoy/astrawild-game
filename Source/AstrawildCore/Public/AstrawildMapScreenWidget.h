#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildMapScreenWidget.generated.h"

class AAstrawildPlayerController;
class UButton;
class UCanvasPanel;
class UTextBlock;

/**
 * PCR-3 (PG-3 gap closed): the world map screen. The 12-zone open world
 * (17 POIs, 2 villages, 3 dungeons, live world events) previously had NO
 * navigation surface — orientation was zone banners only.
 *
 * Pure-C++ UMG snapshot map (read-only — no pings, no live re-render loop;
 * reopening the screen refreshes every marker):
 *
 *   ┌ World Map ── Dawn Fields · Threat 1 ────────── [Close] ┐
 *   │ ┌Dawn Fields┐ ┌Glimmerwood┐ ┌Ember Ridge ┐ ┌Frostveil ┐ │
 *   │ │ Threat 1  │ │ Threat 2  │ │ Threat 3   │ │ Threat 3 │ │
 *   │ └───────────┘ └───────────┘ └────────────┘ └──────────┘ │
 *   │   ◆POI ●you Vvillage Ddungeon !event  (per-zone rects,  │
 *   │   discovered-POI dots, villages, dungeons, active event  │
 *   │   markers, the player, the active-objective highlight)   │
 *   └──────────────────────────────────────────────────────────┘
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildMapScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Pure world→map projection (automation-tested): the world rectangle is
     * mapped onto the panel with uniform scale (no axis distortion — the
     * world is 3.2km × 2.4km and must stay proportionally recognizable).
     */
    static FVector2D ProjectWorldToMap(const FVector2D& WorldPoint, const FBox2D& WorldBounds, const FVector2D& MapSize);

    /** Rebuild the zone grid + every marker (call on open). */
    void RefreshMap();

    UAstrawildMapScreenWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleCloseClicked();

    /** Zone-colored, positioned rect with name + threat (12 total). */
    void AddZoneCell(const struct FAstrawildZoneDescriptor& Zone, const FBox2D& WorldBounds, const FVector2D& MapSize);

    /** Small glyph marker at a world position (legend row explains them). */
    void AddGlyphMarker(const TCHAR* Glyph, const FLinearColor& Color, const FVector2D& WorldPoint,
        const FBox2D& WorldBounds, const FVector2D& MapSize, const FString& MarkerName);

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> MapCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> LegendText;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
