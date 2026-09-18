#include "AstrawildMapScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDungeonGeneratorActor.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPlayerController.h"
#include "AstrawildPOIMarkerActor.h"
#include "AstrawildPOISubsystem.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildVillageActor.h"
#include "AstrawildWorldEventSubsystem.h"
#include "AstrawildZoneSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace
{
    constexpr float MapPanelWidth = 720.0f;
    constexpr float MapPanelHeight = 700.0f;
    constexpr float MapGridHeight = 520.0f; // zone grid area (4 x 3 world grid)
}

// ---------------------------------------------------------------------------
// World→map projection (pure — the automation contract pins this)
// ---------------------------------------------------------------------------

FVector2D UAstrawildMapScreenWidget::ProjectWorldToMap(const FVector2D& WorldPoint, const FBox2D& WorldBounds, const FVector2D& MapSize)
{
    // Uniform scale (fit both axes, center the shorter one) so the world's
    // 4:3 proportions read correctly on the map — never stretched.
    const FVector2D WorldSize = WorldBounds.GetSize();
    if (WorldSize.X <= 0.0f || WorldSize.Y <= 0.0f || MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
    {
        return FVector2D::ZeroVector; // degenerate bounds map to the corner
    }
    const float Scale = FMath::Min(MapSize.X / WorldSize.X, MapSize.Y / WorldSize.Y);
    const FVector2D ScaledWorld = WorldSize * Scale;
    const FVector2D Offset = (MapSize - ScaledWorld) * 0.5f; // center the letterbox
    return FVector2D(
        Offset.X + (WorldPoint.X - WorldBounds.Min.X) * Scale,
        Offset.Y + (WorldPoint.Y - WorldBounds.Min.Y) * Scale);
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildMapScreenWidget::UAstrawildMapScreenWidget()
{
    // F-05 convention: focusable so M/ESC close without a prior mouse click.
    bIsFocusable = true;
}

FReply UAstrawildMapScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::M || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleMapScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildMapScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshMap();
}

void UAstrawildMapScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.55f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("World Map")));

    SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapSubtitle"));
    SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
    SubtitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    SubtitleText->SetAutoWrapText(true);

    MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapGridCanvas"));

    LegendText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapLegend"));
    LegendText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.74f, 0.78f, 1.0f)));
    LegendText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));
    LegendText->SetText(FText::FromString(TEXT("\u25C6 discovered POI  \u25CF you  V village  D dungeon  ! world event  (reopen to refresh)")));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MapClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [M]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildMapScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-MapPanelWidth * 0.5f, -MapPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(MapPanelWidth, 30.0f));
    }
    if (UCanvasPanelSlot* SubtitleSlot = Canvas->AddChildToCanvas(SubtitleText))
    {
        SubtitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SubtitleSlot->SetPosition(FVector2D(-MapPanelWidth * 0.5f, -MapPanelHeight * 0.5f + 34.0f));
        SubtitleSlot->SetSize(FVector2D(MapPanelWidth, 44.0f));
    }
    if (UCanvasPanelSlot* GridSlot = Canvas->AddChildToCanvas(MapCanvas))
    {
        GridSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        GridSlot->SetPosition(FVector2D(-MapPanelWidth * 0.5f, -MapPanelHeight * 0.5f + 80.0f));
        GridSlot->SetSize(FVector2D(MapPanelWidth, MapGridHeight));
    }
    if (UCanvasPanelSlot* LegendSlot = Canvas->AddChildToCanvas(LegendText))
    {
        LegendSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        LegendSlot->SetPosition(FVector2D(-MapPanelWidth * 0.5f, MapPanelHeight * 0.5f - 78.0f));
        LegendSlot->SetSize(FVector2D(MapPanelWidth, 30.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(MapPanelWidth * 0.5f - 130.0f, MapPanelHeight * 0.5f - 40.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildMapScreenWidget::AddZoneCell(const FAstrawildZoneDescriptor& Zone, const FBox2D& WorldBounds, const FVector2D& MapSize)
{
    // Zone rect from its world bounds → map coordinates.
    const FVector2D Min = ProjectWorldToMap(Zone.Bounds.Min, WorldBounds, MapSize);
    const FVector2D Max = ProjectWorldToMap(Zone.Bounds.Max, WorldBounds, MapSize);
    const FVector2D Size = (Max - Min).Abs();

    // Read-only zone cell (button = known-good colored surface in pure C++).
    UButton* Cell = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),
        *FString::Printf(TEXT("MapZone_%s"), *Zone.ZoneId.ToString()));
    Cell->SetBackgroundColor(FLinearColor(
        Zone.GroundTint.R * 0.22f + 0.06f,
        Zone.GroundTint.G * 0.22f + 0.08f,
        Zone.GroundTint.B * 0.22f + 0.10f,
        0.92f));
    Cell->SetIsEnabled(false); // display surface, not an interactive control

    UTextBlock* CellText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapZoneLabel"));
    CellText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));
    CellText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CellText->SetJustification(ETextJustify::Center);
    CellText->SetAutoWrapText(true);
    CellText->SetText(FText::FromString(FString::Printf(TEXT("%s\nThreat %d\n%s"),
        *Zone.DisplayName.ToString(), Zone.ThreatLevel,
        Zone.HazardType == EAstrawildZoneHazard::None ? TEXT("") : *UEnum::GetDisplayValueAsText(Zone.HazardType).ToString())));
    Cell->AddChild(CellText);

    if (UCanvasPanelSlot* CellSlot = MapCanvas->AddChildToCanvas(Cell))
    {
        CellSlot->SetAnchors(FAnchors(0.0f, 0.0f));
        CellSlot->SetPosition(Min);
        CellSlot->SetSize(Size);
    }
}

void UAstrawildMapScreenWidget::AddGlyphMarker(const TCHAR* Glyph, const FLinearColor& Color, const FVector2D& WorldPoint,
    const FBox2D& WorldBounds, const FVector2D& MapSize, const FString& MarkerName)
{
    const FVector2D MapPoint = ProjectWorldToMap(WorldPoint, WorldBounds, MapSize);

    UTextBlock* Marker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
        *FString::Printf(TEXT("MapMarker_%s"), *MarkerName));
    Marker->SetColorAndOpacity(FSlateColor(Color));
    Marker->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 15));
    Marker->SetText(FText::FromString(Glyph));
    Marker->SetToolTipText(FText::FromString(MarkerName));

    if (UCanvasPanelSlot* MarkerSlot = MapCanvas->AddChildToCanvas(Marker))
    {
        MarkerSlot->SetAnchors(FAnchors(0.0f, 0.0f));
        MarkerSlot->SetPosition(MapPoint - FVector2D(7.0f, 10.0f)); // glyph centering
        MarkerSlot->SetSize(FVector2D(40.0f, 22.0f));
    }
}

void UAstrawildMapScreenWidget::RefreshMap()
{
    if (!MapCanvas)
    {
        return;
    }

    UWorld* World = GetWorld();
    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    if (!World || !PC)
    {
        return;
    }

    MapCanvas->ClearChildren();

    // --- World bounds (union of the 12 zone rects) ---
    const TArray<FAstrawildZoneDescriptor>& Zones = UAstrawildZoneSubsystem::GetAllZones();
    FBox2D WorldBounds(ForceInit);
    for (const FAstrawildZoneDescriptor& Zone : Zones)
    {
        WorldBounds += Zone.Bounds;
    }
    const FVector2D MapSize(MapPanelWidth, MapGridHeight);

    // --- Zone cells ---
    for (const FAstrawildZoneDescriptor& Zone : Zones)
    {
        AddZoneCell(Zone, WorldBounds, MapSize);
    }

    // --- Player + current zone subtitle ---
    FString Subtitle;
    FName QuestHighlightPoi = NAME_None;
    if (const APawn* Pawn = PC->GetPawn())
    {
        const FVector2D PlayerXY(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y);
        AddGlyphMarker(TEXT("\u25CF"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), PlayerXY, WorldBounds, MapSize, TEXT("You"));

        if (const FAstrawildZoneDescriptor* CurrentZone = UAstrawildZoneSubsystem::FindZone(
                UAstrawildZoneSubsystem::GetZoneAt(Pawn->GetActorLocation())))
        {
            Subtitle += FString::Printf(TEXT("You are in: %s"), *CurrentZone->DisplayName.ToString());
        }
    }

    // --- Active objective line + quest-target POI resolution ---
    if (const UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
    {
        const TArray<FAstrawildQuestObjective> Objectives = Quests->GetActiveObjectives();
        for (const FAstrawildQuestObjective& Objective : Objectives)
        {
            if (!Objective.ObjectiveText.IsEmpty())
            {
                Subtitle += FString::Printf(TEXT("\n[ ] %s (%d/%d)"),
                    *Objective.ObjectiveText.ToString(), Objective.ProgressCount, Objective.RequiredCount);
                if (QuestHighlightPoi.IsNone())
                {
                    QuestHighlightPoi = Objective.TargetId; // first objective wins the highlight
                }
            }
        }
    }
    SubtitleText->SetText(FText::FromString(Subtitle));

    // --- Discovered POI dots (registry truth + discovery state) ---
    UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
    const UAstrawildPOISubsystem* POIs = World->GetSubsystem<UAstrawildPOISubsystem>();
    if (Registry && POIs)
    {
        for (const UAstrawildPOIDefinition* POI : Registry->GetAllPOIs())
        {
            if (!POI || !POIs->IsPOIDiscovered(POI->PoiId))
            {
                continue; // undiscovered POIs stay off the map (exploration value)
            }
            const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZone(POI->Zone);
            if (!Zone)
            {
                continue;
            }
            const FVector2D POIWorld = FVector2D(Zone->Bounds.GetCenter()) + POI->OffsetFromZoneCenter;
            const bool bIsQuestTarget = (POI->PoiId == QuestHighlightPoi);
            AddGlyphMarker(TEXT("\u25C6"),
                bIsQuestTarget ? FLinearColor(1.0f, 0.85f, 0.2f, 1.0f) : FLinearColor(0.55f, 0.85f, 1.0f, 1.0f),
                POIWorld, WorldBounds, MapSize, POI->DisplayName.ToString());
        }
    }

    // --- Villages (live actor positions) ---
    for (TActorIterator<AVillageActor> It(World); It; ++It)
    {
        const AVillageActor* Village = *It;
        if (!Village)
        {
            continue;
        }
        AddGlyphMarker(TEXT("V"), FLinearColor(0.55f, 0.95f, 0.65f, 1.0f),
            FVector2D(Village->GetActorLocation().X, Village->GetActorLocation().Y),
            WorldBounds, MapSize, Village->GetName());
    }

    // --- Dungeons (live generator positions) ---
    for (TActorIterator<ADungeonGeneratorActor> It(World); It; ++It)
    {
        const ADungeonGeneratorActor* Dungeon = *It;
        if (!Dungeon)
        {
            continue;
        }
        AddGlyphMarker(TEXT("D"), FLinearColor(0.85f, 0.55f, 0.95f, 1.0f),
            FVector2D(Dungeon->GetActorLocation().X, Dungeon->GetActorLocation().Y),
            WorldBounds, MapSize, Dungeon->GetName());
    }

    // --- Active world events (runtime snapshot) ---
    if (const UAstrawildWorldEventSubsystem* WorldEvents = World->GetSubsystem<UAstrawildWorldEventSubsystem>())
    {
        for (const FAstrawildWorldEventSaveData& Event : WorldEvents->GetActiveRuntimeEvents())
        {
            const UAstrawildWorldEventDefinition* Def = Registry ? Registry->FindWorldEvent(Event.EventId) : nullptr;
            const FString MarkerName = Def ? Def->DisplayName.ToString() : Event.EventId.ToString();
            if (Event.Zone == EAstrawildZone::None && Event.Location.IsZero())
            {
                continue; // world-wide events have no pin (the HUD banner covers them)
            }
            AddGlyphMarker(TEXT("!"), FLinearColor(1.0f, 0.45f, 0.25f, 1.0f),
                FVector2D(Event.Location.X, Event.Location.Y), WorldBounds, MapSize, MarkerName);
        }
    }
}

void UAstrawildMapScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleMapScreen();
    }
}
