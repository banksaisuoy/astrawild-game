// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Interfaces/AstrawildDamageableInterface.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

UAstrawildCombatComponent::UAstrawildCombatComponent()
	: BaseMeleeRange(220.0f)
	, BaseMeleeRadius(85.0f)
	, AttackCooldownDuration(0.45f)
	, StaminaCostPerAttack(10.0f)
	, CriticalHitChancePercent(12.0f)
	, CriticalMultiplier(1.5f)
	, CurrentComboIndex(0)
	, ComboResetTimeout(1.1f)
	, CurrentAttackCooldown(0.0f)
	, ComboTimer(0.0f)
	, NextAttackInstanceId(1)
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

	if (ComboTimer > 0.0f)
	{
		ComboTimer -= DeltaTime;
		if (ComboTimer <= 0.0f)
		{
			CurrentComboIndex = 0; // Combo timed out -> reset to step 1
		}
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
	return PerformMeleeCombo(DamageElement);
}

bool UAstrawildCombatComponent::PerformMeleeCombo(EAstrawildElement DamageElement, const FGameplayTag& StatusToApply, float StatusDuration)
{
	if (!CanAttack())
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld())
	{
		return false;
	}

	// 1. Drain stamina
	UAstrawildAttributeComponent* Attr = GetAttributes();
	if (Attr)
	{
		Attr->ModifyStamina(-StaminaCostPerAttack);
	}

	// 2. Advance combo & calculate combo multiplier
	CurrentComboIndex = (CurrentComboIndex + 1) % 4;
	if (CurrentComboIndex == 0)
	{
		CurrentComboIndex = 1;
	}

	ComboTimer = ComboResetTimeout;
	CurrentAttackCooldown = AttackCooldownDuration;
	OnComboStep.Broadcast(CurrentComboIndex);

	const float ComboMultipliers[3] = { 1.0f, 1.25f, 1.60f };
	const float DamageMult = ComboMultipliers[CurrentComboIndex - 1];
	const float EffectiveRadius = BaseMeleeRadius * (1.0f + ((CurrentComboIndex - 1) * 0.15f));

	// 3. Increment attack instance ID & clear hit set (anti-double-hit guard)
	const int32 AttackId = ++NextAttackInstanceId;
	HitActorsThisAttack.Empty();

	const FVector Start = OwnerActor->GetActorLocation();
	const FVector Forward = OwnerActor->GetActorForwardVector();
	const FVector End = Start + (Forward * BaseMeleeRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<FHitResult> HitResults;
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(EffectiveRadius);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_WorldDynamic,
		SweepShape,
		QueryParams
	);

	const float AttackerPower = Attr ? Attr->AttackPower : 20.0f;
	const float BaseRawDamage = AttackerPower * DamageMult;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || HitActorsThisAttack.Contains(HitActor))
			{
				continue; // Already hit by this swing!
			}
			HitActorsThisAttack.Add(HitActor);

			// Check harvestable node
			AAstrawildHarvestableNode* HarvestNode = Cast<AAstrawildHarvestableNode>(HitActor);
			if (HarvestNode)
			{
				int32 Yield = 0;
				HarvestNode->Harvest(DamageMult, EAstrawildHarvestType::Foraging, OwnerActor, Yield);
				continue;
			}

			// Build Damage Event
			FAstrawildDamageEvent DamageEvent;
			DamageEvent.BaseDamage = BaseRawDamage;
			DamageEvent.DamageElement = DamageElement;
			DamageEvent.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Melee"), false);
			DamageEvent.DamageCauser = OwnerActor;
			DamageEvent.InstigatorActor = OwnerActor;
			DamageEvent.HitLocation = Hit.ImpactPoint;
			DamageEvent.HitDirection = Forward;
			DamageEvent.KnockbackImpulse = 400.0f * DamageMult;
			DamageEvent.AppliedStatusTag = StatusToApply;
			DamageEvent.StatusDuration = StatusDuration;
			DamageEvent.AttackInstanceId = AttackId;

			ApplyDamageEvent(HitActor, DamageEvent);
		}
	}

	return true;
}

float UAstrawildCombatComponent::ApplyDamageEvent(AActor* TargetActor, const FAstrawildDamageEvent& DamageEvent)
{
	if (!TargetActor)
	{
		return 0.0f;
	}

	// 1. If target implements IAstrawildDamageableInterface, route authoritatively
	if (TargetActor->Implements<UAstrawildDamageableInterface>())
	{
		const float Dealt = IAstrawildDamageableInterface::Execute_TakeAstrawildDamage(TargetActor, DamageEvent);
		OnDamageDealt.Broadcast(TargetActor, Dealt, DamageEvent.DamageElement, false, CurrentComboIndex);
		return Dealt;
	}

	// 2. Fallback to direct UAstrawildAttributeComponent calculation
	return ApplyDamageToTarget(TargetActor, DamageEvent.BaseDamage, DamageEvent.DamageElement, DamageEvent.InstigatorActor.Get());
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

	// 1. Armor mitigation: 100 / (100 + Defense)
	const float Defense = FMath::Max(0.0f, TargetAttr->DefensePower);
	const float DefenseFactor = 100.0f / (100.0f + Defense);

	// 2. Elemental multiplier
	const float ElementMultiplier = FAstrawildElementalMatrix::GetMultiplier(DamageElement, TargetAttr->ElementalAffinity);

	// 3. Critical strike
	const bool bIsCrit = FMath::RandRange(1.0f, 100.0f) <= CriticalHitChancePercent;
	const float CritFactor = bIsCrit ? CriticalMultiplier : 1.0f;

	// 4. Random variance (+- 6%)
	const float Variance = FMath::RandRange(0.94f, 1.06f);

	const float FinalDamage = FMath::Max(1.0f, RawDamage * DefenseFactor * ElementMultiplier * CritFactor * Variance);

	TargetAttr->ModifyHealth(-FinalDamage, DamageDealer);

	UE_LOG(LogAstrawildCombat, Log, TEXT("%s dealt %.1f [%s] damage to %s (Crit: %d, Multiplier: %.2f)"),
		DamageDealer ? *DamageDealer->GetName() : TEXT("Unknown"),
		FinalDamage,
		*UEnum::GetValueAsString(DamageElement),
		*TargetActor->GetName(),
		bIsCrit,
		ElementMultiplier);

	OnDamageDealt.Broadcast(TargetActor, FinalDamage, DamageElement, bIsCrit, CurrentComboIndex);

	UAstrawildCombatComponent* TargetCombat = TargetActor->FindComponentByClass<UAstrawildCombatComponent>();
	if (TargetCombat)
	{
		TargetCombat->OnDamageReceived.Broadcast(DamageDealer, FinalDamage, DamageElement, bIsCrit);
	}

	return FinalDamage;
}

void UAstrawildCombatComponent::ShowAttackTelegraph(const FVector& Origin, float Radius, float DurationSeconds, const FColor& TelegraphColor)
{
	UWorld* World = GetWorld();
	if (World)
	{
		DrawDebugCircle(World, Origin + FVector(0, 0, 5.0f), Radius, 32, TelegraphColor, false, DurationSeconds, 0, 4.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		UE_LOG(LogAstrawildCombat, Log, TEXT("Displayed Attack Telegraph at %s (Radius: %.1f, Duration: %.2fs)"), *Origin.ToString(), Radius, DurationSeconds);
	}
}