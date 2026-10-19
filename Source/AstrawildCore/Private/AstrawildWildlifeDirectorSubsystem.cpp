#include "AstrawildWildlifeDirectorSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoAIController.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEcosystemSubsystem.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildTerrainTileActor.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildZoneSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

UAstrawildWildlifeDirectorSubsystem::UAstrawildWildlifeDirectorSubsystem()
{
}

bool UAstrawildWildlifeDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildWildlifeDirectorSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildWildlifeDirectorSubsystem, STATGROUP_Tickables);
}

void UAstrawildWildlifeDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Cache the replicated world seed (deterministic across clients in SP —
    // the HostileSpawnerSubsystem init idiom). The boot pass re-reads it at
    // execution time so a late SetWorldSeed (load path) is always honored.
    if (const AAstrawildGameState* GS = InWorld.GetGameState<AAstrawildGameState>())
    {
        CachedWorldSeed = GS->WorldSeed;
    }

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Wildlife director online (per-zone cap %d, rare threshold %.2f, legendary one-off threshold %.2f)."),
        PerZoneSpawnCap, RareDifficultyThreshold, OneOffLegendaryDifficultyThreshold);
}

void UAstrawildWildlifeDirectorSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bSignalsWired)
    {
        WireBroadcastSignals();
    }

    if (bBootPassDone)
    {
        return;
    }

    // Boot-pass ordering: run AFTER the world bootstrapper finished its own
    // SpawnWildEchoes so the two spawners never interleave. The bootstrapper
    // flags completion at the end of its authority BeginPlay, which always
    // precedes the first world tick (GameMode::BeginPlay spawns it during
    // world begin-play — the same init-from-tick idiom HostileSpawner uses).
    const AAstrawildWorldBootstrapper* Bootstrapper = nullptr;
    for (TActorIterator<AAstrawildWorldBootstrapper> It(World); It; ++It)
    {
        Bootstrapper = *It;
        break;
    }
    if (!Bootstrapper || !Bootstrapper->HasCompletedAuthoritySpawn())
    {
        // Defensive fallback for worlds that never grow a bootstrapper
        // (preview/test levels): populate from the static zone table after a
        // short wait instead of staying empty forever.
        BootPassWaitElapsed += DeltaTime;
        if (BootPassWaitElapsed < BootPassWaitTimeoutSeconds)
        {
            return;
        }
    }

    RunBootSpawnPass();
}

void UAstrawildWildlifeDirectorSubsystem::WireBroadcastSignals()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    // First real subscribers for the previously-dead time/weather broadcasts
    // (audit finding D: OnHourChanged/OnDayChanged/OnWeatherChanged had zero
    // subscribers). The HuntSubsystem wired-once idiom.
    if (UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>())
    {
        Time->OnHourChanged.AddDynamic(this, &UAstrawildWildlifeDirectorSubsystem::HandleHourChanged);
        Time->OnDayChanged.AddDynamic(this, &UAstrawildWildlifeDirectorSubsystem::HandleDayChanged);
    }
    if (UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>())
    {
        Weather->OnWeatherChanged.AddDynamic(this, &UAstrawildWildlifeDirectorSubsystem::HandleWeatherChanged);
    }
    bSignalsWired = true;
}

// ---------------------------------------------------------------------------
// Pure static contracts (world-free — the ComputeVisualBand precedent)
// ---------------------------------------------------------------------------

void UAstrawildWildlifeDirectorSubsystem::GetBossEncounterSpeciesIds(TArray<FName>& OutIds)
{
    // FR-7 finale encounters: the Glass Tyrant world boss (Stormcrest), its
    // Eye Sentinel phase-2 summons, and the Drowned Sovereign dungeon boss.
    OutIds = { TEXT("Echo_GlassTyrant"), TEXT("Echo_EyeSentinel"), TEXT("Echo_DrownedSovereign") };
}

bool UAstrawildWildlifeDirectorSubsystem::IsBossEncounterSpecies(const FName SpeciesId)
{
    TArray<FName> BossIds;
    GetBossEncounterSpeciesIds(BossIds);
    return BossIds.Contains(SpeciesId);
}

void UAstrawildWildlifeDirectorSubsystem::CollectBestiaryRows(UAstrawildItemRegistrySubsystem* Registry, TArray<FAstrawildBestiaryRow>& OutRows)
{
    OutRows.Reset();
    if (!Registry)
    {
        return;
    }

    // Evolved-only forms: species that are the TARGET of another species'
    // evolution chain — earned through the level+bond gate, never spawned wild.
    TSet<FName> EvolutionTargets;
    for (const UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
    {
        if (Def && !Def->EvolveToDefinitionId.IsNone())
        {
            EvolutionTargets.Add(Def->EvolveToDefinitionId);
        }
    }

    for (UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
    {
        if (!Def || Def->DefinitionId.IsNone())
        {
            continue;
        }
        FAstrawildBestiaryRow Row;
        Row.SpeciesId = Def->DefinitionId;
        Row.HomeZone = Def->HomeZone;
        Row.CaptureDifficulty = Def->CaptureDifficulty;
        Row.PrimaryElement = Def->Element;
        Row.bEvolvedOnlyForm = EvolutionTargets.Contains(Def->DefinitionId);
        OutRows.Add(Row);
    }

    // Deterministic order (id-sorted) — the plan visits species breadth-first.
    OutRows.Sort([](const FAstrawildBestiaryRow& A, const FAstrawildBestiaryRow& B)
    {
        return A.SpeciesId.ToString() < B.SpeciesId.ToString();
    });
}

TArray<FAstrawildDirectorSpawnRow> UAstrawildWildlifeDirectorSubsystem::ComputeDirectorSpawnPlan(
    const TArray<FAstrawildBestiaryRow>& Bestiary,
    const TArray<FAstrawildZoneWildlifeRow>& LegacyRows,
    const int32 PerZoneCap)
{
    TArray<FAstrawildDirectorSpawnRow> Plan;
    if (PerZoneCap <= 0)
    {
        return Plan;
    }

    // Legacy coverage is species-level: a species placed by ANY legacy
    // ZoneWildlife row keeps its authored placement and density.
    TSet<FName> LegacySpecies;
    for (const FAstrawildZoneWildlifeRow& Row : LegacyRows)
    {
        LegacySpecies.Add(Row.SpeciesId);
    }

    struct FEligibleRow
    {
        const FAstrawildBestiaryRow* Row = nullptr;
        int32 DesiredCount = 0;
    };
    TArray<FEligibleRow> Eligible;
    for (const FAstrawildBestiaryRow& Row : Bestiary)
    {
        if (Row.SpeciesId.IsNone() || Row.HomeZone == EAstrawildZone::None)
        {
            continue; // no home zone -> no director placement
        }
        if (LegacySpecies.Contains(Row.SpeciesId))
        {
            continue; // the bootstrapper's zone table already places this species
        }
        if (IsBossEncounterSpecies(Row.SpeciesId))
        {
            continue; // boss species live behind their authored encounters
        }
        if (Row.bEvolvedOnlyForm)
        {
            continue; // evolved forms are earned through the evolution gate
        }
        if (Row.CaptureDifficulty >= OneOffLegendaryDifficultyThreshold)
        {
            continue; // legendary one-offs keep their authored single placement
        }
        FEligibleRow Entry;
        Entry.Row = &Row;
        Entry.DesiredCount = Row.CaptureDifficulty >= RareDifficultyThreshold ? 1 : 2;
        Eligible.Add(Entry);
    }

    // Breadth-first, deterministic: visit species in id order so every species
    // receives its 1st individual before any receives its 2nd.
    Eligible.Sort([](const FEligibleRow& A, const FEligibleRow& B)
    {
        return A.Row->SpeciesId.ToString() < B.Row->SpeciesId.ToString();
    });

    TMap<FName, int32> ZoneUsed;
    TMap<FName, int32> PlanIndexBySpecies;
    const auto PlaceIndividual = [&](const FEligibleRow& Entry) -> bool
    {
        const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZone(Entry.Row->HomeZone);
        if (!Zone)
        {
            return false; // unknown home zone -> fail closed (row data must resolve)
        }
        int32& Used = ZoneUsed.FindOrAdd(Zone->ZoneId);
        if (Used >= PerZoneCap)
        {
            return false;
        }
        ++Used;
        if (int32* PlanIndex = PlanIndexBySpecies.Find(Entry.Row->SpeciesId))
        {
            ++Plan[*PlanIndex].Count;
        }
        else
        {
            FAstrawildDirectorSpawnRow NewRow;
            NewRow.SpeciesId = Entry.Row->SpeciesId;
            NewRow.ZoneId = Zone->ZoneId;
            NewRow.Count = 1;
            PlanIndexBySpecies.Add(Entry.Row->SpeciesId, Plan.Add(NewRow));
        }
        return true;
    };

    for (const FEligibleRow& Entry : Eligible)
    {
        PlaceIndividual(Entry); // 1st individual for every species (cap permitting)
    }
    for (const FEligibleRow& Entry : Eligible)
    {
        if (Entry.DesiredCount >= 2)
        {
            PlaceIndividual(Entry); // 2nd individual for common species
        }
    }

    // Deterministic execution order: zone, then species.
    Plan.Sort([](const FAstrawildDirectorSpawnRow& A, const FAstrawildDirectorSpawnRow& B)
    {
        if (A.ZoneId == B.ZoneId)
        {
            return A.SpeciesId.ToString() < B.SpeciesId.ToString();
        }
        return A.ZoneId.ToString() < B.ZoneId.ToString();
    });
    return Plan;
}

bool UAstrawildWildlifeDirectorSubsystem::ShouldBeDormant(const EAstrawildActivityPattern Pattern, const float HourOfDay, const bool bRecentlyEngaged)
{
    // Engagement always wins: a creature in combat / capture-in-progress /
    // recently damaged never vanishes on the player, regardless of the clock.
    if (bRecentlyEngaged)
    {
        return false;
    }
    // The exact window math lives in ONE place — the shared pure helper that
    // AAstrawildEchoCharacter::IsCurrentlyActiveTime also routes through, so
    // AI sleep and world visibility can never disagree.
    return !AAstrawildEchoCharacter::IsActivityWindowOpen(Pattern, HourOfDay);
}

TArray<EAstrawildElementType> UAstrawildWildlifeDirectorSubsystem::GetWeatherBoostedElements(const EAstrawildWeatherState Weather)
{
    // Total map over the 8 real weather states using ONLY the 6 real element
    // enum values (Light/Ash/Flora/Frost/Pulse/Ember): every state boosts a
    // non-empty set, and every element is reachable from at least one state.
    switch (Weather)
    {
    case EAstrawildWeatherState::Clear:
        return { EAstrawildElementType::Light, EAstrawildElementType::Flora };
    case EAstrawildWeatherState::Cloudy:
        return { EAstrawildElementType::Pulse, EAstrawildElementType::Ash };
    case EAstrawildWeatherState::Rain:
        return { EAstrawildElementType::Frost, EAstrawildElementType::Flora, EAstrawildElementType::Pulse };
    case EAstrawildWeatherState::HeavyRain:
        return { EAstrawildElementType::Frost, EAstrawildElementType::Pulse };
    case EAstrawildWeatherState::Storm:
        return { EAstrawildElementType::Pulse, EAstrawildElementType::Ember, EAstrawildElementType::Light };
    case EAstrawildWeatherState::Fog:
        return { EAstrawildElementType::Ash, EAstrawildElementType::Flora };
    case EAstrawildWeatherState::Heat:
        return { EAstrawildElementType::Ember, EAstrawildElementType::Ash };
    case EAstrawildWeatherState::Cold:
        return { EAstrawildElementType::Frost, EAstrawildElementType::Light };
    default:
        return {};
    }
}

EAstrawildZone UAstrawildWildlifeDirectorSubsystem::ResolveNearestZone(const FVector2D& WorldXY)
{
    const EAstrawildZone At = UAstrawildZoneSubsystem::GetZoneAt(FVector(WorldXY.X, WorldXY.Y, 0.0f));
    if (At != EAstrawildZone::None)
    {
        return At;
    }

    // Off the grid (skiff flight over open water, dev voids): nearest zone center.
    const FAstrawildZoneDescriptor* Best = nullptr;
    float BestDistSq = -1.0f;
    for (const FAstrawildZoneDescriptor& Zone : UAstrawildZoneSubsystem::GetAllZones())
    {
        const float DistSq = FVector2D::DistSquared(Zone.GetCenter(), WorldXY);
        if (!Best || DistSq < BestDistSq)
        {
            Best = &Zone;
            BestDistSq = DistSq;
        }
    }
    return Best ? Best->Zone : EAstrawildZone::DawnFields;
}

// ---------------------------------------------------------------------------
// (a) Boot-time bestiary-wide spawn pass
// ---------------------------------------------------------------------------

void UAstrawildWildlifeDirectorSubsystem::RunBootSpawnPass()
{
    bBootPassDone = true;

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry)
    {
        return;
    }

    // Fresh seed read (the load path can SetWorldSeed after world begin play).
    if (const AAstrawildGameState* GS = World->GetGameState<AAstrawildGameState>())
    {
        CachedWorldSeed = GS->WorldSeed;
    }

    CollectBestiaryRows(Registry, BestiaryRows);
    AAstrawildWorldBootstrapper::GetLegacyZoneWildlife(LegacyRows);
    SpawnPlan = ComputeDirectorSpawnPlan(BestiaryRows, LegacyRows, PerZoneSpawnCap);

    int32 Spawned = 0;
    for (const FAstrawildDirectorSpawnRow& Row : SpawnPlan)
    {
        const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(Row.ZoneId);
        if (!Zone)
        {
            continue;
        }
        // Deterministic per-species placement stream (the WorldEventSubsystem
        // species-boost seed idiom: world seed ^ species hash).
        FRandomStream Stream(CachedWorldSeed ^ static_cast<uint32>(GetTypeHash(Row.SpeciesId)));
        for (int32 i = 0; i < Row.Count; ++i)
        {
            if (SpawnWildEchoInZone(Row.SpeciesId, *Zone, Stream))
            {
                ++Spawned;
            }
        }
    }

    // Apply the dormancy rule immediately so boot-time night creatures read as
    // hidden from the first frame instead of appearing and then vanishing.
    if (const AAstrawildGameState* GS = World->GetGameState<AAstrawildGameState>())
    {
        ApplyHourTurnover(GS->GetTimeOfDayHours());
    }

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Wildlife director: boot pass complete — %d plan rows, %d individuals spawned (per-zone cap %d)."),
        SpawnPlan.Num(), Spawned, PerZoneSpawnCap);
}

// ---------------------------------------------------------------------------
// (b) Hour-boundary dormancy turnover
// ---------------------------------------------------------------------------

void UAstrawildWildlifeDirectorSubsystem::HandleHourChanged(const int32 HourOfDay)
{
    ApplyHourTurnover(static_cast<float>(HourOfDay));
}

void UAstrawildWildlifeDirectorSubsystem::ApplyHourTurnover(const float HourOfDay)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    // One uniform rule for every wild echo — legacy AND director spawns alike.
    // Bosses are a SIBLING class (AAstrawildEchoBossCharacter : ACharacter), so
    // the EchoCharacter iterator never visits them; captured/rostered echoes
    // are excluded by the bCaptured filter; corpses by IsDefeated.
    int32 DormantCount = 0;
    int32 ActiveCount = 0;
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (!IsValid(Echo) || !IsValid(Echo->EchoDefinition) || Echo->bCaptured || Echo->IsDefeated())
        {
            continue;
        }
        const bool bDormant = ShouldBeDormant(
            Echo->EchoDefinition->ActivityPattern, HourOfDay, IsEchoEngaged(Echo));
        ApplyDormancy(Echo, bDormant);
        bDormant ? ++DormantCount : ++ActiveCount;
    }

    UE_LOG(LogAstrawildWorld, Verbose,
        TEXT("Wildlife director: hour turnover at %.1f — %d dormant, %d active wild echoes."),
        HourOfDay, DormantCount, ActiveCount);
}

void UAstrawildWildlifeDirectorSubsystem::ApplyDormancy(AAstrawildEchoCharacter* Echo, const bool bDormant)
{
    if (!IsValid(Echo))
    {
        return;
    }

    if (bDormant)
    {
        Echo->SetActorHiddenInGame(true);
        Echo->SetActorEnableCollision(false);
        Echo->SetActorTickEnabled(false);
        if (AAstrawildEchoAIController* AI = Cast<AAstrawildEchoAIController>(Echo->GetController()))
        {
            AI->StopMovement(); // the ExecuteSleep stop idiom
            AI->SetActorTickEnabled(false);
        }
        Echo->SetAIState(EAstrawildEchoAIState::Sleep);
    }
    else
    {
        Echo->SetActorTickEnabled(true);
        Echo->SetActorHiddenInGame(false);
        Echo->SetActorEnableCollision(true);
        if (AAstrawildEchoAIController* AI = Cast<AAstrawildEchoAIController>(Echo->GetController()))
        {
            AI->SetActorTickEnabled(true); // the AI resumes thinking on its own
        }
    }
}

bool UAstrawildWildlifeDirectorSubsystem::IsEchoEngaged(const AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo))
    {
        return false;
    }

    // In combat right now (replicated AI state set by the controller).
    if (Echo->CurrentAIState == EAstrawildEchoAIState::Combat)
    {
        return true;
    }
    // Active damage-over-time / status effects — hit recently.
    if (!Echo->StatusEffects.IsEmpty())
    {
        return true;
    }
    // Wounded: wild echoes do not regenerate health, so damage below max is a
    // conservative "took player damage" signal (a mid-capture target can never
    // dorm out from under the player).
    if (Echo->CurrentHealth < Echo->GetMaxHealth())
    {
        return true;
    }
    // Capture-in-progress proxy: a player inside approach/tracking range
    // (capture tracking distance is 900cm — this covers the approach too).
    const UWorld* World = GetWorld();
    if (World)
    {
        for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
        {
            const AAstrawildPlayerCharacter* Player = *It;
            if (IsValid(Player) && FVector::Dist(Player->GetActorLocation(), Echo->GetActorLocation()) <= EngagementRadius)
            {
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// (c) Weather-gated rare appearances
// ---------------------------------------------------------------------------

void UAstrawildWildlifeDirectorSubsystem::HandleWeatherChanged(const EAstrawildWeatherState NewState, const EAstrawildWeatherState OldState)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    ++WeatherChangeCounter;
    HandleWeatherRareAppearance(NewState);
}

void UAstrawildWildlifeDirectorSubsystem::HandleWeatherRareAppearance(const EAstrawildWeatherState NewState)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry || BestiaryRows.IsEmpty())
    {
        return;
    }

    const TArray<EAstrawildElementType> Boosted = GetWeatherBoostedElements(NewState);
    if (Boosted.IsEmpty())
    {
        return; // fail-closed (the map is total, but never trust it blindly)
    }

    // The player's zone (nearest zone center when standing off-grid; Dawn
    // Fields when nobody has a pawn yet — the same default the camp uses).
    EAstrawildZone PlayerZone = EAstrawildZone::DawnFields;
    if (const AAstrawildPlayerCharacter* Player = FindLocalPlayer())
    {
        PlayerZone = ResolveNearestZone(FVector2D(Player->GetActorLocation().X, Player->GetActorLocation().Y));
    }
    const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(PlayerZone);
    if (!ZoneDesc)
    {
        return;
    }

    // Candidates: boosted-element species homed in the player's zone, with the
    // same exclusion classes as the boot plan (bosses / evolved-only forms /
    // legendary one-offs). BestiaryRows is id-sorted, so this is deterministic.
    TArray<const FAstrawildBestiaryRow*> Candidates;
    for (const FAstrawildBestiaryRow& Row : BestiaryRows)
    {
        if (Row.HomeZone != PlayerZone || Row.bEvolvedOnlyForm || IsBossEncounterSpecies(Row.SpeciesId))
        {
            continue;
        }
        if (Row.CaptureDifficulty >= OneOffLegendaryDifficultyThreshold)
        {
            continue;
        }
        if (Boosted.Contains(Row.PrimaryElement))
        {
            Candidates.Add(&Row);
        }
    }
    if (Candidates.IsEmpty())
    {
        return;
    }

    // Deterministic pick: world seed ^ weather-change counter (the directive's
    // determinism discipline — the same weather sequence picks the same rares).
    FRandomStream Stream(CachedWorldSeed ^ static_cast<uint32>(WeatherChangeCounter));
    int32 PickCount = Stream.FRand() < 0.35f ? 2 : 1;
    PickCount = FMath::Min(PickCount, Candidates.Num());

    int32 Spawned = 0;
    for (int32 Pick = 0; Pick < PickCount; ++Pick)
    {
        const int32 LastIndex = Candidates.Num() - 1;
        const int32 Index = FMath::Clamp(FMath::FloorToInt32(Stream.FRandRange(0.0f, static_cast<float>(LastIndex))), 0, LastIndex);
        const FAstrawildBestiaryRow* Row = Candidates[Index];
        Candidates.RemoveAt(Index);
        if (!Row)
        {
            continue;
        }
        if (AAstrawildEchoCharacter* Echo = SpawnWildEchoInZone(Row->SpeciesId, *ZoneDesc, Stream))
        {
            ++Spawned;
            Echo->SetAIState(EAstrawildEchoAIState::Alert); // the emergence reads as a sighting
            if (const UAstrawildEchoDefinition* Def = Registry->FindEcho(Row->SpeciesId))
            {
                BroadcastToast(FText::FromString(FString::Printf(
                    TEXT("The weather stirs — a %s emerges!"), *Def->DisplayName.ToString())));
            }
        }
    }

    if (Spawned > 0)
    {
        UE_LOG(LogAstrawildWorld, Log,
            TEXT("Wildlife director: weather transition %d spawned %d rare appearance(s) in %s."),
            WeatherChangeCounter, Spawned, *ZoneDesc->ZoneId.ToString());
    }
}

// ---------------------------------------------------------------------------
// (d) Day-cycle deficit refill
// ---------------------------------------------------------------------------

void UAstrawildWildlifeDirectorSubsystem::HandleDayChanged(const int32 NewDayNumber, const bool bWasAutomatic)
{
    HandleDayRefill(NewDayNumber);
}

void UAstrawildWildlifeDirectorSubsystem::HandleDayRefill(const int32 DayNumber)
{
    UWorld* World = GetWorld();
    UAstrawildEcosystemSubsystem* Ecosystem = World ? World->GetSubsystem<UAstrawildEcosystemSubsystem>() : nullptr;
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || World->GetNetMode() == NM_Client || !Ecosystem || !Registry)
    {
        return;
    }
    if (!bBootPassDone)
    {
        return; // no plan yet — the boot pass builds the world's baseline.
    }

    // Honest counting (documented choice): EcosystemSubsystem::GetWildPopulation
    // is the LIVE wild count per species — capture-aware, defeat-aware and
    // idempotent by actor key — so the deficit never over-spawns past a
    // species' world target even when individuals wander across zone borders.
    // Zone discipline is enforced on the PLACEMENT side (deficits place in the
    // row's home zone) plus the daily per-zone cap below.
    DailyZoneSpawnCount.Reset();

    int32 Spawned = 0;
    const auto RefillRow = [&](const FName SpeciesId, const EAstrawildZone Zone, const int32 TargetCount)
    {
        UAstrawildEchoDefinition* Definition = Registry->FindEcho(SpeciesId);
        const FAstrawildZoneDescriptor* ZoneDesc = Definition ? UAstrawildZoneSubsystem::FindZone(Zone) : nullptr;
        if (!Definition || !ZoneDesc)
        {
            return;
        }
        const int32 Deficit = TargetCount - Ecosystem->GetWildPopulation(SpeciesId);
        if (Deficit <= 0)
        {
            return;
        }
        int32& DailyUsed = DailyZoneSpawnCount.FindOrAdd(ZoneDesc->ZoneId);
        FRandomStream Stream(CachedWorldSeed ^ static_cast<uint32>(DayNumber) ^ static_cast<uint32>(GetTypeHash(SpeciesId)));
        for (int32 i = 0; i < Deficit; ++i)
        {
            if (DailyUsed >= PerZoneSpawnCap)
            {
                break; // daily discipline: at most the cap NEW per zone per day
            }
            if (SpawnWildEchoInZone(SpeciesId, *ZoneDesc, Stream))
            {
                ++DailyUsed;
                ++Spawned;
            }
        }
    };

    for (const FAstrawildDirectorSpawnRow& Row : SpawnPlan)
    {
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZoneById(Row.ZoneId);
        if (ZoneDesc)
        {
            RefillRow(Row.SpeciesId, ZoneDesc->Zone, Row.Count);
        }
    }
    for (const FAstrawildZoneWildlifeRow& Row : LegacyRows)
    {
        RefillRow(Row.SpeciesId, Row.Zone, Row.Count);
    }

    if (Spawned > 0)
    {
        UE_LOG(LogAstrawildWorld, Log,
            TEXT("Wildlife director: day %d refill spawned %d deficit individual(s)."),
            DayNumber, Spawned);
    }
}

// ---------------------------------------------------------------------------
// Shared spawn helper (the SpawnWildEchoes construction path)
// ---------------------------------------------------------------------------

AAstrawildEchoCharacter* UAstrawildWildlifeDirectorSubsystem::SpawnWildEchoInZone(
    const FName SpeciesId, const FAstrawildZoneDescriptor& Zone, FRandomStream& Stream)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildEchoDefinition* Definition = Registry ? Registry->FindEcho(SpeciesId) : nullptr;
    if (!World || !Definition)
    {
        return nullptr;
    }

    const FVector Location = RandomGroundPointInZone(Stream, Zone, CachedWorldSeed);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(
        AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (Echo)
    {
        // Same construction path as SpawnWildEchoes: definition init rolls the
        // personality + stats and re-registers with the ecosystem (the audit
        // C-9 idiom — population counting is idempotent by actor key).
        Echo->InitializeFromDefinition(Definition);
        UE_LOG(LogAstrawildWorld, Verbose, TEXT("Wildlife director: spawned %s at %s."),
            *SpeciesId.ToString(), *Location.ToCompactString());
    }
    return Echo;
}

FVector UAstrawildWildlifeDirectorSubsystem::RandomGroundPointInZone(
    FRandomStream& Stream, const FAstrawildZoneDescriptor& Zone, const int32 Seed)
{
    // The bootstrapper's RandomPointInZoneWith semantics, parameterized by the
    // caller's stream: 8 attempts inside the 40m-inset rect, at least 30m from
    // the zone center, terrain-snapped via the pure height field.
    const FVector2D Center = Zone.GetCenter();
    FVector2D Point(Center.X + 3000.0f, Center.Y);
    for (int32 Attempt = 0; Attempt < 8; ++Attempt)
    {
        const FVector2D Sample(
            Stream.FRandRange(Zone.Bounds.Min.X + 4000.0f, Zone.Bounds.Max.X - 4000.0f),
            Stream.FRandRange(Zone.Bounds.Min.Y + 4000.0f, Zone.Bounds.Max.Y - 4000.0f));
        if (FVector2D::Distance(Sample, Center) >= 3000.0f)
        {
            Point = Sample;
            break;
        }
    }
    return FVector(Point.X, Point.Y, AAstrawildTerrainTileActor::EvalWorldHeight(Point, Seed) + 150.0f);
}

AAstrawildPlayerCharacter* UAstrawildWildlifeDirectorSubsystem::FindLocalPlayer() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        return Cast<AAstrawildPlayerCharacter>(PC->GetPawn());
    }
    return nullptr;
}

void UAstrawildWildlifeDirectorSubsystem::BroadcastToast(const FText& Message) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
        {
            // NotifyPlayer routes to the owning screen (local -> HUD toast,
            // remote -> ClientNotify) — the WorldEventSubsystem wording path.
            AstrawildPC->NotifyPlayer(Message);
        }
    }
}
