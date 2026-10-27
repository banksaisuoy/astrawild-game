#include "AstrawildEchoDexSubsystem.h"

#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "Engine/World.h"

void UAstrawildEchoDexSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    // EDX-1: log the index size once at boot so the canon coverage
    // (species count) is visible in the shipped log tail.
    UE_LOG(LogAstrawild, Log, TEXT("EchoDex online: %d species indexed."),
        GetTotalSpeciesCount());
}

UAstrawildItemRegistrySubsystem* UAstrawildEchoDexSubsystem::GetRegistry() const
{
    return GetWorld() ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

bool UAstrawildEchoDexSubsystem::MatchesQuery(const UAstrawildEchoDefinition* Def,
    const FAstrawildDexQuery& Query, EAstrawildDexDiscoveryState State)
{
    if (!Def || Def->DefinitionId.IsNone())
    {
        return false;
    }

    // Facets — "None"/inactive means the facet is not constraining.
    if (Query.Element != EAstrawildElementType::None && Def->Element != Query.Element)
    {
        return false;
    }
    if (Query.Zone != EAstrawildZone::None && Def->HomeZone != Query.Zone)
    {
        return false;
    }
    if (Query.bFilterRole && Def->Role != Query.Role)
    {
        return false;
    }
    if (Query.bFilterDiscovery && State != Query.Discovery)
    {
        return false;
    }

    // Free text — case-insensitive substring over name AND id. This is the
    // exact capability Palworld ships without (players carry wiki IDs); the
    // community "Pal List Searchbar" mod proves the demand.
    if (!Query.Text.TrimStartAndEnd().IsEmpty())
    {
        const FString Needle = Query.Text.TrimStartAndEnd().ToLower();
        const FString HaystackName = Def->DisplayName.ToString().ToLower();
        const FString HaystackId = Def->DefinitionId.ToString().ToLower();
        if (!HaystackName.Contains(Needle) && !HaystackId.Contains(Needle))
        {
            return false;
        }
    }
    return true;
}

TArray<UAstrawildEchoDefinition*> UAstrawildEchoDexSubsystem::Search(const FAstrawildDexQuery& Query) const
{
    TArray<UAstrawildEchoDefinition*> Results;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
        {
            if (Def && MatchesQuery(Def, Query, GetDiscoveryState(Def->DefinitionId)))
            {
                Results.Add(Def);
            }
        }
    }
    // Stable index order — "sort by No." like the Aniimo index / Paldeck ids.
    Results.Sort([](const UAstrawildEchoDefinition& A, const UAstrawildEchoDefinition& B)
    {
        return A.DefinitionId.ToString() < B.DefinitionId.ToString();
    });
    return Results;
}

int32 UAstrawildEchoDexSubsystem::GetTotalSpeciesCount() const
{
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        return Registry->GetAllEchoDefinitions().Num();
    }
    return 0;
}

UAstrawildEchoDefinition* UAstrawildEchoDexSubsystem::FindSpecies(FName SpeciesId) const
{
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        return Registry->FindEcho(SpeciesId);
    }
    return nullptr;
}

EAstrawildDexDiscoveryState UAstrawildEchoDexSubsystem::GetDiscoveryState(FName SpeciesId) const
{
    if (SpeciesId.IsNone())
    {
        return EAstrawildDexDiscoveryState::Unknown;
    }
    if (CapturedSpecies.Contains(SpeciesId))
    {
        return EAstrawildDexDiscoveryState::Captured;
    }
    // "Seen" derives from the journal — an entry with a recorded encounter or
    // any granted knowledge flag means the player has met the species. No
    // second perception bookkeeping to drift out of sync.
    if (const UAstrawildJournalSubsystem* Journal =
            GetWorld() ? GetWorld()->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr)
    {
        const FAstrawildJournalEntry Entry = Journal->GetEntry(SpeciesId);
        if (Entry.TimesEncountered > 0 || Entry.bScanned || Entry.ObservationProgress > 0.0f)
        {
            return EAstrawildDexDiscoveryState::Seen;
        }
    }
    return EAstrawildDexDiscoveryState::Unknown;
}

int32 UAstrawildEchoDexSubsystem::GetSeenCount() const
{
    int32 Seen = 0;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (const UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
        {
            if (Def && GetDiscoveryState(Def->DefinitionId) != EAstrawildDexDiscoveryState::Unknown)
            {
                ++Seen;
            }
        }
    }
    return Seen;
}

int32 UAstrawildEchoDexSubsystem::GetCapturedCount() const
{
    return CapturedSpecies.Num();
}

bool UAstrawildEchoDexSubsystem::NotifySpeciesCaptured(FName SpeciesId)
{
    if (SpeciesId.IsNone() || CapturedSpecies.Contains(SpeciesId))
    {
        return false; // Idempotent — repeat captures do not re-fire events.
    }
    CapturedSpecies.Add(SpeciesId);
    OnDexUpdated.Broadcast(SpeciesId);
    UE_LOG(LogAstrawild, Log, TEXT("EchoDex: species captured (%s)."), *SpeciesId.ToString());
    return true;
}

float UAstrawildEchoDexSubsystem::GetZoneProgress(EAstrawildZone Zone) const
{
    int32 Total = 0;
    int32 Discovered = 0;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (const UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
        {
            if (!Def || Def->HomeZone != Zone)
            {
                continue;
            }
            ++Total;
            if (GetDiscoveryState(Def->DefinitionId) != EAstrawildDexDiscoveryState::Unknown)
            {
                ++Discovered;
            }
        }
    }
    return Total > 0 ? static_cast<float>(Discovered) / static_cast<float>(Total) : 0.0f;
}

bool UAstrawildEchoDexSubsystem::ShouldRadarAlert(FName SpeciesId) const
{
    const UAstrawildEchoDefinition* Def = nullptr;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        Def = Registry->FindEcho(SpeciesId);
    }
    if (!Def)
    {
        return false;
    }
    // Aniimo rule: radar covers rare species only...
    if (Def->CaptureDifficulty < RareDifficultyThreshold)
    {
        return false;
    }
    // ...that the player has NOT already met...
    if (GetDiscoveryState(SpeciesId) != EAstrawildDexDiscoveryState::Unknown)
    {
        return false;
    }
    // ...in a zone whose collection progress unlocked the radar (~80%).
    return GetZoneProgress(Def->HomeZone) >= RadarUnlockProgress;
}

void UAstrawildEchoDexSubsystem::BroadcastRadarAlert(FName SpeciesId)
{
    if (SpeciesId.IsNone())
    {
        return;
    }
    OnRadarAlert.Broadcast(SpeciesId);
    UE_LOG(LogAstrawild, Log, TEXT("EchoDex radar ping: rare species nearby (%s)."),
        *SpeciesId.ToString());
}
