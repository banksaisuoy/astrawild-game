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
	: MaxCaptureRange(1800.0f)
	, SelectedPartyIndex(0)
	, CurrentCaptureState(EAstrawildCaptureState::None)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UAstrawildCaptureComponent::ThrowResonator(float Power, FGameplayTag ResonatorItemTag)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld())
	{
		return false;
	}

	// 1. Determine Resonator tag
	FGameplayTag ActualTag = ResonatorItemTag.IsValid() ? ResonatorItemTag : FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);

	// 2. Verify inventory
	UAstrawildInventoryComponent* Inv = OwnerActor->FindComponentByClass<UAstrawildInventoryComponent>();
	if (Inv && !Inv->HasItem(ActualTag, 1))
	{
		LastCaptureFeedback = FText::FromString(TEXT("Capture Failed: Out of Astra Resonators in inventory!"));
		OnCaptureFeedback.Broadcast(LastCaptureFeedback, false);
		UE_LOG(LogAstrawildEcho, Warning, TEXT("Cannot throw Resonator: No %s in inventory!"), *ActualTag.ToString());
		return false;
	}

	if (Inv)
	{
		Inv->RemoveItem(ActualTag, 1);
	}

	// 3. Spawn projectile
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

	TSubclassOf<AAstrawildCaptureProjectile> SpawnClass = ProjectileClass ? ProjectileClass : AAstrawildCaptureProjectile::StaticClass();
	AAstrawildCaptureProjectile* Proj = GetWorld()->SpawnActor<AAstrawildCaptureProjectile>(SpawnClass, SpawnLoc, EyeRot, SpawnParams);
	if (Proj)
	{
		Proj->ResonatorPower = Power;
		CurrentCaptureState = EAstrawildCaptureState::InFlight;
		UE_LOG(LogAstrawildEcho, Log, TEXT("Threw Astra Resonator [%s] with Power %.2f"), *ActualTag.ToString(), Power);
		return true;
	}

	return false;
}

bool UAstrawildCaptureComponent::ValidateCapturePrerequisites(AAstrawildEchoBase* TargetEcho, FText& OutFailureReason) const
{
	if (!TargetEcho)
	{
		OutFailureReason = FText::FromString(TEXT("Target is invalid."));
		return false;
	}

	if (!TargetEcho->Attributes || !TargetEcho->Attributes->IsAlive())
	{
		OutFailureReason = FText::FromString(TEXT("Target is fainted or already dead."));
		return false;
	}

	if (TargetEcho->CurrentState == EAstrawildEchoState::Captured || TargetEcho->CurrentState == EAstrawildEchoState::SummonedCompanion)
	{
		OutFailureReason = FText::FromString(TEXT("Echo is already tamed/captured."));
		return false;
	}

	if (TargetEcho->bIsUndergoingCapture)
	{
		OutFailureReason = FText::FromString(TEXT("Echo is already undergoing resonance capture by another attempt!"));
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && FVector::Dist(OwnerActor->GetActorLocation(), TargetEcho->GetActorLocation()) > MaxCaptureRange)
	{
		OutFailureReason = FText::FromString(TEXT("Echo is out of resonance capture range."));
		return false;
	}

	return true;
}

float UAstrawildCaptureComponent::CalculateCaptureProbability(AAstrawildEchoBase* TargetEcho, float ResonatorPower) const
{
	if (!TargetEcho || !TargetEcho->Attributes)
	{
		return 0.0f;
	}

	const float HealthPercent = TargetEcho->Attributes->GetHealthPercent();
	const float SpeciesModifier = TargetEcho->SpeciesData ? TargetEcho->SpeciesData->CaptureDifficultyModifier : 1.0f;

	// Status condition bonus (+20% bonus if target is affected by any status)
	float StatusBonus = 1.0f;
	if (TargetEcho->Attributes->ActiveStatusEffects.Num() > 0)
	{
		StatusBonus = 1.25f;
	}

	// Capture Formula: ((1.0 - HP%*0.75) * ResonatorPower * StatusBonus) / SpeciesModifier
	float CaptureChance = (1.0f - (HealthPercent * 0.75f)) * ResonatorPower * StatusBonus * SpeciesModifier;
	return FMath::Clamp(CaptureChance, 0.08f, 0.98f);
}

float UAstrawildCaptureComponent::CalculateInitialTrust(AAstrawildEchoBase* TargetEcho) const
{
	if (!TargetEcho || !TargetEcho->Attributes)
	{
		return 50.0f;
	}

	const float HealthPercent = TargetEcho->Attributes->GetHealthPercent();
	const bool bHasStatus = TargetEcho->Attributes->ActiveStatusEffects.Num() > 0;

	// Gentle capture (high HP + status trick) -> High trust
	if (HealthPercent >= 0.45f && bHasStatus)
	{
		return 75.0f; // Compassionate Resonance
	}

	// Brutal low health capture (< 15%) -> Lower initial trust
	if (HealthPercent <= 0.15f)
	{
		return 35.0f; // Wary / Traumatized
	}

	return 50.0f; // Baseline balance
}

bool UAstrawildCaptureComponent::AttemptCapture(AAstrawildEchoBase* TargetEcho, float ResonatorPower, int32& OutShakesCompleted, FText& OutStatusReason)
{
	OutShakesCompleted = 0;

	// 1. Authoritative Validation & Lock (Anti-Exploit / Double Request Guard)
	if (!ValidateCapturePrerequisites(TargetEcho, OutStatusReason))
	{
		CurrentCaptureState = EAstrawildCaptureState::Breakout;
		LastCaptureFeedback = OutStatusReason;
		OnCaptureFeedback.Broadcast(OutStatusReason, false);
		return false;
	}

	// Acquire lock
	TargetEcho->bIsUndergoingCapture = true;
	CurrentCaptureState = EAstrawildCaptureState::Rolling;

	// 2. Calculate Probability
	const float CaptureChance = CalculateCaptureProbability(TargetEcho, ResonatorPower);
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
		// 3. CAPTURE SUCCESS
		CurrentCaptureState = EAstrawildCaptureState::Success;

		FAstrawildCapturedEchoData CapturedData = TargetEcho->ExportCapturedData();
		CapturedData.UniqueEchoId = FGuid::NewGuid();
		CapturedData.OwnershipState = EAstrawildEchoOwnership::TamedCompanion;
		CapturedData.TrustScore = CalculateInitialTrust(TargetEcho);

		AddCapturedEcho(CapturedData);

		const FString SpeciesName = TargetEcho->SpeciesData ? TargetEcho->SpeciesData->SpeciesName.ToString() : TEXT("Echo");
		LastCaptureFeedback = FText::FromString(FString::Printf(TEXT("Resonance Capture Success! %s joined your party (Trust: %.0f%%)."),
			*SpeciesName, CapturedData.TrustScore));

		UE_LOG(LogAstrawildEcho, Log, TEXT("CAPTURE SUCCESS: %s captured with Stable GUID %s (Initial Trust: %.1f%%)"),
			*SpeciesName, *CapturedData.UniqueEchoId.ToString(), CapturedData.TrustScore);

		const int32 SlotIndex = ActiveParty.Num() - 1;
		OnCaptureSuccess.Broadcast(TargetEcho, CapturedData, SlotIndex);
		OnCaptureFeedback.Broadcast(LastCaptureFeedback, true);

		TargetEcho->SetEchoState(EAstrawildEchoState::Captured);
		TargetEcho->Destroy();
		return true;
	}
	else
	{
		// 4. BREAKOUT / FAILED
		TargetEcho->bIsUndergoingCapture = false;
		CurrentCaptureState = EAstrawildCaptureState::Breakout;

		const FString SpeciesName = TargetEcho->SpeciesData ? TargetEcho->SpeciesData->SpeciesName.ToString() : TEXT("Echo");
		OutStatusReason = FText::FromString(FString::Printf(TEXT("%s broke out of the resonance field! (Shakes: %d)"), *SpeciesName, OutShakesCompleted));
		LastCaptureFeedback = OutStatusReason;

		UE_LOG(LogAstrawildEcho, Log, TEXT("CAPTURE FAILED: %s broke out (Shakes: %d, Chance: %.1f%%)"),
			*SpeciesName, OutShakesCompleted, CaptureChance * 100.0f);

		TargetEcho->SetEchoState(EAstrawildEchoState::WildHostile);
		OnCaptureFailed.Broadcast(TargetEcho, OutShakesCompleted, OutStatusReason);
		OnCaptureFeedback.Broadcast(LastCaptureFeedback, false);
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
		UE_LOG(LogAstrawildEcho, Log, TEXT("Summoned companion: %s (Trust: %.1f%%)"), *EchoData.CustomNickname.ToString(), EchoData.TrustScore);
		OnEchoSummoned.Broadcast(Companion);
		return true;
	}

	return false;
}

void UAstrawildCaptureComponent::RecallActiveCompanion()
{
	if (ActiveSummonedEcho.IsValid())
	{
		if (ActiveParty.IsValidIndex(SelectedPartyIndex) && ActiveSummonedEcho->Attributes)
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