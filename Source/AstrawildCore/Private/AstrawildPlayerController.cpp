#include "AstrawildPlayerController.h"

#include "AstrawildCore.h"
#include "AstrawildHudWidget.h"
#include "AstrawildInventoryScreenWidget.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPauseMenuWidget.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchScreenWidget.h"
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

// --- Final production run: inventory / research / pause screens ---

void AAstrawildPlayerController::ToggleInventoryScreen()
{
    if (!IsLocalController())
    {
        return;
    }

    const bool bOpen = !IsInventoryOpen();

    // Close siblings first — one full-screen UI at a time.
    CloseShop();
    if (IsResearchOpen())
    {
        ToggleResearchScreen();
    }
    if (IsPauseMenuOpen())
    {
        TogglePauseMenu();
    }

    if (bOpen)
    {
        if (!InventoryScreen)
        {
            const TSubclassOf<UAstrawildInventoryScreenWidget> WidgetClass = InventoryScreenClass
                ? InventoryScreenClass.Get()
                : TSubclassOf<UAstrawildInventoryScreenWidget>(UAstrawildInventoryScreenWidget::StaticClass());
            InventoryScreen = CreateWidget<UAstrawildInventoryScreenWidget>(this, WidgetClass);
        }
        if (InventoryScreen)
        {
            InventoryScreen->RefreshInventory();
            InventoryScreen->AddToViewport(10);
            FInputModeUIOnly InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
    else
    {
        if (InventoryScreen)
        {
            InventoryScreen->RemoveFromParent();
        }
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

bool AAstrawildPlayerController::IsInventoryOpen() const
{
    return InventoryScreen && InventoryScreen->IsInViewport();
}

void AAstrawildPlayerController::ToggleResearchScreen()
{
    if (!IsLocalController())
    {
        return;
    }

    const bool bOpen = !IsResearchOpen();

    CloseShop();
    if (IsInventoryOpen())
    {
        ToggleInventoryScreen();
    }
    if (IsPauseMenuOpen())
    {
        TogglePauseMenu();
    }

    if (bOpen)
    {
        if (!ResearchScreen)
        {
            const TSubclassOf<UAstrawildResearchScreenWidget> WidgetClass = ResearchScreenClass
                ? ResearchScreenClass.Get()
                : TSubclassOf<UAstrawildResearchScreenWidget>(UAstrawildResearchScreenWidget::StaticClass());
            ResearchScreen = CreateWidget<UAstrawildResearchScreenWidget>(this, WidgetClass);
        }
        if (ResearchScreen)
        {
            ResearchScreen->RefreshResearch();
            ResearchScreen->AddToViewport(10);
            FInputModeUIOnly InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
    else
    {
        if (ResearchScreen)
        {
            ResearchScreen->RemoveFromParent();
        }
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

bool AAstrawildPlayerController::IsResearchOpen() const
{
    return ResearchScreen && ResearchScreen->IsInViewport();
}

void AAstrawildPlayerController::TogglePauseMenu()
{
    if (!IsLocalController())
    {
        return;
    }

    const bool bOpen = !IsPauseMenuOpen();

    CloseShop();
    if (IsInventoryOpen())
    {
        ToggleInventoryScreen();
    }
    if (IsResearchOpen())
    {
        ToggleResearchScreen();
    }

    if (bOpen)
    {
        if (!PauseMenuWidget)
        {
            const TSubclassOf<UAstrawildPauseMenuWidget> WidgetClass = PauseMenuClass
                ? PauseMenuClass.Get()
                : TSubclassOf<UAstrawildPauseMenuWidget>(UAstrawildPauseMenuWidget::StaticClass());
            PauseMenuWidget = CreateWidget<UAstrawildPauseMenuWidget>(this, WidgetClass);
        }
        if (PauseMenuWidget)
        {
            // Pause the world while the menu is up (single-player/listen-server).
            if (UWorld* World = GetWorld())
            {
                World->SetPauserPlayerState(PlayerState);
            }
            PauseMenuWidget->AddToViewport(20);
            FInputModeUIOnly InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
    else
    {
        if (PauseMenuWidget)
        {
            PauseMenuWidget->RemoveFromParent();
        }
        if (UWorld* World = GetWorld())
        {
            World->SetPauserPlayerState(nullptr);
        }
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

bool AAstrawildPlayerController::IsPauseMenuOpen() const
{
    return PauseMenuWidget && PauseMenuWidget->IsInViewport();
}

bool AAstrawildPlayerController::IsAnyScreenOpen() const
{
    return IsShopOpen() || IsInventoryOpen() || IsResearchOpen() || IsPauseMenuOpen();
}
