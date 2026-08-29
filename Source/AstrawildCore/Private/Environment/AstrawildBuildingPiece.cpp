// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildBuildingPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildColonyWorkComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "AstrawildLogChannels.h"

AAstrawildBuildingPiece::AAstrawildBuildingPiece()
	: BuildingUniqueId(FGuid::NewGuid())
	, BuildingType(EAstrawildBuildingType::Campfire)
	, MaxHealth(500.0f)
	, CurrentHealth(500.0f)
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	InteractionTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionTrigger"));
	InteractionTrigger->SetupAttachment(RootComponent);
	InteractionTrigger->SetSphereRadius(250.0f);
	InteractionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	ContainerInventory = CreateDefaultSubobject<UAstrawildInventoryComponent>(TEXT("ContainerInventory"));
	ContainerInventory->MaxSlots = 24;
	ColonyWork = CreateDefaultSubobject<UAstrawildColonyWorkComponent>(TEXT("ColonyWork"));
}

void AAstrawildBuildingPiece::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;

	if (MeshComponent && !MeshComponent->GetStaticMesh())
	{
		if (BuildingType == EAstrawildBuildingType::Campfire)
		{
			UStaticMesh* PropMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Astrawild/Meshes/Props/SM_CampfireBase.SM_CampfireBase"));
			if (PropMesh)
			{
				MeshComponent->SetStaticMesh(PropMesh);
				MeshComponent->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.6f));
			}
			else
			{
				UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
				if (CylinderMesh)
				{
					MeshComponent->SetStaticMesh(CylinderMesh);
					MeshComponent->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.4f));
				}
			}
		}
		else if (BuildingType == EAstrawildBuildingType::RestBed)
		{
			UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			if (CubeMesh)
			{
				MeshComponent->SetStaticMesh(CubeMesh);
				MeshComponent->SetRelativeScale3D(FVector(2.2f, 1.2f, 0.4f));
			}
		}
		else if (BuildingType == EAstrawildBuildingType::CraftingBench)
		{
			UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			if (CubeMesh)
			{
				MeshComponent->SetStaticMesh(CubeMesh);
				MeshComponent->SetRelativeScale3D(FVector(2.0f, 1.0f, 1.0f));
			}
		}
		else
		{
			UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			if (CubeMesh)
			{
				MeshComponent->SetStaticMesh(CubeMesh);
				MeshComponent->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.8f));
			}
		}
	}
}

void AAstrawildBuildingPiece::Interact(AActor* InteractorActor)
{
	if (!InteractorActor)
	{
		return;
	}

	UE_LOG(LogAstrawild, Log, TEXT("Player interacted with building: %s (Type: %s)"), *GetName(), *UEnum::GetValueAsString(BuildingType));

	// Rest shelter recovery functionality
	if (BuildingType == EAstrawildBuildingType::RestBed || BuildingType == EAstrawildBuildingType::Campfire)
	{
		UAstrawildAttributeComponent* Attr = InteractorActor->FindComponentByClass<UAstrawildAttributeComponent>();
		if (Attr)
		{
			Attr->ResetToMax();
			UE_LOG(LogAstrawild, Log, TEXT("Rested at %s. Health and stamina fully restored!"), *GetName());
		}
	}

	OnBuildingInteracted.Broadcast(InteractorActor, this);
}

bool AAstrawildBuildingPiece::DismantleBuilding(AActor* InInstigator)
{
	if (!InInstigator)
	{
		return false;
	}

	UAstrawildInventoryComponent* Inv = InInstigator->FindComponentByClass<UAstrawildInventoryComponent>();
	if (Inv)
	{
		// Default refunds if not explicitly assigned
		if (DismantleRefund.Num() == 0)
		{
			if (BuildingType == EAstrawildBuildingType::Campfire)
			{
				DismantleRefund.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false), 4 });
				DismantleRefund.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false), 2 });
			}
			else if (BuildingType == EAstrawildBuildingType::RestBed)
			{
				DismantleRefund.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false), 6 });
			}
			else if (BuildingType == EAstrawildBuildingType::CraftingBench)
			{
				DismantleRefund.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false), 5 });
				DismantleRefund.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false), 3 });
			}
		}

		for (const FAstrawildRecipeIngredient& Refund : DismantleRefund)
		{
			Inv->AddItem(Refund.ItemTag, Refund.Quantity);
		}
	}

	UE_LOG(LogAstrawild, Log, TEXT("Dismantled %s. Refunded materials to %s."), *GetName(), *InInstigator->GetName());
	OnBuildingDestroyed.Broadcast(this);
	Destroy();
	return true;
}

void AAstrawildBuildingPiece::TakeBuildingDamage(float DamageAmount)
{
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogAstrawild, Log, TEXT("Building destroyed: %s"), *GetName());
		OnBuildingDestroyed.Broadcast(this);
		Destroy();
	}
}

FAstrawildBuildingSaveData AAstrawildBuildingPiece::GetSaveData() const
{
	FAstrawildBuildingSaveData Data;
	Data.BuildingGuid = BuildingUniqueId;
	Data.BuildingTag = BuildingTag;
	Data.BuildingType = BuildingType;
	Data.WorldTransform = GetActorTransform();
	Data.CurrentHealth = CurrentHealth;
	if (ContainerInventory)
	{
		Data.ContainerInventory = ContainerInventory->GetSlots();
	}
	return Data;
}

void AAstrawildBuildingPiece::LoadSaveData(const FAstrawildBuildingSaveData& SaveData)
{
	BuildingUniqueId = SaveData.BuildingGuid;
	BuildingTag = SaveData.BuildingTag;
	BuildingType = SaveData.BuildingType;
	CurrentHealth = SaveData.CurrentHealth;
	SetActorTransform(SaveData.WorldTransform);

	if (ContainerInventory)
	{
		// Loading an empty saved container must clear any level-default contents.
		ContainerInventory->LoadInventorySlots(SaveData.ContainerInventory);
	}
}

FText AAstrawildBuildingPiece::GetInteractionPrompt_Implementation(AActor* Interactor)
{
	switch (BuildingType)
	{
	case EAstrawildBuildingType::Campfire:
		return FText::FromString(TEXT("[E] Warm Up & Rest at Campfire"));
	case EAstrawildBuildingType::RestBed:
		return FText::FromString(TEXT("[E] Sleep & Restore Energy"));
	case EAstrawildBuildingType::CraftingBench:
		return FText::FromString(TEXT("[E] Open Crafting Bench"));
	case EAstrawildBuildingType::StorageChest:
		return FText::FromString(TEXT("[E] Open Storage Chest"));
	default:
		return FText::FromString(TEXT("[E] Interact"));
	}
}

bool AAstrawildBuildingPiece::CanInteract_Implementation(AActor* Interactor)
{
	return CurrentHealth > 0.0f && Interactor != nullptr;
}

bool AAstrawildBuildingPiece::PerformInteraction_Implementation(AActor* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	Interact(Interactor);
	return true;
}