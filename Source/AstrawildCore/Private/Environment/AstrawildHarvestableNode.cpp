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

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void AAstrawildHarvestableNode::BeginPlay()
{
	Super::BeginPlay();
	CurrentNodeHealth = MaxNodeHealth;

	if (MeshComponent && !MeshComponent->GetStaticMesh())
	{
		FColor NodeTint = FColor::White;
		if (HarvestType == EAstrawildHarvestType::Lumber)
		{
			UStaticMesh* PropMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Astrawild/Meshes/Props/SM_SunwoodLog.SM_SunwoodLog"));
			if (PropMesh)
			{
				MeshComponent->SetStaticMesh(PropMesh);
				MeshComponent->SetRelativeScale3D(FVector(1.8f, 1.8f, 1.8f));
			}
			else
			{
				UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
				if (CylinderMesh)
				{
					MeshComponent->SetStaticMesh(CylinderMesh);
					MeshComponent->SetRelativeScale3D(FVector(1.5f, 1.5f, 5.0f));
				}
			}
			NodeTint = FColor(121, 85, 72); // Wooden Bark Brown
		}
		else if (HarvestType == EAstrawildHarvestType::Mining)
		{
			UStaticMesh* PropMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Astrawild/Meshes/Props/SM_LumenRock.SM_LumenRock"));
			if (PropMesh)
			{
				MeshComponent->SetStaticMesh(PropMesh);
				MeshComponent->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
			}
			else
			{
				UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				if (CubeMesh)
				{
					MeshComponent->SetStaticMesh(CubeMesh);
					MeshComponent->SetRelativeScale3D(FVector(2.0f, 2.0f, 1.6f));
				}
			}
			NodeTint = FColor(0, 188, 212); // Astra Crystal Cyan
		}
		else
		{
			UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
			if (SphereMesh)
			{
				MeshComponent->SetStaticMesh(SphereMesh);
				MeshComponent->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.0f));
			}
			NodeTint = FColor(139, 195, 74); // Vibrant Plant Green
		}

		UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"));
		if (BaseMat)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(NodeTint));
				DynMat->SetVectorParameterValue(TEXT("Albedo"), FLinearColor(NodeTint));
				MeshComponent->SetMaterial(0, DynMat);
			}
		}
	}
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

	// Calculate the exact primary yield before mutating the node so a full
	// inventory cannot consume world resources without granting the result.
	const int32 BaseYield = FMath::RandRange(MinYieldPerHit, MaxYieldPerHit);
	OutQuantityHarvested = FMath::Max(1, FMath::RoundToInt(BaseYield * (ToolType == HarvestType ? 1.5f : 1.0f)));
	UAstrawildInventoryComponent* Inv = HarvesterActor->FindComponentByClass<UAstrawildInventoryComponent>();
	if (!Inv || !Inv->CanAddItem(PrimaryResourceTag, OutQuantityHarvested))
	{
		OutQuantityHarvested = 0;
		UE_LOG(LogAstrawildInventory, Warning, TEXT("Cannot harvest %s: inventory cannot accept the full yield."), *GetName());
		return false;
	}

	const float EffectiveDamage = FMath::Max(10.0f, ToolPower * 20.0f * EfficiencyMultiplier);
	CurrentNodeHealth = FMath::Max(0.0f, CurrentNodeHealth - EffectiveDamage);

	// Give directly to harvester inventory
	const bool bPrimaryAdded = Inv->AddItem(PrimaryResourceTag, OutQuantityHarvested);

			// Rare secondary drop roll (e.g. Astra Shards from minerals)
	if (RareSecondaryResourceTag.IsValid() && FMath::RandRange(1, 100) <= RareDropChancePercent)
	{
		const int32 RareQty = 1;
		Inv->AddItem(RareSecondaryResourceTag, RareQty);
		UE_LOG(LogAstrawild, Log, TEXT("Rare Drop harvested: %s x%d"), *RareSecondaryResourceTag.ToString(), RareQty);
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