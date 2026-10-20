#include "AstrawildSettingsWidget.h"

#include "AstrawildAudioDirectorSubsystem.h" // CGC-3: the static SetMasterVolume hook.
#include "AstrawildPlayerCharacter.h" // CGC-4: look sensitivity static + FollowCamera for FOV.
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h" // CGC-4: UAstrawildSettingsSave (the preferences object).
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Camera/CameraComponent.h" // FollowCamera->SetFieldOfView (the settings FOV row).
#include "Engine/World.h"
#include "HAL/IConsoleManager.h" // r.Gamma — the AstrawildPerformanceManager Set idiom.
#include "Kismet/GameplayStatics.h" // GetPlayerPawn + SaveGameToSlot/LoadGameFromSlot.

namespace
{
    constexpr float SettingsPanelWidth = 560.0f;
    constexpr float SettingsPanelHeight = 560.0f;
    constexpr float SettingsRowHeight = 52.0f;
}

// ---------------------------------------------------------------------------
// PURE STATIC setting math (world-free — test 138 pins the boundaries)
// ---------------------------------------------------------------------------

float UAstrawildSettingsWidget::StepSettingValue(float Current, float Min, float Max, float Step, const bool bUp)
{
    // Step then clamp: a step past either bound lands ON the bound (volume
    // 0.95 + 0.1 -> 1.0, sensitivity 0.5 - 0.1 -> 0.5, FOV 110 + 5 -> 110).
    const float Stepped = bUp ? Current + Step : Current - Step;
    return FMath::Clamp(Stepped, Min, Max);
}

FString UAstrawildSettingsWidget::FormatVolumeText(const float Volume)
{
    // 0.8 -> "80%" — the percent the player actually sets, never a raw float.
    return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Volume * 100.0f));
}

FString UAstrawildSettingsWidget::FormatSensitivityText(const float Value)
{
    // 1.0 -> "×1.0" — one decimal matches the 0.1 step granularity.
    return FString::Printf(TEXT("×%.1f"), Value);
}

FString UAstrawildSettingsWidget::FormatFovText(const float FieldOfView)
{
    // 90 -> "90°" — whole degrees match the 5-degree step.
    return FString::Printf(TEXT("%.0f°"), FieldOfView);
}

FString UAstrawildSettingsWidget::FormatGammaText(const float Gamma)
{
    // 1.0 -> "1.00" — two decimals match the 0.05 step granularity.
    return FString::Printf(TEXT("%.2f"), Gamma);
}

// ---------------------------------------------------------------------------
// ASTRAWILD_Settings slot persistence
// ---------------------------------------------------------------------------

UAstrawildSettingsSave* UAstrawildSettingsWidget::MakeDefaultSettings()
{
    // PURE factory — no disk IO. NewObject with no outer lands in the
    // transient package (the world-free automation idiom used across tests).
    UAstrawildSettingsSave* Settings = NewObject<UAstrawildSettingsSave>();
    if (Settings)
    {
        Settings->MasterVolume = 1.0f;
        Settings->Sensitivity = 1.0f;
        Settings->FieldOfView = 90.0f;
        Settings->Gamma = 1.0f;
    }
    return Settings;
}

UAstrawildSettingsSave* UAstrawildSettingsWidget::LoadSettingsOrDefaults()
{
    // Fail-closed: a missing/unreadable slot means DEFAULTS (a fresh machine
    // starts at 100% / ×1.0 / 90° / 1.00 and never knows anything failed).
    // The LoadGameFromSlot round-trip is ENGINE-UNVERIFIED until the engine
    // run — same UGameplayStatics call the world saves use (user index 0).
    if (USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0))
    {
        if (UAstrawildSettingsSave* Settings = Cast<UAstrawildSettingsSave>(Raw))
        {
            return Settings;
        }
    }
    return MakeDefaultSettings();
}

bool UAstrawildSettingsWidget::SaveSettings(const UAstrawildSettingsSave* Settings)
{
    if (!Settings)
    {
        return false;
    }
    // The SaveSubsystem SaveGameToSlot precedent (AstrawildSaveSubsystem.cpp),
    // pointed at the SEPARATE preferences slot — the world-save schema and
    // checksum machinery are never touched.
    return UGameplayStatics::SaveGameToSlot(Settings, SettingsSlotName, 0);
}

void UAstrawildSettingsWidget::ApplySettings(UWorld* World, const UAstrawildSettingsSave* Settings)
{
    if (!Settings)
    {
        return;
    }

    // 1) Master volume — the CGC-3 static hook: clamps 0..1 and scales every
    //    live director loop immediately (no world needed, never fails).
    UAstrawildAudioDirectorSubsystem::SetMasterVolume(Settings->MasterVolume);

    // 2) Look sensitivity — the player-character static consumed in Look().
    AAstrawildPlayerCharacter::SetLookSensitivityMultiplier(Settings->Sensitivity);

    // 3) Field of view — the local player's pawn camera (fail-closed: no
    //    world / no pawn / no camera = silent skip; the boot re-applies on
    //    OnPossess for the late-possess case).
    if (World)
    {
        if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
        {
            if (Player->FollowCamera)
            {
                Player->FollowCamera->SetFieldOfView(Settings->FieldOfView);
            }
        }
    }

    // 4) Gamma — the r.Gamma engine console variable (the
    //    AstrawildPerformanceManager FindConsoleVariable/Set idiom). Null var
    //    (stripped build) fails closed to a silent skip.
    if (IConsoleVariable* GammaVar = IConsoleManager::Get().FindConsoleVar(TEXT("r.Gamma")))
    {
        GammaVar->Set(Settings->Gamma, ECVF_SetByGameSetting);
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildSettingsWidget::UAstrawildSettingsWidget()
{
    // F-05 convention: focusable so ESC closes without a prior mouse click.
    bIsFocusable = true;
}

FReply UAstrawildSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleSettingsScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Load the persisted preferences first (defaults on a fresh machine).
    SettingsData = LoadSettingsOrDefaults();
    BuildWidgetTree();
    RefreshValueLabels();
}

void UAstrawildSettingsWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.92f, 0.95f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Settings")));

    // Row factory: "Label  [−]  value  [+]" — the inventory row-button idiom
    // (simple UButtons with a UTextBlock child; no sliders, no assets).
    auto MakeRowButton = [this](const FName& Name, const TCHAR* Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsRowButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
        ButtonLabel->SetText(FText::FromString(Label));
        Button->AddChild(ButtonLabel);
        return Button;
    };

    struct FRowSpec
    {
        const TCHAR* Label;
        UButton** DownButton;
        UButton** UpButton;
        UTextBlock** ValueText;
    };

    // The four setting rows, in display order.
    const FRowSpec Rows[] = {
        { TEXT("Master Volume"),     &VolumeDownButton,      &VolumeUpButton,      &VolumeValueText },
        { TEXT("Mouse Sensitivity"), &SensitivityDownButton, &SensitivityUpButton, &SensitivityValueText },
        { TEXT("Field of View"),     &FovDownButton,         &FovUpButton,         &FovValueText },
        { TEXT("Gamma"),             &GammaDownButton,       &GammaUpButton,       &GammaValueText },
    };

    UVerticalBox* RowsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRows"));

    for (int32 RowIndex = 0; RowIndex < 4; ++RowIndex)
    {
        const FRowSpec& Spec = Rows[RowIndex];

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
            *FString::Printf(TEXT("SettingsRow%d"), RowIndex));

        UTextBlock* RowLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsRowLabel"));
        RowLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));
        RowLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));
        RowLabel->SetText(FText::FromString(Spec.Label));

        *Spec.DownButton = MakeRowButton(*FString::Printf(TEXT("SettingsDown%d"), RowIndex), TEXT("−"), FLinearColor(0.28f, 0.34f, 0.42f, 1.0f));
        *Spec.UpButton = MakeRowButton(*FString::Printf(TEXT("SettingsUp%d"), RowIndex), TEXT("+"), FLinearColor(0.28f, 0.34f, 0.42f, 1.0f));

        *Spec.ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsRowValue"));
        (*Spec.ValueText)->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 0.9f, 1.0f)));
        (*Spec.ValueText)->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
        (*Spec.ValueText)->SetMinDesiredWidth(64.0f);

        if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(RowLabel)))
        {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot* DownSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(*Spec.DownButton)))
        {
            DownSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            DownSlot->SetVerticalAlignment(VAlign_Center);
            DownSlot->SetHorizontalAlignment(HAlign_Right);
        }
        if (UHorizontalBoxSlot* ValueSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(*Spec.ValueText)))
        {
            ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            ValueSlot->SetVerticalAlignment(VAlign_Center);
            ValueSlot->SetHorizontalAlignment(HAlign_Center);
        }
        if (UHorizontalBoxSlot* UpSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(*Spec.UpButton)))
        {
            UpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            UpSlot->SetVerticalAlignment(VAlign_Center);
        }

        if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(RowsBox->AddChildToVerticalBox(Row)))
        {
            RowSlot->SetSize(FSlateChildSize(ESlateSizeRule(ESlateSizeRule::Automatic)));
            RowSlot->SetPadding(FMargin(0.0f, (SettingsRowHeight - 32.0f) * 0.5f, 0.0f, 0.0f));
        }
    }

    // Handler wiring (OnClicked delegates cannot carry the row — one thunk per
    // button, the pause-menu skill-slot idiom).
    VolumeDownButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleVolumeDownClicked);
    VolumeUpButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleVolumeUpClicked);
    SensitivityDownButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleSensitivityDownClicked);
    SensitivityUpButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleSensitivityUpClicked);
    FovDownButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleFovDownClicked);
    FovUpButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleFovUpClicked);
    GammaDownButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleGammaDownClicked);
    GammaUpButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleGammaUpClicked);

    ResetButton = MakeRowButton(TEXT("SettingsReset"), TEXT("Reset defaults"), FLinearColor(0.42f, 0.32f, 0.22f, 1.0f));
    ResetButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleResetClicked);

    CloseButton = MakeRowButton(TEXT("SettingsClose"), TEXT("Close [ESC]"), FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildSettingsWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-SettingsPanelWidth * 0.5f, -SettingsPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(SettingsPanelWidth, 30.0f));
    }
    if (UCanvasPanelSlot* RowsSlot = Canvas->AddChildToCanvas(RowsBox))
    {
        RowsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        RowsSlot->SetPosition(FVector2D(-SettingsPanelWidth * 0.5f, -SettingsPanelHeight * 0.5f + 52.0f));
        RowsSlot->SetSize(FVector2D(SettingsPanelWidth, SettingsRowHeight * 4.0f));
    }
    if (UCanvasPanelSlot* ResetSlot = Canvas->AddChildToCanvas(ResetButton))
    {
        ResetSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ResetSlot->SetPosition(FVector2D(-SettingsPanelWidth * 0.5f, SettingsPanelHeight * 0.5f - 96.0f));
        ResetSlot->SetSize(FVector2D(170.0f, 32.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(SettingsPanelWidth * 0.5f - 140.0f, SettingsPanelHeight * 0.5f - 40.0f));
        CloseSlot->SetSize(FVector2D(140.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildSettingsWidget::RefreshValueLabels()
{
    if (!SettingsData)
    {
        return;
    }
    if (VolumeValueText)
    {
        VolumeValueText->SetText(FText::FromString(FormatVolumeText(SettingsData->MasterVolume)));
    }
    if (SensitivityValueText)
    {
        SensitivityValueText->SetText(FText::FromString(FormatSensitivityText(SettingsData->Sensitivity)));
    }
    if (FovValueText)
    {
        FovValueText->SetText(FText::FromString(FormatFovText(SettingsData->FieldOfView)));
    }
    if (GammaValueText)
    {
        GammaValueText->SetText(FText::FromString(FormatGammaText(SettingsData->Gamma)));
    }
}

void UAstrawildSettingsWidget::CommitSettings()
{
    // Apply immediately (the live session), then persist (the preferences
    // slot). A failed slot write is silent — the session still applies.
    ApplySettings(GetWorld(), SettingsData);
    SaveSettings(SettingsData);
    RefreshValueLabels();
}

void UAstrawildSettingsWidget::HandleVolumeDownClicked()
{
    if (SettingsData)
    {
        SettingsData->MasterVolume = StepSettingValue(SettingsData->MasterVolume, VolumeMin, VolumeMax, VolumeStep, false);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleVolumeUpClicked()
{
    if (SettingsData)
    {
        SettingsData->MasterVolume = StepSettingValue(SettingsData->MasterVolume, VolumeMin, VolumeMax, VolumeStep, true);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleSensitivityDownClicked()
{
    if (SettingsData)
    {
        SettingsData->Sensitivity = StepSettingValue(SettingsData->Sensitivity, SensitivityMin, SensitivityMax, SensitivityStep, false);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleSensitivityUpClicked()
{
    if (SettingsData)
    {
        SettingsData->Sensitivity = StepSettingValue(SettingsData->Sensitivity, SensitivityMin, SensitivityMax, SensitivityStep, true);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleFovDownClicked()
{
    if (SettingsData)
    {
        SettingsData->FieldOfView = StepSettingValue(SettingsData->FieldOfView, FovMin, FovMax, FovStep, false);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleFovUpClicked()
{
    if (SettingsData)
    {
        SettingsData->FieldOfView = StepSettingValue(SettingsData->FieldOfView, FovMin, FovMax, FovStep, true);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleGammaDownClicked()
{
    if (SettingsData)
    {
        SettingsData->Gamma = StepSettingValue(SettingsData->Gamma, GammaMin, GammaMax, GammaStep, false);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleGammaUpClicked()
{
    if (SettingsData)
    {
        SettingsData->Gamma = StepSettingValue(SettingsData->Gamma, GammaMin, GammaMax, GammaStep, true);
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleResetClicked()
{
    if (SettingsData)
    {
        SettingsData->MasterVolume = 1.0f;
        SettingsData->Sensitivity = 1.0f;
        SettingsData->FieldOfView = 90.0f;
        SettingsData->Gamma = 1.0f;
        CommitSettings();
    }
}

void UAstrawildSettingsWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleSettingsScreen();
    }
}
