// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildTrainingDummy.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "AstrawildLogChannels.h"

AAstrawildTrainingDummy::AAstrawildTrainingDummy()
	: TotalDamageTaken(0.0f)
	, TotalHitCount(0)
	, RecentDPS(0.0f)
	, AutoResetHealthDelay(3.5f)
	, TimeSinceLastHit(0.0f)
	, DamageInCurrentWindow(0.0f)
	, WindowTimer(0.0f)
	, FlinchTimer(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	RootComponent = DummyMesh;
	DummyMesh->SetCollisionProfileName(TEXT("BlockAll"));

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CubeMesh)
	{
		DummyMesh->SetStaticMesh(CubeMesh);
		DummyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	Attributes = CreateDefaultSubobject<UAstrawildAttributeComponent>(TEXT("Attributes"));
	Attributes->MaxHealth = 1000.0f;
	Attributes->CurrentHealth = 1000.0f;
	Attributes->DefensePower = 15.0f;
	Attributes->ElementalAffinity = EAstrawildElement::Neutral;
}

void AAstrawildTrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	OriginalScale = DummyMesh ? DummyMesh->GetComponentScale() : FVector::OneVector;
	ResetDummyStats();
}

void AAstrawildTrainingDummy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastHit += DeltaTime;
	WindowTimer += DeltaTime;

	// Calculate rolling DPS every 1.0 second
	if (WindowTimer >= 1.0f)
	{
		RecentDPS = DamageInCurrentWindow / WindowTimer;
		DamageInCurrentWindow = 0.0f;
		WindowTimer = 0.0f;
	}

	// Auto-heal when peaceful
	if (TimeSinceLastHit >= AutoResetHealthDelay && Attributes && Attributes->CurrentHealth < Attributes->MaxHealth)
	{
		Attributes->ResetToMax();
	}

	// Flinch visual recovery
	if (FlinchTimer > 0.0f && DummyMesh)
	{
		FlinchTimer -= DeltaTime;
		if (FlinchTimer <= 0.0f)
		{
			DummyMesh->SetWorldScale3D(OriginalScale);
		}
	}
}

void AAstrawildTrainingDummy::ResetDummyStats()
{
	TotalDamageTaken = 0.0f;
	TotalHitCount = 0;
	RecentDPS = 0.0f;
	DamageInCurrentWindow = 0.0f;
	TimeSinceLastHit = 0.0f;
	if (Attributes)
	{
		Attributes->ResetToMax();
	}
	UE_LOG(LogAstrawildCombat, Log, TEXT("Training Dummy stats reset."));
}

float AAstrawildTrainingDummy::TakeAstrawildDamage_Implementation(const FAstrawildDamageEvent& DamageEvent)
{
	if (!Attributes)
	{
		return 0.0f;
	}

	// 1. Armor mitigation: 100 / (100 + Defense)
	const float DefenseFactor = 100.0f / (100.0f + Attributes->DefensePower);
	const float ElementMult = FAstrawildElementalMatrix::GetMultiplier(DamageEvent.DamageElement, Attributes->ElementalAffinity);
	const float FinalDamage = FMath::Max(1.0f, DamageEvent.BaseDamage * DefenseFactor * ElementMult);

	Attributes->ModifyHealth(-FinalDamage, DamageEvent.InstigatorActor.Get());

	TotalDamageTaken += FinalDamage;
	TotalHitCount++;
	DamageInCurrentWindow += FinalDamage;
	TimeSinceLastHit = 0.0f;

	// Visual flinch scale pulse
	if (DummyMesh)
	{
		DummyMesh->SetWorldScale3D(OriginalScale * FVector(1.15f, 1.15f, 0.90f));
		FlinchTimer = 0.12f;
	}

	// Apply status effect if attached
	if (DamageEvent.AppliedStatusTag.IsValid() && DamageEvent.StatusDuration > 0.0f)
	{
		Attributes->ApplyStatusEffect(DamageEvent.AppliedStatusTag, DamageEvent.StatusDuration, 1.0f, DamageEvent.InstigatorActor.Get());
	}

	UE_LOG(LogAstrawildCombat, Log, TEXT("[DUMMY] Took %.1f [%s] damage from %s! (Total: %.0f, Hits: %d, DPS: %.1f)"),
		FinalDamage, *UEnum::GetValueAsString(DamageEvent.DamageElement),
		DamageEvent.InstigatorActor.IsValid() ? *DamageEvent.InstigatorActor->GetName() : TEXT("Player"),
		TotalDamageTaken, TotalHitCount, RecentDPS);

	return FinalDamage;
}

bool AAstrawildTrainingDummy::CanTakeDamage_Implementation(AActor* Attacker)
{
	return true;
}

EAstrawildElement AAstrawildTrainingDummy::GetElementalAffinity_Implementation() const
{
	return Attributes ? Attributes->ElementalAffinity : EAstrawildElement::Neutral;
}

FText AAstrawildTrainingDummy::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	const float HPPct = Attributes ? (Attributes->GetHealthPercent() * 100.0f) : 100.0f;
	return FText::FromString(FString::Printf(TEXT("[E] Reset Training Dummy [HP: %.0f%% | DPS: %.1f | Hits: %d]"), HPPct, RecentDPS, TotalHitCount));
}

bool AAstrawildTrainingDummy::CanInteract_Implementation(AActor* Interactor)
{
	return Interactor != nullptr;
}

bool AAstrawildTrainingDummy::PerformInteraction_Implementation(AActor* Interactor)
{
	ResetDummyStats();
	return true;
}