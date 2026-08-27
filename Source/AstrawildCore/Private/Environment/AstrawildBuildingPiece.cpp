// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildBuildingPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AstrawildInventoryComponent.h"
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
}

void AAstrawildBuildingPiece::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
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

	if (ContainerInventory && SaveData.ContainerInventory.Num() > 0)
	{
		ContainerInventory->LoadInventorySlots(SaveData.ContainerInventory);
	}
}