#include "AstrawildPlayerController.h"

#include "AstrawildCheatManager.h"
#include "AstrawildCore.h"
#include "AstrawildCraftingScreenWidget.h"
#include "AstrawildCraftingStationActor.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDialogueComponent.h"
#include "AstrawildDialogueWidget.h"
#include "AstrawildHudWidget.h"
#include "AstrawildInventoryScreenWidget.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalScreenWidget.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPauseMenuWidget.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "GameFramework/PlayerState.h" // LCP-4: player key
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

        if (AAstrawildPlayerCharacter* PlayerChar = Cast<AAstrawildPlayerCharacter>(InPawn))
        {
            PlayerChar->ApplyMappingContext();
        }
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

    // DP-8 (NPC depth): the dialogue component remembers who is being talked
    // to so affinity-gated replies evaluate against the LIVE relationship
    // while the screen is open (cleared in CloseDialogue).
    if (DialogueComponent)
    {
        DialogueComponent->SetTalkingNpc(Npc);
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

    // DP-8: the conversation is over — drop the talking NPC so no stale
    // affinity state leaks into any later evaluation.
    if (DialogueComponent)
    {
        DialogueComponent->SetTalkingNpc(nullptr);
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
    if (IsCraftingOpen())
    {
        ToggleCraftingScreen();
    }
    if (IsJournalOpen())
    {
        ToggleJournalScreen();
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
            // Final-audit F-05: give the screen keyboard focus so TAB/ESC close
            // without requiring a prior mouse click.
            InventoryScreen->SetKeyboardFocus();
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
    if (IsCraftingOpen())
    {
        ToggleCraftingScreen();
    }
    if (IsJournalOpen())
    {
        ToggleJournalScreen();
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
            // Final-audit F-05: keyboard focus for the advertised K/ESC close.
            ResearchScreen->SetKeyboardFocus();
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

void AAstrawildPlayerController::ToggleCraftingScreen()
{
    if (!IsLocalController())
    {
        return;
    }

    const bool bOpen = !IsCraftingOpen();

    // Close siblings first — one full-screen UI at a time.
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
    if (IsJournalOpen())
    {
        ToggleJournalScreen();
    }
    if (IsPauseMenuOpen())
    {
        TogglePauseMenu();
    }

    if (bOpen)
    {
        if (!CraftingScreen)
        {
            const TSubclassOf<UAstrawildCraftingScreenWidget> WidgetClass = CraftingScreenClass
                ? CraftingScreenClass
                : TSubclassOf<UAstrawildCraftingScreenWidget>(UAstrawildCraftingScreenWidget::StaticClass());
            CraftingScreen = CreateWidget<UAstrawildCraftingScreenWidget>(this, WidgetClass);
        }
        if (CraftingScreen)
        {
            CraftingScreen->RefreshRecipes();
            CraftingScreen->AddToViewport(10);
            // Final-audit F-05: keyboard focus for the ESC close.
            CraftingScreen->SetKeyboardFocus();
            FInputModeUIOnly InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
    else
    {
        if (CraftingScreen)
        {
            CraftingScreen->RemoveFromParent();
        }
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

bool AAstrawildPlayerController::IsCraftingOpen() const
{
    return CraftingScreen && CraftingScreen->IsInViewport();
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
    if (IsCraftingOpen())
    {
        ToggleCraftingScreen();
    }
    if (IsJournalOpen())
    {
        ToggleJournalScreen();
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
            // Final-audit F-05: keyboard focus for ESC-resume.
            PauseMenuWidget->SetKeyboardFocus();
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

// --- PCR-1 (PG-1): the Field Journal (bestiary) screen ---

void AAstrawildPlayerController::ToggleJournalScreen()
{
    if (!IsLocalController())
    {
        return;
    }

    const bool bOpen = !IsJournalOpen();

    // Close siblings first — one full-screen UI at a time.
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
    if (IsCraftingOpen())
    {
        ToggleCraftingScreen();
    }
    if (IsPauseMenuOpen())
    {
        TogglePauseMenu();
    }

    if (bOpen)
    {
        if (!JournalScreen)
        {
            const TSubclassOf<UAstrawildJournalScreenWidget> WidgetClass = JournalScreenClass
                ? JournalScreenClass
                : TSubclassOf<UAstrawildJournalScreenWidget>(UAstrawildJournalScreenWidget::StaticClass());
            JournalScreen = CreateWidget<UAstrawildJournalScreenWidget>(this, WidgetClass);
        }
        if (JournalScreen)
        {
            JournalScreen->RefreshJournal();
            JournalScreen->AddToViewport(10);
            // F-05 convention: keyboard focus so P/ESC close without a mouse click.
            JournalScreen->SetKeyboardFocus();
            FInputModeUIOnly InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
    }
    else
    {
        if (JournalScreen)
        {
            JournalScreen->RemoveFromParent();
        }
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

bool AAstrawildPlayerController::IsJournalOpen() const
{
    return JournalScreen && JournalScreen->IsInViewport();
}

bool AAstrawildPlayerController::IsAnyScreenOpen() const
{
    return IsShopOpen() || IsDialogueOpen() || IsInventoryOpen() || IsResearchOpen() || IsCraftingOpen() || IsPauseMenuOpen() || IsJournalOpen();
}


// ===========================================================================
// LCP-3 — LAN co-op client routing (first Client RPCs in the module)
// ===========================================================================

void AAstrawildPlayerController::ClientNotify_Implementation(const FText& Message)
{
    Notify(Message);
}

void AAstrawildPlayerController::NotifyPlayer(const FText& Message)
{
    // Whatever machine owns this controller's screen gets the message: the
    // listen-host/standalone PC is its own local controller (Notify directly);
    // a server-side PC for a REMOTE player routes to that client.
    if (IsLocalController())
    {
        Notify(Message);
    }
    else
    {
        ClientNotify(Message);
    }
}

void AAstrawildPlayerController::ClientOpenVendorShop_Implementation(AAstrawildNPCCharacter* Vendor)
{
    OpenShop(Vendor);
}

void AAstrawildPlayerController::ClientOpenVendorDialogue_Implementation(AAstrawildNPCCharacter* Npc)
{
    OpenDialogue(Npc);
}

void AAstrawildPlayerController::ClientOpenCraftingScreen_Implementation(AAstrawildCraftingStationActor* Station)
{
    if (!IsCraftingOpen())
    {
        ToggleCraftingScreen();
    }
}

void AAstrawildPlayerController::ServerVendorTrade_Implementation(AAstrawildNPCCharacter* Vendor, const FName ItemId, const int32 Quantity, const bool bBuy)
{
    // Fail-closed server validation: the vendor must be a valid, interactable
    // NPC near this player's pawn (the client-side shop screen can be spoofed;
    // the trade cannot). TryPurchase/TrySell re-check authority + price +
    // currency internally (they were already authority-guarded — this RPC is
    // the routing the shop widget previously lacked for remote clients).
    APawn* Pawn = GetPawn();
    if (!Vendor || !Pawn || !Vendor->NpcDefinition)
    {
        return;
    }
    if (FVector::DistSquared(Pawn->GetActorLocation(), Vendor->GetActorLocation()) > FMath::Square(600.0f))
    {
        NotifyPlayer(FText::FromString(TEXT("Too far from the vendor.")));
        return;
    }
    if (Quantity <= 0 || Quantity > 99)
    {
        return; // quantity sanity (row buttons trade x1; the gate is cheap insurance)
    }

    const EAstrawildVendorResult Result = bBuy
        ? Vendor->TryPurchase(Pawn, ItemId, Quantity)
        : Vendor->TrySell(Pawn, ItemId, Quantity);
    const TCHAR* ResultText = TEXT("declined");
    switch (Result)
    {
    case EAstrawildVendorResult::Success:       ResultText = bBuy ? TEXT("bought") : TEXT("sold"); break;
    case EAstrawildVendorResult::NotAVendor:    ResultText = TEXT("not a vendor"); break;
    case EAstrawildVendorResult::NotAWare:      ResultText = TEXT("not in this shop"); break;
    case EAstrawildVendorResult::NotEnoughCurrency: ResultText = TEXT("not enough currency"); break;
    case EAstrawildVendorResult::TooHeavy:      ResultText = TEXT("too heavy to carry"); break;
    case EAstrawildVendorResult::TooFarAway:    ResultText = TEXT("too far from the vendor"); break;
    default: break;
    }
    NotifyPlayer(FText::FromString(FString::Printf(TEXT("Trade %s."), ResultText)));
}

void AAstrawildPlayerController::ServerSubmitDialogueChoice_Implementation(AAstrawildNPCCharacter* Npc, const FName NodeId, const int32 ChoiceIndex)
{
    // Structural re-validation from registry truth (the client sent node +
    // choice indices from static content, but a modified client could send
    // anything — everything resolves again here, fail-closed).
    if (!Npc || !Npc->NpcDefinition || !DialogueComponent)
    {
        return;
    }
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildDialogueTreeDefinition* Tree = Registry ? Registry->FindDialogueTree(Npc->NpcDefinition->DialogueTreeId) : nullptr;
    const FAstrawildDialogueChoice* Choice = UAstrawildDialogueComponent::ResolveValidatedChoice(Tree, NodeId, ChoiceIndex);
    if (!Choice)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("LCP-3: rejected dialogue choice (npc=%s node=%s index=%d)."),
            *GetNameSafe(Npc), *NodeId.ToString(), ChoiceIndex);
        return;
    }

    // Conditional re-validation + consequence application run on THIS (server)
    // component — the talking NPC mirrors what OpenDialogue would have set.
    DialogueComponent->SetTalkingNpc(Npc);
    if (!DialogueComponent->EvaluateChoiceConditions(*Choice))
    {
        return; // the client may have shown a stale/filtered list; the server disagrees — no-op
    }
    DialogueComponent->ApplyChoiceConsequences(*Choice);
}

FName AAstrawildPlayerController::GetPlayerKey() const
{
    // Stable identity: player name first (set in the session flow / engine
    // login), session-unique slot id as the fallback (join order). Documented
    // caveat in LAN_COOP_SPEC §5: reconnect across sessions restores by NAME —
    // unnamed slots restore only within a matching join order.
    if (const APlayerState* PS = GetPlayerState())
    {
        const FString Name = PS->GetPlayerName();
        if (!Name.IsEmpty())
        {
            return FName(*Name);
        }
        return FName(*FString::Printf(TEXT("PlayerSlot_%d"), PS->GetPlayerId()));
    }
    return FName(*FString::Printf(TEXT("PlayerSlot_%d"), NetPlayerIndex));
}
