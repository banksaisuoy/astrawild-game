#include "AstrawildNPCAIController.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildVillageActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

AAstrawildNPCAIController::AAstrawildNPCAIController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AAstrawildNPCAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    NpcPawn = Cast<AAstrawildNPCCharacter>(InPawn);
    if (!NpcPawn.IsValid())
    {
        return;
    }

    SpawnAnchor = InPawn->GetActorLocation();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ThinkTimerHandle, this, &AAstrawildNPCAIController::Think, ThinkIntervalSeconds, true);
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("NPC AI controller possessed %s."), *InPawn->GetName());
}

void AAstrawildNPCAIController::OnUnPossess()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);
    }
    Super::OnUnPossess();
}

void AAstrawildNPCAIController::Think()
{
    AAstrawildNPCCharacter* Npc = NpcPawn.Get();
    UWorld* World = GetWorld();
    if (!Npc || !World)
    {
        return;
    }

    // Conversation beat: a player just interacted — stand still and face them.
    if (World->GetTimeSeconds() - Npc->GetLastInteractedTime() < 5.0f)
    {
        StopMovement();
        if (AActor* Partner = Npc->GetLastInteractedActor())
        {
            FVector ToPartner = Partner->GetActorLocation() - Npc->GetActorLocation();
            ToPartner.Z = 0.0f;
            if (!ToPartner.IsNearlyZero())
            {
                Npc->SetActorRotation(ToPartner.Rotation());
            }
        }
        return;
    }

    const bool bIsGuard = Npc->IsGuard();
    if (bIsGuard)
    {
        ExecuteGuardDuty();
    }
    else
    {
        ExecutePatrol();
    }
}

void AAstrawildNPCAIController::ExecuteGuardDuty()
{
    AAstrawildNPCCharacter* Npc = NpcPawn.Get();
    UWorld* World = GetWorld();
    if (!Npc || !World)
    {
        return;
    }

    // Re-acquire a hostile wild Echo near the village.
    if (!GuardTarget.IsValid())
    {
        GuardTarget = FindNearestHostileEcho(GuardAggroRadius);
    }

    AAstrawildEchoCharacter* Target = GuardTarget.Get();
    if (Target && !Target->IsDefeated() && !Target->bCaptured)
    {
        const float Distance = FVector::Dist(Npc->GetActorLocation(), Target->GetActorLocation());
        if (Distance > GuardAggroRadius * 1.5f)
        {
            // Target fled — resume patrol.
            GuardTarget = nullptr;
            return;
        }

        SetNpcWalkSpeed(ChaseRunSpeed);
        if (Distance <= GuardAttackRange)
        {
            StopMovement();
            if (World->GetTimeSeconds() - LastAttackTime >= GuardAttackCooldownSeconds)
            {
                LastAttackTime = World->GetTimeSeconds();
                Target->ApplyElementalDamage(GuardDamage, EAstrawildElementType::None);
                UE_LOG(LogAstrawildAI, Verbose, TEXT("Guard %s struck hostile %s."), *Npc->GetName(), *Target->GetName());
            }
        }
        else
        {
            MoveToActor(Target, GuardAttackRange * 0.8f, true, true, true);
        }
        return;
    }

    GuardTarget = nullptr;

    // No threats: guards patrol like villagers but at a brisker pace.
    const float SavedPatrolSpeed = PatrolWalkSpeed;
    ExecutePatrol();
    SetNpcWalkSpeed(SavedPatrolSpeed * 1.3f);
}

void AAstrawildNPCAIController::ExecutePatrol()
{
    AAstrawildNPCCharacter* Npc = NpcPawn.Get();
    UWorld* World = GetWorld();
    if (!Npc || !World)
    {
        return;
    }

    SetNpcWalkSpeed(PatrolWalkSpeed);

    // Idle beat between waypoints.
    if (World->GetTimeSeconds() < NextWaypointTime)
    {
        return;
    }

    AAstrawildVillageActor* Village = Npc->GetHomeVillage();

    // Night: gather around the campfire and stay put.
    if (IsNight())
    {
        if (Village)
        {
            const FVector Campfire = Village->GetCampfireLocation();
            if (FVector::Dist(Npc->GetActorLocation(), Campfire) > 600.0f)
            {
                MoveToLocation(Campfire, 350.0f, true, true, true);
            }
            else
            {
                StopMovement();
            }
        }
        return;
    }

    FVector TargetPoint;
    if (Village && Village->GetWaypointCount() > 0)
    {
        TargetPoint = Village->GetWaypoint(Npc->AdvancePatrolIndex());
    }
    else
    {
        // No village: wander around the spawn anchor (800 cm leash).
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        TargetPoint = SpawnAnchor + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * FMath::FRandRange(300.0f, 800.0f);
    }

    MoveToLocation(TargetPoint, 120.0f, true, true, true);
    NextWaypointTime = World->GetTimeSeconds() + WaypointPauseSeconds + FMath::FRandRange(0.0f, 4.0f);
}

AAstrawildEchoCharacter* AAstrawildNPCAIController::FindNearestHostileEcho(const float MaxDistance) const
{
    const AAstrawildNPCCharacter* Npc = NpcPawn.Get();
    UWorld* World = GetWorld();
    if (!Npc || !World)
    {
        return nullptr;
    }

    AAstrawildEchoCharacter* Nearest = nullptr;
    float NearestDist = MaxDistance;

    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || Echo->IsDefeated() || Echo->bCaptured)
        {
            continue;
        }
        if (!Echo->EchoDefinition || !Echo->EchoDefinition->bHostileToPlayers)
        {
            continue;
        }

        const float Dist = FVector::Dist(Npc->GetActorLocation(), Echo->GetActorLocation());
        if (Dist < NearestDist)
        {
            Nearest = Echo;
            NearestDist = Dist;
        }
    }
    return Nearest;
}

bool AAstrawildNPCAIController::IsNight() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    if (const UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>())
    {
        const int32 Hour = Time->GetCurrentMinute() / 60;
        return Hour >= 21 || Hour < 6;
    }
    return false;
}

void AAstrawildNPCAIController::SetNpcWalkSpeed(const float NewSpeed)
{
    AAstrawildNPCCharacter* Npc = NpcPawn.Get();
    if (Npc && Npc->GetCharacterMovement())
    {
        Npc->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
    }
}
