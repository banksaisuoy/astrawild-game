#include "Animation/AstrawildAnimInstance.h"

#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildAttributeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAstrawildAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CachedPawn = TryGetPawnOwner();
    CachedPlayer = Cast<AAstrawildCharacter>(CachedPawn.Get());
    CachedEcho = Cast<AAstrawildEchoBase>(CachedPawn.Get());
}

void UAstrawildAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!CachedPawn.IsValid())
    {
        CachedPawn = TryGetPawnOwner();
        CachedPlayer = Cast<AAstrawildCharacter>(CachedPawn.Get());
        CachedEcho = Cast<AAstrawildEchoBase>(CachedPawn.Get());
    }

    APawn* Pawn = CachedPawn.Get();
    if (!IsValid(Pawn))
    {
        return;
    }

    const FVector Velocity = Pawn->GetVelocity();
    GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
    Direction = CalculateDirection(Velocity, Pawn->GetActorRotation());

    if (const ACharacter* Character = Cast<ACharacter>(Pawn))
    {
        if (const UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            bIsInAir = Movement->IsFalling();
        }
    }

    if (CachedPlayer.IsValid())
    {
        const AAstrawildCharacter* Player = CachedPlayer.Get();
        bIsSprinting = Player->bIsSprinting;
        bIsDodging = Player->bIsDodging;
        PlayerMovementState = Player->CurrentMovementState;

        if (Player->Attributes && Player->Attributes->MaxHealth > 0.0f)
        {
            HealthNormalized = FMath::Clamp(Player->Attributes->CurrentHealth / Player->Attributes->MaxHealth, 0.0f, 1.0f);
            ElementalAffinity = Player->Attributes->ElementalAffinity;
        }
    }

    if (CachedEcho.IsValid())
    {
        const AAstrawildEchoBase* Echo = CachedEcho.Get();
        EchoState = Echo->CurrentState;
        if (Echo->Attributes && Echo->Attributes->MaxHealth > 0.0f)
        {
            HealthNormalized = FMath::Clamp(Echo->Attributes->CurrentHealth / Echo->Attributes->MaxHealth, 0.0f, 1.0f);
            ElementalAffinity = Echo->Attributes->ElementalAffinity;
        }
    }
}
