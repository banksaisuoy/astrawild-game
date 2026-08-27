// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildEchoAIController.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "NavigationSystem.h"
#include "AstrawildLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AAstrawildEchoAIController::AAstrawildEchoAIController()
	: WanderRadius(1200.0f)
	, AggroDetectionRadius(850.0f)
	, CombatAttackRange(220.0f)
	, LeashDistance(2600.0f)
	, SimulationLODCloseDistance(3000.0f) // 30 meters
	, SimulationLODDormantDistance(6500.0f) // 65 meters
	, bShowAIDebug(false)
	, HomeLocation(FVector::ZeroVector)
	, TimeUntilNextWander(0.0f)
	, CombatCooldownTimer(0.0f)
	, ThrottledTickTimer(0.0f)
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

void AAstrawildEchoAIController::ToggleAIDebug()
{
	bShowAIDebug = !bShowAIDebug;
}

void AAstrawildEchoAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ControlledEcho.IsValid() || !ControlledEcho->Attributes || !ControlledEcho->Attributes->IsAlive())
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!PlayerPawn)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist(ControlledEcho->GetActorLocation(), PlayerPawn->GetActorLocation());

	// 1. SIMULATION LOD OPTIMIZATION: Dormant range (> 65m)
	if (DistanceToPlayer > SimulationLODDormantDistance && ControlledEcho->CurrentState != EAstrawildEchoState::SummonedCompanion)
	{
		return; // Sleep execution completely to preserve CPU budget
	}

	// 2. SIMULATION LOD OPTIMIZATION: Mid-range throttle (30m - 65m)
	if (DistanceToPlayer > SimulationLODCloseDistance && ControlledEcho->CurrentState != EAstrawildEchoState::SummonedCompanion)
	{
		ThrottledTickTimer += DeltaTime;
		if (ThrottledTickTimer < 0.25f)
		{
			return; // Tick only 4 times per second
		}
		DeltaTime = ThrottledTickTimer;
		ThrottledTickTimer = 0.0f;
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

	if (bShowAIDebug)
	{
		DrawAIDebugVisuals();
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
	// 1. Perception scan for nearby player
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		const float Dist = FVector::Dist(ControlledEcho->GetActorLocation(), PlayerPawn->GetActorLocation());
		if (Dist <= AggroDetectionRadius * 0.55f)
		{
			TargetActor = PlayerPawn;
			ControlledEcho->SetEchoState(EAstrawildEchoState::WildHostile);
			UE_LOG(LogAstrawildEcho, Log, TEXT("Wild Echo %s detected player at distance %.0f cm! Entering combat state."), *ControlledEcho->GetName(), Dist);
			return;
		}
	}

	// 2. Periodic wandering around territory
	TimeUntilNextWander -= DeltaTime;
	if (TimeUntilNextWander <= 0.0f)
	{
		TimeUntilNextWander = FMath::RandRange(4.0f, 9.0f);
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

	const float DistanceFromHome = FVector::Dist(ControlledEcho->GetActorLocation(), HomeLocation);

	// Leash check: if creature is pulled too far from territory, break combat and return
	if (DistanceFromHome > LeashDistance)
	{
		UE_LOG(LogAstrawildEcho, Log, TEXT("Echo %s exceeded leash distance (%.0f cm). Returning home."), *ControlledEcho->GetName(), DistanceFromHome);
		TargetActor = nullptr;
		ControlledEcho->SetEchoState(EAstrawildEchoState::WildPassive);
		MoveToLocation(HomeLocation, 50.0f);
		return;
	}

	const float DistanceToTarget = FVector::Dist(ControlledEcho->GetActorLocation(), TargetActor->GetActorLocation());

	if (DistanceToTarget > AggroDetectionRadius * 2.2f)
	{
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
			CombatCooldownTimer = 1.6f;
		}
	}
	else
	{
		MoveToActor(TargetActor.Get(), CombatAttackRange * 0.75f);
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
		const FVector FleeDestination = ControlledEcho->GetActorLocation() + (DangerDir * 900.0f);
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

	if (TargetActor.IsValid())
	{
		UpdateWildHostile(DeltaTime);
		return;
	}

	const float DistToLeader = FVector::Dist(ControlledEcho->GetActorLocation(), FollowLeaderActor->GetActorLocation());
	if (DistToLeader > 380.0f)
	{
		MoveToActor(FollowLeaderActor.Get(), 220.0f);
	}
	else if (DistToLeader < 180.0f)
	{
		StopMovement();
	}
}

void AAstrawildEchoAIController::DrawAIDebugVisuals()
{
	UWorld* World = GetWorld();
	if (!World || !ControlledEcho.IsValid())
	{
		return;
	}

	const FVector EchoLoc = ControlledEcho->GetActorLocation();
	const FString StateStr = UEnum::GetValueAsString(ControlledEcho->CurrentState);
	const float HP = ControlledEcho->Attributes ? ControlledEcho->Attributes->CurrentHealth : 0.0f;
	const float MaxHP = ControlledEcho->Attributes ? ControlledEcho->Attributes->MaxHealth : 1.0f;

	// 1. Draw 3D Floating State & Target Text above Echo head
	const FString StatusText = FString::Printf(TEXT("[%s] HP: %.0f/%.0f | Target: %s"),
		*StateStr, HP, MaxHP, TargetActor.IsValid() ? *TargetActor->GetName() : TEXT("None"));
	DrawDebugString(World, EchoLoc + FVector(0, 0, 90.0f), StatusText, nullptr, FColor::Yellow, 0.0f, true, 1.0f);

	// 2. Draw Aggro Sight Perception Sphere
	DrawDebugSphere(World, EchoLoc, AggroDetectionRadius, 16, FColor::Orange, false, -1.0f, 0, 1.5f);

	// 3. Draw Leash Territory Boundary Circle
	DrawDebugCircle(World, HomeLocation + FVector(0, 0, 10), LeashDistance, 32, FColor::Purple, false, -1.0f, 0, 2.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);

	// 4. Draw Line to Target if in combat
	if (TargetActor.IsValid())
	{
		DrawDebugLine(World, EchoLoc, TargetActor->GetActorLocation(), FColor::Red, false, -1.0f, 0, 2.5f);
	}
}