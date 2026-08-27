// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildHarvestableNode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildQuestComponent.h"
#include "AstrawildLogChannels.h"

AAstrawildHarvestableNode::AAstrawildHarvestableNode()
	: NodeUniqueId(FGuid::NewGuid())
	, HarvestType(EAstrawildHarvestType::Lumber)
	, MinYieldPerHit(1)
	, MaxYieldPerHit(3)
	, RareDropChancePercent(15)
	, MaxNodeHealth(100.0f)
	, CurrentNodeHealth(100.0f)
	, RespawnDurationSeconds(60.0f)
	, bIsDepleted(false)
	, CurrentRespawnTimer(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AAstrawildHarvestableNode::BeginPlay()
{
	Super::BeginPlay();
	CurrentNodeHealth = MaxNodeHealth;
}

void AAstrawildHarvestableNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDepleted)
	{
		CurrentRespawnTimer -= DeltaTime;
		if (CurrentRespawnTimer <= 0.0f)
		{
			RespawnNode();
		}
	}
}

bool AAstrawildHarvestableNode::Harvest(float ToolPower, EAstrawildHarvestType ToolType, AActor* HarvesterActor, int32& OutQuantityHarvested)
{
	OutQuantityHarvested = 0;
	if (bIsDepleted || !HarvesterActor)
	{
		return false;
	}

	// Tool match bonus / penalty
	float EfficiencyMultiplier = 1.0f;
	if (ToolType == HarvestType)
	{
		EfficiencyMultiplier = 2.0f;
	}
	else if (ToolType != EAstrawildHarvestType::Foraging)
	{
		EfficiencyMultiplier = 0.5f;
	}

	const float EffectiveDamage = FMath::Max(10.0f, ToolPower * 20.0f * EfficiencyMultiplier);
	CurrentNodeHealth = FMath::Max(0.0f, CurrentNodeHealth - EffectiveDamage);

	// Calculate primary yield
	const int32 BaseYield = FMath::RandRange(MinYieldPerHit, MaxYieldPerHit);
	OutQuantityHarvested = FMath::Max(1, FMath::RoundToInt(BaseYield * (ToolType == HarvestType ? 1.5f : 1.0f)));

	// Give directly to harvester inventory if present
	UAstrawildInventoryComponent* Inv = HarvesterActor->FindComponentByClass<UAstrawildInventoryComponent>();
	bool bPrimaryAdded = false;
	if (Inv)
	{
		bPrimaryAdded = Inv->AddItem(PrimaryResourceTag, OutQuantityHarvested);

		// Rare secondary drop roll (e.g. Astra Shards from minerals)
		if (RareSecondaryResourceTag.IsValid() && FMath::RandRange(1, 100) <= RareDropChancePercent)
		{
			const int32 RareQty = 1;
			Inv->AddItem(RareSecondaryResourceTag, RareQty);
			UE_LOG(LogAstrawild, Log, TEXT("Rare Drop harvested: %s x%d"), *RareSecondaryResourceTag.ToString(), RareQty);
		}
	}
	if (bPrimaryAdded)
	{
		if (UAstrawildQuestComponent* Quest = HarvesterActor->FindComponentByClass<UAstrawildQuestComponent>())
		{
			Quest->AddProgressForTarget(EAstrawildQuestObjectiveType::Collect, PrimaryResourceTag, OutQuantityHarvested);
		}
	}

	OnNodeHarvested.Broadcast(HarvesterActor, PrimaryResourceTag, OutQuantityHarvested);
	UE_LOG(LogAstrawild, Log, TEXT("Harvested %s: %d items. Node remaining HP: %.1f"), *GetName(), OutQuantityHarvested, CurrentNodeHealth);

	if (CurrentNodeHealth <= 0.0f)
	{
		DepleteNode();
	}

	return true;
}

void AAstrawildHarvestableNode::DepleteNode()
{
	bIsDepleted = true;
	CurrentRespawnTimer = RespawnDurationSeconds;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	UE_LOG(LogAstrawild, Log, TEXT("Harvest Node %s depleted. Respawning in %.1f seconds."), *GetName(), RespawnDurationSeconds);
	OnNodeDepleted.Broadcast(this);
}

void AAstrawildHarvestableNode::RespawnNode()
{
	bIsDepleted = false;
	CurrentNodeHealth = MaxNodeHealth;
	CurrentRespawnTimer = 0.0f;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	UE_LOG(LogAstrawild, Log, TEXT("Harvest Node %s has respawned."), *GetName());
	OnNodeRespawned.Broadcast(this);
}

FAstrawildHarvestNodeSaveData AAstrawildHarvestableNode::GetSaveData() const
{
	FAstrawildHarvestNodeSaveData Data;
	Data.NodeGuid = NodeUniqueId;
	Data.WorldTransform = GetActorTransform();
	Data.bIsDepleted = bIsDepleted;
	Data.RespawnTimeRemaining = CurrentRespawnTimer;
	return Data;
}

void AAstrawildHarvestableNode::LoadSaveData(const FAstrawildHarvestNodeSaveData& SaveData)
{
	NodeUniqueId = SaveData.NodeGuid;
	bIsDepleted = SaveData.bIsDepleted;
	CurrentRespawnTimer = SaveData.RespawnTimeRemaining;

	if (bIsDepleted)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
	else
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		CurrentNodeHealth = MaxNodeHealth;
	}
}

FText AAstrawildHarvestableNode::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	if (bIsDepleted)
	{
		return FText::FromString(TEXT("Depleted"));
	}

	FString ActionName = TEXT("Gather");
	if (HarvestType == EAstrawildHarvestType::Lumber)
	{
		ActionName = TEXT("Chop");
	}
	else if (HarvestType == EAstrawildHarvestType::Mining)
	{
		ActionName = TEXT("Mine");
	}

	return FText::FromString(FString::Printf(TEXT("[E] %s %s"), *ActionName, *PrimaryResourceTag.ToString()));
}

bool AAstrawildHarvestableNode::CanInteract_Implementation(AActor* Interactor)
{
	return !bIsDepleted && Interactor != nullptr;
}

bool AAstrawildHarvestableNode::PerformInteraction_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	int32 OutYield = 0;
	return Harvest(1.0f, HarvestType, Interactor, OutYield);
}