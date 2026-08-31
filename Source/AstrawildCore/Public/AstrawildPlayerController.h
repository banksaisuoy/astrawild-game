#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

class AAstrawildNPCCharacter;
class UAstrawildQuestComponent;
class UAstrawildHudWidget;
class UAstrawildInventoryScreenWidget;
class UAstrawildPauseMenuWidget;
class UAstrawildResearchScreenWidget;
class UAstrawildShopWidget;
class UAstrawildDialogueWidget;
class UAstrawildDialogueComponent;

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

    /** Pause menu class override point (defaults to the pure-C++ widget). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI")
    TSubclassOf<UAstrawildPauseMenuWidget> PauseMenuClass;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void TogglePauseMenu();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI")
    bool IsPauseMenuOpen() const;

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
    TObjectPtr<UAstrawildPauseMenuWidget> PauseMenuWidget;
};
