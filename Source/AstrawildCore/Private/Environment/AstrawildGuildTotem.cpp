#include "Environment/AstrawildGuildTotem.h"

#include "Engine/World.h"
#include "World/AstrawildGuildSubsystem.h"

AAstrawildGuildTotem::AAstrawildGuildTotem()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);
}

void AAstrawildGuildTotem::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority() && GetWorld())
    {
        if (UAstrawildGuildSubsystem* Guilds = GetWorld()->GetSubsystem<UAstrawildGuildSubsystem>())
        {
            if (InitialGuildTag.IsValid())
            {
                Guilds->RegisterGuild(InitialGuildTag);
            }
            FAstrawildGuildTerritory Territory;
            Territory.GuildTag = InitialGuildTag;
            Territory.TotemTag = TotemTag;
            Territory.WorldLocation = GetActorLocation();
            Territory.Radius = TerritoryRadius;
            if (InitialGuildTag.IsValid())
            {
                Guilds->RegisterTerritoryTotem(Territory);
            }
        }
    }
}

void AAstrawildGuildTotem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

bool AAstrawildGuildTotem::CaptureForGuild(const FGameplayTag& GuildTag, AActor* InInstigator)
{
    if (!HasAuthority() || !GetWorld())
    {
        return false;
    }
    if (UAstrawildGuildSubsystem* Guilds = GetWorld()->GetSubsystem<UAstrawildGuildSubsystem>())
    {
        return Guilds->CaptureTerritory(GuildTag, TotemTag, InInstigator);
    }
    return false;
}
