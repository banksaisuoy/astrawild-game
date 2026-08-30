#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildResearchScreenWidget.generated.h"

class UAstrawildResearchScreenWidget;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Final production run (P1 gap closed): research screen row. Pure-C++ UMG.
 * States: UNLOCKED (dimmed, no button), AVAILABLE (Unlock button), LOCKED
 * (missing prerequisites listed, button disabled).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildResearchRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildResearchScreenWidget* ParentScreen, FName TechId);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleUnlockClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildResearchScreenWidget> ParentScreen;

    FName RowTechId = NAME_None;

    UPROPERTY()
    TObjectPtr<UButton> UnlockButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText;
};

/**
 * The research tree screen — restores player agency over the tree (the Research
 * Desk previously auto-bought the cheapest unlock). Lists every technology with
 * era, cost and prerequisites; the player picks the branch.
 *
 *   ┌ Research ────────── 14 RP ───────────── [Close] ┐
 *   │ Basic Crafting            Primitive    ✓ Unlocked │
 *   │ Armory            12 RP   Mechanical   [Unlock]   │
 *   │ Electrical        20 RP   Electrical   Needs: Mechanics │
 *   └──────────────────────────────────────────────────┘
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildResearchScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Rebuild the tech listing (call after unlocks / point changes). */
    void RefreshResearch();

protected:
    virtual void NativeConstruct() override;

private:
    friend class UAstrawildResearchRowWidget;

    void BuildWidgetTree();

    UFUNCTION()
    void HandleCloseClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PointsText;

    UPROPERTY()
    TObjectPtr<UVerticalBox> TechBox;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
