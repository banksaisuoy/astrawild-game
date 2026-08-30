#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

class AAstrawildNPCCharacter;
class UAstrawildQuestComponent;
class UAstrawildHudWidget;
class UAstrawildShopWidget;

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

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY()
    TObjectPtr<UAstrawildHudWidget> HudWidget;

    UPROPERTY()
    TObjectPtr<UAstrawildShopWidget> ShopWidget;
};
