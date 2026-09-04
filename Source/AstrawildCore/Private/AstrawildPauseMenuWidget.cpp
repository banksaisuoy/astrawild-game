#include "AstrawildPauseMenuWidget.h"

#include "AstrawildAttributeComponent.h"
#include "AstrawildCore.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr float PausePanelWidth = 340.0f;
    // DP-4: 3 menu buttons + the loadout caption + 3 slot buttons — the pause
    // panel grew with the skill loadout section.
    constexpr float PauseMenuHeight = 340.0f;
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
        SkillSlotLabels[SlotIndex]->SetText(Skill == EAstrawildPlayerSkillId::None
            ? FText::FromString(FString::Printf(TEXT("Skill Slot %d: — empty"), SlotIndex + 1))
            : FText::FromString(FString::Printf(TEXT("Skill Slot %d: %s"), SlotIndex + 1,
                *UEnum::GetDisplayValueAsText(Skill).ToString())));
    }
}
