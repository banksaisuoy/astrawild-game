#include "AstrawildTurretComponent.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildProjectileActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UAstrawildTurretComponent::UAstrawildTurretComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.25f;
}

bool UAstrawildTurretComponent::IsAuthority() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    return Owner->HasAuthority();
}

bool UAstrawildTurretComponent::IsInRange(const FVector& A, const FVector& B, float Range)
{
    const float SafeRange = FMath::Max(100.0f, Range);
    return FVector::DistSquared(A, B) <= SafeRange * SafeRange;
}

AAstrawildEchoCharacter* UAstrawildTurretComponent::SelectTarget(const TArray<AAstrawildEchoCharacter*>& Candidates,
    const FVector& TurretLocation)
{
    AAstrawildEchoCharacter* Best = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (AAstrawildEchoCharacter* Candidate : Candidates)
    {
        if (!IsValid(Candidate) || Candidate->IsDefeated() || Candidate->bCaptured)
        {
            continue; // turret doctrine: hostile, wild echoes only — party-safe.
        }
        if (!IsInRange(TurretLocation, Candidate->GetActorLocation(), TurretRange))
        {
            continue;
        }
        const float DistanceSquared = FVector::DistSquared(TurretLocation, Candidate->GetActorLocation());
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Best = Candidate;
        }
    }

    return Best;
}

void UAstrawildTurretComponent::FireAt(AAstrawildEchoCharacter* Target)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !IsValid(Owner) || !IsValid(Target))
    {
        return;
    }

    // Spawn the standard bolt from the turret crown toward the target and
    // launch it through the shared projectile pipeline (element None = kinetic).
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    const FVector MuzzleLocation = Owner->GetActorLocation() + FVector(0.0f, 0.0f, 140.0f);
    AAstrawildProjectileActor* Bolt = World->SpawnActor<AAstrawildProjectileActor>(
        AAstrawildProjectileActor::StaticClass(), MuzzleLocation, FRotator::ZeroRotator, Params);
    if (!Bolt)
    {
        return;
    }

    const FVector Direction = (Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f) - MuzzleLocation).GetSafeNormal();
    Bolt->Launch(Direction, BoltDamage, EAstrawildElementType::None, Owner);

    UE_LOG(LogAstrawildBuilding, Verbose, TEXT("Turret %s fired at %s"),
        *Owner->GetName(), *Target->GetName());
}

void UAstrawildTurretComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsAuthority())
    {
        return;
    }

    FireCooldownRemaining = FMath::Max(0.0f, FireCooldownRemaining - DeltaTime);
    if (FireCooldownRemaining > 0.0f)
    {
        return;
    }

    // Power gate: unpowered turrets never fire (grid defense doctrine).
    const AAstrawildBuildingActor* Building = Cast<AAstrawildBuildingActor>(GetOwner());
    if (IsValid(Building) && !Building->bIsPowered)
    {
        return;
    }

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !IsValid(Owner))
    {
        return;
    }

    // Gather hostiles in range (TActorIterator filtered by hostility).
    TArray<AAstrawildEchoCharacter*> Candidates;
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (IsValid(Echo) && !Echo->bCaptured && !Echo->IsDefeated() &&
            IsValid(Echo->EchoDefinition) && Echo->EchoDefinition->bHostileToPlayers)
        {
            if (IsInRange(Owner->GetActorLocation(), Echo->GetActorLocation(), TurretRange))
            {
                Candidates.Add(Echo);
            }
        }
    }

    AAstrawildEchoCharacter* Target = SelectTarget(Candidates, Owner->GetActorLocation());
    if (Target)
    {
        FireAt(Target);
        FireCooldownRemaining = FireIntervalSeconds;
    }
}
