// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildInteractableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildQuestComponent.h"
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

	bHasBeenInteracted = true;
	UE_LOG(LogAstrawild, Log, TEXT("%s interacted with %s: %s"), *Interactor->GetName(), *GetName(), *DetailedDescription.ToString());

	// Grant reward item if configured
	if (RewardItemTag.IsValid() && RewardItemQuantity > 0)
	{
		UAstrawildInventoryComponent* Inv = Interactor->FindComponentByClass<UAstrawildInventoryComponent>();
		if (Inv)
		{
			Inv->AddItem(RewardItemTag, RewardItemQuantity);
			UE_LOG(LogAstrawildInventory, Log, TEXT("Granted %s x%d from %s"), *RewardItemTag.ToString(), RewardItemQuantity, *GetName());
		}
	}

	if (QuestTargetTag.IsValid())
	{
		if (UAstrawildQuestComponent* Quest = Interactor->FindComponentByClass<UAstrawildQuestComponent>())
		{
			Quest->AddProgressForTarget(EAstrawildQuestObjectiveType::Interact, QuestTargetTag, 1);
		}
	}

	OnInteracted.Broadcast(Interactor, this);
	return true;
}