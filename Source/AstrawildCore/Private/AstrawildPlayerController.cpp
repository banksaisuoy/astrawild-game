#include "AstrawildPlayerController.h"

#include "AstrawildCore.h"
#include "AstrawildHudWidget.h"
#include "AstrawildLog.h"
#include "AstrawildQuestComponent.h"
#include "Blueprint/UserWidget.h"

AAstrawildPlayerController::AAstrawildPlayerController()
{
    PrimaryActorTick.bCanEverTick = false;

    QuestComponent = CreateDefaultSubobject<UAstrawildQuestComponent>(TEXT("Quests"));
}

void AAstrawildPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    // C++-built HUD — no UMG asset dependency (directive §29/§50).
    const TSubclassOf<UAstrawildHudWidget> WidgetClass = HudWidgetClass
        ? HudWidgetClass.Get()
        : TSubclassOf<UAstrawildHudWidget>(UAstrawildHudWidget::StaticClass());

    HudWidget = CreateWidget<UAstrawildHudWidget>(this, WidgetClass);
    if (HudWidget)
    {
        HudWidget->AddToViewport();
        UE_LOG(LogAstrawild, Log, TEXT("HUD widget created."));
    }
}

void AAstrawildPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void AAstrawildPlayerController::Notify(const FText& Message)
{
    // Audit C-2/C-7: gameplay feedback path — HUD notification previously existed but
    // had zero callers, so players never saw research/work/capture outcomes.
    if (HudWidget)
    {
        HudWidget->PushNotification(Message);
    }
}
