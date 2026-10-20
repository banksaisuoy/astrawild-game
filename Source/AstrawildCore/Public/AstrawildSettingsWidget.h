#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildSettingsWidget.generated.h"

class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UAstrawildSettingsSave;

/**
 * CGC-4 (audit 2-c): the settings screen — the first player-adjustable
 * anything. Four rows, each "label  [−]  value  [+]" (simple UButtons built
 * with ConstructWidget, the inventory row-button idiom — no sliders), plus a
 * "Reset defaults" row-button:
 *
 *   ┌ Settings ─────────────────────────────┐
 *   │ Master Volume    [−]   100%   [+]     │  → UAstrawildAudioDirectorSubsystem::SetMasterVolume (CGC-3 hook, immediate)
 *   │ Mouse Sensitivity [−]  ×1.0   [+]     │  → AAstrawildPlayerCharacter::SetLookSensitivityMultiplier (consumed in Look())
 *   │ Field of View     [−]   90°   [+]     │  → local pawn FollowCamera->SetFieldOfView
 *   │ Gamma             [−]  1.00   [+]     │  → r.Gamma console variable (IConsoleManager, fail-closed)
 *   │ [Reset defaults]          [Close]     │
 *   └───────────────────────────────────────┘
 *
 * Every change applies IMMEDIATELY and persists to the ASTRAWILD_Settings slot
 * (a separate preferences slot — never the world-save schema). Pure-C++ UMG
 * (the PCR/CGC-2 screen pattern); reachable through the pause menu only, ESC
 * and the Close button dismiss it. Step/clamp/format math lives in PURE STATIC
 * functions (world-free, test 138 pins them); the engine-side apply paths are
 * FAIL-CLOSED everywhere (missing console var / missing camera / missing slot
 * = silent skip, never a crash).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // -------------------------------------------------------------------
    // PURE STATIC setting math (world-free — the test-138 contract)
    // -------------------------------------------------------------------

    /**
     * Step a setting by ±Step and clamp to [Min, Max]. The single clamp/step
     * authority for every row (volume 0.0..1.0 ×0.1, sensitivity 0.5..2.0
     * ×0.1, FOV 70..110 ×5, gamma 0.8..1.6 ×0.05).
     */
    static float StepSettingValue(float Current, float Min, float Max, float Step, bool bUp);

    /** Display formatters — one per row, pinned by test 138. */
    static FString FormatVolumeText(float Volume);       // "80%"
    static FString FormatSensitivityText(float Value);   // "×1.0"
    static FString FormatFovText(float FieldOfView);     // "90°"
    static FString FormatGammaText(float Gamma);         // "1.00"

    // -------------------------------------------------------------------
    // Range constants (single source of truth for rows AND tests)
    // -------------------------------------------------------------------

    static constexpr float VolumeMin = 0.0f;
    static constexpr float VolumeMax = 1.0f;
    static constexpr float VolumeStep = 0.1f;
    static constexpr float SensitivityMin = 0.5f;
    static constexpr float SensitivityMax = 2.0f;
    static constexpr float SensitivityStep = 0.1f;
    static constexpr float FovMin = 70.0f;
    static constexpr float FovMax = 110.0f;
    static constexpr float FovStep = 5.0f;
    static constexpr float GammaMin = 0.8f;
    static constexpr float GammaMax = 1.6f;
    static constexpr float GammaStep = 0.05f;

    // -------------------------------------------------------------------
    // ASTRAWILD_Settings slot persistence (separate from the world saves)
    // -------------------------------------------------------------------

    /** The preferences slot name — deliberately NOT ASTRAWILD_Main/Auto. */
    static constexpr const TCHAR* SettingsSlotName = TEXT("ASTRAWILD_Settings");

    /**
     * PURE factory: a settings object carrying the four defaults
     * (volume 1.0 / sensitivity 1.0 / FOV 90 / gamma 1.0). Pinned by test 138
     * without any disk IO.
     */
    static UAstrawildSettingsSave* MakeDefaultSettings();

    /**
     * Load the preferences slot; a missing or unreadable slot returns the
     * DEFAULTS (fail-closed — fresh machines start at 1.0/×1.0/90°/1.00).
     * The LoadGameFromSlot round-trip itself is ENGINE-UNVERIFIED until the
     * engine run; the null-path semantics are the pure part.
     */
    static UAstrawildSettingsSave* LoadSettingsOrDefaults();

    /** Write the preferences slot (UGameplayStatics::SaveGameToSlot, user index 0 — the SaveSubsystem precedent). */
    static bool SaveSettings(const UAstrawildSettingsSave* Settings);

    /**
     * Apply the four settings to the running session — the SAME path the boot
     * (PlayerController BeginPlay/OnPossess) and the screen rows use:
     * volume through the CGC-3 static hook (immediate, scales live loops),
     * sensitivity through the AAstrawildPlayerCharacter static (consumed in
     * Look()), FOV on the local pawn's FollowCamera, gamma through the
     * r.Gamma console variable. ALL paths fail closed (null world / missing
     * pawn / missing camera / missing console var = silent skip).
     */
    static void ApplySettings(UWorld* World, const UAstrawildSettingsSave* Settings);

    UAstrawildSettingsWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    /** Rebuild the four value labels from the working settings object. */
    void RefreshValueLabels();

    /** Persist the working settings object to the slot. */
    void CommitSettings();

    UFUNCTION()
    void HandleVolumeDownClicked();

    UFUNCTION()
    void HandleVolumeUpClicked();

    UFUNCTION()
    void HandleSensitivityDownClicked();

    UFUNCTION()
    void HandleSensitivityUpClicked();

    UFUNCTION()
    void HandleFovDownClicked();

    UFUNCTION()
    void HandleFovUpClicked();

    UFUNCTION()
    void HandleGammaDownClicked();

    UFUNCTION()
    void HandleGammaUpClicked();

    /** Restore all four settings to the defaults (applies + persists). */
    UFUNCTION()
    void HandleResetClicked();

    UFUNCTION()
    void HandleCloseClicked();

    /** The working settings object (mutated by the rows, persisted on change). */
    UPROPERTY()
    TObjectPtr<UAstrawildSettingsSave> SettingsData;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    // One value label per row (the [−]/[+] buttons are stateless — handlers
    // read + step + re-apply through the working object).
    UPROPERTY()
    TObjectPtr<UTextBlock> VolumeValueText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SensitivityValueText;

    UPROPERTY()
    TObjectPtr<UTextBlock> FovValueText;

    UPROPERTY()
    TObjectPtr<UTextBlock> GammaValueText;

    UPROPERTY()
    TObjectPtr<UButton> VolumeDownButton;

    UPROPERTY()
    TObjectPtr<UButton> VolumeUpButton;

    UPROPERTY()
    TObjectPtr<UButton> SensitivityDownButton;

    UPROPERTY()
    TObjectPtr<UButton> SensitivityUpButton;

    UPROPERTY()
    TObjectPtr<UButton> FovDownButton;

    UPROPERTY()
    TObjectPtr<UButton> FovUpButton;

    UPROPERTY()
    TObjectPtr<UButton> GammaDownButton;

    UPROPERTY()
    TObjectPtr<UButton> GammaUpButton;

    UPROPERTY()
    TObjectPtr<UButton> ResetButton;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
