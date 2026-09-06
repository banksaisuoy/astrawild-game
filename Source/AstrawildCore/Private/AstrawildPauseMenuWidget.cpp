#include "AstrawildPauseMenuWidget.h"

#include "AstrawildAttributeComponent.h"
#include "AstrawildCore.h"
#include "AstrawildGameState.h" // DCP-2: post-game gate for the NG+ button.
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "AstrawildLANSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr float PausePanelWidth = 340.0f;
    // DP-4: 3 menu buttons + the loadout caption + 3 slot buttons — the pause
    // panel grew with the skill loadout section.
    constexpr float PauseMenuHeight = 520.0f; // LCP-6: + LAN CO-OP panel
    constexpr int32 SkillLoadoutSlots = 3;
}


UAstrawildPauseMenuWidget::UAstrawildPauseMenuWidget()
{
    // Final-audit F-05: focusable so ESC-resume reaches NativeOnKeyDown in UIOnly mode.
    bIsFocusable = true;
}

FReply UAstrawildPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Final-audit F-05: ESC resumes — the universal pause convention the menu
    // previously only claimed via its Resume button.
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->TogglePauseMenu();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshSkillSlotLabels();
    RefreshLanStatus(); // LCP-6
}

void UAstrawildPauseMenuWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.92f, 0.75f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 22));
    TitleText->SetText(FText::FromString(TEXT("ASTRAWILD — Paused")));

    MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMenu"));

    auto MakeMenuButton = [this](const FName& Name, const FString& Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
        ButtonLabel->SetText(FText::FromString(Label));
        Button->AddChild(ButtonLabel);
        return Button;
    };

    ResumeButton = MakeMenuButton(TEXT("PauseResume"), TEXT("Resume"), FLinearColor(0.18f, 0.42f, 0.38f, 1.0f));
    ResumeButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleResumeClicked);

    SaveButton = MakeMenuButton(TEXT("PauseSave"), TEXT("Save Now"), FLinearColor(0.2f, 0.3f, 0.5f, 1.0f));
    SaveButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleSaveClicked);

    // PCR-1: the Field Journal entry (gamepad-reachable; P on keyboard).
    JournalButton = MakeMenuButton(TEXT("PauseJournal"), TEXT("Field Journal [P]"), FLinearColor(0.28f, 0.45f, 0.3f, 1.0f));
    JournalButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleJournalClicked);

    // PCR-2: the Echo Roster entry (gamepad-reachable; L on keyboard).
    RosterButton = MakeMenuButton(TEXT("PauseRoster"), TEXT("Echo Roster [L]"), FLinearColor(0.2f, 0.4f, 0.5f, 1.0f));
    RosterButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleRosterClicked);

    // PCR-3: the World Map entry (gamepad-reachable; M on keyboard).
    MapButton = MakeMenuButton(TEXT("PauseMap"), TEXT("World Map [M]"), FLinearColor(0.45f, 0.4f, 0.22f, 1.0f));
    MapButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleMapClicked);

    // PCR-5: the Hunt Board entry (gamepad-reachable; U on keyboard).
    HuntButton = MakeMenuButton(TEXT("PauseHunt"), TEXT("Hunt Board [U]"), FLinearColor(0.5f, 0.35f, 0.2f, 1.0f));
    HuntButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleHuntClicked);

    // DCP-2: the New Game+ entry. Visible ONLY while post-game is active —
    // the widget is rebuilt on every open (the controller toggles create a
    // fresh instance), so a construct-time visibility check is sufficient.
    // StartNewGamePlus independently re-checks the gate (fail-closed).
    NGPlusButton = MakeMenuButton(TEXT("PauseNGPlus"), TEXT("New Game+ (the Vale remembers)"), FLinearColor(0.55f, 0.35f, 0.55f, 1.0f));
    NGPlusButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleNGPlusClicked);
    {
        const AAstrawildGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AAstrawildGameState>() : nullptr;
        const bool bPostGame = GameState && GameState->IsPostGameActive();
        NGPlusButton->SetVisibility(bPostGame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    QuitButton = MakeMenuButton(TEXT("PauseQuit"), TEXT("Quit To Desktop"), FLinearColor(0.5f, 0.2f, 0.16f, 1.0f));
    QuitButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleQuitClicked);

    // DP-4: the skill loadout section (build identity) — three cycling slots.
    SkillLoadoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseSkillLoadoutTitle"));
    SkillLoadoutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.86f, 0.95f, 1.0f)));
    SkillLoadoutText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13));
    SkillLoadoutText->SetText(FText::FromString(TEXT("SKILL LOADOUT (Y smart-cast) — click a slot to cycle")));

    SkillSlotButtons.SetNum(SkillLoadoutSlots);
    SkillSlotLabels.SetNum(SkillLoadoutSlots);
    for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlots; ++SlotIndex)
    {
        UButton* SlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),
            *FString::Printf(TEXT("PauseSkillSlot%d"), SlotIndex + 1));
        SlotButton->SetBackgroundColor(FLinearColor(0.24f, 0.26f, 0.36f, 1.0f));
        UTextBlock* SlotLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseSkillSlotLabel"));
        SlotLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        SlotLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13));
        SlotButton->AddChild(SlotLabel);
        SkillSlotButtons[SlotIndex] = SlotButton;
        SkillSlotLabels[SlotIndex] = SlotLabel;
    }
    SkillSlotButtons[0]->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleSkillSlot0Clicked);
    SkillSlotButtons[1]->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleSkillSlot1Clicked);
    SkillSlotButtons[2]->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleSkillSlot2Clicked);

    // FPP-1: the growth readout — five attributes with level + XP progress so
    // a player can see what to grind and what the next milestone unlocks.
    AttributesText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseAttributes"));
    AttributesText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.9f, 0.72f, 1.0f)));
    AttributesText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    AttributesText->SetAutoWrapText(true);

    // LCP-6: the LAN CO-OP panel — host a game, find + join one, or connect
    // directly by address (PART 6). The status line always names the active
    // mode so it is OBVIOUS who is hosting.
    LanTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseLanTitle"));
    LanTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.95f, 0.85f, 1.0f)));
    LanTitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13));
    LanTitleText->SetText(FText::FromString(TEXT("LAN CO-OP (4 PLAYERS)")));

    LanStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseLanStatus"));
    LanStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.84f, 0.9f, 1.0f)));
    LanStatusText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));
    LanStatusText->SetAutoWrapText(true);

    LanHostButton = MakeMenuButton(TEXT("PauseLanHost"), TEXT("Host LAN Game"), FLinearColor(0.16f, 0.45f, 0.38f, 1.0f));
    LanHostButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleLanHostClicked);

    LanFindJoinButton = MakeMenuButton(TEXT("PauseLanFindJoin"), TEXT("Find + Join LAN Game"), FLinearColor(0.2f, 0.35f, 0.5f, 1.0f));
    LanFindJoinButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleLanFindJoinClicked);

    LanAddressBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("PauseLanAddress"));
    LanAddressBox->SetHintText(FText::FromString(TEXT("host IP (e.g. 192.168.1.5:7777)")));

    LanDirectConnectButton = MakeMenuButton(TEXT("PauseLanDirect"), TEXT("Direct Connect"), FLinearColor(0.34f, 0.3f, 0.45f, 1.0f));
    LanDirectConnectButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleLanDirectConnectClicked);

    if (UVerticalBoxSlot* BtnSlot1 = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(ResumeButton)))
    {
        BtnSlot1->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BtnSlot1->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* BtnSlot2 = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(SaveButton)))
    {
        BtnSlot2->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BtnSlot2->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* JournalSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(JournalButton)))
    {
        JournalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        JournalSlot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* RosterSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(RosterButton)))
    {
        RosterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        RosterSlot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* MapSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(MapButton)))
    {
        MapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        MapSlot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* HuntSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(HuntButton)))
    {
        HuntSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        HuntSlot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* NGPlusSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(NGPlusButton)))
    {
        NGPlusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        NGPlusSlot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* LoadoutTitleSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(SkillLoadoutText)))
    {
        LoadoutTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LoadoutTitleSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 2.0f));
    }
    for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlots; ++SlotIndex)
    {
        if (UVerticalBoxSlot* SlotBtnSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(SkillSlotButtons[SlotIndex])))
        {
            SlotBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            SlotBtnSlot->SetPadding(FMargin(0.0f, 3.0f));
        }
    }
    if (AttributesText)
    {
        if (UVerticalBoxSlot* AttributesSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(AttributesText)))
        {
            AttributesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            AttributesSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 2.0f));
        }
    }
    if (UVerticalBoxSlot* LanTitleSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanTitleText)))
    {
        LanTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LanTitleSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 2.0f));
    }
    if (UVerticalBoxSlot* LanStatusSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanStatusText)))
    {
        LanStatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LanStatusSlot->SetPadding(FMargin(0.0f, 2.0f));
    }
    if (UVerticalBoxSlot* LanHostSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanHostButton)))
    {
        LanHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LanHostSlot->SetPadding(FMargin(0.0f, 3.0f));
    }
    if (UVerticalBoxSlot* LanFindSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanFindJoinButton)))
    {
        LanFindSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LanFindSlot->SetPadding(FMargin(0.0f, 3.0f));
    }
    if (UVerticalBoxSlot* LanAddrSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanAddressBox)))
    {
        LanAddrSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LanAddrSlot->SetPadding(FMargin(0.0f, 3.0f));
    }
    if (UVerticalBoxSlot* LanDirectSlot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(LanDirectConnectButton)))
    {
        LanDirectSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        LanDirectSlot->SetPadding(FMargin(0.0f, 3.0f));
    }
    if (UVerticalBoxSlot* BtnSlot3 = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(QuitButton)))
    {
        BtnSlot3->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BtnSlot3->SetPadding(FMargin(0.0f, 6.0f));
    }

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-PausePanelWidth * 0.5f, -220.0f));
        TitleSlot->SetSize(FVector2D(PausePanelWidth, 36.0f));
    }
    if (UCanvasPanelSlot* MenuSlot = Canvas->AddChildToCanvas(MenuBox))
    {
        MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        MenuSlot->SetPosition(FVector2D(-PausePanelWidth * 0.5f, -170.0f));
        MenuSlot->SetSize(FVector2D(PausePanelWidth, PauseMenuHeight));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildPauseMenuWidget::HandleResumeClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->TogglePauseMenu();
    }
}

void UAstrawildPauseMenuWidget::HandleJournalClicked()
{
    // PCR-1: close the pause menu first (it holds SetPause + UIOnly input),
    // then hand the screen to the journal — TogglePauseMenu closes the
    // journal as a sibling only when the journal is ALREADY open, so the
    // order here is safe for both first-open and reopen cases.
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        if (PC->IsPauseMenuOpen())
        {
            PC->TogglePauseMenu();
        }
        PC->ToggleJournalScreen();
    }
}

void UAstrawildPauseMenuWidget::HandleRosterClicked()
{
    // PCR-2: same close-then-open sequence as the journal button.
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        if (PC->IsPauseMenuOpen())
        {
            PC->TogglePauseMenu();
        }
        PC->ToggleRosterScreen();
    }
}

void UAstrawildPauseMenuWidget::HandleMapClicked()
{
    // PCR-3: same close-then-open sequence as the journal/roster buttons.
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        if (PC->IsPauseMenuOpen())
        {
            PC->TogglePauseMenu();
        }
        PC->ToggleMapScreen();
    }
}

void UAstrawildPauseMenuWidget::HandleHuntClicked()
{
    // PCR-5: same close-then-open sequence as the other screen buttons.
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        if (PC->IsPauseMenuOpen())
        {
            PC->TogglePauseMenu();
        }
        PC->ToggleHuntScreen();
    }
}

void UAstrawildPauseMenuWidget::HandleSaveClicked()
{
    bool bSaved = false;
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            // Final-audit F11: SaveWorld returns false on clients/corruption —
            // the old code toasted "Saved." unconditionally (false feedback).
            bSaved = SaveSubsystem->SaveWorld(World, TEXT("ASTRAWILD_Main"));
        }
    }

    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->Notify(FText::FromString(bSaved ? TEXT("Saved.") : TEXT("Save failed — the host authority writes saves.")));
    }
}

void UAstrawildPauseMenuWidget::HandleQuitClicked()
{
    // PIE ends the session; packaged builds exit to desktop (loop stage QUIT).
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UAstrawildPauseMenuWidget::HandleNGPlusClicked()
{
    // DCP-2: one-click NG+ hand-off. The save subsystem owns the entire
    // reset/carryover authority; the button just routes to it (host only —
    // StartNewGamePlus refuses client worlds fail-closed).
    bool bStarted = false;
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            bStarted = SaveSubsystem->StartNewGamePlus(World);
        }
    }

    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        if (bStarted)
        {
            PC->TogglePauseMenu(); // close the menu — the new cycle is live.
        }
        else
        {
            PC->Notify(FText::FromString(TEXT("New Game+ needs a chosen ending and host authority.")));
        }
    }
}

void UAstrawildPauseMenuWidget::HandleSkillSlot0Clicked()
{
    CycleSkillSlot(0);
}

void UAstrawildPauseMenuWidget::HandleSkillSlot1Clicked()
{
    CycleSkillSlot(1);
}

void UAstrawildPauseMenuWidget::HandleSkillSlot2Clicked()
{
    CycleSkillSlot(2);
}

void UAstrawildPauseMenuWidget::CycleSkillSlot(const int32 SlotIndex)
{
    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
    UAstrawildAttributeComponent* Attributes = Player ? Player->AttributeComponent : nullptr;
    if (!Attributes || SlotIndex < 0 || SlotIndex >= SkillLoadoutSlots)
    {
        return;
    }

    // Cycle domain: the unlocked skills plus the empty state, in order. Skills
    // bound in OTHER slots are skipped (no duplicates); the first click on an
    // empty slot binds the first unlocked skill.
    const TArray<EAstrawildPlayerSkillId> Unlocked = Attributes->GetUnlockedSkills();
    if (Unlocked.IsEmpty())
    {
        if (PC)
        {
            PC->Notify(FText::FromString(TEXT("No skills unlocked yet — grow an attribute first.")));
        }
        return;
    }

    const TArray<EAstrawildPlayerSkillId> Bound = Attributes->GetBoundSkills();
    const EAstrawildPlayerSkillId Current = Bound.IsValidIndex(SlotIndex)
        ? Bound[SlotIndex] : EAstrawildPlayerSkillId::None;

    int32 CurrentState = Unlocked.Num(); // The empty state.
    if (Current != EAstrawildPlayerSkillId::None)
    {
        const int32 FoundIndex = Unlocked.IndexOfByKey(Current);
        CurrentState = FoundIndex != INDEX_NONE ? FoundIndex : Unlocked.Num();
    }

    EAstrawildPlayerSkillId Next = Current; // Full circle with every candidate bound elsewhere.
    const int32 CycleLength = Unlocked.Num() + 1;
    for (int32 Step = 1; Step <= CycleLength; ++Step)
    {
        const int32 State = (CurrentState + Step) % CycleLength;
        if (State == Unlocked.Num())
        {
            Next = EAstrawildPlayerSkillId::None; // The empty state is always reachable.
            break;
        }
        const EAstrawildPlayerSkillId Candidate = Unlocked[State];
        if (Candidate == Current || !Attributes->IsSkillBound(Candidate))
        {
            Next = Candidate;
            break;
        }
    }

    bool bApplied = false;
    if (Next == EAstrawildPlayerSkillId::None)
    {
        if (Current != EAstrawildPlayerSkillId::None)
        {
            Attributes->ClearSlot(SlotIndex);
            bApplied = true;
        }
    }
    else
    {
        bApplied = Attributes->BindSkillToSlot(SlotIndex, Next);
    }

    // Rebind feedback rides the HUD toast path (Notify -> PushNotification).
    if (PC)
    {
        if (bApplied)
        {
            PC->Notify(Next == EAstrawildPlayerSkillId::None
                ? FText::FromString(FString::Printf(TEXT("Skill slot %d cleared."), SlotIndex + 1))
                : FText::FromString(FString::Printf(TEXT("Skill slot %d: %s"), SlotIndex + 1,
                    *UEnum::GetDisplayValueAsText(Next).ToString())));
        }
        else if (Next != EAstrawildPlayerSkillId::None && Next != Current)
        {
            PC->Notify(FText::FromString(TEXT("Skill binding refused — the host authority owns the loadout.")));
        }
    }
    RefreshSkillSlotLabels();
}

void UAstrawildPauseMenuWidget::RefreshSkillSlotLabels()
{
    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
    UAstrawildAttributeComponent* Attributes = Player ? Player->AttributeComponent : nullptr;
    const TArray<EAstrawildPlayerSkillId> Bound = Attributes ? Attributes->GetBoundSkills()
        : TArray<EAstrawildPlayerSkillId>();

    for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlots; ++SlotIndex)
    {
        if (!SkillSlotLabels.IsValidIndex(SlotIndex) || !SkillSlotLabels[SlotIndex])
        {
            continue;
        }
        const EAstrawildPlayerSkillId Skill = Bound.IsValidIndex(SlotIndex)
            ? Bound[SlotIndex] : EAstrawildPlayerSkillId::None;
        if (Skill == EAstrawildPlayerSkillId::None)
        {
            SkillSlotLabels[SlotIndex]->SetText(FText::FromString(FString::Printf(
                TEXT("Skill Slot %d: — empty (click to cycle your unlocked skills)"), SlotIndex + 1)));
            continue;
        }

        // FPP-1: name + what it DOES + the live cooldown state — a loadout
        // choice can now be made from the menu alone, and "not ready" reads
        // as "recharging", never as "the key does nothing".
        const FText Description = UAstrawildAttributeComponent::GetSkillDescription(Skill);
        const float CooldownRemaining = Attributes ? Attributes->GetSkillCooldownRemaining(Skill) : 0.0f;
        const FString CooldownLine = CooldownRemaining > 0.05f
            ? FString::Printf(TEXT("  [recharging %.0fs]"), CooldownRemaining)
            : FString(TEXT("  [READY]"));
        SkillSlotLabels[SlotIndex]->SetAutoWrapText(true);
        SkillSlotLabels[SlotIndex]->SetText(FText::FromString(FString::Printf(
            TEXT("Skill Slot %d: %s\n%s%s"),
            SlotIndex + 1,
            *UEnum::GetDisplayValueAsText(Skill).ToString(),
            *Description.ToString(),
            *CooldownLine)));
    }

    RefreshAttributeStats();
}

void UAstrawildPauseMenuWidget::RefreshAttributeStats()
{
    if (!AttributesText)
    {
        return;
    }

    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
    UAstrawildAttributeComponent* Attributes = Player ? Player->AttributeComponent : nullptr;
    if (!Attributes)
    {
        AttributesText->SetText(FText::GetEmpty());
        return;
    }

    struct FRow { EAstrawildAttributeType Type; const TCHAR* Label; };
    const FRow Rows[] = {
        { EAstrawildAttributeType::Might,    TEXT("Might (melee dmg)") },
        { EAstrawildAttributeType::Vigor,    TEXT("Vigor (max HP)") },
        { EAstrawildAttributeType::Agility,  TEXT("Agility (speed/stamina)") },
        { EAstrawildAttributeType::Instinct, TEXT("Instinct (capture)") },
        { EAstrawildAttributeType::Craft,    TEXT("Craft (craft speed)") },
    };

    // FPP-1: "Might 3 (XP 40/300)" per attribute — XP sources now have a
    // visible destination, and skill milestones stop being invisible walls.
    FString Line = TEXT("GROWTH — ");
    for (int32 i = 0; i < 5; ++i)
    {
        const int32 Level = Attributes->GetLevel(Rows[i].Type);
        const float XP = Attributes->GetXP(Rows[i].Type);
        const float XPNext = Attributes->GetXPToNextLevel(Rows[i].Type);
        if (i > 0)
        {
            Line += TEXT("  ·  ");
        }
        Line += FString::Printf(TEXT("%s %d"), Rows[i].Label, Level);
        if (XPNext > 0.0f)
        {
            Line += FString::Printf(TEXT(" (XP %.0f/%.0f)"), XP, XPNext);
        }
        else
        {
            Line += TEXT(" (MAX)");
        }
    }
    AttributesText->SetText(FText::FromString(Line));
}


// ---------------------------------------------------------------------------
// LCP-6 — LAN CO-OP panel
// ---------------------------------------------------------------------------

UAstrawildLANSessionSubsystem* UAstrawildPauseMenuWidget::GetLanSubsystem() const
{
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UAstrawildLANSessionSubsystem>() : nullptr;
}

void UAstrawildPauseMenuWidget::RefreshLanStatus()
{
    if (!LanStatusText)
    {
        return;
    }
    const FString Mode = UAstrawildLANSessionSubsystem::DescribeSessionMode(this);
    if (UAstrawildLANSessionSubsystem* Lan = GetLanSubsystem())
    {
        const int32 Found = Lan->GetDiscoveredSessions().Num();
        LanStatusText->SetText(FText::FromString(Found > 0
            ? FString::Printf(TEXT("%s — %d LAN game(s) found"), *Mode, Found)
            : Mode));
    }
    else
    {
        LanStatusText->SetText(FText::FromString(Mode));
    }
}

void UAstrawildPauseMenuWidget::HandleLanHostClicked()
{
    if (UAstrawildLANSessionSubsystem* Lan = GetLanSubsystem())
    {
        if (Lan->HostLANGame())
        {
            if (LanStatusText)
            {
                LanStatusText->SetText(FText::FromString(TEXT("HOSTING — re-entering as listen server (world saved + auto-loaded)...")));
            }
        }
        else if (LanStatusText)
        {
            LanStatusText->SetText(FText::FromString(TEXT("Cannot host from here (already a client).")));
        }
    }
}

void UAstrawildPauseMenuWidget::HandleLanFindJoinClicked()
{
    if (UAstrawildLANSessionSubsystem* Lan = GetLanSubsystem())
    {
        Lan->StartLanDiscovery();
        const TArray<FAstrawildLanSessionInfo> Sessions = Lan->GetDiscoveredSessions();
        if (!Sessions.IsEmpty())
        {
            Lan->JoinSession(Sessions[0]);
            if (LanStatusText)
            {
                LanStatusText->SetText(FText::FromString(FString::Printf(TEXT("Joining %s:%d..."),
                    *Sessions[0].HostAddress, Sessions[0].HostPort)));
            }
        }
        else if (LanStatusText)
        {
            LanStatusText->SetText(FText::FromString(TEXT("Searching — the beacon broadcasts 1/s; click Find + Join again in a second.")));
        }
    }
}

void UAstrawildPauseMenuWidget::HandleLanDirectConnectClicked()
{
    if (!LanAddressBox)
    {
        return;
    }
    const FString Address = LanAddressBox->GetText().ToString();
    if (UAstrawildLANSessionSubsystem* Lan = GetLanSubsystem())
    {
        if (Lan->ConnectDirect(Address))
        {
            if (LanStatusText)
            {
                LanStatusText->SetText(FText::FromString(FString::Printf(TEXT("Connecting to %s..."), *Address)));
            }
        }
        else if (LanStatusText)
        {
            LanStatusText->SetText(FText::FromString(TEXT("Address malformed — use 192.168.x.x or 192.168.x.x:7777")));
        }
    }
}
