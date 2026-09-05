#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

class AAstrawildNPCCharacter;
class UAstrawildQuestComponent;
class UAstrawildHudWidget;
class UAstrawildCraftingScreenWidget;
class UAstrawildInventoryScreenWidget;
class UAstrawildPauseMenuWidget;
class UAstrawildResearchScreenWidget;
class UAstrawildShopWidget;
class UAstrawildDialogueWidget;
class UAstrawildDialogueComponent;
class UAstrawildJournalScreenWidget;

/**
 * ASTRAWILD player controller: hosts the quest component (survives respawn) and
 * creates the C++-built HUD for the owning local player (no UMG assets required).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAstrawildPlayerController();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TObjectPtr<UAstrawildQuestComponent> QuestComponent;

    /** Production V2 Batch 3: persistent dialogue state (story flags + consequence routing). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dialogue")
    TObjectPtr<UAstrawildDialogueComponent> DialogueComponent;

    /** Dialogue screen class override point (defaults to the pure-C++ dialogue widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildDialogueWidget> DialogueWidgetClass;

    /** HUD class override point for future UMG-styled HUDs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildHudWidget> HudWidgetClass;

    /** Shop screen class override point (defaults to the pure-C++ shop widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildShopWidget> ShopWidgetClass;

    /**
     * Audit C-2/C-7: transient HUD notification for gameplay feedback (research
     * unlocks, work-site collection, capture results). No-op when no HUD exists.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void Notify(const FText& Message);

    // --- LCP-3: LAN co-op client routing (the first Client RPCs in the module) ---

    /**
     * LCP-3: server -> owning client notification. Host-local callers keep using
     * Notify(); server-side code that must reach a REMOTE player's screen routes
     * through NotifyPlayer (local -> Notify, remote -> ClientNotify).
     */
    UFUNCTION(Client, Reliable)
    void ClientNotify(const FText& Message);

    /** LCP-3: delivery helper - whatever machine owns this controller's screen gets the message. */
    void NotifyPlayer(const FText& Message);

    /**
     * LCP-4: STABLE per-player identity for roster partition + per-player save
     * blocks: the PlayerState name when set, else a session-unique slot id.
     * NOT the pawn object name (session-unique but unstable across saves).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Player")
    FName GetPlayerKey() const;

    /** LCP-3: server -> owning client shop open (remote vendor interaction). */
    UFUNCTION(Client, Reliable)
    void ClientOpenVendorShop(class AAstrawildNPCCharacter* Vendor);

    /** LCP-3: server -> owning client dialogue open (remote NPC conversation). */
    UFUNCTION(Client, Reliable)
    void ClientOpenVendorDialogue(class AAstrawildNPCCharacter* Npc);

    /** LCP-3: server -> owning client crafting screen open (remote station interaction). */
    UFUNCTION(Client, Reliable)
    void ClientOpenCraftingScreen(class AAstrawildCraftingStationActor* Station);

    /**
     * LCP-3: remote shop row transaction intent. Server validates the vendor
     * distance + routes through the authority-guarded TryPurchase/TrySell.
     */
    UFUNCTION(Server, Reliable)
    void ServerVendorTrade(class AAstrawildNPCCharacter* Vendor, FName ItemId, int32 Quantity, bool bBuy);

    /**
     * LCP-3: remote dialogue choice submission. The client sends the node +
     * choice index it displayed (static registry content); the server
     * re-resolves and re-validates everything structurally + conditionally
     * (fail-closed) before applying consequences through the authority
     * DialogueComponent.
     */
    UFUNCTION(Server, Reliable)
    void ServerSubmitDialogueChoice(class AAstrawildNPCCharacter* Npc, FName NodeId, int32 ChoiceIndex);

    /**
     * Batch 5 — Item C: open the vendor shop screen for the given NPC (switches
     * to UI-only input + mouse cursor). Local controller only; no-op otherwise.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Vendor|UI")
    void OpenShop(AAstrawildNPCCharacter* Vendor);

    /** Batch 5 — Item C: close the shop screen and restore game-only input. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Vendor|UI")
    void CloseShop();

    /** True while the shop screen is on the viewport. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Vendor|UI")
    bool IsShopOpen() const;

    // --- Production V2 Batch 3: dialogue screen (P12 Story/NPC) ---

    /** Open the conversation screen for the given NPC (UI-only input + mouse cursor). Local controller only. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Dialogue|UI")
    void OpenDialogue(AAstrawildNPCCharacter* Npc);

    /** Close the conversation screen and restore game-only input. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Dialogue|UI")
    void CloseDialogue();

    /** True while the dialogue screen is on the viewport. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Dialogue|UI")
    bool IsDialogueOpen() const;

    // --- Final production run: inventory / research / pause screens ---

    /** Inventory screen class override point (defaults to the pure-C++ widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildInventoryScreenWidget> InventoryScreenClass;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void ToggleInventoryScreen();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsInventoryOpen() const;

    /** Research screen class override point (defaults to the pure-C++ widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildResearchScreenWidget> ResearchScreenClass;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void ToggleResearchScreen();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsResearchOpen() const;

    /** Final-audit F-02: the crafting screen (recipe list + gates + timers + cancel)
     *  existed fully implemented but was never instantiated — stations auto-crafted
     *  the first passing recipe with no player agency. This is its open path. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildCraftingScreenWidget> CraftingScreenClass;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void ToggleCraftingScreen();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsCraftingOpen() const;

    /** Pause menu class override point (defaults to the pure-C++ widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildPauseMenuWidget> PauseMenuClass;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void TogglePauseMenu();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsPauseMenuOpen() const;

    // --- PCR: Field Journal / (Roster + Map arrive in PCR-2/PCR-3) ---

    /** PCR-1 (PG-1): Field Journal screen class override point (defaults to the pure-C++ widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildJournalScreenWidget> JournalScreenClass;

    /** PCR-1: toggle the Field Journal (bestiary) screen — key P. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void ToggleJournalScreen();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsJournalOpen() const;

    /** True when any full-screen UI owns the input (blocks gameplay shortcuts). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsAnyScreenOpen() const;

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY()
    TObjectPtr<UAstrawildHudWidget> HudWidget;

    UPROPERTY()
    TObjectPtr<UAstrawildShopWidget> ShopWidget;

    UPROPERTY()
    TObjectPtr<UAstrawildDialogueWidget> DialogueWidget;

    UPROPERTY()
    TObjectPtr<UAstrawildInventoryScreenWidget> InventoryScreen;

    UPROPERTY()
    TObjectPtr<UAstrawildResearchScreenWidget> ResearchScreen;

    UPROPERTY()
    TObjectPtr<UAstrawildCraftingScreenWidget> CraftingScreen;

    UPROPERTY()
    TObjectPtr<UAstrawildPauseMenuWidget> PauseMenuWidget;

    UPROPERTY()
    TObjectPtr<UAstrawildJournalScreenWidget> JournalScreen;
};
