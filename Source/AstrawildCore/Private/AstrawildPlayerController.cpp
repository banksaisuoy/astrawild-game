#include "AstrawildPlayerController.h"

#include "AstrawildCheatManager.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDialogueComponent.h"
#include "AstrawildDialogueWidget.h"
#include "AstrawildHudWidget.h"
#include "AstrawildInventoryScreenWidget.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPauseMenuWidget.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchScreenWidget.h"
#include "AstrawildShopWidget.h"
#include "Blueprint/UserWidget.h"

AAstrawildPlayerController::AAstrawildPlayerController()
{
    PrimaryActorTick.bCanEverTick = false;

    CheatClass = UAstrawildCheatManager::StaticClass();
    QuestComponent = CreateDefaultSubobject<UAstrawildQuestComponent>(TEXT("Quests"));
    // Batch 3 — persistent dialogue state lives beside the quest component so
    // story flags survive death/respawn (saved with the v4 payload).
    DialogueComponent = CreateDefaultSubobject<UAstrawildDialogueComponent>(TEXT("Dialogue"));
}

void AAstrawildPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    // Ensure game-only input mode and mouse lock on startup for immediate playable control.
    FInputModeGameOnly GameInputMode;
    SetInputMode(GameInputMode);
    bShowMouseCursor = false;
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);

    // C++-built HUD — no UMG asset dependency (directive §29/§50).
    const TSubclassOf<UAstrawildHudWidget> WidgetClass = HudWidgetClass
        ? HudWidgetClass
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

    if (IsLocalController())
    {
        FInputModeGameOnly GameInputMode;
        SetInputMode(GameInputMode);
        bShowMouseCursor = false;
        SetIgnoreMoveInput(false);
        SetIgnoreLookInput(false);
    }

    if (AAstrawildPlayerCharacter* PlayerChar = Cast<AAstrawildPlayerCharacter>(InPawn))
    {
        PlayerChar->ApplyMappingContext();
    }
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
            ? ShopWidgetClass
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

// --- Production V2 Batch 3: dialogue screen (P12 Story/NPC) ---

void AAstrawildPlayerController::OpenDialogue(AAstrawildNPCCharacter* Npc)
{
    // Local controller only — same screen discipline as the shop. Conversation
    // CONSEQUENCES still route through the authority pipelines (quest start,
    // server inventory adds, research points) inside the dialogue component.
    if (!IsLocalController() || !Npc || !Npc->NpcDefinition)
    {
        return;
    }

    // Resolve the tree through the registry (CODE_DEFAULT today, .uasset trees
    // tomorrow — same-id override contract as every other definition).
    UAstrawildItemRegistrySubsystem* Registry = GetWorld()
        ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>()
        : nullptr;
    UAstrawildDialogueTreeDefinition* Tree = Registry
        ? Registry->FindDialogueTree(Npc->NpcDefinition->DialogueTreeId)
        : nullptr;
    if (!Tree)
    {
        UE_LOG(LogAstrawild, Warning,
            TEXT("OpenDialogue: tree %s not registered — falling back to legacy interact path."),
            *Npc->NpcDefinition->DialogueTreeId.ToString());
        return;
    }

    if (!DialogueWidget)
    {
        const TSubclassOf<UAstrawildDialogueWidget> WidgetClass = DialogueWidgetClass
            ? DialogueWidgetClass
            : TSubclassOf<UAstrawildDialogueWidget>(UAstrawildDialogueWidget::StaticClass());
        DialogueWidget = CreateWidget<UAstrawildDialogueWidget>(this, WidgetClass);
    }
    if (!DialogueWidget)
    {
        return;
    }

    DialogueWidget->InitializeDialogue(Npc, Tree);
    DialogueWidget->AddToViewport(10); // Above the HUD, same layer as the other screens.

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    UE_LOG(LogAstrawild, Log, TEXT("Dialogue screen opened (%s)."), *Npc->GetName());
}

void AAstrawildPlayerController::CloseDialogue()
{
    if (DialogueWidget)
    {
        DialogueWidget->RemoveFromParent();
    }

    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
}

bool AAstrawildPlayerController::IsDialogueOpen() const
{
    return DialogueWidget && DialogueWidget->IsInViewport();
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
    CloseDialogue();
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
                ? InventoryScreenClass
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
    CloseDialogue();
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
                ? ResearchScreenClass
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
    CloseDialogue();
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
                ? PauseMenuClass
                : TSubclassOf<UAstrawildPauseMenuWidget>(UAstrawildPauseMenuWidget::StaticClass());
            PauseMenuWidget = CreateWidget<UAstrawildPauseMenuWidget>(this, WidgetClass);
        }
        if (PauseMenuWidget)
        {
            // Pause the world while the menu is up (single-player/listen-server).
            SetPause(true);
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
        SetPause(false);
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
    return IsShopOpen() || IsDialogueOpen() || IsInventoryOpen() || IsResearchOpen() || IsPauseMenuOpen();
}
