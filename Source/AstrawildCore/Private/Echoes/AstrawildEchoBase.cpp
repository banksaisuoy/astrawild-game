// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AstrawildLogChannels.h"

AAstrawildEchoBase::AAstrawildEchoBase()
	: CurrentState(EAstrawildEchoState::WildPassive)
	, CapturedEchoGuid(FGuid::NewGuid())
{
	PrimaryActorTick.bCanEverTick = true;

	Attributes = CreateDefaultSubobject<UAstrawildAttributeComponent>(TEXT("Attributes"));
	Combat = CreateDefaultSubobject<UAstrawildCombatComponent>(TEXT("Combat"));

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
		InitializeFromSpeciesData(SpeciesData);
	}
}

void AAstrawildEchoBase::InitializeFromSpeciesData(UAstrawildEchoDataAsset* InData)
{
	if (!InData)
	{
		return;
	}

	SpeciesData = InData;
	if (Attributes)
	{
		Attributes->MaxHealth = InData->BaseMaxHealth;
		Attributes->CurrentHealth = InData->BaseMaxHealth;
		Attributes->AttackPower = InData->BaseAttackPower;
		Attributes->DefensePower = InData->BaseDefensePower;
		Attributes->ElementalAffinity = InData->ElementalAffinity;
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = InData->BaseWalkSpeed;
	}

	if (Nickname.IsEmpty())
	{
		Nickname = InData->SpeciesName;
	}

	UE_LOG(LogAstrawildEcho, Log, TEXT("Initialized Echo %s [%s] with MaxHP: %.1f"), *GetName(), *InData->SpeciesName.ToString(), InData->BaseMaxHealth);
}

void AAstrawildEchoBase::SetEchoState(EAstrawildEchoState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	CurrentState = NewState;
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

FAstrawildCapturedEchoData AAstrawildEchoBase::ExportCapturedData() const
{
	FAstrawildCapturedEchoData Data;
	Data.UniqueEchoId = CapturedEchoGuid;
	Data.CustomNickname = Nickname;

	if (SpeciesData)
	{
		Data.SpeciesTag = SpeciesData->SpeciesTag;
		Data.Element = SpeciesData->ElementalAffinity;
	}

	if (Attributes)
	{
		Data.Level = Attributes->Level;
		Data.CurrentEXP = Attributes->CurrentEXP;
		Data.CurrentHealth = Attributes->CurrentHealth;
		Data.MaxHealth = Attributes->MaxHealth;
		Data.AttackPower = Attributes->AttackPower;
		Data.DefensePower = Attributes->DefensePower;
	}

	return Data;
}

void AAstrawildEchoBase::ImportCapturedData(const FAstrawildCapturedEchoData& Data)
{
	CapturedEchoGuid = Data.UniqueEchoId;
	Nickname = Data.CustomNickname;

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
		// Provoked -> Enter Hostile state
		SetEchoState(EAstrawildEchoState::WildHostile);
	}
	else if (CurrentState == EAstrawildEchoState::WildHostile && (CurrentHealth / MaxHealth) < 0.20f)
	{
		// Low health -> Try to flee
		SetEchoState(EAstrawildEchoState::Fleeing);
	}
}