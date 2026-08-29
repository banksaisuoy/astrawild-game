// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildInteractableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildQuestComponent.h"
#include "Components/AstrawildSurvivalComponent.h"
#include "AstrawildLogChannels.h"

AAstrawildInteractableActor::AAstrawildInteractableActor()
	: PromptText(FText::FromString(TEXT("Inspect Ancient Monolith")))
	, DetailedDescription(FText::FromString(TEXT("An ancient stone radiating faint Astra harmonics from the First Dawn.")))
	, bIsOneTimeOnly(false)
	, bHasBeenInteracted(false)
	, RewardItemQuantity(0)
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(RootComponent);
	TriggerSphere->SetSphereRadius(250.0f);
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AAstrawildInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	if (MeshComponent && !MeshComponent->GetStaticMesh())
	{
		UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh)
		{
			MeshComponent->SetStaticMesh(CylinderMesh);
			MeshComponent->SetRelativeScale3D(FVector(1.5f, 1.5f, 6.0f));
		}
	}
}

FText AAstrawildInteractableActor::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsOneTimeOnly && bHasBeenInteracted)
	{
		return FText::FromString(TEXT("Already Inspected"));
	}
	return PromptText;
}

bool AAstrawildInteractableActor::CanInteract_Implementation(AActor* Interactor)
{
	if (!Interactor)
	{
		return false;
	}
	if (bIsOneTimeOnly && bHasBeenInteracted)
	{
		return false;
	}
	return true;
}

bool AAstrawildInteractableActor::PerformInteraction_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	UE_LOG(LogAstrawild, Log, TEXT("%s interacted with %s: %s"), *Interactor->GetName(), *GetName(), *DetailedDescription.ToString());

	// Grant reward item if configured
	if (RewardItemTag.IsValid() && RewardItemQuantity > 0)
	{
		UAstrawildInventoryComponent* Inv = Interactor->FindComponentByClass<UAstrawildInventoryComponent>();
		const bool bRewardAccepted = Inv && Inv->AddItem(RewardItemTag, RewardItemQuantity);
		if (bRewardAccepted)
		{
			UE_LOG(LogAstrawildInventory, Log, TEXT("Granted %s x%d from %s"), *RewardItemTag.ToString(), RewardItemQuantity, *GetName());
			if (UAstrawildQuestComponent* Quest = Interactor->FindComponentByClass<UAstrawildQuestComponent>())
			{
				Quest->AddProgressForTarget(EAstrawildQuestObjectiveType::Collect, RewardItemTag, RewardItemQuantity);
			}
		}
		else
		{
			UE_LOG(LogAstrawildInventory, Warning, TEXT("Reward %s x%d from %s was not accepted; inventory may be full."), *RewardItemTag.ToString(), RewardItemQuantity, *GetName());
			return false;
		}
	}

	bHasBeenInteracted = true;
	if (QuestTargetTag.IsValid())
	{
		if (UAstrawildQuestComponent* Quest = Interactor->FindComponentByClass<UAstrawildQuestComponent>())
		{
			Quest->AddProgressForTarget(QuestObjectiveType, QuestTargetTag, 1);
		}
	}

	if (UAstrawildSurvivalComponent* Survival = Interactor->FindComponentByClass<UAstrawildSurvivalComponent>())
	{
		if (HungerRestoredOnInteract > 0.0f)
		{
			Survival->ConsumeFood(HungerRestoredOnInteract);
		}
		if (ThirstRestoredOnInteract > 0.0f)
		{
			Survival->DrinkWater(ThirstRestoredOnInteract);
		}
	}

	OnInteracted.Broadcast(Interactor, this);
	return true;
}