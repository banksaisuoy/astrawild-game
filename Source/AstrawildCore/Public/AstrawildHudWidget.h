#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildHudWidget.generated.h"

class UCanvasPanel;
class UProgressBar;
class UTextBlock;

/**
 * Pure-C++ HUD (directive §29): builds its entire widget tree in NativeConstruct —
 * zero UMG assets needed. Reads replicated state each refresh. A future CommonUI/UMG
 * screen can subclass or replace it through PlayerController.HudWidgetClass.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UAstrawildHudWidget();

    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Show a transient notification line (craft results, captures, quest beats). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|HUD")
    void PushNotification(const FText& Message);

protected:
    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> HungerBar;

    UPROPERTY()
    TObjectPtr<UProgressBar> ThirstBar;

    UPROPERTY()
    TObjectPtr<UTextBlock> TimeText;

    UPROPERTY()
    TObjectPtr<UTextBlock> WeatherText;

    UPROPERTY()
    TObjectPtr<UTextBlock> QuestText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PromptText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CaptureText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CommandText;

    UPROPERTY()
    TObjectPtr<UTextBlock> NotificationText;

private:
    float RefreshAccumulator = 0.0f;
    float NotificationRemaining = 0.0f;

    void BuildWidgetTree();
    void RefreshState();

    class AAstrawildPlayerCharacter* GetAstrawildPawn() const;
};
