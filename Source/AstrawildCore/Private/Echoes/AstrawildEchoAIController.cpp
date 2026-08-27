// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildEchoAIController.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "NavigationSystem.h"
#include "AstrawildLogChannels.h"
#include "Kismet/GameplayStatics.h"

AAstrawildEchoAIController::AAstrawildEchoAIController()
	: WanderRadius(1200.0f)
	, AggroDetectionRadius(800.0f)
	, CombatAttackRange(220.0f)
	, HomeLocation(FVector::ZeroVector)
	, TimeUntilNextWander(0.0f)
	, CombatCooldownTimer(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAstrawildEchoAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledEcho = Cast<AAstrawildEchoBase>(InPawn);
	if (InPawn)
	{
		HomeLocation = InPawn->GetActorLocation();
	}
}

void AAstrawildEchoAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ControlledEcho.IsValid() || !ControlledEcho->Attributes || !ControlledEcho->Attributes->IsAlive())
	{
		return;
	}

	if (CombatCooldownTimer > 0.0f)
	{
		CombatCooldownTimer -= DeltaTime;
	}

	switch (ControlledEcho->CurrentState)
	{
	case EAstrawildEchoState::WildPassive:
		UpdateWildPassive(DeltaTime);
		break;
	case EAstrawildEchoState::WildHostile:
		UpdateWildHostile(DeltaTime);
		break;
	case EAstrawildEchoState::Fleeing:
		UpdateFleeing(DeltaTime);
		break;
	case EAstrawildEchoState::SummonedCompanion:
		UpdateSummonedCompanion(DeltaTime);
		break;
	default:
		break;
	}
}

void AAstrawildEchoAIController::SetFollowLeader(AActor* Leader)
{
	FollowLeaderActor = Leader;
}

void AAstrawildEchoAIController::SetCombatTarget(AActor* InTarget)
{
	TargetActor = InTarget;
}

void AAstrawildEchoAIController::UpdateWildPassive(float DeltaTime)
{
	// 1. Check for nearby player if aggressive species
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		const float Dist = FVector::Dist(ControlledEcho->GetActorLocation(), PlayerPawn->GetActorLocation());
		if (Dist <= AggroDetectionRadius * 0.5f) // Close proximity trigger
		{
			TargetActor = PlayerPawn;
			ControlledEcho->SetEchoState(EAstrawildEchoState::WildHostile);
			return;
		}
	}

	// 2. Periodic wandering around HomeLocation
	TimeUntilNextWander -= DeltaTime;
	if (TimeUntilNextWander <= 0.0f)
	{
		TimeUntilNextWander = FMath::RandRange(4.0f, 8.0f);
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavLocation RandomLocation;
			if (NavSys->GetRandomReachablePointInRadius(HomeLocation, WanderRadius, RandomLocation))
			{
				MoveToLocation(RandomLocation.Location, 50.0f);
			}
		}
	}
}

void AAstrawildEchoAIController::UpdateWildHostile(float DeltaTime)
{
	if (!TargetActor.IsValid())
	{
		ControlledEcho->SetEchoState(EAstrawildEchoState::WildPassive);
		return;
	}

	const float DistanceToTarget = FVector::Dist(ControlledEcho->GetActorLocation(), TargetActor->GetActorLocation());

	if (DistanceToTarget > AggroDetectionRadius * 2.0f)
	{
		// Target escaped too far
		TargetActor = nullptr;
		ControlledEcho->SetEchoState(EAstrawildEchoState::WildPassive);
		MoveToLocation(HomeLocation, 50.0f);
		return;
	}

	if (DistanceToTarget <= CombatAttackRange)
	{
		StopMovement();
		if (CombatCooldownTimer <= 0.0f && ControlledEcho->Combat)
		{
			ControlledEcho->Combat->PerformMeleeAttack(1.0f, ControlledEcho->Attributes->ElementalAffinity);
			CombatCooldownTimer = 1.8f;
		}
	}
	else
	{
		MoveToActor(TargetActor.Get(), CombatAttackRange * 0.8f);
	}
}

void AAstrawildEchoAIController::UpdateFleeing(float DeltaTime)
{
	if (!TargetActor.IsValid())
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	}

	if (TargetActor.IsValid())
	{
		const FVector DangerDir = (ControlledEcho->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
		const FVector FleeDestination = ControlledEcho->GetActorLocation() + (DangerDir * 800.0f);
		MoveToLocation(FleeDestination, 50.0f);
	}
}

void AAstrawildEchoAIController::UpdateSummonedCompanion(float DeltaTime)
{
	if (!FollowLeaderActor.IsValid())
	{
		FollowLeaderActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	}

	if (!FollowLeaderActor.IsValid())
	{
		return;
	}

	// If companion has a combat target, attack it
	if (TargetActor.IsValid())
	{
		UpdateWildHostile(DeltaTime);
		return;
	}

	// Otherwise, follow player smoothly
	const float DistToLeader = FVector::Dist(ControlledEcho->GetActorLocation(), FollowLeaderActor->GetActorLocation());
	if (DistToLeader > 400.0f)
	{
		MoveToActor(FollowLeaderActor.Get(), 250.0f);
	}
	else if (DistToLeader < 200.0f)
	{
		StopMovement();
	}
}