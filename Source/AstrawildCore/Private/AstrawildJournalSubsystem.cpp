#include "AstrawildJournalSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildWildlifeDirectorSubsystem.h"
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
                // Final production run: hold-to-scan multiplies the observation rate
                // while the equipped scanner is active (PHASE 12 scanner framework).
                const bool bThisPlayerScanning = ActiveScanner.IsValid() && ActiveScanner.Get() == Player;
                const float Multiplier = bThisPlayerScanning ? ActiveScanMultiplier : 1.0f;
                ObservePlayer(Player, 0.5f * Multiplier);
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

        ObserveCandidate(Player, Echo->EchoDefinition->DefinitionId, Echo->GetActorLocation(), DeltaTime);
    }

    // FPP-1: bosses gain bestiary entries too — the journal previously iterated
    // AAstrawildEchoCharacter only, so the four boss species (Glass Tyrant,
    // Drowned Sovereign, Dawnfang, the Warden's kin) could never be observed,
    // leaving their element/weakness identity undiscoverable in the codex.
    for (TActorIterator<AAstrawildEchoBossCharacter> It(World); It; ++It)
    {
        AAstrawildEchoBossCharacter* Boss = *It;
        if (Boss && !Boss->BossSpeciesId.IsNone() && !Boss->IsDefeated())
        {
            ObserveCandidate(Player, Boss->BossSpeciesId, Boss->GetActorLocation(), DeltaTime);
        }
    }
}

void UAstrawildJournalSubsystem::ObserveCandidate(AAstrawildPlayerCharacter* Player,
    const FName DefinitionId, const FVector& TargetLocation, const float DeltaTime)
{
    if (!Player || !Player->FollowCamera || DefinitionId.IsNone())
    {
        return;
    }

    // Cone check (shared by the Echo and boss observation paths): distance +
    // view direction from the observing player's camera.
    const FVector CameraLocation = Player->FollowCamera->GetComponentLocation();
    const FVector ToTarget = TargetLocation - CameraLocation;
    const float Distance = ToTarget.Size();
    // Production V2 (Master Plan §10): scanner tiers extend observation range —
    // the equipped scanner's multiplier resolves per player (stock = base).
    const float EffectiveObservationDistance = ObservationDistance * GetScannerRangeMultiplier(Player);
    if (Distance > EffectiveObservationDistance)
    {
        return;
    }

    const float Dot = FVector::DotProduct(Player->FollowCamera->GetForwardVector(), ToTarget.GetSafeNormal());
    if (Dot < 0.75f) // ~41 degree half-angle view cone.
    {
        return;
    }

    FAstrawildJournalEntry& Entry = Entries.FindOrAdd(DefinitionId);
    const bool bFirstEncounter = Entry.TimesEncountered == 0;
    Entry.EchoDefinitionId = DefinitionId;
    Entry.TimesEncountered += bFirstEncounter ? 1 : 0;

    if (Entry.ObservationProgress < 100.0f)
    {
        // CGC-2: capture the pre-milestone scan flag so the 25% crossing below
        // can be narrated (GrantKnowledgeMilestones flips it internally).
        const bool bWasScanned = Entry.bScanned;
        Entry.ObservationProgress = FMath::Min(100.0f, Entry.ObservationProgress + ObservationProgressPerSecond * DeltaTime);
        GrantKnowledgeMilestones(Entry, DefinitionId);
        OnJournalUpdated.Broadcast(DefinitionId, Entry);

        // CGC-2: the first-study moment — observation milestones were silent
        // (log + RP only). EVERY newly-studied species now announces itself;
        // the FIRST one also teaches where the knowledge lives [P].
        if (!bWasScanned && Entry.bScanned)
        {
            if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
            {
                UWorld* World = GetWorld();
                UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
                const UAstrawildEchoDefinition* Def = Registry ? Registry->FindEcho(DefinitionId) : nullptr;
                const FString SpeciesName = Def && !Def->DisplayName.IsEmpty()
                    ? Def->DisplayName.ToString()
                    : DefinitionId.ToString();
                PC->NotifyPlayer(FText::FromString(FString::Printf(
                    TEXT("New species studied: %s — Field Journal updated"), *SpeciesName))); // LCP-3 remote routing
                PC->ShowOnce(TEXT("Hint_FirstStudy"),
                    FText::FromString(TEXT("The Field Journal [P] holds every species you observe.")));
            }
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

    // CGC-5 (audit 2-d): observation MILESTONES — the 25/50/100/150/200/229
    // title crossings announce themselves at the moment of crossing. Stateless
    // by construction: the crossing rule only fires when the scanned count
    // moves past a threshold, so no save flags exist to drift. Fires on the
    // scanned flag (the same "New species studied" language the CGC-2 toast
    // uses); the count derives live from the Entries store.
    if (bScanNew)
    {
        const int32 ScannedCount = GetScannedSpeciesCount();
        const FString MilestoneTitle = GetMilestoneTitleForCrossing(ScannedCount - 1, ScannedCount);
        if (!MilestoneTitle.IsEmpty())
        {
            UWorld* World = GetWorld();
            UAstrawildItemRegistrySubsystem* Registry = World
                ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
            const int32 TotalSpecies = Registry ? Registry->GetAllEchoDefinitions().Num() : 0;
            const FString MilestoneLine = TotalSpecies > 0
                ? FString::Printf(TEXT("Milestone: %s (%d/%d species studied)"), *MilestoneTitle, ScannedCount, TotalSpecies)
                : FString::Printf(TEXT("Milestone: %s"), *MilestoneTitle);
            if (World)
            {
                for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
                {
                    if (AAstrawildPlayerController* MilestonePC = Cast<AAstrawildPlayerController>(It->Get()))
                    {
                        // CGC-3 cue: knowledge beats carry the Research stinger.
                        MilestonePC->NotifyPlayer(FText::FromString(MilestoneLine), EAstrawildToastCue::Research); // LCP-3 remote routing
                    }
                }
            }
            UE_LOG(LogAstrawildAI, Log, TEXT("Observation milestone crossed: %s (%d species studied)."), *MilestoneTitle, ScannedCount);
        }
    }

    // Publish observation event for quests (directive §25 ObserveEcho objectives).
    if (bScanNew)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_EchoObserved, nullptr, DefinitionId, 1, FVector::ZeroVector);
                // Final production run: scanners complete knowledge faster — surface
                // the completion on the bus too (audio/UI hooks).
                EventBus->PublishEvent(TAG_Astrawild_Event_EchoScanned, nullptr, DefinitionId, 1, FVector::ZeroVector);
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

// --- Final production run (PHASE 12): active scanner framework ---

void UAstrawildJournalSubsystem::BeginActiveScan(AAstrawildPlayerCharacter* Scanner, const float Multiplier)
{
    ActiveScanner = Scanner;
    ActiveScanMultiplier = FMath::Clamp(Multiplier, 1.0f, 10.0f);
}

void UAstrawildJournalSubsystem::EndActiveScan()
{
    ActiveScanner = nullptr;
    ActiveScanMultiplier = 1.0f;
}

bool UAstrawildJournalSubsystem::IsScanActiveFor(const AAstrawildPlayerCharacter* Player) const
{
    return ActiveScanner.IsValid() && Player && ActiveScanner.Get() == Player;
}

void UAstrawildJournalSubsystem::AddExternalObservation(const AAstrawildEchoCharacter* Echo, const float Progress)
{
    // Final production run (PHASE 12): external observation feed (utility drone).
    if (!IsValid(Echo) || !IsValid(Echo->EchoDefinition) || Progress <= 0.0f)
    {
        return;
    }

    const FName DefinitionId = Echo->EchoDefinition->DefinitionId;
    FAstrawildJournalEntry& Entry = Entries.FindOrAdd(DefinitionId);
    if (Entry.TimesEncountered == 0)
    {
        Entry.EchoDefinitionId = DefinitionId;
        Entry.TimesEncountered = 1;
    }

    if (Entry.ObservationProgress < 100.0f)
    {
        Entry.ObservationProgress = FMath::Min(100.0f, Entry.ObservationProgress + Progress);
        GrantKnowledgeMilestones(Entry, DefinitionId);
        OnJournalUpdated.Broadcast(DefinitionId, Entry);
    }
}

float UAstrawildJournalSubsystem::GetScannerRangeMultiplier(const AAstrawildPlayerCharacter* Player)
{
    // Production V2 (Master Plan §10): scanner tiers extend observation range.
    // Resolves the equipped scanner item through the inventory component.
    if (!Player)
    {
        return 1.0f;
    }
    if (const UAstrawildInventoryComponent* Inventory = Player->FindComponentByClass<UAstrawildInventoryComponent>())
    {
        return Inventory->GetEquippedScannerRangeMultiplier();
    }
    return 1.0f;
}

// --- CGC-5 (audit 2-d): collection long-tail — census + titles (pure) ---

bool UAstrawildJournalSubsystem::IsJournalEntryObserved(const FAstrawildJournalEntry& Entry)
{
    // Canonical "any real contact" rule — the journal screen's discovered
    // test single-sourced (scan flag, any knowledge flag, partial progress,
    // or an encounter all reveal the species).
    return Entry.bScanned || Entry.bFoodDiscovered || Entry.bHabitatDiscovered
        || Entry.bWeaknessDiscovered || Entry.ObservationProgress > 0.0f || Entry.TimesEncountered > 0;
}

TSet<FName> UAstrawildJournalSubsystem::GetObservedSpeciesIds() const
{
    TSet<FName> Observed;
    for (const TPair<FName, FAstrawildJournalEntry>& Pair : Entries)
    {
        if (IsJournalEntryObserved(Pair.Value))
        {
            Observed.Add(Pair.Key);
        }
    }
    return Observed;
}

int32 UAstrawildJournalSubsystem::GetScannedSpeciesCount() const
{
    int32 Count = 0;
    for (const TPair<FName, FAstrawildJournalEntry>& Pair : Entries)
    {
        Count += Pair.Value.bScanned ? 1 : 0;
    }
    return Count;
}

TArray<FAstrawildZoneCensusRow> UAstrawildJournalSubsystem::ComputeZoneCensus(
    UAstrawildItemRegistrySubsystem* Registry, const TSet<FName>& ObservedSpeciesIds)
{
    // One row per REAL zone (enum order = the zone table's reading order);
    // species with HomeZone None count toward no zone (none exist today —
    // the definition default is DawnFields — but the rule stays honest).
    TArray<FAstrawildZoneCensusRow> Census;
    Census.Reserve(static_cast<int32>(EAstrawildZone::Count) - 1);
    for (int32 ZoneIndex = 1; ZoneIndex < static_cast<int32>(EAstrawildZone::Count); ++ZoneIndex)
    {
        FAstrawildZoneCensusRow Row;
        Row.Zone = static_cast<EAstrawildZone>(ZoneIndex);
        Census.Add(Row);
    }

    // The CGC-1 bestiary view is the single HomeZone join (never a second
    // copy of the species->zone rule). Null registry -> 12 zeroed rows.
    TArray<FAstrawildBestiaryRow> Bestiary;
    UAstrawildWildlifeDirectorSubsystem::CollectBestiaryRows(Registry, Bestiary);
    for (const FAstrawildBestiaryRow& BestiaryRow : Bestiary)
    {
        const int32 ZoneIndex = static_cast<int32>(BestiaryRow.HomeZone);
        if (ZoneIndex <= 0 || ZoneIndex >= static_cast<int32>(EAstrawildZone::Count))
        {
            continue;
        }
        FAstrawildZoneCensusRow& ZoneRow = Census[ZoneIndex - 1];
        ++ZoneRow.Total;
        if (ObservedSpeciesIds.Contains(BestiaryRow.SpeciesId))
        {
            ++ZoneRow.Observed;
        }
    }
    return Census;
}

FString UAstrawildJournalSubsystem::GetMilestoneTitleForCrossing(const int32 PreviousKnownCount, const int32 NewKnownCount)
{
    // Crossing semantics Previous < T <= New fire exactly once per threshold —
    // stateless, no save flags. Highest crossed threshold wins (the jump
    // 0 -> 229 announces the journal-complete title alone). Threshold set:
    // 25 / 50 / 100 / 150 / 200 / 229 (229 = the species census; census
    // discipline pins it — see the validator's census gates).
    struct FMilestone
    {
        int32 Threshold;
        const TCHAR* Title;
    };
    static const FMilestone Milestones[] = {
        { 25,  TEXT("Field Novice of the Vale") },
        { 50,  TEXT("Field Scholar of the Vale") },
        { 100, TEXT("Field Warden of the Vale") },
        { 150, TEXT("Vale Naturalist") },
        { 200, TEXT("Master of the Wilds") },
        { 229, TEXT("Voice of the Vale \u2014 the journal is complete") },
    };
    for (int32 Index = ARRAY_COUNT(Milestones) - 1; Index >= 0; --Index)
    {
        if (PreviousKnownCount < Milestones[Index].Threshold && Milestones[Index].Threshold <= NewKnownCount)
        {
            return Milestones[Index].Title;
        }
    }
    return FString();
}

FString UAstrawildJournalSubsystem::GetCurrentTitleForCount(const int32 KnownCount)
{
    // Live stateless derivation for the journal summary: the highest title
    // whose threshold the count has reached ("" before the 25th species).
    struct FMilestone
    {
        int32 Threshold;
        const TCHAR* Title;
    };
    static const FMilestone Milestones[] = {
        { 25,  TEXT("Field Novice of the Vale") },
        { 50,  TEXT("Field Scholar of the Vale") },
        { 100, TEXT("Field Warden of the Vale") },
        { 150, TEXT("Vale Naturalist") },
        { 200, TEXT("Master of the Wilds") },
        { 229, TEXT("Voice of the Vale \u2014 the journal is complete") },
    };
    for (int32 Index = ARRAY_COUNT(Milestones) - 1; Index >= 0; --Index)
    {
        if (KnownCount >= Milestones[Index].Threshold)
        {
            return Milestones[Index].Title;
        }
    }
    return FString();
}
