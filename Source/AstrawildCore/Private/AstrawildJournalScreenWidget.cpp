#include "AstrawildJournalScreenWidget.h"

#include "AstrawildAbilityLibrary.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoMutator.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildMountComponent.h"
#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
    constexpr float JournalPanelWidth = 760.0f;
    constexpr float JournalPanelHeight = 620.0f;
}

// ---------------------------------------------------------------------------
// Knowledge classification (pure — the automation contract pins this)
// ---------------------------------------------------------------------------

UAstrawildJournalScreenWidget::EKnowledgeState UAstrawildJournalScreenWidget::ClassifyKnowledgeState(const FAstrawildJournalEntry& Entry)
{
    if (IsEntryDiscovered(Entry))
    {
        const bool bAllKnowledge = Entry.bScanned && Entry.bFoodDiscovered && Entry.bHabitatDiscovered && Entry.bWeaknessDiscovered;
        return bAllKnowledge ? EKnowledgeState::Studied : EKnowledgeState::Observed;
    }
    return EKnowledgeState::Unknown;
}

bool UAstrawildJournalScreenWidget::IsEntryDiscovered(const FAstrawildJournalEntry& Entry)
{
    // Any real contact with the species reveals it in the journal: a completed
    // scan, any knowledge flag, partial observation progress, or an encounter.
    return Entry.bScanned || Entry.bFoodDiscovered || Entry.bHabitatDiscovered
        || Entry.bWeaknessDiscovered || Entry.ObservationProgress > 0.0f || Entry.TimesEncountered > 0;
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildJournalScreenWidget::UAstrawildJournalScreenWidget()
{
    // Final-audit F-05 convention: focusable so P/ESC close without a prior
    // mouse click in UIOnly input mode.
    bIsFocusable = true;
}

FReply UAstrawildJournalScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::P || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleJournalScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildJournalScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshJournal();
}

void UAstrawildJournalScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("JournalRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.92f, 0.78f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Field Journal")));

    SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalSummary"));
    SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    SummaryText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));

    SpeciesList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JournalSpeciesList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JournalClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [P]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildJournalScreenWidget::HandleCloseClicked);

    // --- DCP-5: the detail view (swaps with the list in the same region) ---
    DetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("JournalDetailPanel"));
    DetailPanel->SetBrushColor(FLinearColor(0.07f, 0.09f, 0.11f, 0.96f));
    DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JournalDetailScroll"));
    DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalDetailText"));
    DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));
    DetailText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    DetailText->SetAutoWrapText(true);
    DetailText->SetMinDesiredWidth(JournalPanelWidth - 60.0f);
    DetailScroll->AddChild(DetailText);
    DetailPanel->SetContent(DetailScroll);

    BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JournalBack"));
    BackButton->SetBackgroundColor(FLinearColor(0.2f, 0.35f, 0.3f, 1.0f));
    UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalBackLabel"));
    BackLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    BackLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    BackLabel->SetText(FText::FromString(TEXT("Back to list")));
    BackButton->AddChild(BackLabel);
    BackButton->OnClicked.AddDynamic(this, &UAstrawildJournalScreenWidget::HandleBackToListClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(JournalPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* SummarySlot = Canvas->AddChildToCanvas(SummaryText))
    {
        SummarySlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SummarySlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f + 36.0f));
        SummarySlot->SetSize(FVector2D(JournalPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(SpeciesList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f + 68.0f));
        ListSlot->SetSize(FVector2D(JournalPanelWidth, JournalPanelHeight - 120.0f));
    }
    if (UCanvasPanelSlot* DetailSlot = Canvas->AddChildToCanvas(DetailPanel))
    {
        // Same region as the list — the two views swap (only one visible).
        DetailSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        DetailSlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f + 68.0f));
        DetailSlot->SetSize(FVector2D(JournalPanelWidth, JournalPanelHeight - 164.0f));
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UCanvasPanelSlot* BackSlot = Canvas->AddChildToCanvas(BackButton))
    {
        // Sits beside the Close button in detail mode.
        BackSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        BackSlot->SetPosition(FVector2D(JournalPanelWidth * 0.5f - 270.0f, JournalPanelHeight * 0.5f - 38.0f));
        BackSlot->SetSize(FVector2D(130.0f, 32.0f));
        BackButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(JournalPanelWidth * 0.5f - 130.0f, JournalPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildJournalScreenWidget::RefreshJournal()
{
    if (!SpeciesList)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    if (!Registry || !Journal)
    {
        return;
    }

    SpeciesList->ClearChildren();

    const TArray<UAstrawildEchoDefinition*> Definitions = Registry->GetAllEchoDefinitions();

    int32 KnownCount = 0;
    int32 StudiedCount = 0;
    int32 RowIndex = 0;

    for (const UAstrawildEchoDefinition* Def : Definitions)
    {
        if (!Def)
        {
            continue;
        }

        const FAstrawildJournalEntry Entry = Journal->GetEntry(Def->DefinitionId);
        const EKnowledgeState State = ClassifyKnowledgeState(Entry);
        if (State != EKnowledgeState::Unknown)
        {
            ++KnownCount;
        }
        if (State == EKnowledgeState::Studied)
        {
            ++StudiedCount;
        }

        // One row per species. DCP-5: DISCOVERED rows are clickable (the row
        // widget — click opens the per-species detail view); UNKNOWN entries
        // keep the bare read-only TextBlock (nothing to detail, 229 rows stay
        // cheap where interaction is meaningless).
        const bool bDiscoveredRow = State != EKnowledgeState::Unknown;

        FString RowText;
        FLinearColor RowColor(0.62f, 0.64f, 0.68f, 1.0f); // dim unknown
        if (State == EKnowledgeState::Unknown)
        {
            RowText = TEXT("???  —  signal unresolved (observe to reveal)");
        }
        else
        {
            RowColor = State == EKnowledgeState::Studied
                ? FLinearColor(0.62f, 0.92f, 0.72f, 1.0f)
                : FLinearColor(0.95f, 0.93f, 0.85f, 1.0f);

            // FPP-1: the weakness flag names the ELEMENT once discovered — the
            // knowledge that actually changes combat decisions, not a bare checkmark.
            const FString WeaknessFlag = Entry.bWeaknessDiscovered
                ? (Def->WeaknessElement != EAstrawildElementType::None
                    ? UEnum::GetDisplayValueAsText(Def->WeaknessElement).ToString()
                    : FString(TEXT("none found")))
                : FString(TEXT("\u2717"));

            const FString FlagLine = FString::Printf(TEXT("Scanned %s  Food %s  Habitat %s  Weakness: %s"),
                Entry.bScanned ? TEXT("\u2713") : TEXT("\u2717"),
                Entry.bFoodDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
                Entry.bHabitatDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
                *WeaknessFlag);

            // FPP-1: what the species can DO — its abilities (with the level that
            // unlocks them) and whether it can carry a rider. The codex now answers
            // "why is this Echo worth capturing" instead of only naming it.
            FString AbilityLine;
            const TArray<FName> AbilityIds = UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(Def);
            for (const FName AbilityId : AbilityIds)
            {
                const FAstrawildAbilityData* Ability = UAstrawildAbilityLibrary::FindAbility(AbilityId);
                if (!Ability)
                {
                    continue;
                }
                if (!AbilityLine.IsEmpty())
                {
                    AbilityLine += TEXT(", ");
                }
                AbilityLine += FString::Printf(TEXT("%s (Lv %d)"),
                    *Ability->DisplayName.ToString(), FMath::Max(1, Ability->UnlockLevel));
            }
            if (!AbilityLine.IsEmpty())
            {
                AbilityLine = FString::Printf(TEXT("\nAbilities: %s"), *AbilityLine);
            }
            if (Def->Passive != EAstrawildEchoPassive::None)
            {
                AbilityLine += FString::Printf(TEXT("\nPassive: %s"),
                    *UEnum::GetDisplayValueAsText(Def->Passive).ToString());
            }
            if (UAstrawildMountComponent::IsRideableSpecies(Def->Family, Def->BodyPlan, Def->SizeClass))
            {
                AbilityLine += FString::Printf(TEXT("\nRideable (Bond %d)"),
                    FMath::RoundToInt(UAstrawildMountComponent::MountBondGate));
            }

            RowText = FString::Printf(TEXT("%s\n%s · %s · %s · %s%s\n%s  %d%% observed · %d encounter%s%s"),
                *Def->DisplayName.ToString(),
                *UEnum::GetDisplayValueAsText(Def->Element).ToString(),
                *UEnum::GetDisplayValueAsText(Def->Role).ToString(),
                *UEnum::GetDisplayValueAsText(Def->Rarity).ToString(),
                *UEnum::GetDisplayValueAsText(AAstrawildEchoCharacter::ComputeVisualBand(
                    Def->Family, Def->BodyPlan, Def->SizeClass)).ToString(),
                *AbilityLine,
                *FlagLine,
                FMath::RoundToInt(Entry.ObservationProgress),
                Entry.TimesEncountered,
                Entry.TimesEncountered == 1 ? TEXT("") : TEXT("s"),
                TEXT(""));
        }

        if (bDiscoveredRow)
        {
            UAstrawildJournalRowWidget* RowWidget = WidgetTree->ConstructWidget<UAstrawildJournalRowWidget>(
                UAstrawildJournalRowWidget::StaticClass(), *FString::Printf(TEXT("JournalRow%d"), RowIndex++));
            if (RowWidget)
            {
                RowWidget->InitializeRow(this, Def->DefinitionId, RowText, RowColor);
                if (UScrollBoxSlot* RowSlot = SpeciesList->AddChild(RowWidget))
                {
                    RowSlot->SetPadding(FMargin(2.0f, 2.0f, 2.0f, 1.0f));
                }
            }
        }
        else
        {
            UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("JournalRow%d"), RowIndex++));
            Row->SetColorAndOpacity(FSlateColor(RowColor));
            Row->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
            Row->SetText(FText::FromString(RowText));
            if (UScrollBoxSlot* RowSlot = SpeciesList->AddChild(Row))
            {
                RowSlot->SetPadding(FMargin(6.0f, 5.0f, 6.0f, 2.0f));
            }
        }
    }

    // Totals always derive from the registry — never a hardcoded census value.
    SummaryText->SetText(FText::FromString(FString::Printf(TEXT("%d of %d species observed · %d fully studied"),
        KnownCount, Definitions.Num(), StudiedCount)));
}

void UAstrawildJournalScreenWidget::ShowSpeciesDetail(const FName SpeciesId)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    UAstrawildEchoDefinition* Def = Registry ? Registry->FindEcho(SpeciesId) : nullptr;
    if (!Def || !Journal)
    {
        return;
    }

    DetailSpeciesId = SpeciesId;
    const FAstrawildJournalEntry Entry = Journal->GetEntry(SpeciesId);
    if (DetailText)
    {
        DetailText->SetText(FText::FromString(BuildSpeciesDetailText(Def, Entry)));
    }
    if (SummaryText)
    {
        // The summary line doubles as the detail's context header.
        SummaryText->SetText(FText::FromString(FString::Printf(TEXT("%s — codex entry %d"),
            *Def->DisplayName.ToString(), FMath::Max(1, Def->CodexIndex))));
    }
    if (SpeciesList)
    {
        SpeciesList->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::HitTestInvisible); // Text-only: scrolls by wheel over the panel.
    }
    if (BackButton)
    {
        BackButton->SetVisibility(ESlateVisibility::Visible);
    }
}

void UAstrawildJournalScreenWidget::BackToList()
{
    DetailSpeciesId = NAME_None;
    if (SpeciesList)
    {
        SpeciesList->SetVisibility(ESlateVisibility::Visible);
    }
    if (DetailPanel)
    {
        DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (BackButton)
    {
        BackButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    RefreshJournal(); // Totals + fresh rows (scans may have progressed).
}

void UAstrawildJournalScreenWidget::HandleBackToListClicked()
{
    BackToList();
}

FString UAstrawildJournalScreenWidget::BuildSpeciesDetailText(const UAstrawildEchoDefinition* Def, const FAstrawildJournalEntry& Entry)
{
    if (!Def)
    {
        return TEXT("signal unresolved — observe to reveal");
    }

    // Identity block.
    FString Detail = FString::Printf(TEXT("%s\n%s · %s · %s · %s\nFamily %s · %s · %s\n\n"),
        *Def->DisplayName.ToString(),
        *UEnum::GetDisplayValueAsText(Def->Element).ToString(),
        *UEnum::GetDisplayValueAsText(Def->Role).ToString(),
        *UEnum::GetDisplayValueAsText(Def->Rarity).ToString(),
        *UEnum::GetDisplayValueAsText(AAstrawildEchoCharacter::ComputeVisualBand(
            Def->Family, Def->BodyPlan, Def->SizeClass)).ToString(),
        *UEnum::GetDisplayValueAsText(Def->Family).ToString(),
        *UEnum::GetDisplayValueAsText(Def->BodyPlan).ToString(),
        *UEnum::GetDisplayValueAsText(Def->SizeClass).ToString());

    if (!Def->Description.IsEmpty())
    {
        Detail += Def->Description.ToString() + TEXT("\n\n");
    }

    // Base stats (core data — visible once the species is revealed).
    Detail += FString::Printf(TEXT("Stats: HP %d · ATK %d · DEF %d · SPD %d · Stamina %d · Capture resilience %d\n"),
        FMath::RoundToInt(Def->BaseStats.MaxHealth),
        FMath::RoundToInt(Def->BaseStats.AttackPower),
        FMath::RoundToInt(Def->BaseStats.Defense),
        FMath::RoundToInt(Def->BaseStats.MoveSpeed),
        FMath::RoundToInt(Def->BaseStats.Stamina),
        FMath::RoundToInt(Def->BaseStats.CaptureResilience));

    // Combat knowledge — gated exactly like the row list.
    if (Entry.bWeaknessDiscovered)
    {
        const FString WeaknessName = Def->WeaknessElement != EAstrawildElementType::None
            ? UEnum::GetDisplayValueAsText(Def->WeaknessElement).ToString()
            : FString(TEXT("none found"));
        Detail += FString::Printf(TEXT("Weakness: %s · Resist: %s\n"),
            *WeaknessName, *UEnum::GetDisplayValueAsText(Def->ElementalResistance).ToString());
    }
    else
    {
        Detail += TEXT("Weakness: \u2717 (undiscovered)\n");
    }

    // Habits.
    Detail += FString::Printf(TEXT("Habits: %s · %s"),
        *UEnum::GetDisplayValueAsText(Def->ActivityPattern).ToString(),
        *UEnum::GetDisplayValueAsText(Def->DominantPersonality).ToString());
    if (Def->bHostileToPlayers)
    {
        Detail += TEXT(" · HOSTILE");
    }
    Detail += TEXT("\n");

    // Habitat + food — knowledge-gated (the exploration loop keeps its value).
    if (Entry.bHabitatDiscovered)
    {
        Detail += FString::Printf(TEXT("Habitat: %s"),
            *UEnum::GetDisplayValueAsText(Def->HomeZone).ToString());
        if (Def->HabitatBiomeIds.Num() > 0)
        {
            Detail += TEXT(" (");
            for (int32 i = 0; i < Def->HabitatBiomeIds.Num(); ++i)
            {
                if (i > 0)
                {
                    Detail += TEXT(", ");
                }
                Detail += Def->HabitatBiomeIds[i].ToString();
            }
            Detail += TEXT(")");
        }
        Detail += TEXT("\n");
    }
    if (Entry.bFoodDiscovered && Def->PreferredFoodIds.Num() > 0)
    {
        Detail += TEXT("Food: ");
        for (int32 i = 0; i < Def->PreferredFoodIds.Num(); ++i)
        {
            if (i > 0)
            {
                Detail += TEXT(", ");
            }
            Detail += Def->PreferredFoodIds[i].ToString();
        }
        Detail += TEXT("\n");
    }

    // Ability kit (the "why capture this" answer — same source as the rows).
    {
        FString AbilityLine;
        const TArray<FName> AbilityIds = UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(Def);
        for (const FName AbilityId : AbilityIds)
        {
            const FAstrawildAbilityData* Ability = UAstrawildAbilityLibrary::FindAbility(AbilityId);
            if (!Ability)
            {
                continue;
            }
            if (!AbilityLine.IsEmpty())
            {
                AbilityLine += TEXT(", ");
            }
            AbilityLine += FString::Printf(TEXT("%s (Lv %d)"), *Ability->DisplayName.ToString(), FMath::Max(1, Ability->UnlockLevel));
        }
        if (!AbilityLine.IsEmpty())
        {
            Detail += FString::Printf(TEXT("Abilities: %s\n"), *AbilityLine);
        }
        if (Def->Passive != EAstrawildEchoPassive::None)
        {
            Detail += FString::Printf(TEXT("Passive: %s\n"), *UEnum::GetDisplayValueAsText(Def->Passive).ToString());
        }
        if (UAstrawildMountComponent::IsRideableSpecies(Def->Family, Def->BodyPlan, Def->SizeClass))
        {
            Detail += FString::Printf(TEXT("Rideable (Bond %d)\n"),
                FMath::RoundToInt(UAstrawildMountComponent::MountBondGate));
        }
    }

    // Sci-Fantasy mutation spec (the species' body recipe — theme, base
    // geometry, part scales, attachments, material language, VFX, voice).
    {
        const FEchoMutationSpec* Spec = FAstrawildEchoMutator::FindSpec(Def->DefinitionId);
        FEchoMutationSpec DeterministicSpec;
        if (!Spec)
        {
            DeterministicSpec = FAstrawildEchoMutator::BuildDeterministicSpec(Def);
            Spec = &DeterministicSpec;
        }
        if (Spec)
        {
            Detail += FString::Printf(TEXT("\nBody plan: %s theme · base %s\n"),
                *UEnum::GetDisplayValueAsText(Spec->Theme).ToString(),
                *Spec->BaseMeshId.ToString());
            Detail += FString::Printf(TEXT("  Scales H %.2f / T %.2f / L %.2f / X %.2f · %d attachment%s · %s · %s\n"),
                Spec->HeadScale, Spec->TorsoScale, Spec->LimbScale, Spec->TailScale,
                FMath::PopCount(static_cast<uint32>(Spec->AttachmentMask)),
                FMath::PopCount(static_cast<uint32>(Spec->AttachmentMask)) == 1 ? TEXT("") : TEXT("s"),
                *UEnum::GetDisplayValueAsText(Spec->MaterialTheme).ToString(),
                *UEnum::GetDisplayValueAsText(Spec->VfxType).ToString());
        }
    }

    // Evolution line + economy.
    if (!Def->EvolveToDefinitionId.IsNone())
    {
        Detail += FString::Printf(TEXT("\nEvolves to %s (Lv %d · Bond %d)\n"),
            *Def->EvolveToDefinitionId.ToString(), FMath::Max(1, Def->EvolveRequiredLevel),
            FMath::RoundToInt(Def->EvolveRequiredBond));
    }
    if (Def->DefeatLoot.Num() > 0)
    {
        Detail += TEXT("Loot: ");
        for (int32 i = 0; i < Def->DefeatLoot.Num(); ++i)
        {
            if (i > 0)
            {
                Detail += TEXT(", ");
            }
            Detail += FString::Printf(TEXT("%s x%d"),
                *Def->DefeatLoot[i].ItemId.ToString(), Def->DefeatLoot[i].Quantity);
        }
        Detail += TEXT("\n");
    }
    Detail += FString::Printf(TEXT("Capture difficulty: %.0f%%\n"), FMath::Clamp(Def->CaptureDifficulty, 0.0f, 1.0f) * 100.0f);

    // Study state footer.
    Detail += FString::Printf(TEXT("\nStudy: scanned %s · food %s · habitat %s · weakness %s · %d%% observed · %d encounter%s"),
        Entry.bScanned ? TEXT("\u2713") : TEXT("\u2717"),
        Entry.bFoodDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
        Entry.bHabitatDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
        Entry.bWeaknessDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
        FMath::RoundToInt(Entry.ObservationProgress),
        Entry.TimesEncountered,
        Entry.TimesEncountered == 1 ? TEXT("") : TEXT("s"));

    return Detail;
}

// ---------------------------------------------------------------------------
// DCP-5: clickable journal row (the RosterRow pattern)
// ---------------------------------------------------------------------------

void UAstrawildJournalRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return; // Already built (double-construct guard).
    }

    RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JournalRowButton"));
    RowButton->SetBackgroundColor(FLinearColor(0.14f, 0.17f, 0.20f, 0.55f)); // Subtle: rows read as text until hovered.
    RowButton->SetClickMethod(EButtonClickMethod::MouseDown);
    WidgetTree->RootWidget = RowButton;

    RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalRowLabel"));
    RowLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    RowButton->AddChild(RowLabel);

    RowButton->OnClicked.AddDynamic(this, &UAstrawildJournalRowWidget::HandleClicked);
}

void UAstrawildJournalRowWidget::InitializeRow(UAstrawildJournalScreenWidget* InParent, const FName InSpeciesId, const FString& RowText, const FLinearColor& RowColor)
{
    ParentScreen = InParent;
    SpeciesId = InSpeciesId;

    if (WidgetTree && !WidgetTree->RootWidget)
    {
        NativeConstruct();
    }
    if (RowLabel)
    {
        RowLabel->SetColorAndOpacity(FSlateColor(RowColor));
        RowLabel->SetText(FText::FromString(RowText));
    }
}

void UAstrawildJournalRowWidget::HandleClicked()
{
    if (ParentScreen && !SpeciesId.IsNone())
    {
        ParentScreen->ShowSpeciesDetail(SpeciesId);
    }
}

void UAstrawildJournalScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleJournalScreen();
    }
}
