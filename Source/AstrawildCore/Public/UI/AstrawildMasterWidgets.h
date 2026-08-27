#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/AstrawildEchoDexRow.h"
#include "Data/AstrawildTechnologyData.h"
#include "AstrawildMasterWidgets.generated.h"

class UAstrawildInventoryWidget;
class UAstrawildCraftingWidget;
class UProgressBar;
class UTextBlock;
class UDataTable;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildGameplayHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> HungerBar;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> ThirstBar;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> SANBar;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|HUD")
    float HealthPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|HUD")
    float StaminaPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|HUD")
    float HungerPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|HUD")
    float ThirstPercent = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|HUD")
    float SANPercent = 1.0f;

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|HUD")
    void RefreshGameplayState();

private:
    float RefreshAccumulator = 0.0f;
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildEchoDexWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|UI|EchoDex")
    TObjectPtr<UDataTable> EchoDexTable;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|EchoDex")
    TArray<FAstrawildEchoDexRow> VisibleEntries;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|EchoDex")
    int32 SelectedDexOrder = INDEX_NONE;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|EchoDex")
    int32 RefreshDex();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|EchoDex")
    bool SelectDexOrder(int32 DexOrder);
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildTechnologyTreeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|UI|Technology")
    TObjectPtr<UDataTable> TechnologyTable;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Technology")
    TArray<FAstrawildTechnologyNodeRow> VisibleNodes;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Technology")
    TArray<FGameplayTag> UnlockedTechnologyTags;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Technology")
    int32 ResearchPoints = 0;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Technology")
    int32 RefreshTechnologyState();
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildDungeonStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Dungeon")
    FName ActiveDungeonId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Dungeon")
    float RemainingTimeSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Dungeon")
    int32 ParticipantCount = 0;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Dungeon")
    void RefreshDungeonState();
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildMasterHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildGameplayHUDWidget> GameplayHUD;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildInventoryWidget> InventoryPanel;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildCraftingWidget> CraftingPanel;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildEchoDexWidget> EchoDexPanel;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildTechnologyTreeWidget> TechnologyPanel;

    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UAstrawildDungeonStatusWidget> DungeonPanel;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Master")
    void RefreshAllPanels();
};
