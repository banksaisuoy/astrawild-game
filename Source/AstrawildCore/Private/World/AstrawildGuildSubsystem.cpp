#include "World/AstrawildGuildSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UAstrawildGuildSubsystem::UAstrawildGuildSubsystem()
{
    ArenaTickIntervalSeconds = 0.1f;
    ArenaTeamSize = 4;
}

void UAstrawildGuildSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ArenaTimerHandle, this, &UAstrawildGuildSubsystem::HandleArenaTick, FMath::Max(0.01f, ArenaTickIntervalSeconds), true, FMath::Max(0.01f, ArenaTickIntervalSeconds));
    }
}

void UAstrawildGuildSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ArenaTimerHandle);
    }
    RegisteredGuilds.Reset();
    BuffNodes.Reset();
    GuildBuffLevels.Reset();
    Territories.Reset();
    ArenaTeams.Reset();
    ArenaMatchState = FAstrawildGuildArenaMatchState();
    ActiveArenaTag = FGameplayTag();
    Super::Deinitialize();
}

bool UAstrawildGuildSubsystem::HasAuthorityForGuild() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

bool UAstrawildGuildSubsystem::RegisterGuild(const FGameplayTag& GuildTag)
{
    return HasAuthorityForGuild() && GuildTag.IsValid() && RegisteredGuilds.Add(GuildTag).IsValidId();
}

bool UAstrawildGuildSubsystem::RegisterBuffNode(const FAstrawildGuildBuffNode& BuffNode)
{
    if (!HasAuthorityForGuild() || !BuffNode.BuffTag.IsValid())
    {
        return false;
    }
    FAstrawildGuildBuffNode Sanitized = BuffNode;
    Sanitized.MaxLevel = FMath::Max(1, Sanitized.MaxLevel);
    BuffNodes.Add(Sanitized.BuffTag, MoveTemp(Sanitized));
    return true;
}

bool UAstrawildGuildSubsystem::SetGuildBuffLevel(const FGameplayTag& GuildTag, const FGameplayTag& BuffTag, const int32 NewLevel)
{
    if (!HasAuthorityForGuild() || !RegisteredGuilds.Contains(GuildTag))
    {
        return false;
    }
    const FAstrawildGuildBuffNode* Node = BuffNodes.Find(BuffTag);
    if (!Node || NewLevel < 0 || NewLevel > Node->MaxLevel)
    {
        return false;
    }
    if (Node->RequiredBuffTag.IsValid() && NewLevel > 0 && GetGuildBuffLevel(GuildTag, Node->RequiredBuffTag) <= 0)
    {
        return false;
    }
    GuildBuffLevels.FindOrAdd(GuildTag).Add(BuffTag, NewLevel);
    OnGuildBuffChanged.Broadcast(GuildTag, BuffTag, NewLevel);
    return true;
}

int32 UAstrawildGuildSubsystem::GetGuildBuffLevel(const FGameplayTag& GuildTag, const FGameplayTag& BuffTag) const
{
    const TMap<FGameplayTag, int32>* Levels = GuildBuffLevels.Find(GuildTag);
    const int32* Level = Levels ? Levels->Find(BuffTag) : nullptr;
    return Level ? *Level : 0;
}

bool UAstrawildGuildSubsystem::RegisterTerritoryTotem(const FAstrawildGuildTerritory& Territory)
{
    if (!HasAuthorityForGuild() || !Territory.GuildTag.IsValid() || !Territory.TotemTag.IsValid() || !RegisteredGuilds.Contains(Territory.GuildTag))
    {
        return false;
    }
    FAstrawildGuildTerritory Sanitized = Territory;
    Sanitized.Radius = FMath::Max(1.0f, Sanitized.Radius);
    Territories.Add(Sanitized.TotemTag, MoveTemp(Sanitized));
    return true;
}

bool UAstrawildGuildSubsystem::CaptureTerritory(const FGameplayTag& GuildTag, const FGameplayTag& TotemTag, AActor* Instigator)
{
    if (!HasAuthorityForGuild() || !Instigator || !RegisteredGuilds.Contains(GuildTag))
    {
        return false;
    }
    FAstrawildGuildTerritory* Territory = Territories.Find(TotemTag);
    if (!Territory || FVector::DistSquared(Instigator->GetActorLocation(), Territory->WorldLocation) > FMath::Square(Territory->Radius))
    {
        return false;
    }
    Territory->GuildTag = GuildTag;
    OnTerritoryCaptured.Broadcast(GuildTag, TotemTag);
    return true;
}

bool UAstrawildGuildSubsystem::GetTerritory(const FGameplayTag& TotemTag, FAstrawildGuildTerritory& OutTerritory) const
{
    OutTerritory = FAstrawildGuildTerritory();
    const FAstrawildGuildTerritory* Territory = Territories.Find(TotemTag);
    if (!Territory)
    {
        return false;
    }
    OutTerritory = *Territory;
    return true;
}

bool UAstrawildGuildSubsystem::RegisterArenaTeam(const FGameplayTag& TeamTag, const TArray<AActor*>& Members)
{
    if (!HasAuthorityForGuild() || ArenaMatchState.bMatchActive || !TeamTag.IsValid() || Members.Num() != ArenaTeamSize || ArenaTeams.Contains(TeamTag))
    {
        return false;
    }
    FAstrawildGuildArenaTeam Team;
    Team.TeamTag = TeamTag;
    for (AActor* Member : Members)
    {
        if (!IsValid(Member) || Team.Members.Contains(Member))
        {
            return false;
        }
        Team.Members.Add(Member);
    }
    ArenaTeams.Add(TeamTag, MoveTemp(Team));
    return true;
}

bool UAstrawildGuildSubsystem::StartArenaMatch(const FGameplayTag& ArenaTag)
{
    if (!HasAuthorityForGuild() || ArenaMatchState.bMatchActive || !ArenaTag.IsValid() || ArenaTeams.Num() != 2)
    {
        return false;
    }
    for (const TPair<FGameplayTag, FAstrawildGuildArenaTeam>& Pair : ArenaTeams)
    {
        if (Pair.Value.Members.Num() != ArenaTeamSize)
        {
            return false;
        }
    }
    ActiveArenaTag = ArenaTag;
    ArenaMatchState = FAstrawildGuildArenaMatchState();
    ArenaMatchState.bMatchActive = true;
    OnArenaMatchStarted.Broadcast(ActiveArenaTag);
    return true;
}

bool UAstrawildGuildSubsystem::AddArenaScore(const FGameplayTag& TeamTag, const int32 ScoreDelta)
{
    if (!HasAuthorityForGuild() || !ArenaMatchState.bMatchActive || ScoreDelta <= 0)
    {
        return false;
    }
    FAstrawildGuildArenaTeam* Team = ArenaTeams.Find(TeamTag);
    if (!Team || Team->bEliminated)
    {
        return false;
    }
    Team->Score = FMath::Clamp(Team->Score + ScoreDelta, 0, 999999);
    return true;
}

void UAstrawildGuildSubsystem::EndArenaMatch(const int32 WinningTeamIndex)
{
    if (!HasAuthorityForGuild() || !ArenaMatchState.bMatchActive)
    {
        return;
    }
    ArenaMatchState.bMatchActive = false;
    ArenaMatchState.WinningTeamIndex = WinningTeamIndex;
    OnArenaMatchFinished.Broadcast(ActiveArenaTag, WinningTeamIndex);
}

bool UAstrawildGuildSubsystem::GetArenaTeamState(const FGameplayTag& TeamTag, FAstrawildGuildArenaTeam& OutTeam) const
{
    OutTeam = FAstrawildGuildArenaTeam();
    const FAstrawildGuildArenaTeam* Team = ArenaTeams.Find(TeamTag);
    if (!Team)
    {
        return false;
    }
    OutTeam = *Team;
    return true;
}

void UAstrawildGuildSubsystem::HandleArenaTick()
{
    if (HasAuthorityForGuild() && ArenaMatchState.bMatchActive)
    {
        ArenaMatchState.MatchElapsedSeconds += FMath::Max(0.01f, ArenaTickIntervalSeconds);
    }
    RemoveInvalidArenaMembers();
}

void UAstrawildGuildSubsystem::RemoveInvalidArenaMembers()
{
    for (TPair<FGameplayTag, FAstrawildGuildArenaTeam>& Pair : ArenaTeams)
    {
        for (int32 Index = Pair.Value.Members.Num() - 1; Index >= 0; --Index)
        {
            if (!Pair.Value.Members[Index].IsValid())
            {
                Pair.Value.Members.RemoveAt(Index);
            }
        }
    }
}
