#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildControlsScreenWidget.generated.h"

class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;

/**
 * CGC-2 (E): the read-only Controls screen — the in-game reference for the
 * 33-action input census (CGC-4 added the F6 HUD toggle). Until now the only
 * control map lived in
 * Docs/ASTRAWILD_INPUT_REFERENCE.md; a new player had no way to discover that
 * V scans or that TAB opens the pack.
 *
 * Pure-C++ UMG (the PCR screen pattern — same construction style as the
 * journal/research/inventory screens): a ScrollBox of UTextBlock rows built
 * in NativeConstruct from the static table below, plus the gamepad companion
 * note (INPUT_REFERENCE §1b) as a footer line.
 *
 * Reachable ONLY through the pause menu (the gamepad path) — no letter-key
 * binding of its own; ESC and the Close
 * button dismiss it.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildControlsScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * The full control map: exactly 33 rows — one per runtime input action
     * (BuildRuntimeInputDefaults count incl. Descend + the CGC-4 ToggleHUD)
     * — transcribed from
     * Docs/ASTRAWILD_INPUT_REFERENCE.md §1 with the gamepad companion binding
     * noted per row. Static so the census stays a pinned, world-free contract
     * (the automation test asserts the row count == 33).
     */
    static const TArray<FString>& GetControlRows();

    UAstrawildControlsScreenWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleCloseClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SummaryText;

    /** One row per control (ScrollBox keeps all 32 visible without scaling). */
    UPROPERTY()
    TObjectPtr<UScrollBox> RowsList;

    /** INPUT_REFERENCE §1b: the gamepad companion summary + smart-cast chord. */
    UPROPERTY()
    TObjectPtr<UTextBlock> GamepadNoteText;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
