// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildCaptureComponent.h"
#include "Echoes/AstrawildCaptureProjectile.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Echoes/AstrawildEchoAIController.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UAstrawildCaptureComponent::UAstrawildCaptureComponent()
	: SelectedPartyIndex(0)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UAstrawildCaptureComponent::ThrowResonator(float Power)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld() || !ProjectileClass)
	{
		return false;
	}

	// Verify player has Resonator in inventory
	UAstrawildInventoryComponent* Inv = OwnerActor->FindComponentByClass<UAstrawildInventoryComponent>();
	const FGameplayTag ResonatorTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);
	if (Inv && !Inv->HasItem(ResonatorTag, 1))
	{
		UE_LOG(LogAstrawildEcho, Warning, TEXT("Cannot throw Resonator: No Resonators in inventory!"));
		return false;
	}

	if (Inv)
	{
		Inv->RemoveItem(ResonatorTag, 1);
	}

	FVector EyeLoc;
	FRotator EyeRot;
	ACharacter* Char = Cast<ACharacter>(OwnerActor);
	if (Char && Char->GetController())
	{
		Char->GetController()->GetPlayerViewPoint(EyeLoc, EyeRot);
	}
	else
	{
		EyeLoc = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
		EyeRot = OwnerActor->GetActorRotation();
	}

	const FVector SpawnLoc = EyeLoc + (EyeRot.Vector() * 80.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAstrawildCaptureProjectile* Proj = GetWorld()->SpawnActor<AAstrawildCaptureProjectile>(ProjectileClass, SpawnLoc, EyeRot, SpawnParams);
	if (Proj)
	{
		Proj->ResonatorPower = Power;
		UE_LOG(LogAstrawildEcho, Log, TEXT("Threw Astra Resonator with Power %.2f"), Power);
		return true;
	}

	return false;
}

bool UAstrawildCaptureComponent::AttemptCapture(AAstrawildEchoBase* TargetEcho, float ResonatorPower, int32& OutShakesCompleted)
{
	OutShakesCompleted = 0;
	if (!TargetEcho || !TargetEcho->Attributes)
	{
		return false;
	}

	if (TargetEcho->CurrentState == EAstrawildEchoState::Captured || TargetEcho->CurrentState == EAstrawildEchoState::SummonedCompanion)
	{
		UE_LOG(LogAstrawildEcho, Warning, TEXT("Cannot capture Echo: Already captured/companion."));
		return false;
	}

	const float HealthPercent = TargetEcho->Attributes->GetHealthPercent();
	const float SpeciesModifier = TargetEcho->SpeciesData ? TargetEcho->SpeciesData->CaptureDifficultyModifier : 1.0f;

	// Capture Formula: (1.0 - HP%) * ResonatorPower * SpeciesModifier
	// Lower HP -> Higher capture probability
	float CaptureChance = (1.0f - (HealthPercent * 0.85f)) * ResonatorPower * SpeciesModifier;
	CaptureChance = FMath::Clamp(CaptureChance, 0.05f, 0.98f);

	const float Roll = FMath::FRand();

	// Calculate shake milestones
	if (Roll < CaptureChance * 0.33f)
	{
		OutShakesCompleted = 1;
	}
	if (Roll < CaptureChance * 0.66f)
	{
		OutShakesCompleted = 2;
	}
	if (Roll <= CaptureChance)
	{
		OutShakesCompleted = 3;
	}

	if (Roll <= CaptureChance)
	{
		// Capture Success
		FAstrawildCapturedEchoData CapturedData = TargetEcho->ExportCapturedData();
		AddCapturedEcho(CapturedData);

		UE_LOG(LogAstrawildEcho, Log, TEXT("CAPTURE SUCCESS! Captured %s (HP: %.1f, Chance: %.1f%%)"),
			*CapturedData.SpeciesTag.ToString(), HealthPercent * 100.0f, CaptureChance * 100.0f);

		const int32 SlotIndex = ActiveParty.Num() - 1;
		OnCaptureSuccess.Broadcast(TargetEcho, CapturedData, SlotIndex);

		TargetEcho->SetEchoState(EAstrawildEchoState::Captured);
		TargetEcho->Destroy();
		return true;
	}
	else
	{
		// Breakout
		UE_LOG(LogAstrawildEcho, Log, TEXT("CAPTURE FAILED! Echo broke out (Shakes: %d, Chance: %.1f%%)"), OutShakesCompleted, CaptureChance * 100.0f);
		TargetEcho->SetEchoState(EAstrawildEchoState::WildHostile);
		OnCaptureFailed.Broadcast(TargetEcho, OutShakesCompleted);
		return false;
	}
}

void UAstrawildCaptureComponent::AddCapturedEcho(const FAstrawildCapturedEchoData& InData)
{
	if (ActiveParty.Num() < 5)
	{
		ActiveParty.Add(InData);
	}
	else
	{
		ReserveStorage.Add(InData);
	}
}

bool UAstrawildCaptureComponent::SummonSelectedCompanion()
{
	if (!ActiveParty.IsValidIndex(SelectedPartyIndex) || !GetWorld())
	{
		return false;
	}

	RecallActiveCompanion();

	const FAstrawildCapturedEchoData& EchoData = ActiveParty[SelectedPartyIndex];
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	const FVector SpawnLoc = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 200.0f);
	const FRotator SpawnRot = OwnerActor->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Default fallback to AstrawildEchoBase
	AAstrawildEchoBase* Companion = GetWorld()->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), SpawnLoc, SpawnRot, SpawnParams);
	if (Companion)
	{
		Companion->ImportCapturedData(EchoData);
		Companion->SetEchoState(EAstrawildEchoState::SummonedCompanion);

		AAstrawildEchoAIController* AIC = Cast<AAstrawildEchoAIController>(Companion->GetController());
		if (AIC)
		{
			AIC->SetFollowLeader(OwnerActor);
		}

		ActiveSummonedEcho = Companion;
		UE_LOG(LogAstrawildEcho, Log, TEXT("Summoned companion: %s"), *EchoData.SpeciesTag.ToString());
		OnEchoSummoned.Broadcast(Companion);
		return true;
	}

	return false;
}

void UAstrawildCaptureComponent::RecallActiveCompanion()
{
	if (ActiveSummonedEcho.IsValid())
	{
		// Save current health back to party
		if (ActiveParty.IsValidIndex(SelectedPartyIndex))
		{
			ActiveParty[SelectedPartyIndex].CurrentHealth = ActiveSummonedEcho->Attributes->CurrentHealth;
		}

		UE_LOG(LogAstrawildEcho, Log, TEXT("Recalled companion: %s"), *ActiveSummonedEcho->GetName());
		ActiveSummonedEcho->Destroy();
		ActiveSummonedEcho = nullptr;
		OnEchoRecalled.Broadcast();
	}
}

void UAstrawildCaptureComponent::SelectNextPartySlot()
{
	if (ActiveParty.Num() > 0)
	{
		SelectedPartyIndex = (SelectedPartyIndex + 1) % ActiveParty.Num();
	}
}

void UAstrawildCaptureComponent::SelectPrevPartySlot()
{
	if (ActiveParty.Num() > 0)
	{
		SelectedPartyIndex = (SelectedPartyIndex - 1 + ActiveParty.Num()) % ActiveParty.Num();
	}
}

void UAstrawildCaptureComponent::LoadPartyData(const TArray<FAstrawildCapturedEchoData>& InParty, const TArray<FAstrawildCapturedEchoData>& InStorage)
{
	RecallActiveCompanion();
	ActiveParty = InParty;
	ReserveStorage = InStorage;
	SelectedPartyIndex = 0;
}