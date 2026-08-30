#include "AstrawildPlayerController.h"

#include "AstrawildCore.h"
#include "AstrawildHudWidget.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildShopWidget.h"
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

void AAstrawildPlayerController::OpenShop(AAstrawildNPCCharacter* Vendor)
{
    // Local controller only — remote clients route through Server RPCs in the
    // future MP batch (TryPurchase itself is server-authoritative already).
    if (!IsLocalController() || !Vendor)
    {
        return;
    }

    if (!ShopWidget)
    {
        const TSubclassOf<UAstrawildShopWidget> WidgetClass = ShopWidgetClass
            ? ShopWidgetClass.Get()
            : TSubclassOf<UAstrawildShopWidget>(UAstrawildShopWidget::StaticClass());
        ShopWidget = CreateWidget<UAstrawildShopWidget>(this, WidgetClass);
    }
    if (!ShopWidget)
    {
        return;
    }

    ShopWidget->InitializeShop(Vendor);
    ShopWidget->AddToViewport(10); // Above the HUD (default Z-order).

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Shop screen opened (%s)."), *Vendor->GetName());
}

void AAstrawildPlayerController::CloseShop()
{
    if (ShopWidget)
    {
        ShopWidget->RemoveFromParent();
    }

    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
}

bool AAstrawildPlayerController::IsShopOpen() const
{
    return ShopWidget && ShopWidget->IsInViewport();
}
