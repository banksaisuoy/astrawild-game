#include "AstrawildMountComponent.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildZoneSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

namespace
{
    /** DP-3: water mounts swim exactly where the sea zones are (pure zone query). */
    bool MountInSeaZone(const AAstrawildEchoCharacter* Echo)
    {
        return IsValid(Echo) && UAstrawildZoneSubsystem::IsSeaZone(
            UAstrawildZoneSubsystem::GetZoneAt(Echo->GetActorLocation()));
    }
}

UAstrawildMountComponent::UAstrawildMountComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
    SetIsReplicatedByDefault(true);
}

void UAstrawildMountComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildMountComponent, Rider);
}

bool UAstrawildMountComponent::IsAuthority() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    return Owner->GetLocalRole() == ROLE_Authority;
}

bool UAstrawildMountComponent::IsRideableSpecies(EAstrawildEchoFamily Family, EAstrawildBodyPlan BodyPlan, EAstrawildSizeClass SizeClass)
{
    // Size gate: nobody rides a Tiny/Small creature (they ride YOU, in spirit).
    if (SizeClass == EAstrawildSizeClass::Tiny || SizeClass == EAstrawildSizeClass::Small)
    {
        return false;
    }

    // Body plan gate: quadrupeds and true avians carry a saddle; bipeds,
    // amorphous blobs and floating wisps do not. DP-3 exception: aquatic
    // serpents — dolphin-wyrms and sea-striders carry a rider through the
    // shallows the way their quadruped kin carry one on land.
    const bool bAquaticSerpent = (Family == EAstrawildEchoFamily::Aquatic && BodyPlan == EAstrawildBodyPlan::Serpent);
    if (BodyPlan != EAstrawildBodyPlan::Quadruped && BodyPlan != EAstrawildBodyPlan::Avian && !bAquaticSerpent)
    {
        return false;
    }

    // Family gate: the classic mount families plus the DP-3 sea-riders. Flora
    // Kindred and Spirits are companion casters, Construct/Ancient are tech
    // encounters.
    switch (Family)
    {
    case EAstrawildEchoFamily::Beast:
    case EAstrawildEchoFamily::Dragon:
    case EAstrawildEchoFamily::Avian:
    case EAstrawildEchoFamily::Insectoid:
    case EAstrawildEchoFamily::Aquatic:
        return true;
    default:
        return false;
    }
}

float UAstrawildMountComponent::ComputeMountSpeed(float SpeciesMoveSpeed)
{
    // Riding beats walking: a mount carries its rider at 1.25x species speed.
    return FMath::Max(200.0f, SpeciesMoveSpeed) * MountSpeedMultiplier;
}

FVector UAstrawildMountComponent::ComputeRiderSeatOffset(EAstrawildSizeClass SizeClass)
{
    // Procedural-body seat contract (replaced by the skeletal socket set once
    // rigs ship — the socket NAMES are already pinned in the header).
    switch (SizeClass)
    {
    case EAstrawildSizeClass::Large:
        return FVector(-10.0f, 0.0f, 150.0f);
    case EAstrawildSizeClass::Huge:
        return FVector(-20.0f, 0.0f, 220.0f);
    default: // Medium
        return FVector(-5.0f, 0.0f, 110.0f);
    }
}

bool UAstrawildMountComponent::CanBeMountedBy(const AAstrawildPlayerCharacter* Player) const
{
    const AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    if (!IsValid(Echo) || !IsValid(Player) || !IsValid(Echo->EchoDefinition))
    {
        return false;
    }

    if (Echo->IsDefeated() || !Echo->bCaptured)
    {
        return false;
    }

    // Ownership: you ride your own creatures (co-op safety).
    if (Echo->OwnerPlayerId != Player->GetFName())
    {
        return false;
    }

    // Trust gate: a nervous creature shrugs off the saddle until bonded.
    if (Echo->Bond < MountBondGate)
    {
        return false;
    }

    return IsRideableSpecies(Echo->EchoDefinition->Family, Echo->EchoDefinition->BodyPlan, Echo->EchoDefinition->SizeClass);
}

bool UAstrawildMountComponent::MountRider(AAstrawildPlayerCharacter* Player)
{
    if (!IsAuthority() || IsMounted() || !CanBeMountedBy(Player))
    {
        return false;
    }

    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    if (!IsValid(Echo))
    {
        return false;
    }

    Rider = Player;
    RiderForwardAxis = RiderTurnAxis = RiderVerticalAxis = 0.0f;

    // Flying mounts (avian family or derived flying locomotion) enter MOVE_Flying.
    // DP-3: water-locomotion species enter MOVE_Swimming when mounted in a sea
    // zone (the transition logic keeps shore riding on legs).
    bFlyingMount = (Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Flying);
    bWaterMount = (Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Water);
    if (UCharacterMovementComponent* EchoMovement = Echo->GetCharacterMovement())
    {
        const float MountSpeed = ComputeMountSpeed(Echo->GetCachedStats().MoveSpeed);
        EchoMovement->MaxWalkSpeed = MountSpeed;
        EchoMovement->MaxFlySpeed = MountSpeed;
        EchoMovement->MaxSwimSpeed = MountSpeed;
        if (bFlyingMount)
        {
            EchoMovement->SetMovementMode(MOVE_Flying);
        }
        else if (bWaterMount && MountInSeaZone(Echo))
        {
            EchoMovement->SetMovementMode(MOVE_Swimming);
        }
    }

    // Seat the rider (skiff pilot pattern): attach above the back, capsule
    // collision off, pawn movement disabled — the MOUNT drives.
    const FVector SeatOffset = ComputeRiderSeatOffset(Echo->EchoDefinition->SizeClass);
    Player->AttachToComponent(Echo->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, NAME_None);
    Player->SetActorRelativeLocation(SeatOffset);
    if (UCharacterMovementComponent* RiderMovement = Player->GetCharacterMovement())
    {
        RiderMovement->DisableMovement();
        RiderMovement->StopMovementImmediately();
    }
    Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Player->SetMountedEcho(Echo);

    // Park the AI: a ridden creature obeys the reins, not its instincts.
    if (AAIController* EchoController = Cast<AAIController>(Echo->GetController()))
    {
        EchoController->StopMovement();
    }

    OnMountStateChanged.Broadcast(true, Player);
    UE_LOG(LogAstrawildAI, Log, TEXT("Mount: %s boarded %s (%s)"),
        *Player->GetName(), *Echo->GetName(),
        bFlyingMount ? TEXT("flying") : (bWaterMount ? TEXT("sea-riding") : TEXT("land")));

    return true;
}

void UAstrawildMountComponent::DismountRider()
{
    // FCR-1-c fix (H-c2): the old early-return on an invalid Rider left the mount
    // PERMANENTLY stuck (Rider never cleared, echo never restored, IsMounted()
    // blocked every future MountRider). Cleanup must complete even when the rider
    // actor is gone — only the player-side restore is skipped.
    if (!IsAuthority())
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = Rider;
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    Rider = nullptr;
    RiderForwardAxis = RiderTurnAxis = RiderVerticalAxis = 0.0f;

    if (IsValid(Player))
    {
        // Dismount beside the creature, back on foot.
        const FVector ExitLocation = IsValid(Echo)
            ? Echo->GetActorLocation() + Echo->GetActorRightVector() * 220.0f + FVector(0.0f, 0.0f, 80.0f)
            : Player->GetActorLocation();
        Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Player->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        if (UCharacterMovementComponent* RiderMovement = Player->GetCharacterMovement())
        {
            RiderMovement->SetMovementMode(MOVE_Walking);
        }
        Player->SetMountedEcho(nullptr);
    }

    if (IsValid(Echo))
    {
        if (UCharacterMovementComponent* EchoMovement = Echo->GetCharacterMovement())
        {
            // FCR-1-a fix (M-a12): a Flying-class species returns to true flight on
            // dismount (the old MOVE_Falling drop grounded the flyer forever — nothing
            // ever restored MOVE_Flying). Land mounts walk again; DP-3: a swimming
            // water mount returns to shore legs when the rider leaves.
            if (bFlyingMount || Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Flying)
            {
                EchoMovement->SetMovementMode(MOVE_Flying);
                EchoMovement->MaxFlySpeed = FMath::Max(0.0f, Echo->GetCachedStats().MoveSpeed);
            }
            else if (bWaterMount && EchoMovement->IsSwimming())
            {
                EchoMovement->SetMovementMode(MOVE_Walking);
            }
            EchoMovement->MaxWalkSpeed = Echo->GetCachedStats().MoveSpeed;
            EchoMovement->MaxSwimSpeed = FMath::Max(200.0f, Echo->GetCachedStats().MoveSpeed);
        }
        // Return to follower duty right away.
        Echo->SetAIState(EAstrawildEchoAIState::Idle);
    }

    OnMountStateChanged.Broadcast(false, Player);
    UE_LOG(LogAstrawildAI, Log, TEXT("Mount: rider dismounted%s"),
        IsValid(Player) ? TEXT("") : TEXT(" (rider gone — cleanup only)"));
}

void UAstrawildMountComponent::ReceiveRiderMove(float ForwardAxis, float TurnAxis)
{
    RiderForwardAxis = FMath::Clamp(ForwardAxis, -1.0f, 1.0f);
    RiderTurnAxis = FMath::Clamp(TurnAxis, -1.0f, 1.0f);
}

void UAstrawildMountComponent::ReceiveRiderVertical(float VerticalAxis)
{
    RiderVerticalAxis = FMath::Clamp(VerticalAxis, -1.0f, 1.0f);
}

void UAstrawildMountComponent::DriveMountedMovement(float DeltaTime)
{
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    if (!IsValid(Echo) || !IsValid(Rider))
    {
        return;
    }

    // The mount faces the rider's camera yaw (reins steering) — the rider aims
    // freely while seated (mounted combat stays usable).
    if (const AController* RiderController = Rider->GetController())
    {
        const FRotator YawOnly(0.0f, RiderController->GetControlRotation().Yaw, 0.0f);
        Echo->SetActorRotation(FMath::RInterpTo(Echo->GetActorRotation(), YawOnly, DeltaTime, 8.0f));
    }

    UCharacterMovementComponent* EchoMovement = Echo->GetCharacterMovement();
    if (!EchoMovement)
    {
        return;
    }

    // Forward drives the mount; turning banks the body (strafing is unnatural
    // for creatures — the turn axis rotates via camera-facing above).
    const FVector ForwardDirection = Echo->GetActorForwardVector();
    EchoMovement->AddInputVector(ForwardDirection * RiderForwardAxis, false);

    if (bFlyingMount)
    {
        // SPACE/CTRL altitude on flying mounts (skiff parity).
        EchoMovement->AddInputVector(FVector::UpVector * RiderVerticalAxis, false);
    }
    else if (bWaterMount)
    {
        // DP-3: water mounts transition with the waterline — swimming through
        // the sea zones (SPACE surfaces / CTRL dives), walking the shore.
        const bool bInSea = MountInSeaZone(Echo);
        if (bInSea && EchoMovement->MovementMode != MOVE_Swimming)
        {
            EchoMovement->SetMovementMode(MOVE_Swimming);
        }
        else if (!bInSea && EchoMovement->MovementMode == MOVE_Swimming)
        {
            EchoMovement->SetMovementMode(MOVE_Walking);
        }
        if (EchoMovement->MovementMode == MOVE_Swimming)
        {
            EchoMovement->AddInputVector(FVector::UpVector * RiderVerticalAxis, false);
        }
    }

    // Keep the rider glued in case of external teleport/push.
    Rider->SetActorRelativeLocation(ComputeRiderSeatOffset(Echo->EchoDefinition ? Echo->EchoDefinition->SizeClass : EAstrawildSizeClass::Medium));
}

void UAstrawildMountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsAuthority() || !IsMounted())
    {
        return;
    }

    // Rider validity: a disconnected/destroyed rider auto-dismounts.
    if (!IsValid(Rider))
    {
        DismountRider();
        return;
    }

    // Ridden creature defeated → emergency dismount beside it.
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    if (IsValid(Echo) && Echo->IsDefeated())
    {
        DismountRider();
        return;
    }

    DriveMountedMovement(DeltaTime);
}
