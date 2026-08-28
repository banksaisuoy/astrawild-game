#include "Environment/AstrawildLaunchPad.h"

#include "Engine/World.h"
#include "World/AstrawildSpaceFlightSubsystem.h"

AAstrawildLaunchPad::AAstrawildLaunchPad()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);
}

void AAstrawildLaunchPad::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (UAstrawildSpaceFlightSubsystem* Flight = GetWorld()->GetSubsystem<UAstrawildSpaceFlightSubsystem>())
        {
            FAstrawildLaunchPadDefinition Definition;
            Definition.PadTag = PadTag;
            Definition.DestinationBiomeTag = DestinationBiomeTag;
            Definition.WorldLocation = GetActorLocation();
            Definition.InteractionRadius = InteractionRadius;
            Definition.LaunchDurationSeconds = LaunchDurationSeconds;
            Flight->RegisterLaunchPad(Definition);
        }
    }
}

void AAstrawildLaunchPad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority() && GetWorld())
    {
        if (UAstrawildSpaceFlightSubsystem* Flight = GetWorld()->GetSubsystem<UAstrawildSpaceFlightSubsystem>())
        {
            Flight->UnregisterLaunchPad(PadTag);
        }
    }
    Super::EndPlay(EndPlayReason);
}

bool AAstrawildLaunchPad::RequestLaunch(AActor* Pilot)
{
    if (!HasAuthority() || !Pilot || !GetWorld())
    {
        return false;
    }
    if (UAstrawildSpaceFlightSubsystem* Flight = GetWorld()->GetSubsystem<UAstrawildSpaceFlightSubsystem>())
    {
        return Flight->RequestLaunch(Pilot, PadTag);
    }
    return false;
}
