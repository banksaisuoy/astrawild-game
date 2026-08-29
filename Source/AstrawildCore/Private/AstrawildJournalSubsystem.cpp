#include "AstrawildJournalSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResearchSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

UAstrawildJournalSubsystem::UAstrawildJournalSubsystem()
{
}

bool UAstrawildJournalSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildJournalSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildJournalSubsystem, STATGROUP_Tickables);
}

void UAstrawildJournalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

UAstrawildResearchSubsystem* UAstrawildJournalSubsystem::GetResearch() const
{
    const UWorld* World = GetWorld();
    return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;
}

void UAstrawildJournalSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    // Throttled observation sweep (T-6 fix — no per-frame actor iteration).
    ObservationSweepAccumulator += DeltaTime;
    if (ObservationSweepAccumulator < 0.5f)
    {
        return;
    }
    ObservationSweepAccumulator = 0.0f;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (const APlayerController* PC = It->Get())
        {
            if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
            {
                ObservePlayer(Player, 0.5f);
            }
        }
    }
}

void UAstrawildJournalSubsystem::ObservePlayer(AAstrawildPlayerCharacter* Player, const float DeltaTime)
{
    if (!Player || !Player->FollowCamera || !Player->IsAlive())
    {
        return;
    }

    // Cone check: any Echo within observation distance and roughly in view direction.
    const FVector CameraLocation = Player->FollowCamera->GetComponentLocation();
    const FVector ViewDirection = Player->FollowCamera->GetForwardVector();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor*> Echoes;
    Echoes.Reserve(16);
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        Echoes.Add(*It);
    }

    for (AActor* Actor : Echoes)
    {
        AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Actor);
        if (!Echo || !IsValid(Echo->EchoDefinition))
        {
            continue;
        }

        const FVector ToEcho = Echo->GetActorLocation() - CameraLocation;
        const float Distance = ToEcho.Size();
        if (Distance > ObservationDistance)
        {
            continue;
        }

        const float Dot = FVector::DotProduct(ViewDirection, ToEcho.GetSafeNormal());
        if (Dot < 0.75f) // ~41 degree half-angle view cone.
        {
            continue;
        }

        const FName DefinitionId = Echo->EchoDefinition->DefinitionId;
        FAstrawildJournalEntry& Entry = Entries.FindOrAdd(DefinitionId);
        const bool bFirstEncounter = Entry.TimesEncountered == 0;
        Entry.EchoDefinitionId = DefinitionId;
        Entry.TimesEncountered += bFirstEncounter ? 1 : 0;

        if (Entry.ObservationProgress < 100.0f)
        {
            Entry.ObservationProgress = FMath::Min(100.0f, Entry.ObservationProgress + ObservationProgressPerSecond * DeltaTime);
            GrantKnowledgeMilestones(Entry, DefinitionId);
            OnJournalUpdated.Broadcast(DefinitionId, Entry);
        }
    }
}

void UAstrawildJournalSubsystem::GrantKnowledgeMilestones(FAstrawildJournalEntry& Entry, const FName DefinitionId)
{
    // Knowledge unlocks at observation thresholds (directive §20 — gradual discovery).
    const float P = Entry.ObservationProgress;
    const bool bScanNew = !Entry.bScanned && P >= 25.0f;
    const bool bFoodNew = !Entry.bFoodDiscovered && P >= 50.0f;
    const bool bWeaknessNew = !Entry.bWeaknessDiscovered && P >= 75.0f;
    const bool bCompleteNew = P >= 100.0f && !Entry.bHabitatDiscovered;

    Entry.bScanned = Entry.bScanned || P >= 25.0f;
    Entry.bFoodDiscovered = Entry.bFoodDiscovered || P >= 50.0f;
    Entry.bWeaknessDiscovered = Entry.bWeaknessDiscovered || P >= 75.0f;
    Entry.bHabitatDiscovered = Entry.bHabitatDiscovered || P >= 100.0f;

    if (bScanNew || bFoodNew || bWeaknessNew || bCompleteNew)
    {
        if (UAstrawildResearchSubsystem* Research = GetResearch())
        {
            Research->AddResearchPoints(ObservationResearchReward);
        }
        UE_LOG(LogAstrawildAI, Log, TEXT("Journal milestone for %s at %.0f%%."), *DefinitionId.ToString(), P);
    }

    // Publish observation event for quests (directive §25 ObserveEcho objectives).
    if (bScanNew)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_EchoObserved, nullptr, DefinitionId, 1, FVector::ZeroVector);
            }
        }
    }
}

const FAstrawildJournalEntry* UAstrawildJournalSubsystem::FindEntry(const AAstrawildEchoCharacter* Echo) const
{
    if (!IsValid(Echo) || !IsValid(Echo->EchoDefinition))
    {
        return nullptr;
    }
    return Entries.Find(Echo->EchoDefinition->DefinitionId);
}

FAstrawildJournalEntry UAstrawildJournalSubsystem::GetEntry(const FName EchoDefinitionId) const
{
    if (const FAstrawildJournalEntry* Entry = Entries.Find(EchoDefinitionId))
    {
        return *Entry;
    }
    FAstrawildJournalEntry Empty;
    Empty.EchoDefinitionId = EchoDefinitionId;
    return Empty;
}

TArray<FAstrawildJournalEntry> UAstrawildJournalSubsystem::GetAllEntries() const
{
    TArray<FAstrawildJournalEntry> Out;
    Entries.GenerateValueArray(Out);
    return Out;
}

void UAstrawildJournalSubsystem::ExportForSave(TArray<FAstrawildJournalEntry>& OutEntries) const
{
    Entries.GenerateValueArray(OutEntries);
}

void UAstrawildJournalSubsystem::ImportFromSave(const TArray<FAstrawildJournalEntry>& InEntries)
{
    Entries.Reset();
    for (const FAstrawildJournalEntry& Entry : InEntries)
    {
        if (!Entry.EchoDefinitionId.IsNone())
        {
            Entries.Add(Entry.EchoDefinitionId, Entry);
        }
    }
}
