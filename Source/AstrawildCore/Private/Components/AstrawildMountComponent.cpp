#include "Components/AstrawildMountComponent.h"

#include "Echoes/AstrawildEchoBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"

UAstrawildMountComponent::UAstrawildMountComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildMountComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!ActiveMount.IsValid())
    {
        ActiveSpeedMultiplier = 1.0f;
    }
}

bool UAstrawildMountComponent::CanMount(AAstrawildEchoBase* Echo, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();
    if (!Echo)
    {
        OutFailureReason = FText::FromString(TEXT("No Echo was selected."));
        return false;
    }

    if (ActiveMount.IsValid())
    {
        OutFailureReason = FText::FromString(TEXT("A mount is already active."));
        return false;
    }

    if (!Echo->SpeciesData || !Echo->SpeciesData->bCanBeMounted)
    {
        OutFailureReason = FText::FromString(TEXT("This Echo cannot be mounted."));
        return false;
    }

    if (Echo->CurrentState != EAstrawildEchoState::SummonedCompanion)
    {
        OutFailureReason = FText::FromString(TEXT("The Echo must be summoned before mounting."));
        return false;
    }

    const FName MountProfileId = Echo->InstanceData.MountProfileId.IsNone()
        ? Echo->SpeciesData->MountProfileId
        : Echo->InstanceData.MountProfileId;
    if (MountProfileId.IsNone() || !MountProfiles.Contains(MountProfileId))
    {
        OutFailureReason = FText::FromString(TEXT("This Echo has no configured mount profile."));
        return false;
    }

    return true;
}

bool UAstrawildMountComponent::TryMount(AAstrawildEchoBase* Echo)
{
    FText FailureReason;
    if (!CanMount(Echo, FailureReason))
    {
        OnMountFailed.Broadcast(FailureReason);
        return false;
    }

    const FName MountProfileId = Echo->InstanceData.MountProfileId.IsNone()
        ? Echo->SpeciesData->MountProfileId
        : Echo->InstanceData.MountProfileId;
    const FAstrawildMountProfile* Profile = MountProfiles.Find(MountProfileId);
    if (!Profile || !GetOwner())
    {
        OnMountFailed.Broadcast(FText::FromString(TEXT("Mount profile lookup failed.")));
        return false;
    }

    ActiveRider = GetOwner();
    ActiveMount = Echo;
    Echo->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
    Echo->SetActorEnableCollision(false);
    if (Echo->GetCharacterMovement())
    {
        Echo->GetCharacterMovement()->DisableMovement();
    }
    ApplyProfile(*Profile);
    OnMounted.Broadcast(Echo);
    return true;
}

void UAstrawildMountComponent::Dismount()
{
    AAstrawildEchoBase* Echo = ActiveMount.Get();
    if (Echo)
    {
        Echo->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Echo->SetActorEnableCollision(true);
        if (Echo->GetCharacterMovement())
        {
            Echo->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
    }

    ActiveMount = nullptr;
    ActiveRider = nullptr;
    ActiveSpeedMultiplier = 1.0f;
    OnDismounted.Broadcast();
}

void UAstrawildMountComponent::ApplyProfile(const FAstrawildMountProfile& Profile)
{
    ActiveSpeedMultiplier = FMath::Clamp(Profile.SpeedMultiplier, 0.1f, 4.0f);
}
