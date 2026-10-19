#include "AstrawildHudWidget.h"
#include "AstrawildLANSessionSubsystem.h" // LCP-6: session mode line

#include "AstrawildAttributeComponent.h"
#include "AstrawildBuildingComponent.h"
#include "AstrawildCaptureComponent.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDungeonPortalActor.h" // CGC-2: ReachLocation guidance target.
#include "AstrawildPowerSubsystem.h"
#include "AstrawildResourceNode.h" // CGC-2: CollectItem guidance target.
#include "AstrawildWorldEventSubsystem.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildInteractable.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildMountComponent.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildZoneSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h" // DCP-5: toast cue.

// ---------------------------------------------------------------------------
// CGC-2 (B): pure guidance formatting — world-free, pinned by the automation
// contract (a compass without art: text only, no widgets, no world).
// ---------------------------------------------------------------------------

FString UAstrawildHudWidget::FormatBearingText(const float YawToTargetDeg)
{
    // Wrap math (documented per the contract): fmod first (preserves the
    // fractional sector position), then the +360/+fmod pair folds NEGATIVE
    // bearings — atan2 outputs span (-180,180] — back into [0,360). A single
    // Fmod(Yaw+360,360) cannot fold values below -360, so the two-step fmod
    // keeps every finite input honest. round(yaw/45) % 8 then lands each wind
    // center exactly (0=N, 45=NE ... 315=NW) and wraps 350 to N.
    float Wrapped = FMath::Fmod(YawToTargetDeg, 360.0f);
    if (Wrapped < 0.0f)
    {
        Wrapped += 360.0f;
    }
    static const TCHAR* Winds[8] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
                                     TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW") };
    const int32 WindIndex = static_cast<int32>(FMath::RoundToInt(Wrapped / 45.0f)) % 8;
    return Winds[WindIndex];
}

FString UAstrawildHudWidget::FormatDistanceText(const float DistanceCm)
{
    // Planar ground distance -> meters (UE units: 1cm), rounded, floored at 0
    // (a negative/garbage input reads "0m", never a crash), capped at 999m+.
    const int32 Meters = FMath::Max(0, FMath::RoundToInt(DistanceCm / 100.0f));
    if (Meters > 999)
    {
        return TEXT("999m+");
    }
    return FString::Printf(TEXT("%dm"), Meters);
}

void UAstrawildHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    BuildWidgetTree();
}

void UAstrawildHudWidget::BuildWidgetTree()
{
    WidgetTree->RootWidget = nullptr;
    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
    RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    WidgetTree->RootWidget = RootCanvas;

    // --- Helper lambdas ---
    auto MakeText = [this](const FString& Name, const FLinearColor& Color, int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
        Text->SetShadowColorAndOpacity(FLinearColor::Black.CopyWithNewOpacity(0.85f));
        return Text;
    };

    auto MakeBar = [this](const FString& Name, const FLinearColor& Color) -> UProgressBar*
    {
        UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *Name);
        Bar->SetFillColorAndOpacity(Color);
        Bar->SetPercent(1.0f);
        return Bar;
    };

    auto AnchorSlot = [](UCanvasPanelSlot* InSlot, const FVector2D& AnchorMin, const FVector2D& AnchorMax, const FVector2D& Offset, const FVector2D& Size)
    {
        InSlot->SetAnchors(FAnchors(AnchorMin.X, AnchorMin.Y, AnchorMax.X, AnchorMax.Y));
        InSlot->SetPosition(Offset);
        InSlot->SetSize(Size);
    };

    // --- Left-bottom vitals (directive §11) ---
    HealthBar = MakeBar(TEXT("HealthBar"), FLinearColor(0.85f, 0.16f, 0.12f, 1.0f));
    AnchorSlot(RootCanvas->AddChildToCanvas(HealthBar), FVector2D(0.02f, 0.86f), FVector2D(0.02f, 0.86f), FVector2D::ZeroVector, FVector2D(280.0f, 18.0f));

    StaminaBar = MakeBar(TEXT("StaminaBar"), FLinearColor(0.95f, 0.75f, 0.10f, 1.0f));
    AnchorSlot(RootCanvas->AddChildToCanvas(StaminaBar), FVector2D(0.02f, 0.895f), FVector2D(0.02f, 0.895f), FVector2D::ZeroVector, FVector2D(240.0f, 12.0f));

    HungerBar = MakeBar(TEXT("HungerBar"), FLinearColor(0.90f, 0.45f, 0.10f, 1.0f));
    AnchorSlot(RootCanvas->AddChildToCanvas(HungerBar), FVector2D(0.02f, 0.925f), FVector2D(0.02f, 0.925f), FVector2D::ZeroVector, FVector2D(200.0f, 10.0f));

    ThirstBar = MakeBar(TEXT("ThirstBar"), FLinearColor(0.10f, 0.70f, 0.65f, 1.0f));
    AnchorSlot(RootCanvas->AddChildToCanvas(ThirstBar), FVector2D(0.02f, 0.95f), FVector2D(0.02f, 0.95f), FVector2D::ZeroVector, FVector2D(200.0f, 10.0f));

    // --- Top-center world info (directive §12/§13) ---
    TimeText = MakeText(TEXT("TimeText"), FLinearColor::White, 22);
    AnchorSlot(RootCanvas->AddChildToCanvas(TimeText), FVector2D(0.5f, 0.02f), FVector2D(0.5f, 0.02f), FVector2D(-110.0f, 0.0f), FVector2D(220.0f, 26.0f));

    WeatherText = MakeText(TEXT("WeatherText"), FLinearColor(0.75f, 0.85f, 0.95f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(WeatherText), FVector2D(0.5f, 0.055f), FVector2D(0.5f, 0.055f), FVector2D(-110.0f, 0.0f), FVector2D(220.0f, 20.0f));

    // Audit C-2: research points readout — the player needs to see the pool they spend
    // at the Research Desk (previously research state was invisible).
    ResearchText = MakeText(TEXT("ResearchText"), FLinearColor(0.70f, 0.90f, 0.98f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(ResearchText), FVector2D(0.5f, 0.085f), FVector2D(0.5f, 0.085f), FVector2D(-110.0f, 0.0f), FVector2D(220.0f, 20.0f));

    // LCP-6 (PART 6): the active session mode is ALWAYS visible — a quiet
    // top-left line naming the mode (host authority included).
    SessionModeText = MakeText(TEXT("SessionModeText"), FLinearColor(0.55f, 0.75f, 0.7f, 1.0f), 12);
    AnchorSlot(RootCanvas->AddChildToCanvas(SessionModeText), FVector2D(0.02f, 0.02f), FVector2D(0.02f, 0.02f), FVector2D::ZeroVector, FVector2D(340.0f, 16.0f));

    // --- Batch 7: Shattered Vale zone banner (title + flavor + discovery count) ---
    ZoneBannerText = MakeText(TEXT("ZoneBannerText"), FLinearColor(0.95f, 0.92f, 0.80f, 1.0f), 19);
    AnchorSlot(RootCanvas->AddChildToCanvas(ZoneBannerText), FVector2D(0.5f, 0.125f), FVector2D(0.5f, 0.125f), FVector2D(-160.0f, 0.0f), FVector2D(320.0f, 24.0f));

    ZoneSubText = MakeText(TEXT("ZoneSubText"), FLinearColor(0.62f, 0.70f, 0.72f, 1.0f), 12);

    // Production V2: world-event banner (amber, urgent) + power readout + weapon line.
    WorldEventText = MakeText(TEXT("WorldEventText"), FLinearColor(1.0f, 0.72f, 0.28f, 1.0f), 16);
    AnchorSlot(RootCanvas->AddChildToCanvas(WorldEventText), FVector2D(0.5f, 0.185f), FVector2D(0.5f, 0.185f), FVector2D(-180.0f, 0.0f), FVector2D(360.0f, 22.0f));

    PowerText = MakeText(TEXT("PowerText"), FLinearColor(0.55f, 0.95f, 0.85f, 1.0f), 13);
    AnchorSlot(RootCanvas->AddChildToCanvas(PowerText), FVector2D(0.5f, 0.105f), FVector2D(0.5f, 0.105f), FVector2D(-110.0f, 0.0f), FVector2D(220.0f, 18.0f));

    WeaponText = MakeText(TEXT("WeaponText"), FLinearColor(0.98f, 0.80f, 0.55f, 1.0f), 13);
    AnchorSlot(RootCanvas->AddChildToCanvas(WeaponText), FVector2D(0.98f, 0.86f), FVector2D(0.98f, 0.86f), FVector2D(-300.0f, 0.0f), FVector2D(300.0f, 18.0f));
    ZoneSubText->SetAutoWrapText(true);
    AnchorSlot(RootCanvas->AddChildToCanvas(ZoneSubText), FVector2D(0.5f, 0.155f), FVector2D(0.5f, 0.155f), FVector2D(-170.0f, 0.0f), FVector2D(340.0f, 30.0f));

    // Audit C-6: build-mode readout — current piece + piece index/total + controls.
    BuildText = MakeText(TEXT("BuildText"), FLinearColor(0.90f, 0.88f, 0.60f, 1.0f), 15);
    AnchorSlot(RootCanvas->AddChildToCanvas(BuildText), FVector2D(0.5f, 0.72f), FVector2D(0.5f, 0.72f), FVector2D(-260.0f, 0.0f), FVector2D(520.0f, 22.0f));

    // --- Top-left quest tracker (directive §25) ---
    QuestText = MakeText(TEXT("QuestText"), FLinearColor(0.98f, 0.90f, 0.60f, 1.0f), 14);
    QuestText->SetAutoWrapText(true);
    AnchorSlot(RootCanvas->AddChildToCanvas(QuestText), FVector2D(0.02f, 0.02f), FVector2D(0.02f, 0.02f), FVector2D(0.0f, 0.0f), FVector2D(360.0f, 110.0f));

    // --- Center-bottom interaction prompt + capture chance (directive §8) ---
    PromptText = MakeText(TEXT("PromptText"), FLinearColor::White, 16);
    AnchorSlot(RootCanvas->AddChildToCanvas(PromptText), FVector2D(0.5f, 0.82f), FVector2D(0.5f, 0.82f), FVector2D(-200.0f, 0.0f), FVector2D(400.0f, 22.0f));

    CaptureText = MakeText(TEXT("CaptureText"), FLinearColor(0.55f, 0.95f, 0.75f, 1.0f), 15);
    AnchorSlot(RootCanvas->AddChildToCanvas(CaptureText), FVector2D(0.5f, 0.855f), FVector2D(0.5f, 0.855f), FVector2D(-200.0f, 0.0f), FVector2D(400.0f, 22.0f));

    // --- Right-bottom party command (directive §10) ---
    CommandText = MakeText(TEXT("CommandText"), FLinearColor(0.70f, 0.85f, 0.98f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(CommandText), FVector2D(0.98f, 0.93f), FVector2D(0.98f, 0.93f), FVector2D(-300.0f, 0.0f), FVector2D(300.0f, 20.0f));

    // --- GDP: ability readiness line (under the party command) ---
    AbilityText = MakeText(TEXT("AbilityText"), FLinearColor(0.75f, 0.95f, 0.65f, 1.0f), 13);
    AnchorSlot(RootCanvas->AddChildToCanvas(AbilityText), FVector2D(0.98f, 0.955f), FVector2D(0.98f, 0.955f), FVector2D(-300.0f, 0.0f), FVector2D(300.0f, 18.0f));

    // --- Right-bottom equipment readout (wave 3) ---
    EquipmentText = MakeText(TEXT("EquipmentText"), FLinearColor(0.98f, 0.80f, 0.55f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(EquipmentText), FVector2D(0.98f, 0.90f), FVector2D(0.98f, 0.90f), FVector2D(-300.0f, 0.0f), FVector2D(300.0f, 20.0f));

    // --- Notification line ---
    NotificationText = MakeText(TEXT("NotificationText"), FLinearColor(0.95f, 0.95f, 0.85f, 1.0f), 15);
    AnchorSlot(RootCanvas->AddChildToCanvas(NotificationText), FVector2D(0.5f, 0.14f), FVector2D(0.5f, 0.14f), FVector2D(-320.0f, 0.0f), FVector2D(640.0f, 24.0f));

    // --- Final production run (PHASE 14): boss health bar (top-center) + name/phase. ---
    BossHealthBar = MakeBar(TEXT("BossHealthBar"), FLinearColor(0.72f, 0.12f, 0.28f, 1.0f));
    AnchorSlot(RootCanvas->AddChildToCanvas(BossHealthBar), FVector2D(0.5f, 0.20f), FVector2D(0.5f, 0.20f), FVector2D(-200.0f, 0.0f), FVector2D(400.0f, 16.0f));

    BossText = MakeText(TEXT("BossText"), FLinearColor(0.98f, 0.75f, 0.78f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(BossText), FVector2D(0.5f, 0.225f), FVector2D(0.5f, 0.225f), FVector2D(-200.0f, 0.0f), FVector2D(400.0f, 20.0f));

    // --- Final Run (FR-6): full-screen ending banner — shows for the whole
    // post-game once an ending is chosen (persistent verdict, not a toast). ---
    EndingBannerText = MakeText(TEXT("EndingBannerText"), FLinearColor(0.98f, 0.86f, 0.55f, 1.0f), 16);
    EndingBannerText->SetAutoWrapText(true);
    AnchorSlot(RootCanvas->AddChildToCanvas(EndingBannerText), FVector2D(0.5f, 0.255f), FVector2D(0.5f, 0.255f), FVector2D(-220.0f, 0.0f), FVector2D(440.0f, 40.0f));
    EndingBannerText->SetVisibility(ESlateVisibility::Hidden);

    // --- Final production run: scanner + drone companion readout (under capture). ---
    ScanText = MakeText(TEXT("ScanText"), FLinearColor(0.62f, 0.88f, 0.98f, 1.0f), 14);
    AnchorSlot(RootCanvas->AddChildToCanvas(ScanText), FVector2D(0.5f, 0.885f), FVector2D(0.5f, 0.885f), FVector2D(-200.0f, 0.0f), FVector2D(400.0f, 20.0f));

    // --- Sci-Fi reticle at screen center ---
    CrosshairText = MakeText(TEXT("CrosshairText"), FLinearColor(0.29f, 0.86f, 0.78f, 0.85f), 16);
    AnchorSlot(RootCanvas->AddChildToCanvas(CrosshairText), FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(-12.0f, -12.0f), FVector2D(24.0f, 24.0f));
    CrosshairText->SetText(FText::FromString(TEXT("•")));
}

AAstrawildPlayerCharacter* UAstrawildHudWidget::GetAstrawildPawn() const
{
    const APlayerController* PC = GetOwningPlayer();
    return PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
}

void UAstrawildHudWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= 0.15f)
    {
        RefreshAccumulator = 0.0f;
        RefreshState();
    }

    ZoneSweepAccumulator += InDeltaTime;
    if (ZoneSweepAccumulator >= 0.3f)
    {
        ZoneSweepAccumulator = 0.0f;
        RefreshZoneBanner();
    }

    if (NotificationRemaining > 0.0f)
    {
        NotificationRemaining -= InDeltaTime;
        if (NotificationRemaining <= 0.0f && NotificationText)
        {
            NotificationText->SetText(FText::GetEmpty());
        }
    }
}

void UAstrawildHudWidget::PushNotification(const FText& Message)
{
    if (NotificationText)
    {
        NotificationText->SetText(Message);
        NotificationRemaining = 4.0f;
    }

    // DCP-5 (user directive: re-open deferred work): toasts carry a UI cue.
    // First PlaySound2D in the module — the A_UI_Confirm package exists in
    // Content/Audio (imported with the audio batch; the literal-path
    // LoadObject idiom mirrors CaptureComponent's capture stinger). Fail
    // closed: a missing/renamed cue logs nothing and the toast still shows —
    // audio is enrichment, never a gate.
    if (USoundBase* ToastCue = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/A_UI_Confirm")))
    {
        UGameplayStatics::PlaySound2D(this, ToastCue, 0.35f);
    }
}

void UAstrawildHudWidget::RefreshZoneBanner()
{
    AAstrawildPlayerCharacter* Pawn = GetAstrawildPawn();
    if (!Pawn)
    {
        return;
    }

    // Pure static lookup — no replication needed (Batch 7 zone table).
    const EAstrawildZone NewZone = UAstrawildZoneSubsystem::GetZoneAt(Pawn->GetActorLocation());
    if (NewZone == CurrentZone)
    {
        return;
    }
    CurrentZone = NewZone;

    const FAstrawildZoneDescriptor* Desc = UAstrawildZoneSubsystem::FindZone(NewZone);
    if (!Desc)
    {
        if (ZoneBannerText)
        {
            ZoneBannerText->SetText(FText::FromString(TEXT("The Untamed Wilds")));
        }
        if (ZoneSubText)
        {
            ZoneSubText->SetText(FText::FromString(FString::Printf(TEXT("Zones discovered: %d/%d"),
                LocallyDiscoveredZones.Num(), UAstrawildZoneSubsystem::GetZoneCount())));
        }
        return;
    }

    // First visit this session? Celebrate it.
    if (!LocallyDiscoveredZones.Contains(NewZone))
    {
        LocallyDiscoveredZones.Add(NewZone);
        PushNotification(FText::FromString(FString::Printf(TEXT("Region discovered: %s  (%d/%d)"),
            *Desc->DisplayName.ToString(), LocallyDiscoveredZones.Num(), UAstrawildZoneSubsystem::GetZoneCount())));
    }

    if (ZoneBannerText)
    {
        ZoneBannerText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  Threat %d"),
            *Desc->DisplayName.ToString(), Desc->ThreatLevel)));
    }
    if (ZoneSubText)
    {
        ZoneSubText->SetText(FText::FromString(FString::Printf(TEXT("%s  ·  Zones discovered: %d/%d"),
            *Desc->Subtitle.ToString(), LocallyDiscoveredZones.Num(), UAstrawildZoneSubsystem::GetZoneCount())));
    }
}

void UAstrawildHudWidget::RefreshState()
{
    AAstrawildPlayerCharacter* Pawn = GetAstrawildPawn();
    if (!Pawn)
    {
        return;
    }

    // Vitals.
    if (Pawn->SurvivalComponent)
    {
        const FAstrawildSurvivalStats& Stats = Pawn->SurvivalComponent->GetStats();
        if (HealthBar) { HealthBar->SetPercent(Pawn->SurvivalComponent->GetHealthFraction()); }
        if (StaminaBar) { StaminaBar->SetPercent(Pawn->SurvivalComponent->GetStaminaFraction()); }
        if (HungerBar) { HungerBar->SetPercent(Stats.Hunger / 100.0f); }
        if (ThirstBar) { ThirstBar->SetPercent(Stats.Thirst / 100.0f); }
    }

    // World info.
    if (const UWorld* World = GetWorld())
    {
        if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
        {
            if (TimeText)
            {
                TimeText->SetText(FText::FromString(FString::Printf(TEXT("Day %d  %s"),
                    GameState->DayNumber, *GameState->GetTimeOfDayText().ToString())));
            }
            if (WeatherText)
            {
                // Audit fix (M-4): show the REAL ambient temperature from the survival
                // component instead of a hardcoded 20C.
                const float TemperatureC = Pawn->SurvivalComponent
                    ? Pawn->SurvivalComponent->GetStats().Temperature
                    : 20.0f;
                WeatherText->SetText(FText::FromString(FString::Printf(TEXT("%s  %.0fC"),
                    *UEnum::GetDisplayValueAsText(GameState->WeatherState).ToString(),
                    TemperatureC)));
            }
        }
    }

    // Audit C-2: research pool readout.
    if (ResearchText)
    {
        if (const UWorld* World = GetWorld())
        {
            if (World->GetGameInstance())
            {
                if (SessionModeText)
                {
                    // LCP-6 (PART 6): the active session mode is always visible.
                    SessionModeText->SetText(FText::FromString(UAstrawildLANSessionSubsystem::DescribeSessionMode(this)));
                }
                if (const UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
                {
                    ResearchText->SetText(FText::FromString(FString::Printf(TEXT("Research: %d RP"),
                        Research->GetResearchPoints())));
                }
            }
        }
    }

    // Audit C-6: build-mode readout — piece name, index/total and the control hints.
    if (BuildText)
    {
        if (Pawn->BuildingComponent && Pawn->BuildingComponent->IsPlacing())
        {
            int32 PieceIndex = 0;
            int32 PieceCount = 0;
            Pawn->BuildingComponent->GetPlacementPieceInfo(PieceIndex, PieceCount);
            BuildText->SetText(FText::FromString(FString::Printf(TEXT("Build: %s [%d/%d]  wheel:cycle  N:rotate  LMB:place  B:exit"),
                *Pawn->BuildingComponent->GetCurrentDefinitionDisplayName().ToString(), PieceIndex, PieceCount)));
        }
        else
        {
            BuildText->SetText(FText::GetEmpty());
        }
    }

    // Interaction prompt + capture preview.
    AActor* Target = Pawn->FindInteractableActor();
    if (PromptText)
    {
        if (IsValid(Target) && Target->GetClass()->ImplementsInterface(UAstrawildInteractable::StaticClass()))
        {
            PromptText->SetText(IAstrawildInteractable::Execute_GetInteractionPrompt(Target));
        }
        else if (const AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target))
        {
            if (Echo->bCaptured && Echo->OwnerPlayerId == Pawn->GetFName() && Echo->EchoDefinition)
            {
                // FPP-1: E on your OWN echo mounts/evolves it — the old prompt
                // ("Capture Echo — needs Resonator") lied about what E does.
                if (Echo->MountComponent && UAstrawildMountComponent::IsRideableSpecies(
                        Echo->EchoDefinition->Family, Echo->EchoDefinition->BodyPlan, Echo->EchoDefinition->SizeClass))
                {
                    PromptText->SetText(FText::FromString(FString::Printf(
                        TEXT("Your Echo [E]: ride (Bond %.0f/25) or evolve · feed [R]"), Echo->Bond)));
                }
                else
                {
                    PromptText->SetText(FText::FromString(TEXT("Your Echo [E]: evolve check · feed [R]")));
                }
            }
            else
            {
                PromptText->SetText(FText::FromString(TEXT("Capture Echo [E] — needs Resonator")));
            }
        }
        else
        {
            PromptText->SetText(FText::GetEmpty());
        }
    }

    if (CaptureText)
    {
        if (const AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target))
        {
            const float Chance = Pawn->CaptureComponent ? Pawn->CaptureComponent->PreviewCaptureChance(Echo) : 0.0f;
            CaptureText->SetText(FText::FromString(FString::Printf(TEXT("Capture chance: %d%%"), FMath::RoundToInt(Chance * 100.0f))));
        }
        else
        {
            CaptureText->SetText(FText::GetEmpty());
        }
    }

    // Party command.
    if (CommandText)
    {
        CommandText->SetText(FText::FromString(FString::Printf(TEXT("Party command [C]: %s"),
            *UEnum::GetDisplayValueAsText(Pawn->CurrentPartyCommand).ToString())));
    }

    // GDP: ability readiness — how many party Echo abilities + player skills are
    // castable right now (the T/Y keys become readable instead of guessable).
    if (AbilityText)
    {
        int32 ReadyEchoAbilities = 0;
        int32 PartySize = 0;
        if (const UWorld* World = GetWorld())
        {
            for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
            {
                const AAstrawildEchoCharacter* Echo = *It;
                if (Echo && Echo->bCaptured && Echo->OwnerPlayerId == Pawn->GetFName() && !Echo->IsDefeated())
                {
                    PartySize++;
                    ReadyEchoAbilities += Echo->GetKnownAbilityIds().Num() -
                        Echo->AbilityCooldowns.Num(); // Off-cooldown approximation read client-side.
                }
            }
        }

        int32 UnlockedSkills = 0;
        int32 BoundSkillCount = 0;
        if (Pawn->AttributeComponent)
        {
            UnlockedSkills = Pawn->AttributeComponent->GetUnlockedSkills().Num();
            for (const EAstrawildPlayerSkillId BoundSkill : Pawn->AttributeComponent->GetBoundSkills())
            {
                if (BoundSkill != EAstrawildPlayerSkillId::None)
                {
                    ++BoundSkillCount;
                }
            }
        }

        // DP-4: the bound count makes the player's loadout (build identity)
        // readable at a glance — 0 bound = the legacy all-unlocked smart-cast.
        AbilityText->SetText(FText::FromString(FString::Printf(TEXT("[T] Echo abilities ready: %d/%d | [Y] Skills: %d unlocked, %d/3 bound"),
            FMath::Max(0, ReadyEchoAbilities), PartySize, UnlockedSkills, BoundSkillCount)));
    }

    // Equipment readout (wave 3): weapon ATK + shield block.
    if (EquipmentText && Pawn->InventoryComponent)
    {
        const UWorld* World = GetWorld();
        const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
        auto ResolveName = [Registry](const FName ItemId) -> FString
        {
            if (ItemId.IsNone() || !Registry)
            {
                return TEXT("Unarmed");
            }
            const UAstrawildItemDefinition* Def = Registry->FindItem(ItemId);
            return Def ? Def->DisplayName.ToString() : ItemId.ToString();
        };
        EquipmentText->SetText(FText::FromString(FString::Printf(TEXT("Weapon: %s (+%.0f) | Shield: %s | Armor: %s (%.0f)"),
            *ResolveName(Pawn->InventoryComponent->EquippedItemId),
            Pawn->InventoryComponent->GetEquippedWeaponAttackPower(),
            *ResolveName(Pawn->InventoryComponent->EquippedShieldItemId),
            *ResolveName(Pawn->InventoryComponent->EquippedArmorItemId),
            Pawn->InventoryComponent->GetEquippedArmorRating())));

        // Production V2 (Master Plan §8): weapon + ammo readability line.
        if (WeaponText)
        {
            FString WeaponLine;
            if (Pawn->CombatComponent)
            {
                if (UAstrawildWeaponDefinition* WeaponDef = Pawn->CombatComponent->GetEquippedWeaponDefinition())
                {
                    const FString WeaponName = WeaponDef->DisplayName.ToString();
                    WeaponLine = FString::Printf(TEXT("%s | DMG %.0f | Rate %.1f"),
                        *WeaponName, Pawn->CombatComponent->GetRangedDamage(),
                        Pawn->CombatComponent->GetRangedFireInterval());
                    if (!WeaponDef->AmmoItemId.IsNone() && Pawn->InventoryComponent)
                    {
                        WeaponLine += FString::Printf(TEXT(" | AMMO %d"), Pawn->InventoryComponent->GetQuantity(WeaponDef->AmmoItemId));
                    }
                }
                else if (!Pawn->InventoryComponent->EquippedItemId.IsNone())
                {
                    WeaponLine = FString::Printf(TEXT("%s (melee)"), *ResolveName(Pawn->InventoryComponent->EquippedItemId));
                }
            }
            WeaponText->SetText(FText::FromString(WeaponLine));
        }
    }

    // Production V2 (Master Plan §14): power-grid readout.
    if (PowerText)
    {
        if (const UWorld* World = GetWorld())
        {
            if (const UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
            {
                PowerText->SetText(FText::FromString(FString::Printf(TEXT("GRID  +%.0f / -%.0f  |  CELL %.0f"),
                    Power->GetTotalGeneration(), Power->GetTotalDraw(), Power->GetStoredEnergy())));
            }
        }
    }

    // Production V2 (Master Plan §19): active world-event banner.
    if (WorldEventText)
    {
        if (const UWorld* World = GetWorld())
        {
            if (const UAstrawildWorldEventSubsystem* WorldEvents = World->GetSubsystem<UAstrawildWorldEventSubsystem>())
            {
                const FText EventSummary = WorldEvents->GetActiveEventSummaryText();
                WorldEventText->SetText(EventSummary);
                WorldEventText->SetVisibility(EventSummary.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
            }
        }
    }

    // Quest tracker.
    if (QuestText)
    {
        FString Tracker;
        if (const APlayerController* PC = GetOwningPlayer())
        {
            if (const UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
            {
                const TArray<FAstrawildQuestObjective> Objectives = Quests->GetActiveObjectives();

                // CGC-2 (A): the active quest's TITLE leads the block — bare
                // "[ ] Collect 10 Dawnwood" lines never said WHO asked or WHY.
                const FName ActiveQuestId = Quests->GetActiveQuestId();
                if (!ActiveQuestId.IsNone())
                {
                    const UAstrawildItemRegistrySubsystem* Registry = GetWorld()
                        ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>()
                        : nullptr;
                    const UAstrawildQuestDefinition* QuestDef = Registry ? Registry->FindQuest(ActiveQuestId) : nullptr;
                    if (QuestDef && !QuestDef->Title.IsEmpty())
                    {
                        Tracker += FString::Printf(TEXT("%s\n"), *QuestDef->Title.ToString());
                    }
                }

                for (const FAstrawildQuestObjective& Objective : Objectives)
                {
                    Tracker += FString::Printf(TEXT("[ ] %s (%d/%d)\n"),
                        *Objective.ObjectiveText.ToString(), Objective.ProgressCount, Objective.RequiredCount);
                }

                // CGC-2 (B): one bearing/distance guidance line under the
                // objectives (RefreshState cadence — no new ticks).
                Tracker += ResolveObjectiveGuidance(Objectives, Pawn->GetActorLocation());
            }
        }
        QuestText->SetText(FText::FromString(Tracker));
    }

    // Final production run (PHASE 14): boss encounter bar — the NEAREST alive
    // boss within engagement range owns the bar. FPP-1 fix: the previous
    // first-found pick meant an arbitrary dungeon boss owned the HUD from
    // game start; the bar now appears only when a boss is actually engaged.
    if (BossHealthBar && BossText)
    {
        UWorld* World = GetWorld();
        AAstrawildEchoBossCharacter* Boss = nullptr;
        if (World)
        {
            constexpr float BossBarEngagementRangeSq = 4000.0f * 4000.0f;
            float BestDistanceSq = BossBarEngagementRangeSq;
            for (TActorIterator<AAstrawildEchoBossCharacter> It(World); It; ++It)
            {
                AAstrawildEchoBossCharacter* Candidate = *It;
                if (!Candidate || Candidate->IsDefeated())
                {
                    continue;
                }
                const float DistanceSq = FVector::DistSquared(
                    Candidate->GetActorLocation(), Pawn->GetActorLocation());
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    Boss = Candidate;
                }
            }
        }
        CachedBoss = Boss;

        if (Boss && !Boss->IsDefeated())
        {
            BossHealthBar->SetVisibility(ESlateVisibility::Visible);
            BossText->SetVisibility(ESlateVisibility::Visible);
            BossHealthBar->SetPercent(Boss->GetHealthFraction());
            // FR-11: per-boss display name — dynamic, never a hardcoded string.
            BossText->SetText(FText::FromString(FString::Printf(TEXT("%s — Phase %d%s%s"),
                *Boss->GetBossDisplayName().ToString(),
                Boss->CurrentPhase,
                Boss->bEnraged ? TEXT(" ENRAGED") : TEXT(""),
                Boss->bWeakPointExposed ? TEXT(" | WEAK POINT EXPOSED!") : TEXT(""))));
        }
        else
        {
            BossHealthBar->SetVisibility(ESlateVisibility::Hidden);
            BossText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // Final Run (FR-6): ending banner — persistent post-game verdict overlay.
    if (EndingBannerText)
    {
        UWorld* World = GetWorld();
        const AAstrawildGameState* GameState = World ? World->GetGameState<AAstrawildGameState>() : nullptr;
        if (GameState && GameState->EndingState != EAstrawildEndingState::None)
        {
            EndingBannerText->SetText(GameState->GetEndingBannerText());
            EndingBannerText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            EndingBannerText->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // Final production run: scanner + drone companion readout.
    if (ScanText)
    {
        FString ScanLine;
        UWorld* World = GetWorld();
        if (World)
        {
            if (const UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
            {
                if (Journal->IsScanActiveFor(Pawn))
                {
                    ScanLine += TEXT("SCANNING... ");
                }
            }
        }
        if (Pawn->GetActiveDrone())
        {
            ScanLine += TEXT("[Drone active]");
        }
        ScanText->SetText(FText::FromString(ScanLine));
    }

    // Reticle update: contracts to precision amber reticle when aiming, teal dot when hip-firing.
    if (CrosshairText && Pawn)
    {
        if (Pawn->IsAiming())
        {
            CrosshairText->SetText(FText::FromString(TEXT("< + >")));
            CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.60f, 0.19f, 1.0f)));
        }
        else
        {
            CrosshairText->SetText(FText::FromString(TEXT("•")));
            CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(0.29f, 0.86f, 0.78f, 0.85f)));
        }
    }
}

FString UAstrawildHudWidget::ResolveObjectiveGuidance(const TArray<FAstrawildQuestObjective>& Objectives,
    const FVector& PlayerLocation) const
{
    // The FIRST incomplete objective owns the line (mirrors the map highlight's
    // first-objective-wins policy).
    const FAstrawildQuestObjective* Objective = nullptr;
    for (const FAstrawildQuestObjective& Candidate : Objectives)
    {
        if (!Candidate.IsComplete())
        {
            Objective = &Candidate;
            break;
        }
    }
    if (!Objective || Objective->TargetId.IsNone())
    {
        return FString();
    }

    UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World
        ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>()
        : nullptr;

    FString TargetName;
    FVector TargetLocation = FVector::ZeroVector;
    bool bResolved = false;
    bool bAllDormant = false;
    EAstrawildActivityPattern ActivityPattern = EAstrawildActivityPattern::Diurnal;

    switch (Objective->Type)
    {
    case EAstrawildQuestObjectiveType::DiscoverPOI:
    {
        // The MapScreenWidget idiom: POI world point = zone center + the
        // definition's authored offset (the same projection the map pins use).
        const UAstrawildPOIDefinition* POI = Registry ? Registry->FindPOI(Objective->TargetId) : nullptr;
        const FAstrawildZoneDescriptor* Zone = POI ? UAstrawildZoneSubsystem::FindZone(POI->Zone) : nullptr;
        if (POI && Zone)
        {
            const FVector2D WorldXY = FVector2D(Zone->Bounds.GetCenter()) + POI->OffsetFromZoneCenter;
            TargetLocation = FVector(WorldXY.X, WorldXY.Y, PlayerLocation.Z);
            TargetName = POI->DisplayName.IsEmpty() ? Objective->TargetId.ToString() : POI->DisplayName.ToString();
            bResolved = true;
        }
        break;
    }
    case EAstrawildQuestObjectiveType::ObserveEcho:
    case EAstrawildQuestObjectiveType::CaptureEcho:
    case EAstrawildQuestObjectiveType::DefeatCreature:
    {
        const UAstrawildEchoDefinition* Def = Registry ? Registry->FindEcho(Objective->TargetId) : nullptr;
        TargetName = Def && !Def->DisplayName.IsEmpty()
            ? Def->DisplayName.ToString()
            : Objective->TargetId.ToString();
        if (Def)
        {
            ActivityPattern = Def->ActivityPattern;
        }
        if (World && Def)
        {
            // Nearest LIVE WILD instance (captured/defeated skip — the boss-bar
            // TActorIterator idiom). Honest dormancy read: CGC-1's turnover
            // writes bHidden through SetActorHiddenInGame; IsHidden() reads
            // that exact flag. Every live instance hidden -> the fallback line
            // reads the species' rhythm instead of pointing at an invisible dot.
            AAstrawildEchoCharacter* Nearest = nullptr;
            float BestDistanceSq = FLT_MAX;
            bool bAnyLiveWild = false;
            bool bAnyVisible = false;
            for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
            {
                AAstrawildEchoCharacter* Echo = *It;
                if (!Echo || Echo->bCaptured || Echo->IsDefeated() || !Echo->EchoDefinition ||
                    Echo->EchoDefinition->DefinitionId != Objective->TargetId)
                {
                    continue;
                }
                bAnyLiveWild = true;
                if (Echo->IsHidden())
                {
                    continue;
                }
                bAnyVisible = true;
                const float DistanceSq = FVector::DistSquared2D(Echo->GetActorLocation(), PlayerLocation);
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    Nearest = Echo;
                }
            }
            if (Nearest)
            {
                TargetLocation = Nearest->GetActorLocation();
                bResolved = true;
            }
            else if (bAnyLiveWild && !bAnyVisible)
            {
                bAllDormant = true;
            }
        }
        break;
    }
    case EAstrawildQuestObjectiveType::ReachLocation:
    {
        if (World)
        {
            // The matching dungeon portal / survey beacon (PortalId == the
            // objective's TargetId — the LocationReached publisher contract).
            AAstrawildDungeonPortalActor* Nearest = nullptr;
            float BestDistanceSq = FLT_MAX;
            for (TActorIterator<AAstrawildDungeonPortalActor> It(World); It; ++It)
            {
                AAstrawildDungeonPortalActor* Portal = *It;
                if (!Portal || Portal->PortalId != Objective->TargetId)
                {
                    continue;
                }
                const float DistanceSq = FVector::DistSquared2D(Portal->GetActorLocation(), PlayerLocation);
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    Nearest = Portal;
                }
            }
            if (Nearest)
            {
                TargetLocation = Nearest->GetActorLocation();
                // Friendly name: the survey id minus its Location_ prefix
                // (e.g. Location_HollowUnderlight -> HollowUnderlight).
                FString LocationName = Objective->TargetId.ToString();
                if (LocationName.StartsWith(TEXT("Location_")))
                {
                    LocationName = LocationName.RightChop(static_cast<int32>(FCString::Strlen(TEXT("Location_"))));
                }
                TargetName = LocationName;
                bResolved = true;
            }
            else
            {
                TargetName = Objective->TargetId.ToString();
            }
        }
        break;
    }
    case EAstrawildQuestObjectiveType::CollectItem:
    {
        // Cheap mapping: the node actor carries its yield item id directly
        // (ResourceItemId), so "nearest non-depleted node that yields the
        // target" is one honest field comparison per actor.
        const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(Objective->TargetId) : nullptr;
        TargetName = ItemDef && !ItemDef->DisplayName.IsEmpty()
            ? ItemDef->DisplayName.ToString()
            : Objective->TargetId.ToString();
        if (World)
        {
            AAstrawildResourceNode* Nearest = nullptr;
            float BestDistanceSq = FLT_MAX;
            for (TActorIterator<AAstrawildResourceNode> It(World); It; ++It)
            {
                AAstrawildResourceNode* Node = *It;
                if (!Node || Node->ResourceItemId != Objective->TargetId ||
                    AAstrawildResourceNode::IsNodeDepleted(Node->bInfiniteResource, Node->RemainingQuantity))
                {
                    continue;
                }
                const float DistanceSq = FVector::DistSquared2D(Node->GetActorLocation(), PlayerLocation);
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    Nearest = Node;
                }
            }
            if (Nearest)
            {
                TargetLocation = Nearest->GetActorLocation();
                bResolved = true;
            }
        }
        break;
    }
    default:
        // Non-spatial objective types (craft/build/tech/zone/survive) get no
        // compass line — a bearing to "place a workbench" would be a lie.
        return FString();
    }

    if (TargetName.IsEmpty())
    {
        return FString();
    }

    if (bAllDormant)
    {
        // Honest dormancy fallback: the species lives but sleeps under CGC-1's
        // turnover — nocturnal patterns get the genre beat, everything else
        // keeps the generic "search the wilds" nudge.
        if (ActivityPattern == EAstrawildActivityPattern::Nocturnal)
        {
            return FString::Printf(TEXT("\u2192 %s stirs after dark"), *TargetName);
        }
        return FString::Printf(TEXT("\u2192 %s — search the wilds"), *TargetName);
    }

    if (!bResolved)
    {
        // Spatial target, no live world instance right now — honest nudge.
        return FString::Printf(TEXT("\u2192 %s — search the wilds"), *TargetName);
    }

    // Planar (XY) ground distance + compass bearing. World convention: north is
    // the map's up axis = NEGATIVE world Y (the zone table's north row is the
    // lowest Y band), east is +X — so bearing = atan2(east, north) degrees
    // clockwise from true north, exactly what FormatBearingText expects.
    const FVector2D Delta(TargetLocation.X - PlayerLocation.X, TargetLocation.Y - PlayerLocation.Y);
    const float DistanceCm = Delta.Size();
    const float YawToTargetDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.X, -Delta.Y));
    return FString::Printf(TEXT("\u2192 %s ~%s %s"),
        *TargetName, *FormatDistanceText(DistanceCm), *FormatBearingText(YawToTargetDeg));
}
