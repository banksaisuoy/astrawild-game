#include "AstrawildControlsScreenWidget.h"

#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
    constexpr float ControlsPanelWidth = 780.0f;
    constexpr float ControlsPanelHeight = 640.0f;
}

// ---------------------------------------------------------------------------
// The 33-action control table (static — the automation contract pins the size)
// ---------------------------------------------------------------------------

const TArray<FString>& UAstrawildControlsScreenWidget::GetControlRows()
{
    // Transcribed faithfully from Docs/ASTRAWILD_INPUT_REFERENCE.md §1 (the
    // 2026-09-03 PCR-5 sync — 32 runtime actions incl. AWD_Descend, plus the
    // CGC-4 2026-09-07 addition — 33 actions incl. AWD_ToggleHUD on F6, plus
    // the EDX-1 2026-10-27 unification — 34 actions incl. AWD_EchoDex on I) +
    // §1b's gamepad companion mapping. One row per ACTION, not per key.
    static const TArray<FString> Rows = {
        TEXT("W / A / S / D — Move — WASD movement (gamepad: left stick)"),
        TEXT("Mouse — Look — camera (Y negated; gamepad: right stick)"),
        TEXT("Left Shift — Sprint — faster movement, drains stamina (gamepad: RB)"),
        TEXT("Space — Jump — jump / skiff climb (gamepad: A)"),
        TEXT("E — Interact — harvest / craft station / rest / NPC / capture Echo (gamepad: B)"),
        TEXT("Left Mouse — Light Attack — melee or Pulse Lance; confirms building placement (gamepad: RT)"),
        TEXT("Right Mouse (hold) — Block — damage mitigation + slower move (gamepad: LB)"),
        TEXT("F — Heavy Attack — 60 dmg, costs stamina (gamepad: LT)"),
        TEXT("Q — Dodge — invulnerability frames burst (gamepad: X)"),
        TEXT("C — Party Command — Follow/Attack/Defend/Stay/Work cycle (gamepad: D-pad Up)"),
        TEXT("R — Feed — feed an Echo for trust and bond (gamepad: D-pad Right)"),
        TEXT("B — Build Mode — toggle building placement (gamepad: Y)"),
        TEXT("N — Rotate Building — rotate the placement ghost (gamepad: Select)"),
        TEXT("Mouse Wheel — Cycle Building — cycle piece type while placing (KB/M only)"),
        TEXT("G — Smart Consume — eat/drink the best owned consumable (gamepad: D-pad Down)"),
        TEXT("X — Equip Best — equip strongest weapon + shield (gamepad: D-pad Left)"),
        TEXT("Z — Delete Building — dismantle under the crosshair + refund (KB/M only)"),
        TEXT("V (hold) — Scan — x3 journal observation with a Field Scanner (KB/M only)"),
        TEXT("H — Deploy Drone — launch/recall the utility drone (KB/M only)"),
        TEXT("J — Deploy Robot — assign a robot to the nearest work site (KB/M only)"),
        TEXT("TAB — Inventory — pack screen (gamepad: via the pause menu)"),
        TEXT("K — Research — research tree screen (gamepad: via the pause menu)"),
        TEXT("P — Field Journal — bestiary + species knowledge (gamepad: via the pause menu)"),
        TEXT("I — EchoDex — searchable species index: search + element/zone/role facets + discovery gating (EDX-1, KB/M only)"),
        TEXT("L — Echo Roster — captured Echoes + party ring (gamepad: via the pause menu)"),
        TEXT("M — World Map — zone map + quest highlight (gamepad: via the pause menu)"),
        TEXT("U — Hunt Board — post-game cull contracts (gamepad: via the pause menu)"),
        TEXT("Escape — Pause — pause menu (gamepad: Start)"),
        TEXT("F5 — Quick Save — write the world save (KB/M only)"),
        TEXT("F9 — Quick Load — restore the world save (KB/M only)"),
        TEXT("F6 — Toggle HUD — hide/show the 22-element HUD for a clean view (CGC-4, KB/M only)"),
        TEXT("Left Ctrl — Descend — skiff descent (gamepad: left stick click)"),
        TEXT("T — Party Ability — every owned Echo casts its best ready ability (gamepad: right stick click)"),
        TEXT("Y — Smart-Cast — best ready player skill (gamepad: LB + X chord)"),
    };
    return Rows;
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildControlsScreenWidget::UAstrawildControlsScreenWidget()
{
    // F-05 convention: focusable so ESC closes without a prior mouse click.
    bIsFocusable = true;
}

FReply UAstrawildControlsScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // No letter-key toggle by design (no new input action — census 32): ESC
    // and the Close button are the dismiss paths, mirroring the other screens.
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleControlsScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildControlsScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

void UAstrawildControlsScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ControlsRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.92f, 0.95f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Full control map")));

    SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsSummary"));
    SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
    SummaryText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    SummaryText->SetText(FText::FromString(TEXT("33 runtime actions — keyboard/mouse plus a full gamepad companion context (transcribed from the live input build)")));

    RowsList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ControlsRows"));

    GamepadNoteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsGamepadNote"));
    GamepadNoteText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.95f, 0.85f, 1.0f)));
    GamepadNoteText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    GamepadNoteText->SetAutoWrapText(true);
    // INPUT_REFERENCE §1b: the chord line + the keyboard-only footnote.
    GamepadNoteText->SetText(FText::FromString(TEXT(
        "Gamepad: LB held + X = player smart-cast (DCP-6 chord — plain X stays Dodge). "
        "Scan / drone / robot / save / load stay keyboard-only on gamepad; the four face "
        "buttons and D-pad are fully allocated.")));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ControlsClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [ESC]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildControlsScreenWidget::HandleCloseClicked);

    // One read-only text row per action (alternating tint for scan readability).
    const TArray<FString>& Rows = GetControlRows();
    for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
    {
        UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
            *FString::Printf(TEXT("ControlsRow_%d"), RowIndex));
        Row->SetColorAndOpacity(FSlateColor(RowIndex % 2 == 0
            ? FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)
            : FLinearColor(0.82f, 0.86f, 0.88f, 1.0f)));
        Row->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
        Row->SetAutoWrapText(true);
        Row->SetText(FText::FromString(Rows[RowIndex]));
        RowsList->AddChild(Row);
    }

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-ControlsPanelWidth * 0.5f, -ControlsPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(ControlsPanelWidth, 30.0f));
    }
    if (UCanvasPanelSlot* SummarySlot = Canvas->AddChildToCanvas(SummaryText))
    {
        SummarySlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SummarySlot->SetPosition(FVector2D(-ControlsPanelWidth * 0.5f, -ControlsPanelHeight * 0.5f + 34.0f));
        SummarySlot->SetSize(FVector2D(ControlsPanelWidth, 22.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(RowsList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-ControlsPanelWidth * 0.5f, -ControlsPanelHeight * 0.5f + 62.0f));
        ListSlot->SetSize(FVector2D(ControlsPanelWidth, ControlsPanelHeight - 160.0f));
    }
    if (UCanvasPanelSlot* GamepadSlot = Canvas->AddChildToCanvas(GamepadNoteText))
    {
        GamepadSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        GamepadSlot->SetPosition(FVector2D(-ControlsPanelWidth * 0.5f, ControlsPanelHeight * 0.5f - 92.0f));
        GamepadSlot->SetSize(FVector2D(ControlsPanelWidth, 52.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(ControlsPanelWidth * 0.5f - 140.0f, ControlsPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(140.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildControlsScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleControlsScreen();
    }
}
