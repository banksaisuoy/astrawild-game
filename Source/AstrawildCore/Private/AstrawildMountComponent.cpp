#include "AstrawildMountComponent.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

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
    // serpents, amorphous blobs and floating wisps do not.
    if (BodyPlan != EAstrawildBodyPlan::Quadruped && BodyPlan != EAstrawildBodyPlan::Avian)
    {
        return false;
    }

    // Family gate: the classic mount families. Flora Kindred and Spirits are
    // companion casters, Construct/Ancient are tech encounters.
    switch (Family)
    {
    case EAstrawildEchoFamily::Beast:
    case EAstrawildEchoFamily::Dragon:
    case EAstrawildEchoFamily::Avian:
    case EAstrawildEchoFamily::Insectoid:
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
    bFlyingMount = (Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Flying);
    if (UCharacterMovementComponent* EchoMovement = Echo->GetCharacterMovement())
    {
        if (bFlyingMount)
        {
            EchoMovement->SetMovementMode(MOVE_Flying);
        }
        EchoMovement->MaxWalkSpeed = ComputeMountSpeed(Echo->GetCachedStats().MoveSpeed);
        EchoMovement->MaxFlySpeed = ComputeMountSpeed(Echo->GetCachedStats().MoveSpeed);
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
        *Player->GetName(), *Echo->GetName(), bFlyingMount ? TEXT("flying") : TEXT("land"));

    return true;
}

void UAstrawildMountComponent::DismountRider()
{
    AAstrawildPlayerCharacter* Player = Rider;
    if (!IsValid(Player) || !IsAuthority())
    {
        return;
    }

    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    Rider = nullptr;
    RiderForwardAxis = RiderTurnAxis = RiderVerticalAxis = 0.0f;

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

    if (IsValid(Echo))
    {
        if (UCharacterMovementComponent* EchoMovement = Echo->GetCharacterMovement())
        {
            // Land mounts walk again; flying mounts glide back down.
            if (bFlyingMount)
            {
                EchoMovement->SetMovementMode(MOVE_Falling);
            }
            EchoMovement->MaxWalkSpeed = Echo->GetCachedStats().MoveSpeed;
        }
        // Return to follower duty right away.
        Echo->SetAIState(EAstrawildEchoAIState::Idle);
    }

    OnMountStateChanged.Broadcast(false, Player);
    UE_LOG(LogAstrawildAI, Log, TEXT("Mount: rider dismounted"));
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
