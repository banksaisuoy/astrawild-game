// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildPrototypeArena.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Environment/AstrawildInteractableActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "AstrawildLogChannels.h"

AAstrawildPrototypeArena::AAstrawildPrototypeArena()
	: bAutoGenerateOnBeginPlay(true)
	, ArenaSize(6000.0f)
{
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AAstrawildPrototypeArena::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGenerateOnBeginPlay)
	{
		GenerateTestArena();
	}
}

void AAstrawildPrototypeArena::GenerateTestArena()
{
	const FVector Origin = GetActorLocation();

	UE_LOG(LogAstrawild, Log, TEXT("Generating ASTRAWILD Movement & Collision Prototype Arena at %s"), *Origin.ToString());

	// 1. Main Ground Base
	SpawnTestPlatform(Origin + FVector(0, 0, -25), FVector(3000, 3000, 25), FRotator::ZeroRotator, FColor(70, 80, 90));

	// 2. Stepped Elevation Platforms (Heights: 50cm, 100cm, 200cm, 300cm)
	SpawnTestPlatform(Origin + FVector(800, -800, 25), FVector(300, 300, 25), FRotator::ZeroRotator, FColor(100, 110, 120));
	SpawnTestPlatform(Origin + FVector(800, -200, 75), FVector(300, 300, 75), FRotator::ZeroRotator, FColor(110, 120, 130));
	SpawnTestPlatform(Origin + FVector(800, 400, 125), FVector(300, 300, 125), FRotator::ZeroRotator, FColor(120, 130, 140));
	SpawnTestPlatform(Origin + FVector(800, 1000, 175), FVector(300, 300, 175), FRotator::ZeroRotator, FColor(130, 140, 150));

	// 3. Test Slopes / Ramps (15 deg, 30 deg, 45 deg)
	SpawnTestPlatform(Origin + FVector(-800, -800, 60), FVector(400, 200, 15), FRotator(15.0f, 0, 0), FColor(90, 140, 90));
	SpawnTestPlatform(Origin + FVector(-800, -200, 120), FVector(400, 200, 15), FRotator(30.0f, 0, 0), FColor(90, 140, 90));
	SpawnTestPlatform(Origin + FVector(-800, 400, 180), FVector(400, 200, 15), FRotator(45.0f, 0, 0), FColor(90, 140, 90));

	// 4. Narrow Corridors & Collision Obstacles
	SpawnTestPlatform(Origin + FVector(-300, 1200, 150), FVector(400, 30, 150), FRotator::ZeroRotator, FColor(180, 80, 80));
	SpawnTestPlatform(Origin + FVector(300, 1200, 150), FVector(400, 30, 150), FRotator::ZeroRotator, FColor(180, 80, 80));

	// 5. Spawn Interactive Test Entities
	SpawnTestEntities();
}

void AAstrawildPrototypeArena::SpawnTestPlatform(const FVector& Location, const FVector& Extent, const FRotator& Rotation, const FColor& DebugColor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Block = World->SpawnActor<AActor>(AActor::StaticClass(), Location, Rotation, SpawnParams);
	if (Block)
	{
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Block, TEXT("PlatformMesh"));
		MeshComp->RegisterComponent();
		Block->SetRootComponent(MeshComp);
		MeshComp->SetWorldLocationAndRotation(Location, Rotation);
		MeshComp->SetWorldScale3D(Extent / 50.0f); // Standard cube is 100x100x100

		// Use engine basic cube if available
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh)
		{
			MeshComp->SetStaticMesh(CubeMesh);
		}

		MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
		MeshComp->SetMobility(EComponentMobility::Static);
	}
}

void AAstrawildPrototypeArena::SpawnTestEntities()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 1. Harvestable Sunwood Tree
	AAstrawildHarvestableNode* Tree = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(400, -500, 0), FRotator::ZeroRotator, SpawnParams);
	if (Tree)
	{
		Tree->HarvestType = EAstrawildHarvestType::Lumber;
		Tree->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
		Tree->MaxNodeHealth = 100.0f;
	}

	// 2. Harvestable Lumen Stone Node
	AAstrawildHarvestableNode* Rock = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(400, 500, 0), FRotator::ZeroRotator, SpawnParams);
	if (Rock)
	{
		Rock->HarvestType = EAstrawildHarvestType::Mining;
		Rock->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		Rock->RareSecondaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		Rock->RareDropChancePercent = 35;
	}

	// 3. Ancient Dawn Monolith Lore Inspectable
	AAstrawildInteractableActor* Monolith = World->SpawnActor<AAstrawildInteractableActor>(AAstrawildInteractableActor::StaticClass(), Origin + FVector(0, 800, 0), FRotator::ZeroRotator, SpawnParams);
	if (Monolith)
	{
		Monolith->PromptText = FText::FromString(TEXT("[E] Touch Ancient Dawn Monolith"));
		Monolith->DetailedDescription = FText::FromString(TEXT("Harmonics pulse through the stone, bestowing primal resonance upon you."));
		Monolith->RewardItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);
		Monolith->RewardItemQuantity = 3;
	}

	// 4. Test Campfire
	AAstrawildBuildingPiece* Campfire = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(-400, 0, 0), FRotator::ZeroRotator, SpawnParams);
	if (Campfire)
	{
		Campfire->BuildingType = EAstrawildBuildingType::Campfire;
		Campfire->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.Campfire"), false);
	}

	// 5. Test Resting Bed
	AAstrawildBuildingPiece* Bed = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(-400, 300, 0), FRotator::ZeroRotator, SpawnParams);
	if (Bed)
	{
		Bed->BuildingType = EAstrawildBuildingType::RestBed;
		Bed->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.RestBed"), false);
	}
}