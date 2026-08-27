// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildQuestComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AstrawildFeedbackComponent.h"
#include "Components/AstrawildSanComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"

AAstrawildEchoBase::AAstrawildEchoBase()
	: CurrentState(EAstrawildEchoState::WildPassive)
{
	PrimaryActorTick.bCanEverTick = true;

	Attributes = CreateDefaultSubobject<UAstrawildAttributeComponent>(TEXT("Attributes"));
	Combat = CreateDefaultSubobject<UAstrawildCombatComponent>(TEXT("Combat"));
	Feedback = CreateDefaultSubobject<UAstrawildFeedbackComponent>(TEXT("Feedback"));
	San = CreateDefaultSubobject<UAstrawildSanComponent>(TEXT("San"));

	FallbackMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackMeshComponent"));
	FallbackMeshComponent->SetupAttachment(RootComponent);
	FallbackMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	FallbackMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 450.0f;
}

void AAstrawildEchoBase::BeginPlay()
{
	Super::BeginPlay();

	if (Attributes)
	{
		Attributes->OnDeath.AddDynamic(this, &AAstrawildEchoBase::HandleDeath);
		Attributes->OnHealthChanged.AddDynamic(this, &AAstrawildEchoBase::HandleHealthChanged);
	}

	if (SpeciesData)
	{
		InitializeFromSpeciesData(SpeciesData, InstanceData.Level > 0 ? InstanceData.Level : 1);
	}
	else
	{
		UE_LOG(LogAstrawildEcho, Warning, TEXT("Echo %s spawned without SpeciesData! Using safe default fallback parameters."), *GetName());
		ApplyVisualRepresentation();
	}
}

void AAstrawildEchoBase::InitializeFromSpeciesData(UAstrawildEchoDataAsset* InData, int32 InLevel)
{
	if (!InData)
	{
		UE_LOG(LogAstrawildEcho, Error, TEXT("InitializeFromSpeciesData called with null data asset on %s!"), *GetName());
		return;
	}

	SpeciesData = InData;
	InstanceData = InData->CreateInstance(InLevel, InstanceData.CustomNickname);
	const EAstrawildElement PrimaryElement = InData->ElementalAffinities.Num() > 0 ? InData->ElementalAffinities[0] : InData->ElementalAffinity;

	if (Attributes)
	{
		Attributes->MaxHealth = InstanceData.MaxHealth;
		Attributes->CurrentHealth = InstanceData.CurrentHealth;
		Attributes->AttackPower = InstanceData.AttackPower;
		Attributes->DefensePower = InstanceData.DefensePower;
		Attributes->ElementalAffinity = PrimaryElement;
		InstanceData.Element = PrimaryElement;
		Attributes->Level = InstanceData.Level;
	}

	if (San)
	{
		San->MaxSAN = InstanceData.MaxSAN;
		San->CurrentSAN = InstanceData.CurrentSAN;
		San->RecoveryPerSecond = InstanceData.SANRecoveryRate;
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = InData->BaseWalkSpeed;
	}

	ApplyVisualRepresentation();

			UE_LOG(LogAstrawildEcho, Log, TEXT("Initialized Echo %s: %s (Role: %s, Element: %s, Lv: %d, MaxHP: %.0f)"),
			*GetName(), *InData->SpeciesName.ToString(), *UEnum::GetValueAsString(InData->Role),
			*UEnum::GetValueAsString(PrimaryElement), InstanceData.Level, InstanceData.MaxHealth);

}

void AAstrawildEchoBase::ApplyVisualRepresentation()
{
	if (SpeciesData && !SpeciesData->SkeletalMesh.IsNull())
	{
		if (USkeletalMesh* LoadedSkeletalMesh = SpeciesData->SkeletalMesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(LoadedSkeletalMesh);
			GetMesh()->SetVisibility(true);
			if (!SpeciesData->AnimationBlueprintClass.IsNull())
			{
				if (UClass* LoadedAnimClass = SpeciesData->AnimationBlueprintClass.LoadSynchronous())
				{
					GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
				}
			}
			if (FallbackMeshComponent)
			{
				FallbackMeshComponent->SetVisibility(false);
			}
			return;
		}
	}

	if (FallbackMeshComponent)
	{
		FallbackMeshComponent->SetVisibility(true);
	}

	// Check if custom static mesh exists
	if (SpeciesData && !SpeciesData->FallbackStaticMesh.IsNull())
	{
		UStaticMesh* LoadedMesh = SpeciesData->FallbackStaticMesh.LoadSynchronous();
		if (LoadedMesh && FallbackMeshComponent)
		{
			FallbackMeshComponent->SetStaticMesh(LoadedMesh);
			return;
		}
	}

	// Fallback to engine basic cylinder / sphere
	if (FallbackMeshComponent && !FallbackMeshComponent->GetStaticMesh())
	{
		UStaticMesh* BasicCylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (BasicCylinder)
		{
			FallbackMeshComponent->SetStaticMesh(BasicCylinder);
			FallbackMeshComponent->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.9f));
		}
	}
}

void AAstrawildEchoBase::SetEchoState(EAstrawildEchoState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
	InstanceData.OwnershipState = (NewState == EAstrawildEchoState::SummonedCompanion) ? EAstrawildEchoOwnership::TamedCompanion :
		(NewState == EAstrawildEchoState::Working) ? EAstrawildEchoOwnership::CampWorker : EAstrawildEchoOwnership::Wild;

	UE_LOG(LogAstrawildEcho, Log, TEXT("Echo %s transitioned to state: %s"), *GetName(), *UEnum::GetValueAsString(NewState));

	if (SpeciesData && GetCharacterMovement())
	{
		if (CurrentState == EAstrawildEchoState::WildHostile || CurrentState == EAstrawildEchoState::Fleeing)
		{
			GetCharacterMovement()->MaxWalkSpeed = SpeciesData->BaseRunSpeed;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = SpeciesData->BaseWalkSpeed;
		}
	}

	OnEchoStateChanged.Broadcast(this, NewState);
}

bool AAstrawildEchoBase::CastAbility(int32 AbilityIndex, AActor* TargetActor)
{
	if (!SpeciesData || !SpeciesData->InnateAbilities.IsValidIndex(AbilityIndex) || !TargetActor)
	{
		return false;
	}

	const FAstrawildEchoAbility& Ability = SpeciesData->InnateAbilities[AbilityIndex];
	if (!Combat)
	{
		return false;
	}

	UE_LOG(LogAstrawildCombat, Log, TEXT("Echo %s cast ability: %s"), *GetName(), *Ability.AbilityName.ToString());
	Combat->ApplyDamageToTarget(TargetActor, Ability.BaseDamage, Ability.Element, this);
	return true;
}

bool AAstrawildEchoBase::ActivateRolePerk()
{
	return ActivatePartnerSkill();
}

bool AAstrawildEchoBase::ActivatePartnerSkill()
{
	if (!SpeciesData)
	{
		return false;
	}

	const FGameplayTag SkillTag = InstanceData.PartnerSkillTag.IsValid() ? InstanceData.PartnerSkillTag : SpeciesData->PartnerSkillTag;
	const FString SkillName = SkillTag.ToString();
	if (SkillName.StartsWith(TEXT("Partner."), ESearchCase::CaseSensitive))
	{
		if (SkillName.Contains(TEXT("Rush")) || SkillName.Contains(TEXT("Charge")) || SkillName.Contains(TEXT("Lunge")) || SkillName.Contains(TEXT("Redline")))
		{
			if (Attributes)
			{
				Attributes->AttackPower *= 1.15f;
			}
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated partner combat skill [%s]."), *GetName(), *SkillName);
		}
		else if (SkillName.Contains(TEXT("Mend")) || SkillName.Contains(TEXT("Renewal")) || SkillName.Contains(TEXT("Drain")) || SkillName.Contains(TEXT("Ward")))
		{
			if (Attributes)
			{
				Attributes->ModifyHealth(FMath::Max(5.0f, Attributes->MaxHealth * 0.1f), this);
			}
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated partner sustain skill [%s]."), *GetName(), *SkillName);
		}
		else
		{
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated partner utility skill [%s]."), *GetName(), *SkillName);
		}
	}
	else
	{
		// Safe fallback for legacy assets that have no PartnerSkillTag.
		switch (SpeciesData->Role)
		{
		case EAstrawildEchoRole::Exploration:
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated legacy exploration pulse."), *GetName());
			break;
		case EAstrawildEchoRole::Combat:
			if (Attributes)
			{
				Attributes->DefensePower *= 1.5f;
			}
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated legacy bastion shield."), *GetName());
			break;
		case EAstrawildEchoRole::BaseUtility:
			UE_LOG(LogAstrawildEcho, Log, TEXT("%s activated legacy productivity aura at %.1fx."), *GetName(), SpeciesData->WorkEfficiencyMultiplier * 1.5f);
			break;
		}
	}

	OnRoleAbilityUsed.Broadcast(this, SpeciesData->Role);
	return true;
}

FAstrawildEchoInstance AAstrawildEchoBase::ExportCapturedData() const
{
	FAstrawildEchoInstance Data = InstanceData;
	if (Attributes)
	{
		Data.CurrentHealth = Attributes->CurrentHealth;
		Data.MaxHealth = Attributes->MaxHealth;
		Data.Level = Attributes->Level;
		Data.CurrentEXP = Attributes->CurrentEXP;
		Data.AttackPower = Attributes->AttackPower;
		Data.DefensePower = Attributes->DefensePower;
		Data.Element = Attributes->ElementalAffinity;
	}
	if (San)
	{
		Data.MaxSAN = San->MaxSAN;
		Data.CurrentSAN = San->CurrentSAN;
		Data.SANRecoveryRate = San->RecoveryPerSecond;
	}
	return Data;
}

void AAstrawildEchoBase::ImportCapturedData(const FAstrawildEchoInstance& Data)
{
	InstanceData = Data;
	if (Attributes)
	{
		Attributes->Level = Data.Level;
		Attributes->CurrentEXP = Data.CurrentEXP;
		Attributes->CurrentHealth = Data.CurrentHealth;
		Attributes->MaxHealth = Data.MaxHealth;
		Attributes->AttackPower = Data.AttackPower;
		Attributes->DefensePower = Data.DefensePower;
		Attributes->ElementalAffinity = Data.Element;
	}
	if (San)
	{
		San->MaxSAN = FMath::Max(1.0f, Data.MaxSAN);
		San->CurrentSAN = FMath::Clamp(Data.CurrentSAN, 0.0f, San->MaxSAN);
		San->RecoveryPerSecond = FMath::Max(0.0f, Data.SANRecoveryRate);
	}
}

void AAstrawildEchoBase::HandleDeath(AActor* DeadActor)
{
	UE_LOG(LogAstrawildEcho, Log, TEXT("Echo %s has fainted."), *GetName());
	SetEchoState(EAstrawildEchoState::WildPassive);
	SetActorEnableCollision(false);
}

void AAstrawildEchoBase::HandleHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* Instigator)
{
	if (Delta < 0.0f && CurrentState == EAstrawildEchoState::WildPassive && Instigator)
	{
		SetEchoState(EAstrawildEchoState::WildHostile);
	}
	else if (CurrentState == EAstrawildEchoState::WildHostile && (CurrentHealth / MaxHealth) < 0.20f)
	{
		SetEchoState(EAstrawildEchoState::Fleeing);
	}
}

FText AAstrawildEchoBase::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	const FString SpeciesStr = SpeciesData ? SpeciesData->SpeciesName.ToString() : TEXT("Echo");
	const FString ElementStr = SpeciesData ? UEnum::GetValueAsString(SpeciesData->ElementalAffinity) : TEXT("Neutral");
	const FString RoleStr = SpeciesData ? UEnum::GetValueAsString(SpeciesData->Role) : TEXT("Combat");
	const float HPPct = Attributes ? (Attributes->GetHealthPercent() * 100.0f) : 100.0f;

	if (CurrentState == EAstrawildEchoState::SummonedCompanion)
	{
		return FText::FromString(FString::Printf(TEXT("[E] Pet & Command %s (Trust: %.0f%%)"), *SpeciesStr, InstanceData.TrustScore));
	}

	return FText::FromString(FString::Printf(TEXT("Wild %s [Lv.%d | %s | HP: %.0f%%]"), *SpeciesStr, InstanceData.Level, *RoleStr, HPPct));
}

bool AAstrawildEchoBase::CanInteract_Implementation(AActor* Interactor)
{
	return Attributes && Attributes->IsAlive() && Interactor != nullptr;
}

bool AAstrawildEchoBase::PerformInteraction_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (CurrentState == EAstrawildEchoState::SummonedCompanion)
	{
		// Pet / Bond interaction: boosts trust
		InstanceData.TrustScore = FMath::Clamp(InstanceData.TrustScore + 5.0f, 0.0f, 100.0f);
		ActivateRolePerk();
		UE_LOG(LogAstrawildEcho, Log, TEXT("Bonded with companion %s. Trust increased to %.1f%%!"), *GetName(), InstanceData.TrustScore);
		return true;
	}

	UE_LOG(LogAstrawildEcho, Log, TEXT("Inspected wild Echo: %s"), *GetName());
	return true;
}

float AAstrawildEchoBase::TakeAstrawildDamage_Implementation(const FAstrawildDamageEvent& DamageEvent)
{
	if (!CanTakeDamage_Implementation(DamageEvent.InstigatorActor.Get()) || !Attributes)
	{
		return 0.0f;
	}

	// 1. Armor mitigation: 100 / (100 + Defense)
	const float Defense = FMath::Max(0.0f, Attributes->DefensePower);
	const float DefenseFactor = 100.0f / (100.0f + Defense);

	// 2. Elemental Matrix calculation
	const float ElementMult = FAstrawildElementalMatrix::GetMultiplier(DamageEvent.DamageElement, Attributes->ElementalAffinity);

	// 3. Final damage
	const float FinalDamage = FMath::Max(1.0f, DamageEvent.BaseDamage * DefenseFactor * ElementMult);
	Attributes->ModifyHealth(-FinalDamage, DamageEvent.InstigatorActor.Get());
	if (Attributes->CurrentHealth <= 0.0f && DamageEvent.InstigatorActor.IsValid())
	{
		if (UAstrawildQuestComponent* Quest = DamageEvent.InstigatorActor->FindComponentByClass<UAstrawildQuestComponent>())
		{
			Quest->AddProgressForTarget(EAstrawildQuestObjectiveType::Defeat, InstanceData.SpeciesTag, 1);
		}
	}

	// 4. Knockback
	if (DamageEvent.KnockbackImpulse > 0.0f)
	{
		LaunchCharacter(DamageEvent.HitDirection * DamageEvent.KnockbackImpulse, true, true);
	}

	// 5. Apply status effect if attached
	if (DamageEvent.AppliedStatusTag.IsValid() && DamageEvent.StatusDuration > 0.0f)
	{
		Attributes->ApplyStatusEffect(DamageEvent.AppliedStatusTag, DamageEvent.StatusDuration, 1.0f, DamageEvent.InstigatorActor.Get());
	}

	// 6. Agro response if wild
	if (CurrentState == EAstrawildEchoState::WildPassive && DamageEvent.InstigatorActor.IsValid())
	{
		SetEchoState(EAstrawildEchoState::WildHostile);
	}

	UE_LOG(LogAstrawildCombat, Log, TEXT("[DAMAGEABLE] Echo %s took %.1f [%s] damage from %s (HP: %.0f/%.0f)"),
		*GetName(), FinalDamage, *UEnum::GetValueAsString(DamageEvent.DamageElement),
		DamageEvent.InstigatorActor.IsValid() ? *DamageEvent.InstigatorActor->GetName() : TEXT("Unknown"),
		Attributes->CurrentHealth, Attributes->MaxHealth);

	return FinalDamage;
}

bool AAstrawildEchoBase::CanTakeDamage_Implementation(AActor* Attacker)
{
	return Attributes && Attributes->IsAlive();
}

EAstrawildElement AAstrawildEchoBase::GetElementalAffinity_Implementation() const
{
	return Attributes ? Attributes->ElementalAffinity : EAstrawildElement::Neutral;
}