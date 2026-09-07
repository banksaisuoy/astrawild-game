#include "AstrawildDialogueWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDialogueComponent.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    constexpr float DialoguePanelWidth = 760.0f;
    constexpr float DialoguePanelHeight = 460.0f;

    // Sci-fi warm palette (the "dawn glass" family the HUD already uses).
    const FLinearColor SpeakerColor(0.98f, 0.85f, 0.55f, 1.0f);
    const FLinearColor BodyColor(0.95f, 0.93f, 0.85f, 1.0f);
    const FLinearColor ReplyHeaderColor(0.55f, 0.90f, 0.85f, 1.0f);
    const FLinearColor ChoiceColor(0.92f, 0.90f, 0.82f, 1.0f);
}

// ---------------------------------------------------------------------------
// Choice row
// ---------------------------------------------------------------------------
void UAstrawildDialogueChoiceRowWidget::InitializeRow(UAstrawildDialogueWidget* InParent, const FAstrawildDialogueChoice& InChoice, const int32 InChoiceIndex)
{
    ParentDialogue = InParent;
    Choice = InChoice;
    RowChoiceIndex = InChoiceIndex; // LCP-3

    if (WidgetTree && WidgetTree->RootWidget && !ChoiceButton)
    {
        BuildRowTree();
    }
}

void UAstrawildDialogueChoiceRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildDialogueChoiceRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return; // Already built (double construct guard).
    }
    if (!ParentDialogue)
    {
        return;
    }

    ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DialogueChoiceButton"));
    ChoiceButton->SetBackgroundColor(FLinearColor(0.16f, 0.19f, 0.24f, 0.85f));
    WidgetTree->RootWidget = ChoiceButton;

    UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueChoiceLabel"));
    Label->SetColorAndOpacity(FSlateColor(ChoiceColor));
    Label->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));
    Label->SetText(Choice.Text.IsEmpty() ? FText::FromString(TEXT("…")) : Choice.Text);
    Label->SetAutoWrapText(true);
    ChoiceButton->AddChild(Label);

    ChoiceButton->OnClicked.AddDynamic(this, &UAstrawildDialogueChoiceRowWidget::HandleClicked);
}

void UAstrawildDialogueChoiceRowWidget::HandleClicked()
{
    if (ParentDialogue)
    {
        ParentDialogue->SelectChoiceByIndex(RowChoiceIndex, Choice); // LCP-3
    }
}

// ---------------------------------------------------------------------------
// Dialogue screen
// ---------------------------------------------------------------------------
void UAstrawildDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    BuildPanelTree();
}

void UAstrawildDialogueWidget::InitializeDialogue(AAstrawildNPCCharacter* InNpc, UAstrawildDialogueTreeDefinition* InTree)
{
    DialogueNpc = InNpc;
    Tree = InTree;

    if (!RootCanvasGuard())
    {
        BuildPanelTree();
    }

    if (Tree && !Tree->EntryNodeId.IsNone())
    {
        EnterNode(Tree->EntryNodeId);
    }
}

bool UAstrawildDialogueWidget::RootCanvasGuard() const
{
    return WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UAstrawildDialogueWidget::BuildPanelTree()
{
    if (RootCanvasGuard() || BackdropBorder)
    {
        return; // Already built.
    }

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DialogueRootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // Dim the world behind the conversation.
    BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueBackdrop"));
    BackdropBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(BackdropBorder));
    BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackdropSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));

    // Conversation card — anchored low-center like a classic RPG dialogue.
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialoguePanel"));
    Panel->SetBrushColor(FLinearColor(0.07f, 0.08f, 0.10f, 0.96f));
    UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(Panel));
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.85f, 0.5f, 0.85f));
    PanelSlot->SetPosition(FVector2D(-DialoguePanelWidth * 0.5f, -DialoguePanelHeight));
    PanelSlot->SetSize(FVector2D(DialoguePanelWidth, DialoguePanelHeight));

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogueLayout"));
    Panel->SetContent(Layout);

    auto MakeText = [this](const FString& Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        return Text;
    };

    auto AddVertical = [Layout](UWidget* Widget, const float InPadding) -> UVerticalBoxSlot*
    {
        auto* VSlot = Cast<UVerticalBoxSlot>(Layout->AddChildToVerticalBox(Widget));
        VSlot->SetPadding(FMargin(24.0f, InPadding, 24.0f, InPadding));
        return VSlot;
    };

    // Speaker.
    SpeakerText = MakeText(TEXT("DialogueSpeaker"), SpeakerColor, 20);
    AddVertical(SpeakerText, 18.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    // NPC line area (a recessed glass panel).
    LineBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogueLineBorder"));
    LineBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.15f, 0.85f));
    LineText = MakeText(TEXT("DialogueLine"), BodyColor, 15);
    LineText->SetAutoWrapText(true);
    LineText->SetMinDesiredWidth(650.0f);
    LineBorder->SetContent(LineText);
    AddVertical(LineBorder, 8.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    // Continue button (line phase).
    ContinueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DialogueContinue"));
    ContinueButton->SetBackgroundColor(FLinearColor(0.35f, 0.28f, 0.14f, 0.9f));
    ContinueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueContinueLabel"));
    ContinueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ContinueText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13));
    ContinueText->SetText(FText::FromString(TEXT("Continue")));
    ContinueButton->AddChild(ContinueText);
    auto* ContinueSlot = AddVertical(ContinueButton, 8.0f);
    ContinueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    ContinueSlot->SetHorizontalAlignment(HAlign_Right);
    ContinueButton->OnClicked.AddDynamic(this, &UAstrawildDialogueWidget::HandleContinueClicked);

    // Reply header.
    ReplyHeaderText = MakeText(TEXT("DialogueReplyHeader"), ReplyHeaderColor, 13);
    AddVertical(ReplyHeaderText, 10.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    // Choice list.
    ChoiceList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DialogueChoiceList"));
    AddVertical(ChoiceList, 4.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // Leave.
    LeaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DialogueLeave"));
    LeaveButton->SetBackgroundColor(FLinearColor(0.20f, 0.22f, 0.26f, 0.9f));
    UTextBlock* LeaveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DialogueLeaveLabel"));
    LeaveLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.83f, 0.78f, 1.0f)));
    LeaveLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
    LeaveLabel->SetText(FText::FromString(TEXT("Leave")));
    LeaveButton->AddChild(LeaveLabel);
    auto* LeaveSlot = AddVertical(LeaveButton, 8.0f);
    LeaveSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    LeaveSlot->SetHorizontalAlignment(HAlign_Right);
    LeaveButton->OnClicked.AddDynamic(this, &UAstrawildDialogueWidget::HandleLeaveClicked);
}

void UAstrawildDialogueWidget::EnterNode(const FName NodeId)
{
    CurrentNodeId = NodeId;
    CurrentLineIndex = 0;
    CurrentLines.Reset();

    if (!Tree)
    {
        return;
    }

    if (const FAstrawildDialogueNode* Node = Tree->FindNode(NodeId))
    {
        CurrentLines = Node->Lines;
    }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Dialogue: node %s not found — closing."), *NodeId.ToString());
    }

    RefreshBody();
}

void UAstrawildDialogueWidget::AdvanceLine()
{
    if (CurrentLineIndex < CurrentLines.Num())
    {
        ++CurrentLineIndex;
    }
    RefreshBody();
}

void UAstrawildDialogueWidget::SelectChoice(const FAstrawildDialogueChoice& Choice)
{
    SelectChoiceByIndex(INDEX_NONE, Choice);
}

void UAstrawildDialogueWidget::SelectChoiceByIndex(const int32 ChoiceIndex, const FAstrawildDialogueChoice& Choice)
{
    // LCP-3: remote clients submit the choice intent to the server (structural
    // + conditional re-validation happens there, fail-closed); host/standalone
    // apply directly. The widget's own tree advancement below is display-only
    // (registry-static content) and identical on every machine.
    if (AAstrawildPlayerController* PC = GetPlayerController())
    {
        if (PC->GetNetMode() == NM_Client && ChoiceIndex != INDEX_NONE)
        {
            PC->ServerSubmitDialogueChoice(DialogueNpc.Get(), CurrentNodeId, ChoiceIndex);
        }
        else if (UAstrawildDialogueComponent* Dialogue = GetDialogueComponent())
        {
            Dialogue->ApplyChoiceConsequences(Choice);
        }
    }
    else if (UAstrawildDialogueComponent* Dialogue = GetDialogueComponent())
    {
        Dialogue->ApplyChoiceConsequences(Choice);
    }

    // Vendor hand-off: end the conversation and open the shop.
    if (Choice.bOpenShop)
    {
        AAstrawildNPCCharacter* Npc = DialogueNpc.Get();
        CloseDialogue();
        if (AAstrawildPlayerController* PC = GetPlayerController())
        {
            PC->OpenShop(Npc);
        }
        return;
    }

    if (!Choice.GotoNodeId.IsNone() && !Choice.bEndDialogue)
    {
        EnterNode(Choice.GotoNodeId);
    }
    else
    {
        CloseDialogue();
    }
}

void UAstrawildDialogueWidget::CloseDialogue()
{
    AAstrawildPlayerController* PC = GetPlayerController();
    RemoveFromParent();
    if (PC)
    {
        PC->CloseDialogue();
    }
}

void UAstrawildDialogueWidget::HandleContinueClicked()
{
    AdvanceLine();
}

void UAstrawildDialogueWidget::HandleLeaveClicked()
{
    CloseDialogue();
}

FReply UAstrawildDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        CloseDialogue();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildDialogueWidget::RefreshBody()
{
    const bool bChoicePhase = IsChoicePhase();

    // Speaker name: NPC display name, unless the line overrides it.
    FString SpeakerName;
    if (AAstrawildNPCCharacter* Npc = DialogueNpc.Get())
    {
        SpeakerName = Npc->NpcDefinition ? Npc->NpcDefinition->DisplayName.ToString() : Npc->GetName();
    }
    if (!bChoicePhase && !CurrentLines[CurrentLineIndex].SpeakerName.IsEmpty())
    {
        SpeakerName = CurrentLines[CurrentLineIndex].SpeakerName.ToString();
    }
    if (SpeakerText)
    {
        SpeakerText->SetText(FText::FromString(SpeakerName));
    }

    // Line body.
    if (LineText)
    {
        if (bChoicePhase)
        {
            // Hold the last line on screen while the player picks a reply.
            LineText->SetText(CurrentLines.IsEmpty()
                ? FText::GetEmpty()
                : CurrentLines.Last().Text);
        }
        else
        {
            LineText->SetText(CurrentLines[CurrentLineIndex].Text);
        }
    }

    if (ContinueButton)
    {
        ContinueButton->SetVisibility(bChoicePhase ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
        if (ContinueText)
        {
            const bool bLast = CurrentLineIndex == CurrentLines.Num() - 1;
            ContinueText->SetText(FText::FromString(bLast ? TEXT("Reply") : TEXT("Continue")));
        }
    }

    if (ReplyHeaderText)
    {
        ReplyHeaderText->SetText(FText::FromString(TEXT("── your reply ──")));
        ReplyHeaderText->SetVisibility(bChoicePhase ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    // Rebuild the visible replies.
    if (ChoiceList)
    {
        ChoiceList->ClearChildren();

        if (bChoicePhase && Tree)
        {
            if (UAstrawildDialogueComponent* Dialogue = GetDialogueComponent())
            {
                if (const FAstrawildDialogueNode* Node = Tree->FindNode(CurrentNodeId))
                {
                    int32 ChoiceIndex = 0;
                    for (const FAstrawildDialogueChoice& Choice : Node->Choices)
                    {
                        if (Dialogue->EvaluateChoiceConditions(Choice))
                        {
                            UAstrawildDialogueChoiceRowWidget* Row =
                                WidgetTree->ConstructWidget<UAstrawildDialogueChoiceRowWidget>(UAstrawildDialogueChoiceRowWidget::StaticClass());
                            Row->InitializeRow(this, Choice, ChoiceIndex);
                            ChoiceList->AddChild(Row);
                        }
                        ++ChoiceIndex;
                    }
                }
            }
        }

        ChoiceList->SetVisibility(bChoicePhase ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

UAstrawildDialogueComponent* UAstrawildDialogueWidget::GetDialogueComponent() const
{
    const APlayerController* PC = GetOwningPlayer();
    return PC ? PC->FindComponentByClass<UAstrawildDialogueComponent>() : nullptr;
}

AAstrawildPlayerController* UAstrawildDialogueWidget::GetPlayerController() const
{
    return Cast<AAstrawildPlayerController>(GetOwningPlayer());
}
