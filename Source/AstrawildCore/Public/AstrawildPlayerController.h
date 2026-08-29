#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

class UAstrawildQuestComponent;
class UAstrawildHudWidget;

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

    /**
     * Audit C-2/C-7: transient HUD notification for gameplay feedback (research
     * unlocks, work-site collection, capture results). No-op when no HUD exists.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI")
    void Notify(const FText& Message);

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY()
    TObjectPtr<UAstrawildHudWidget> HudWidget;
};
