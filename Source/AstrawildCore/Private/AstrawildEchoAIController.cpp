#include "AstrawildEchoAIController.h"

#include "AstrawildAbilityLibrary.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEcosystemSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildWorkSiteActor.h"
#include "BehaviorTree/BlackboardData.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

AAstrawildEchoAIController::AAstrawildEchoAIController()
{
    PrimaryActorTick.bCanEverTick = false;

    // Sight perception (directive §6) — configured from the species definition on possess.
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2200.0f;
    SightConfig->PeripheralVisionAngleDegrees = 75.0f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->SetMaxAge(6.0f);

    UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
    SetPerceptionComponent(*Perception);
    if (Perception)
    {
        Perception->ConfigureSense(*SightConfig);
        Perception->SetDominantSense(SightConfig->GetSenseImplementation());
        Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AAstrawildEchoAIController::HandlePerception);
        // Final-audit M-8: forgotten stimuli previously had NO handler — aggro
        // latched through walls/line-of-sight loss until the 4km combat rescan.
        Perception->OnTargetPerceptionForgotten.AddDynamic(this, &AAstrawildEchoAIController::HandlePerceptionForgotten);
    }
}

void AAstrawildEchoAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    EchoPawn = Cast<AAstrawildEchoCharacter>(InPawn);
    AAstrawildEchoCharacter* Echo = EchoPawn.Get();
    if (!Echo)
    {
        return;
    }

    HomeLocation = Echo->GetActorLocation();

    // Scale perception to the species definition + personality.
    if (IsValid(Echo->EchoDefinition))
    {
        float SightRadius = Echo->EchoDefinition->SightRadius * Echo->GetAggroRadiusMultiplier();
        SightConfig->SightRadius = SightRadius;
        SightConfig->LoseSightRadius = Echo->EchoDefinition->LoseSightRadius * Echo->GetAggroRadiusMultiplier();
        if (UAIPerceptionComponent* Perception = GetPerceptionComponent())
        {
            Perception->ConfigureSense(*SightConfig);
        }
    }

    // Final-audit M-8: the damage feed — wild echoes fight back / flee on hits.
    Echo->OnDamaged.AddDynamic(this, &AAstrawildEchoAIController::HandleDamaged);

    // GDP-2: flying species move in true 3D — flying movement mode ignores the
    // navmesh plane and lets the steering helpers below drive it directly.
    if (Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Flying)
    {
        if (UCharacterMovementComponent* Movement = Echo->GetCharacterMovement())
        {
            Movement->SetMovementMode(MOVE_Flying);
        }
    }

    GetWorldTimerManager().SetTimer(ThinkTimerHandle, FTimerDelegate::CreateUObject(this, &AAstrawildEchoAIController::Think), ThinkIntervalSeconds, false);
}

void AAstrawildEchoAIController::OnUnPossess()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }
    // Final-audit M-8: unhook the damage feed with the perception feed.
    if (AAstrawildEchoCharacter* Echo = EchoPawn.Get())
    {
        Echo->OnDamaged.RemoveAll(this);
    }
    EchoPawn = nullptr;
    Super::OnUnPossess();
}

AAstrawildEchoCharacter* AAstrawildEchoAIController::GetEcho() const
{
    return EchoPawn.Get();
}

UAstrawildEcosystemSubsystem* AAstrawildEchoAIController::GetEcosystem() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildEcosystemSubsystem>() : nullptr;
}

EAstrawildEchoAIState AAstrawildEchoAIController::GetAIState() const
{
    const AAstrawildEchoCharacter* Echo = GetEcho();
    return Echo ? Echo->CurrentAIState : EAstrawildEchoAIState::Idle;
}

void AAstrawildEchoAIController::HandlePerception(AActor* Actor, FAIStimulus Stimulus)
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo || !Stimulus.WasSuccessfullySensed())
    {
        return;
    }

    // Only players matter as threats/subjects for now.
    if (! IsValid(Actor) || !Actor->IsA<AAstrawildPlayerCharacter>())
    {
        return;
    }

    // Production V2 (Master Plan §6): Calm Presence party passive — a healthy
    // captured Echo with the ThreatDampener aura standing with the player keeps
    // wild aggression from locking on (perception is quietly discarded).
    if (AAstrawildEchoCharacter::HasPlayerPartyPassive(GetWorld(), Actor, EAstrawildEchoPassive::ThreatDampener, 1600.0f))
    {
        return;
    }

    bPerceivedThreat = true;

    // Curious personalities investigate unknown actors instead of immediately reacting (directive §5).
    if (!Echo->bCaptured && Echo->Personality == EAstrawildPersonality::Curious && Echo->GetHealthFraction() > 0.8f)
    {
        if (GetAIState() == EAstrawildEchoAIState::Idle || GetAIState() == EAstrawildEchoAIState::Explore)
        {
            TargetActor = Actor;
            TransitionTo(EAstrawildEchoAIState::Investigate);
        }
    }
}

void AAstrawildEchoAIController::HandlePerceptionForgotten(AActor* Actor)
{
    // Final-audit M-8: the stimulus expired (out of LoseSightRadius / LOS lost).
    // Drop the threat latch and the target when it points at the forgotten actor —
    // the next Think re-acquires only what is actually perceivable.
    if (TargetActor.Get() == Actor)
    {
        TargetActor = nullptr;
        bPerceivedThreat = false;
    }
}

void AAstrawildEchoAIController::HandleDamaged(AAstrawildEchoCharacter* Echo, float NewHealth)
{
    // Final-audit M-8: OnDamaged → aggro. The EchoCharacter comment claimed this
    // wiring existed; it never did (Aggressive/Brave "fight back" was dead).
    // Attacker = the damage instigator when the player path sets one; otherwise
    // the nearest player within combat range (the de-facto attacker, single-player-first).
    if (!Echo || Echo->bCaptured || Echo->IsDefeated())
    {
        return; // Party echoes do not aggro their owner's attacks.
    }
    AActor* Attacker = Echo->GetInstigator();
    if (!Attacker || !Attacker->IsA<AAstrawildPlayerCharacter>())
    {
        Attacker = FindNearestPlayer(3000.0f);
    }
    if (Attacker)
    {
        bPerceivedThreat = true;
        TargetActor = Attacker;
        // Aggressive/Brave personalities turn immediately; others let the
        // next Think decide (flee for skittish species).
        if (Echo->Personality == EAstrawildPersonality::Aggressive ||
            Echo->Personality == EAstrawildPersonality::Brave)
        {
            TransitionTo(EAstrawildEchoAIState::Combat);
        }
    }
}

void AAstrawildEchoAIController::Think()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    UWorld* World = GetWorld();
    if (!Echo || !World || Echo->IsDefeated())
    {
        return;
    }

    // Batch 3 — Item B: while staggered, skip decisions/actions but STILL re-arm the
    // think timer below (a naive early-return here would permanently kill the AI loop).
    if (Echo->IsStaggered())
    {
        StopMovement();
    }
    else
    {
        TransitionTo(DecideState());
        ExecuteState(ThinkIntervalSeconds);
    }

    // LOD-aware think rate (directive §34): far creatures think slower.
    float Interval = ThinkIntervalSeconds;
    if (const UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
    {
        const float TierInterval = UAstrawildEcosystemSubsystem::GetRecommendedUpdateInterval(Ecosystem->GetTierForEcho(Echo));
        Interval = FMath::Max(ThinkIntervalSeconds, TierInterval);
    }

    World->GetTimerManager().SetTimer(ThinkTimerHandle, FTimerDelegate::CreateUObject(this, &AAstrawildEchoAIController::Think), Interval, false);
}

EAstrawildEchoAIState AAstrawildEchoAIController::DecideState()
{
    const AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return EAstrawildEchoAIState::Idle;
    }

    // --- Captured Echoes obey commands (directive §10) ---
    if (Echo->bCaptured)
    {
        switch (Echo->ActiveCommand)
        {
        case EAstrawildEchoCommand::Follow:
            return EAstrawildEchoAIState::Follow;
        case EAstrawildEchoCommand::Stay:
        case EAstrawildEchoCommand::HoldPosition:
            return EAstrawildEchoAIState::Idle;
        case EAstrawildEchoCommand::Retreat:
            return EAstrawildEchoAIState::Follow;
        case EAstrawildEchoCommand::Defend:
            return EAstrawildEchoAIState::Protect;
        case EAstrawildEchoCommand::Attack:
            return TargetActor.IsValid() ? EAstrawildEchoAIState::Combat : EAstrawildEchoAIState::Follow;
        case EAstrawildEchoCommand::Work:
            return Echo->AssignedWorkSite.IsValid() ? EAstrawildEchoAIState::Work : EAstrawildEchoAIState::Follow;
        default:
            return EAstrawildEchoAIState::Follow;
        }
    }

    // --- Wild decision tree ---

    // Critical needs override everything (directive §4/§5).
    if (Echo->Needs.IsCritical())
    {
        return EAstrawildEchoAIState::SearchFood;
    }

    // Health-based flee (personality scales the threshold, directive §5).
    const float FleeThreshold = BaseFleeHealthFraction * Echo->GetFleeHealthThresholdMultiplier();
    if (Echo->GetHealthFraction() <= FMath::Clamp(FleeThreshold, 0.05f, 0.9f))
    {
        return EAstrawildEchoAIState::Flee;
    }

    // Food-chain hunting (directive §7): predators stalk their prey species before players.
    if (GetEcosystem() && GetEcosystem()->IsPredator(Echo->EchoDefinition ? Echo->EchoDefinition->DefinitionId : NAME_None))
    {
        if (!TargetActor.IsValid())
        {
            // Hunt prey when healthy; players remain a target once perceived (below).
            if (!bPerceivedThreat || Echo->GetHealthFraction() > 0.75f)
            {
                TargetActor = GetEcosystem()->FindPreyFor(Echo, 4000.0f);
            }
        }
        if (TargetActor.IsValid() && !Cast<AAstrawildPlayerCharacter>(TargetActor.Get()))
        {
            return EAstrawildEchoAIState::Combat; // Creature-vs-creature hunting.
        }
    }

    // Hostile species aggro on perceived players (directive §21).
    const bool bHostileSpecies = IsValid(Echo->EchoDefinition) && Echo->EchoDefinition->bHostileToPlayers;
    if (bHostileSpecies && bPerceivedThreat)
    {
        return EAstrawildEchoAIState::Combat;
    }

    // Social personalities keep herds together (directive §5/§7).
    if (Echo->Personality == EAstrawildPersonality::Social && !Echo->bCaptured)
    {
        if (GetEcosystem() && GetEcosystem()->FindHerdAnchor(Echo, 2000.0f))
        {
            return EAstrawildEchoAIState::Socialize;
        }
    }

    // Sleep outside the activity window (directive §13).
    if (!Echo->IsCurrentlyActiveTime())
    {
        return EAstrawildEchoAIState::Sleep;
    }

    // Hungry creatures forage (directive §7).
    if (Echo->Needs.Hunger < 40.0f)
    {
        return EAstrawildEchoAIState::SearchFood;
    }

    if (GetAIState() == EAstrawildEchoAIState::Investigate && TargetActor.IsValid())
    {
        return EAstrawildEchoAIState::Investigate;
    }

    // Default: relaxed exploration from home.
    return EAstrawildEchoAIState::Explore;
}

void AAstrawildEchoAIController::TransitionTo(const EAstrawildEchoAIState NewState)
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    if (Echo->CurrentAIState != NewState)
    {
        UE_LOG(LogAstrawildAI, Verbose, TEXT("Echo %s AI: %d -> %d."), *Echo->GetName(), static_cast<int32>(Echo->CurrentAIState), static_cast<int32>(NewState));
        // Route through the public setter (audit H-8) so OnAIStateChanged broadcasts
        // for UI/audio consumers instead of silently writing the replicated enum.
        Echo->SetAIState(NewState);
    }
}

void AAstrawildEchoAIController::ExecuteState(const float DeltaThinkSeconds)
{
    switch (GetAIState())
    {
    case EAstrawildEchoAIState::Explore:
        ExecuteExplore();
        break;
    case EAstrawildEchoAIState::Flee:
        ExecuteFlee();
        break;
    case EAstrawildEchoAIState::Combat:
        ExecuteCombat(DeltaThinkSeconds);
        break;
    case EAstrawildEchoAIState::Follow:
        ExecuteFollow();
        break;
    case EAstrawildEchoAIState::Protect:
        ExecuteProtect();
        break;
    case EAstrawildEchoAIState::Work:
        ExecuteWork();
        break;
    case EAstrawildEchoAIState::Sleep:
        ExecuteSleep();
        break;
    case EAstrawildEchoAIState::SearchFood:
        ExecuteSearchFood();
        break;
    case EAstrawildEchoAIState::Investigate:
        ExecuteFollow(); // Move toward the investigate target.
        break;
    case EAstrawildEchoAIState::Socialize:
        ExecuteSocialize();
        break;
    case EAstrawildEchoAIState::Idle:
    default:
        StopMovement();
        break;
    }
}

void AAstrawildEchoAIController::ExecuteExplore()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    UWorld* World = GetWorld();
    if (!Echo || !World)
    {
        return;
    }

    if (World->GetTimeSeconds() < NextWanderTime && GetMoveStatus() != EPathFollowingStatus::Idle)
    {
        return;
    }

    NextWanderTime = World->GetTimeSeconds() + FMath::FRandRange(2.0f, 6.0f);
    const FVector WanderPoint = HomeLocation + FVector(
        FMath::FRandRange(-ExploreRadius, ExploreRadius),
        FMath::FRandRange(-ExploreRadius, ExploreRadius),
        0.0f);
    MoveTowardPoint(WanderPoint, 100.0f);
}

void AAstrawildEchoAIController::ExecuteFlee()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    AActor* Threat = FindNearestPlayer(3000.0f);
    if (!Threat)
    {
        // No threat nearby — recover toward home.
        MoveTowardPoint(HomeLocation, 120.0f);
        return;
    }

    const FVector AwayDirection = (Echo->GetActorLocation() - Threat->GetActorLocation()).GetSafeNormal2D();
    const FVector FleePoint = Echo->GetActorLocation() + AwayDirection * 1200.0f;
    MoveTowardPoint(FleePoint, 100.0f);
}

void AAstrawildEchoAIController::ExecuteCombat(const float DeltaThinkSeconds)
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    AActor* Target = TargetActor.Get();
    if (!Target)
    {
        // Acquire nearest player as combat target — never the owner for captured
        // Echoes (audit H-7: commanded Echoes used to hunt their own player).
        const FName ExcludeId = Echo->bCaptured ? Echo->OwnerPlayerId : NAME_None;
        Target = FindNearestPlayer(4000.0f, ExcludeId);
        TargetActor = Target;
    }

    if (!Target)
    {
        bPerceivedThreat = false;
        StopMovement();
        return;
    }

    const float Distance = FVector::Dist(Echo->GetActorLocation(), Target->GetActorLocation());

    // GDP-1: every creature fights like itself — abilities resolve before melee,
    // gated by level/cooldown/range inside the ability engine. Bosses keep their
    // own phase patterns (their controller logic stays untouched).
    if (TryCastCombatAbility(Echo, Target))
    {
        return; // The cast consumed this beat; melee resumes next think.
    }

    if (Distance > AttackRange)
    {
        MoveTowardTarget(Target, AttackRange * 0.8f);
    }
    else
    {
        StopMovement();
        TryAttackTarget(Target, DeltaThinkSeconds);
    }
}

bool AAstrawildEchoAIController::TryCastCombatAbility(AAstrawildEchoCharacter* Echo, AActor* Target)
{
    if (!Echo || !IsValid(Echo) || !Echo->EchoDefinition.IsValid())
    {
        return false;
    }

    // Bosses run their own choreography — no ability spam on top of phases.
    if (Echo->IsA<AAstrawildEchoBossCharacter>())
    {
        return false;
    }

    UWorld* World = GetWorld();
    const float Distance = Target ? FVector::Dist(Echo->GetActorLocation(), Target->GetActorLocation()) : 0.0f;

    // Tactical intent: heal when the pack is hurting, shield when about to die.
    const float HealthFraction = Echo->GetHealthFraction();
    bool bWantsHeal = HealthFraction < 0.35f;
    if (!bWantsHeal && Echo->bCaptured && Echo->OwnerPlayerId != NAME_None && World)
    {
        // A captured medic heals when its owner's other echoes are low.
        for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
        {
            const AAstrawildEchoCharacter* Party = *It;
            if (Party && Party != Echo && Party->bCaptured && !Party->IsDefeated() &&
                Party->OwnerPlayerId == Echo->OwnerPlayerId && Party->GetHealthFraction() < 0.4f)
            {
                bWantsHeal = true;
                break;
            }
        }
    }
    const bool bWantsShield = HealthFraction < 0.3f;

    const FName AbilityId = Echo->PickCombatAbility(Distance, bWantsHeal, bWantsShield);
    if (AbilityId == NAME_None)
    {
        return false;
    }

    return Echo->ExecuteAbility(AbilityId, Target);
}

void AAstrawildEchoAIController::ExecuteFollow()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    // Captured Echoes follow their owner; investigating wild Echoes approach the subject.
    AActor* Subject = nullptr;
    if (Echo->bCaptured)
    {
        Subject = FindNearestPlayer(100000.0f);
        // Final-audit M-7: stranded party recovery. Beyond the 1km scan the echo
        // used to StopMovement() FOREVER (fast skiff travel / failed pathing / flee
        // chains left it permanently behind). No subject + captured + far from any
        // player → teleport home beside the owner (the same ring the roster spawn
        // uses). A recall, not a crawl.
        if (!Subject)
        {
            AActor* AnyPlayer = FindNearestPlayer(10000000.0f);
            if (AnyPlayer)
            {
                const FVector Ring = AnyPlayer->GetActorLocation() + AnyPlayer->GetActorForwardVector() * 320.0f + FVector(0.0f, 0.0f, 120.0f);
                Echo->SetActorLocation(Ring, false, nullptr, ETeleportType::TeleportPhysics);
                Subject = AnyPlayer;
                UE_LOG(LogAstrawildAI, Log, TEXT("Party echo %s recalled to its owner (was stranded)."), *Echo->GetName());
            }
            else
            {
                StopMovement();
                return;
            }
        }
    }
    else if (TargetActor.IsValid())
    {
        Subject = TargetActor.Get();
        // Stop approaching once close — curiosity satisfied.
        if (FVector::Dist(Echo->GetActorLocation(), Subject->GetActorLocation()) < 250.0f)
        {
            StopMovement();
            return;
        }
    }

    if (!Subject)
    {
        StopMovement();
        return;
    }

    const float Distance = FVector::Dist(Echo->GetActorLocation(), Subject->GetActorLocation());
    if (Distance > FollowDistance * 1.5f)
    {
        MoveTowardTarget(Subject, FollowDistance);
    }
    else if (Distance < FollowDistance * 0.5f && GetMoveStatus() != EPathFollowingStatus::Idle)
    {
        StopMovement();
    }
}

void AAstrawildEchoAIController::ExecuteProtect()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    // Protective behavior: intercept hostiles near the owner, else stay close (directive §5/§10).
    AActor* NearestPlayer = FindNearestPlayer(100000.0f);
    if (!NearestPlayer)
    {
        return;
    }

    // Final-audit F-12: the hostile scan used GetAllActorsOfClass every think
    // (0.25s per defending echo — 3 echoes = 12 full-world scans/second). The list
    // is now cached for one second around the protected player.
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const bool bCacheValid = (Now - HostilesCacheTime) < 1.0
        && HostilesCacheAnchor.Get() == NearestPlayer
        && CachedNearbyHostiles.Num() >= 0;
    if (!bCacheValid)
    {
        CachedNearbyHostiles.Reset();
        const double CacheTimeBefore = Now;
        if (UWorld* World = GetWorld())
        {
            TArray<AActor*> Echoes;
            UGameplayStatics::GetAllActorsOfClass(World, AAstrawildEchoCharacter::StaticClass(), Echoes);
            for (AActor* Actor : Echoes)
            {
                AAstrawildEchoCharacter* Other = Cast<AAstrawildEchoCharacter>(Actor);
                if (!Other || Other == Echo || !IsValid(Other->EchoDefinition) || !Other->EchoDefinition->bHostileToPlayers)
                {
                    continue;
                }
                const float Distance = FVector::Dist(NearestPlayer->GetActorLocation(), Other->GetActorLocation());
                if (Distance < 800.0f)
                {
                    CachedNearbyHostiles.Add(Other);
                }
            }
        }
        HostilesCacheTime = CacheTimeBefore;
        HostilesCacheAnchor = NearestPlayer;
    }

    AAstrawildEchoCharacter* NearestHostile = nullptr;
    float BestDistance = 800.0f;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : CachedNearbyHostiles)
    {
        AAstrawildEchoCharacter* Other = Weak.Get();
        if (!Other)
        {
            continue;
        }
        const float Distance = FVector::Dist(NearestPlayer->GetActorLocation(), Other->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            NearestHostile = Other;
        }
    }

    if (NearestHostile)
    {
        const float Distance = FVector::Dist(Echo->GetActorLocation(), NearestHostile->GetActorLocation());
        if (Distance > AttackRange)
        {
            MoveTowardTarget(NearestHostile, AttackRange * 0.8f);
        }
        else
        {
            StopMovement();
            TryAttackTarget(NearestHostile, ThinkIntervalSeconds);
        }
    }
    else
    {
        const float Distance = FVector::Dist(Echo->GetActorLocation(), NearestPlayer->GetActorLocation());
        if (Distance > FollowDistance)
        {
            MoveTowardTarget(NearestPlayer, FollowDistance * 0.8f);
        }
    }
}

void AAstrawildEchoAIController::ExecuteWork()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    AAstrawildWorkSiteActor* Site = Echo->AssignedWorkSite.Get();
    if (!Site)
    {
        return;
    }

    const float Distance = FVector::Dist(Echo->GetActorLocation(), Site->GetActorLocation());
    if (Distance > Site->WorkRange)
    {
        MoveTowardTarget(Site, Site->WorkRange * 0.8f);
    }
    else
    {
        StopMovement();
        // Work progress + energy drain is applied by the site itself each tick (directive §18).
    }
}

void AAstrawildEchoAIController::ExecuteSleep()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo)
    {
        return;
    }

    StopMovement();
    // Sleeping recovers energy server-side (directive §4 Needs).
    FAstrawildEchoNeeds& Needs = Echo->Needs;
    Needs.Energy = FMath::Clamp(Needs.Energy + 2.0f * ThinkIntervalSeconds, 0.0f, 100.0f);
}

void AAstrawildEchoAIController::ExecuteSearchFood()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    UWorld* World = GetWorld();
    if (!Echo || !World)
    {
        return;
    }

    // Wander while hungry; the ecosystem will grow feeding grounds in future content passes.
    ExecuteExplore();
    Echo->Needs.Hunger = FMath::Clamp(Echo->Needs.Hunger + 0.1f, 0.0f, 100.0f); // Grazing trickle.
}

void AAstrawildEchoAIController::ExecuteSocialize()
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    if (!Echo || !GetEcosystem())
    {
        return;
    }

    // Herd cohesion (directive §7): drift toward the nearest same-species herd anchor.
    AAstrawildEchoCharacter* Anchor = GetEcosystem()->FindHerdAnchor(Echo, 2500.0f);
    if (Anchor)
    {
        const float Distance = FVector::Dist(Echo->GetActorLocation(), Anchor->GetActorLocation());
        if (Distance > 350.0f)
        {
            MoveTowardTarget(Anchor, 300.0f);
        }
        else
        {
            StopMovement();
        }
    }

    // Socializing lifts mood slowly (directive §5 Social).
    Echo->Needs.Mood = FMath::Clamp(Echo->Needs.Mood + 0.2f * ThinkIntervalSeconds, 0.0f, 100.0f);
}

bool AAstrawildEchoAIController::TryAttackTarget(AActor* Target, const float DeltaThinkSeconds)
{
    AAstrawildEchoCharacter* Echo = GetEcho();
    UWorld* World = GetWorld();
    if (!Echo || !World || !IsValid(Target))
    {
        return false;
    }

    if (World->GetTimeSeconds() - LastAttackTime < AttackCooldownSeconds)
    {
        return false;
    }

    LastAttackTime = World->GetTimeSeconds();

    // Element from species definition.
    const EAstrawildElementType Element = IsValid(Echo->EchoDefinition) ? Echo->EchoDefinition->Element : EAstrawildElementType::None;
    const float Damage = Echo->GetAttackPower() * AttackDamageMultiplier;

    if (AAstrawildEchoCharacter* TargetEcho = Cast<AAstrawildEchoCharacter>(Target))
    {
        TargetEcho->ApplyElementalDamage(Damage, Element);
        return true;
    }

    if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(Target))
    {
        if (UAstrawildCombatComponent* Combat = Player->FindComponentByClass<UAstrawildCombatComponent>())
        {
            const float Mitigated = Combat->GetMitigatedIncomingDamage(Damage);
            if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
            {
                Survival->ApplyDamage(Mitigated);

                // Batch 3 — Item A: the attacker's element applies its status effect to
                // the PLAYER too (Ember burn, Frost chill, Flora poison, Pulse shock).
                if (Element != EAstrawildElementType::None)
                {
                    const FAstrawildStatusEffect StatusEffect =
                        UAstrawildCombatComponent::MakeElementalStatusEffect(Element, Mitigated);
                    if (!StatusEffect.StatusId.IsNone())
                    {
                        Survival->AddStatusEffect(StatusEffect);
                    }
                }

                // Batch 3 — Item B: heavy incoming hits stagger the player.
                if (Mitigated >= Combat->StaggerDamageThreshold)
                {
                    Combat->ApplyStagger(Combat->PlayerStaggerSeconds);
                }
                return true;
            }
        }
    }

    return false;
}

AActor* AAstrawildEchoAIController::FindNearestPlayer(const float MaxDistance, const FName ExcludePlayerId) const
{
    const AAstrawildEchoCharacter* Echo = GetEcho();
    const UWorld* World = GetWorld();
    if (!Echo || !World)
    {
        return nullptr;
    }

    AActor* Best = nullptr;
    float BestDistance = MaxDistance;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        if (!PC || !PC->GetPawn())
        {
            continue;
        }
        // Owner exclusion (audit H-7) — captured Echoes never target their own player.
        if (!ExcludePlayerId.IsNone() && PC->GetPawn()->GetFName() == ExcludePlayerId)
        {
            continue;
        }
        const float Distance = FVector::Dist(Echo->GetActorLocation(), PC->GetPawn()->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = PC->GetPawn();
        }
    }
    return Best;
}

// ===========================================================================
// GDP-2 — locomotion-aware movement helpers
// ===========================================================================

bool AAstrawildEchoAIController::IsFlyingMover() const
{
    const AAstrawildEchoCharacter* Echo = EchoPawn.Get();
    return Echo && Echo->GetLocomotionClass() == EAstrawildLocomotionClass::Flying;
}

void AAstrawildEchoAIController::SteerFlyingToward(const FVector& Point, const float HoverHeight)
{
    AAstrawildEchoCharacter* Echo = EchoPawn.Get();
    if (!Echo)
    {
        return;
    }

    // Desired 3D direction: horizontal toward the point, vertical eased toward
    // the point's altitude + hover offset (a gentle bob keeps flight readable).
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    const double Now = FMath::Fmod(World->GetTimeSeconds(), 10.0);
    const float Bob = FMath::Sin(Now * 1.7f) * 35.0f;
    FVector Direction = Point - Echo->GetActorLocation();
    Direction.Z = (Point.Z + HoverHeight + Bob) - Echo->GetActorLocation().Z;
    Direction = Direction.GetSafeNormal();

    Echo->AddMovementInput(Direction, 1.0f);

    // Face the travel direction so silhouettes read naturally.
    if (FVector2D XY(Direction.X, Direction.Y).SizeSquared() > 0.01f)
    {
        Echo->SetActorRotation(FRotator(0.0f, Direction.Rotation().Yaw, 0.0f));
    }
}

void AAstrawildEchoAIController::MoveTowardPoint(const FVector& Point, const float AcceptanceRadius)
{
    if (IsFlyingMover())
    {
        SteerFlyingToward(Point);
    }
    else
    {
        MoveToLocation(Point, AcceptanceRadius, true, true, true);
    }
}

void AAstrawildEchoAIController::MoveTowardTarget(AActor* Target, const float AcceptanceRadius)
{
    if (!IsValid(Target))
    {
        return;
    }
    if (IsFlyingMover())
    {
        SteerFlyingToward(Target->GetActorLocation());
    }
    else
    {
        MoveToActor(Target, AcceptanceRadius, true, true, true);
    }
}
