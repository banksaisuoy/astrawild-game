// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

UAstrawildCombatComponent::UAstrawildCombatComponent()
	: BaseMeleeRange(200.0f)
	, BaseMeleeRadius(75.0f)
	, AttackCooldownDuration(0.6f)
	, StaminaCostPerAttack(10.0f)
	, CriticalHitChancePercent(10.0f)
	, CriticalMultiplier(1.5f)
	, CurrentAttackCooldown(0.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	GetAttributes();
}

void UAstrawildCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (CurrentAttackCooldown > 0.0f)
	{
		CurrentAttackCooldown -= DeltaTime;
	}
}

UAstrawildAttributeComponent* UAstrawildCombatComponent::GetAttributes()
{
	if (!CachedAttributes.IsValid() && GetOwner())
	{
		CachedAttributes = GetOwner()->FindComponentByClass<UAstrawildAttributeComponent>();
	}
	return CachedAttributes.Get();
}

bool UAstrawildCombatComponent::CanAttack() const
{
	if (CurrentAttackCooldown > 0.0f)
	{
		return false;
	}

	UAstrawildAttributeComponent* Attr = const_cast<UAstrawildCombatComponent*>(this)->GetAttributes();
	if (Attr && Attr->CurrentStamina < StaminaCostPerAttack)
	{
		return false;
	}

	return true;
}

bool UAstrawildCombatComponent::PerformMeleeAttack(float DamageMultiplier, EAstrawildElement DamageElement)
{
	if (!CanAttack())
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	// Drain stamina
	UAstrawildAttributeComponent* Attr = GetAttributes();
	if (Attr)
	{
		Attr->ModifyStamina(-StaminaCostPerAttack);
	}

	CurrentAttackCooldown = AttackCooldownDuration;

	const FVector Start = OwnerActor->GetActorLocation();
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector End = Start + (Forward * BaseMeleeRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> HitResults;
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(BaseMeleeRadius);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_WorldDynamic,
		SweepShape,
		QueryParams
	);

	float AttackerPower = Attr ? Attr->AttackPower : 20.0f;
	float TotalRawDamage = AttackerPower * DamageMultiplier;

	TSet<AActor*> HitActors;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActors.Contains(HitActor))
			{
				continue;
			}
			HitActors.Add(HitActor);

			// Check if target is a Harvestable Node
			AAstrawildHarvestableNode* HarvestNode = Cast<AAstrawildHarvestableNode>(HitActor);
			if (HarvestNode)
			{
				int32 Yield = 0;
				HarvestNode->Harvest(1.0f, EAstrawildHarvestType::Foraging, OwnerActor, Yield);
				continue;
			}

			// Combat damage against characters / echoes
			ApplyDamageToTarget(HitActor, TotalRawDamage, DamageElement, OwnerActor);
		}
	}

	return true;
}

float UAstrawildCombatComponent::ApplyDamageToTarget(AActor* TargetActor, float RawDamage, EAstrawildElement DamageElement, AActor* DamageDealer)
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	UAstrawildAttributeComponent* TargetAttr = TargetActor->FindComponentByClass<UAstrawildAttributeComponent>();
	if (!TargetAttr || !TargetAttr->IsAlive())
	{
		return 0.0f;
	}

	// 1. Armor mitigation formula: 100 / (100 + Defense)
	const float Defense = FMath::Max(0.0f, TargetAttr->DefensePower);
	const float DefenseFactor = 100.0f / (100.0f + Defense);

	// 2. Elemental advantage
	const float ElementMultiplier = FAstrawildElementalMatrix::GetMultiplier(DamageElement, TargetAttr->ElementalAffinity);

	// 3. Critical hit check
	const bool bIsCrit = FMath::RandRange(1.0f, 100.0f) <= CriticalHitChancePercent;
	const float CritFactor = bIsCrit ? CriticalMultiplier : 1.0f;

	// 4. Random variance (+- 8%)
	const float Variance = FMath::RandRange(0.92f, 1.08f);

	const float FinalDamage = FMath::Max(1.0f, RawDamage * DefenseFactor * ElementMultiplier * CritFactor * Variance);

	// Apply health deduction
	TargetAttr->ModifyHealth(-FinalDamage, DamageDealer);

	UE_LOG(LogAstrawildCombat, Log, TEXT("%s dealt %.1f [%s] damage to %s (Crit: %d, Multiplier: %.2f)"),
		DamageDealer ? *DamageDealer->GetName() : TEXT("Unknown"),
		FinalDamage,
		*UEnum::GetValueAsString(DamageElement),
		*TargetActor->GetName(),
		bIsCrit,
		ElementMultiplier);

	OnDamageDealt.Broadcast(TargetActor, FinalDamage, DamageElement, bIsCrit);

	UAstrawildCombatComponent* TargetCombat = TargetActor->FindComponentByClass<UAstrawildCombatComponent>();
	if (TargetCombat)
	{
		TargetCombat->OnDamageReceived.Broadcast(DamageDealer, FinalDamage, DamageElement, bIsCrit);
	}

	return FinalDamage;
}