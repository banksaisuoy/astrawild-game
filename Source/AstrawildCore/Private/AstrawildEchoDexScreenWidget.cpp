#include "AstrawildEchoDexScreenWidget.h"

#include "AstrawildEchoDexSubsystem.h"
#include "AstrawildJournalScreenWidget.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildPlayerController.h"

#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

namespace
{
    constexpr float DexPanelWidth = 900.0f;
    constexpr float DexPanelHeight = 700.0f;

    FString ElementLabel(const EAstrawildElementType Element)
    {
        switch (Element)
        {
        case EAstrawildElementType::Light:  return TEXT("Light");
        case EAstrawildElementType::Ash:    return TEXT("Ash");
        case EAstrawildElementType::Flora:  return TEXT("Flora");
        case EAstrawildElementType::Frost:  return TEXT("Frost");
        case EAstrawildElementType::Pulse:  return TEXT("Pulse");
        case EAstrawildElementType::Ember:  return TEXT("Ember");
        default:                            return TEXT("All Elements");
        }
    }

    FString ZoneLabel(const EAstrawildZone Zone)
    {
        switch (Zone)
        {
        case EAstrawildZone::DawnFields:          return TEXT("Dawn Fields");
        case EAstrawildZone::DuskMarsh:           return TEXT("Dusk Marsh");
        case EAstrawildZone::EmberRidge:          return TEXT("Ember Ridge");
        case EAstrawildZone::FrostveilExpanse:    return TEXT("Frostveil Expanse");
        case EAstrawildZone::Glimmerwood:         return TEXT("Glimmerwood");
        case EAstrawildZone::HollowApproach:      return TEXT("Hollow Approach");
        case EAstrawildZone::AzureShallows:       return TEXT("Azure Shallows");
        case EAstrawildZone::TidebreakerIsles:    return TEXT("Tidebreaker Isles");
        case EAstrawildZone::SunscarDesert:       return TEXT("Sunscar Desert");
        case EAstrawildZone::StormcrestHighlands: return TEXT("Stormcrest Highlands");
        case EAstrawildZone::VerdantReach:        return TEXT("Verdant Reach");
        case EAstrawildZone::PearlseaReef:        return TEXT("Pearlsea Reef");
        default:                                  return TEXT("All Zones");
        }
    }

    FString RoleLabel(const EAstrawildEchoRole Role)
    {
        switch (Role)
        {
        case EAstrawildEchoRole::Explorer: return TEXT("Explorer");
        case EAstrawildEchoRole::Combat:   return TEXT("Combat");
        case EAstrawildEchoRole::Base:     return TEXT("Base");
        default:                           return TEXT("Support");
        }
    }

    FString DiscoveryLabel(const EAstrawildDexDiscoveryState State)
    {
        switch (State)
        {
        case EAstrawildDexDiscoveryState::Unknown:  return TEXT("Undiscovered");
        case EAstrawildDexDiscoveryState::Seen:     return TEXT("Seen");
        default:                                    return TEXT("Captured");
        }
    }

    /** Cycle Element: All -> Light -> ... -> Ember -> All. */
    EAstrawildElementType NextElement(const EAstrawildElementType Current)
    {
        switch (Current)
        {
        case EAstrawildElementType::None:  return EAstrawildElementType::Light;
        case EAstrawildElementType::Light: return EAstrawildElementType::Ash;
        case EAstrawildElementType::Ash:   return EAstrawildElementType::Flora;
        case EAstrawildElementType::Flora: return EAstrawildElementType::Frost;
        case EAstrawildElementType::Frost: return EAstrawildElementType::Pulse;
        case EAstrawildElementType::Pulse: return EAstrawildElementType::Ember;
        default:                           return EAstrawildElementType::None;
        }
    }

    /** Cycle Zone: All -> DawnFields -> ... -> PearlseaReef -> All (canon order). */
    EAstrawildZone NextZone(const EAstrawildZone Current)
    {
        switch (Current)
        {
        case EAstrawildZone::None:               return EAstrawildZone::DawnFields;
        case EAstrawildZone::DawnFields:         return EAstrawildZone::DuskMarsh;
        case EAstrawildZone::DuskMarsh:          return EAstrawildZone::EmberRidge;
        case EAstrawildZone::EmberRidge:         return EAstrawildZone::FrostveilExpanse;
        case EAstrawildZone::FrostveilExpanse:   return EAstrawildZone::Glimmerwood;
        case EAstrawildZone::Glimmerwood:        return EAstrawildZone::HollowApproach;
        case EAstrawildZone::HollowApproach:     return EAstrawildZone::AzureShallows;
        case EAstrawildZone::AzureShallows:      return EAstrawildZone::TidebreakerIsles;
        case EAstrawildZone::TidebreakerIsles:   return EAstrawildZone::SunscarDesert;
        case EAstrawildZone::SunscarDesert:      return EAstrawildZone::StormcrestHighlands;
        case EAstrawildZone::StormcrestHighlands:return EAstrawildZone::VerdantReach;
        case EAstrawildZone::VerdantReach:       return EAstrawildZone::PearlseaReef;
        default:                                 return EAstrawildZone::None;
        }
    }

    /** Cycle Role: All -> Explorer -> Combat -> Base -> Support -> All. */
    void NextRole(FAstrawildDexQuery& Query)
    {
        if (!Query.bFilterRole)
        {
            Query.bFilterRole = true;
            Query.Role = EAstrawildEchoRole::Explorer;
        }
        else
        {
            switch (Query.Role)
            {
            case EAstrawildEchoRole::Explorer: Query.Role = EAstrawildEchoRole::Combat; break;
            case EAstrawildEchoRole::Combat:   Query.Role = EAstrawildEchoRole::Base;   break;
            case EAstrawildEchoRole::Base:     Query.Role = EAstrawildEchoRole::Support;break;
            default:                           Query.bFilterRole = false;               break;
            }
        }
    }

    /** Cycle Discovery: All -> Undiscovered -> Seen -> Captured -> All. */
    void NextDiscovery(FAstrawildDexQuery& Query)
    {
        if (!Query.bFilterDiscovery)
        {
            Query.bFilterDiscovery = true;
            Query.Discovery = EAstrawildDexDiscoveryState::Unknown;
        }
        else
        {
            switch (Query.Discovery)
            {
            case EAstrawildDexDiscoveryState::Unknown:  Query.Discovery = EAstrawildDexDiscoveryState::Seen;     break;
            case EAstrawildDexDiscoveryState::Seen:     Query.Discovery = EAstrawildDexDiscoveryState::Captured; break;
            default:                                    Query.bFilterDiscovery = false;                          break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildDexRowWidget::BuildRow(const UAstrawildEchoDefinition* Def,
    EAstrawildDexDiscoveryState State, bool bHighlighted)
{
    if (!Def || WidgetTree->RootWidget)
    {
        return;
    }
    SpeciesId = Def->DefinitionId;

    UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DexRowButton"));
    // Nexus #4020 parity — highlighted rows get the warm tint, idle rows dim.
    RowButton->SetBackgroundColor(bHighlighted
        ? FLinearColor(0.85f, 0.62f, 0.18f, 0.55f)
        : FLinearColor(0.16f, 0.18f, 0.22f, 0.85f));
    RowButton->OnClicked.AddDynamic(this, &UAstrawildDexRowWidget::HandleClicked);

    RowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexRowText"));
    const TCHAR* StateTag = State == EAstrawildDexDiscoveryState::Captured ? TEXT("[*] ")
        : State == EAstrawildDexDiscoveryState::Seen ? TEXT("[o] ")
        : TEXT("[?] ");
    // Paldeck gating: undiscovered rows never leak the species name.
    const FString ShownName = State == EAstrawildDexDiscoveryState::Unknown ? TEXT("???") : Def->DisplayName.ToString();
    const FString ShownElement = State == EAstrawildDexDiscoveryState::Unknown ? TEXT("???") : ElementLabel(Def->Element);
    const FString ShownZone = State == EAstrawildDexDiscoveryState::Unknown ? TEXT("???") : ZoneLabel(Def->HomeZone);
    RowText->SetText(FText::FromString(FString::Printf(TEXT("%s%-26s %-8s %s"),
        StateTag, *ShownName, *ShownElement, *ShownZone)));
    RowText->SetColorAndOpacity(FSlateColor(bHighlighted
        ? FLinearColor(1.0f, 0.85f, 0.35f, 1.0f)
        : FLinearColor(0.92f, 0.94f, 0.96f, 1.0f)));
    RowText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));
    RowButton->AddChild(RowText);

    WidgetTree->RootWidget = RowButton;
}

void UAstrawildDexRowWidget::HandleClicked()
{
    if (OwnerScreen.IsValid())
    {
        OwnerScreen->SelectSpecies(SpeciesId);
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildEchoDexScreenWidget::UAstrawildEchoDexScreenWidget()
{
    // Final-audit F-05 parity: focusable so ESC reaches NativeOnKeyDown in UIOnly mode.
    bIsFocusable = true;
}

void UAstrawildEchoDexScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildNativeUi();
    RefreshIndex();

    if (UAstrawildEchoDexSubsystem* Dex = GetDex())
    {
        Dex->OnDexUpdated.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleDexUpdated);
    }
}

void UAstrawildEchoDexScreenWidget::NativeDestruct()
{
    if (UAstrawildEchoDexSubsystem* Dex = GetDex())
    {
        Dex->OnDexUpdated.RemoveDynamic(this, &UAstrawildEchoDexScreenWidget::HandleDexUpdated);
    }
    Super::NativeDestruct();
}

FReply UAstrawildEchoDexScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleEchoDexScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UAstrawildEchoDexSubsystem* UAstrawildEchoDexScreenWidget::GetDex() const
{
    return GetWorld() ? GetWorld()->GetSubsystem<UAstrawildEchoDexSubsystem>() : nullptr;
}

void UAstrawildEchoDexScreenWidget::BuildNativeUi()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DexRoot"));

    UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.9f, 0.95f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("ECHODEX — Species Index")));
    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-DexPanelWidth * 0.5f, -DexPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(DexPanelWidth, 30.0f));
    }

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexStatus"));
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    StatusText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
    if (UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(StatusText))
    {
        StatusSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        StatusSlot->SetPosition(FVector2D(-DexPanelWidth * 0.5f, -DexPanelHeight * 0.5f + 34.0f));
        StatusSlot->SetSize(FVector2D(DexPanelWidth, 22.0f));
    }

    // Search + facet row: [Search: ________] [Element] [Zone] [Role] [Discovery]
    UHorizontalBox* QueryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DexQueryRow"));

    auto MakeFacetButton = [this](const FName ButtonName, const FString& InitialLabel) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
        Button->SetBackgroundColor(FLinearColor(0.18f, 0.42f, 0.38f, 1.0f));
        UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), NAME_None);
        LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        LabelText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 11));
        LabelText->SetText(FText::FromString(InitialLabel));
        Button->AddChild(LabelText);
        return Button;
    };

    UTextBlock* SearchLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexSearchLabel"));
    SearchLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SearchLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    SearchLabel->SetText(FText::FromString(TEXT("Search: ")));
    QueryRow->AddChildToHorizontalBox(SearchLabel);

    SearchBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("DexSearchBox"));
    SearchBox->SetHintText(FText::FromString(TEXT("Type a species name…")));
    SearchBox->OnTextChanged.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleSearchChanged);
    if (UHorizontalBoxSlot* SearchSlot = QueryRow->AddChildToHorizontalBox(SearchBox))
    {
        SearchSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ElementButton = MakeFacetButton(TEXT("DexElementFilter"), TEXT("All Elements"));
    ElementButton->OnClicked.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleElementClicked);
    QueryRow->AddChildToHorizontalBox(ElementButton);

    ZoneButton = MakeFacetButton(TEXT("DexZoneFilter"), TEXT("All Zones"));
    ZoneButton->OnClicked.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleZoneClicked);
    QueryRow->AddChildToHorizontalBox(ZoneButton);

    RoleButton = MakeFacetButton(TEXT("DexRoleFilter"), TEXT("All Roles"));
    RoleButton->OnClicked.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleRoleClicked);
    QueryRow->AddChildToHorizontalBox(RoleButton);

    DiscoveryButton = MakeFacetButton(TEXT("DexDiscoveryFilter"), TEXT("Any State"));
    DiscoveryButton->OnClicked.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleDiscoveryClicked);
    QueryRow->AddChildToHorizontalBox(DiscoveryButton);

    if (UCanvasPanelSlot* QuerySlot = Canvas->AddChildToCanvas(QueryRow))
    {
        QuerySlot->SetAnchors(FAnchors(0.5f, 0.5f));
        QuerySlot->SetPosition(FVector2D(-DexPanelWidth * 0.5f, -DexPanelHeight * 0.5f + 62.0f));
        QuerySlot->SetSize(FVector2D(DexPanelWidth, 30.0f));
    }

    ResultsList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DexResults"));
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(ResultsList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-DexPanelWidth * 0.5f, -DexPanelHeight * 0.5f + 100.0f));
        ListSlot->SetSize(FVector2D(DexPanelWidth, DexPanelHeight - 320.0f));
    }

    DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexDetail"));
    DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.92f, 0.95f, 1.0f)));
    DetailText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));
    DetailText->SetAutoWrapText(true);
    if (UCanvasPanelSlot* DetailSlot = Canvas->AddChildToCanvas(DetailText))
    {
        DetailSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        DetailSlot->SetPosition(FVector2D(-DexPanelWidth * 0.5f, DexPanelHeight * 0.5f - 206.0f));
        DetailSlot->SetSize(FVector2D(DexPanelWidth, 168.0f));
    }

    UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DexClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildEchoDexScreenWidget::HandleCloseClicked);
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DexCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [I/ESC]")));
    CloseButton->AddChild(CloseLabel);
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(DexPanelWidth * 0.5f - 140.0f, DexPanelHeight * 0.5f - 36.0f));
        CloseSlot->SetSize(FVector2D(140.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildEchoDexScreenWidget::RefreshFacetLabels()
{
    auto SetButtonLabel = [](UButton* Button, const FString& Label)
    {
        if (Button)
        {
            if (UTextBlock* LabelText = Cast<UTextBlock>(Button->GetChildAt(0)))
            {
                LabelText->SetText(FText::FromString(Label));
            }
        }
    };
    SetButtonLabel(ElementButton, ElementLabel(ActiveQuery.Element));
    SetButtonLabel(ZoneButton, ZoneLabel(ActiveQuery.Zone));
    SetButtonLabel(RoleButton, ActiveQuery.bFilterRole ? RoleLabel(ActiveQuery.Role) : FString(TEXT("All Roles")));
    SetButtonLabel(DiscoveryButton, ActiveQuery.bFilterDiscovery ? DiscoveryLabel(ActiveQuery.Discovery) : FString(TEXT("Any State")));
}

void UAstrawildEchoDexScreenWidget::RefreshIndex()
{
    if (!ResultsList)
    {
        return;
    }
    RefreshFacetLabels();

    ResultsList->ClearChildren();
    UAstrawildEchoDexSubsystem* Dex = GetDex();

    const FString Needle = ActiveQuery.Text.TrimStartAndEnd().ToLower();
    TArray<UAstrawildEchoDefinition*> Results = Dex ? Dex->Search(ActiveQuery) : TArray<UAstrawildEchoDefinition*>();

    UAstrawildDexRowWidget* FirstHighlightedRow = nullptr;
    for (UAstrawildEchoDefinition* Def : Results)
    {
        if (!Def)
        {
            continue;
        }
        const bool bHighlighted = !Needle.IsEmpty()
            && (Def->DisplayName.ToString().ToLower().Contains(Needle)
                || Def->DefinitionId.ToString().ToLower().Contains(Needle));
        const EAstrawildDexDiscoveryState State = Dex ? Dex->GetDiscoveryState(Def->DefinitionId)
            : EAstrawildDexDiscoveryState::Unknown;

        UAstrawildDexRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildDexRowWidget>(
            UAstrawildDexRowWidget::StaticClass(), NAME_None);
        Row->OwnerScreen = this;
        Row->BuildRow(Def, State, bHighlighted);
        ResultsList->AddChild(Row);
        if (bHighlighted && !FirstHighlightedRow)
        {
            FirstHighlightedRow = Row;
        }
    }

    // Nexus #4020 parity — "jumping to the page containing the Pals":
    // jump the list to the first highlighted match.
    if (FirstHighlightedRow)
    {
        ResultsList->ScrollWidgetIntoView(FirstHighlightedRow, false);
    }

    if (StatusText)
    {
        const int32 Total = Dex ? Dex->GetTotalSpeciesCount() : 0;
        const int32 Seen = Dex ? Dex->GetSeenCount() : 0;
        const int32 Captured = Dex ? Dex->GetCapturedCount() : 0;
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("Shown %d / %d   ·   Seen %d   ·   Captured %d"),
            Results.Num(), Total, Seen, Captured)));
    }

    if (Dex && !SelectedSpeciesId.IsNone())
    {
        SelectSpecies(SelectedSpeciesId);
    }
    else if (DetailText)
    {
        DetailText->SetText(FText::FromString(TEXT("Select a species to inspect it.")));
    }
}

void UAstrawildEchoDexScreenWidget::SelectSpecies(FName SpeciesId)
{
    SelectedSpeciesId = SpeciesId;
    if (!DetailText)
    {
        return;
    }
    UAstrawildEchoDexSubsystem* Dex = GetDex();
    if (!Dex || SpeciesId.IsNone())
    {
        DetailText->SetText(FText::FromString(TEXT("Select a species to inspect it.")));
        return;
    }

    const UAstrawildEchoDefinition* Def = Dex->FindSpecies(SpeciesId);
    if (!Def)
    {
        DetailText->SetText(FText::FromString(TEXT("Select a species to inspect it.")));
        return;
    }

    const EAstrawildDexDiscoveryState State = Dex->GetDiscoveryState(SpeciesId);
    if (State == EAstrawildDexDiscoveryState::Unknown)
    {
        // Paldeck gating — undiscovered entries stay locked.
        DetailText->SetText(FText::FromString(TEXT("???\n\nDiscover this Echo in the wild to reveal its entry.")));
        return;
    }

    const UAstrawildJournalSubsystem* Journal = GetWorld()
        ? GetWorld()->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    const FAstrawildJournalEntry Entry = Journal ? Journal->GetEntry(SpeciesId) : FAstrawildJournalEntry();

    FString Detail = UAstrawildJournalScreenWidget::BuildSpeciesDetailText(Def, Entry);
    if (State == EAstrawildDexDiscoveryState::Captured)
    {
        Detail += TEXT("\n[CAPTURED] — full entry unlocked.");
    }
    Detail += FString::Printf(TEXT("\nZone collection: %d%%"),
        FMath::RoundToInt(Dex->GetZoneProgress(Def->HomeZone) * 100.0f));
    DetailText->SetText(FText::FromString(Detail));
}

void UAstrawildEchoDexScreenWidget::HandleElementClicked()
{
    ActiveQuery.Element = NextElement(ActiveQuery.Element);
    RefreshIndex();
}

void UAstrawildEchoDexScreenWidget::HandleZoneClicked()
{
    ActiveQuery.Zone = NextZone(ActiveQuery.Zone);
    RefreshIndex();
}

void UAstrawildEchoDexScreenWidget::HandleRoleClicked()
{
    NextRole(ActiveQuery);
    RefreshIndex();
}

void UAstrawildEchoDexScreenWidget::HandleDiscoveryClicked()
{
    NextDiscovery(ActiveQuery);
    RefreshIndex();
}

void UAstrawildEchoDexScreenWidget::HandleSearchChanged(const FText& Text)
{
    ActiveQuery.Text = Text.ToString();
    RefreshIndex();
}

void UAstrawildEchoDexScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleEchoDexScreen();
    }
}

void UAstrawildEchoDexScreenWidget::HandleDexUpdated(FName SpeciesId)
{
    // Live refresh while open — captures/encounters update the open screen.
    RefreshIndex();
}
