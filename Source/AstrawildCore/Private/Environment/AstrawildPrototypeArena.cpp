// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildPrototypeArena.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Environment/AstrawildInteractableActor.h"
#include "Environment/AstrawildTrainingDummy.h"
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

	// 6. Spawn Prototype Echo 1: Pyrelite (Exploration Specialist)
	AAstrawildEchoBase* Echo1 = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(600, -800, 50), FRotator::ZeroRotator, SpawnParams);
	if (Echo1)
	{
		UAstrawildEchoDataAsset* DA1 = NewObject<UAstrawildEchoDataAsset>();
		DA1->SpeciesTag = FGameplayTag::RequestGameplayTag(FName("Echo.Pyrelite"), false);
		DA1->SpeciesName = FText::FromString(TEXT("Pyrelite"));
		DA1->SpeciesTitle = FText::FromString(TEXT("The Ember Fawn"));
		DA1->ElementalAffinity = EAstrawildElement::Solar;
		DA1->Role = EAstrawildEchoRole::Exploration;
		DA1->BaseMaxHealth = 280.0f;
		DA1->BaseAttackPower = 42.0f;
		DA1->BaseDefensePower = 22.0f;
		DA1->BaseWalkSpeed = 300.0f;
		DA1->BaseRunSpeed = 620.0f;
		DA1->PlaceholderTint = FColor(230, 126, 34); // Solar Amber

		Echo1->InitializeFromSpeciesData(DA1, 2);
	}

	// 7. Spawn Prototype Echo 2: Thornback (Combat Specialist)
	AAstrawildEchoBase* Echo2 = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(600, 0, 50), FRotator::ZeroRotator, SpawnParams);
	if (Echo2)
	{
		UAstrawildEchoDataAsset* DA2 = NewObject<UAstrawildEchoDataAsset>();
		DA2->SpeciesTag = FGameplayTag::RequestGameplayTag(FName("Echo.Thornback"), false);
		DA2->SpeciesName = FText::FromString(TEXT("Thornback"));
		DA2->SpeciesTitle = FText::FromString(TEXT("The Terra Bastion"));
		DA2->ElementalAffinity = EAstrawildElement::Geo;
		DA2->Role = EAstrawildEchoRole::Combat;
		DA2->BaseMaxHealth = 450.0f;
		DA2->BaseAttackPower = 32.0f;
		DA2->BaseDefensePower = 48.0f;
		DA2->BaseWalkSpeed = 220.0f;
		DA2->BaseRunSpeed = 420.0f;
		DA2->PlaceholderTint = FColor(46, 204, 113); // Verdurous Green

		Echo2->InitializeFromSpeciesData(DA2, 3);
	}

	// 8. Spawn Prototype Echo 3: Aquavine (Base Utility Specialist)
	AAstrawildEchoBase* Echo3 = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(600, 800, 50), FRotator::ZeroRotator, SpawnParams);
	if (Echo3)
	{
		UAstrawildEchoDataAsset* DA3 = NewObject<UAstrawildEchoDataAsset>();
		DA3->SpeciesTag = FGameplayTag::RequestGameplayTag(FName("Echo.Aquavine"), false);
		DA3->SpeciesName = FText::FromString(TEXT("Aquavine"));
		DA3->SpeciesTitle = FText::FromString(TEXT("The Dew Serpent"));
		DA3->ElementalAffinity = EAstrawildElement::Torrent;
		DA3->Role = EAstrawildEchoRole::BaseUtility;
		DA3->BaseMaxHealth = 340.0f;
		DA3->BaseAttackPower = 36.0f;
		DA3->BaseDefensePower = 28.0f;
		DA3->BaseWalkSpeed = 260.0f;
		DA3->BaseRunSpeed = 500.0f;
		DA3->WorkEfficiencyMultiplier = 1.5f;
		DA3->PlaceholderTint = FColor(52, 152, 219); // Torrent Cyan

		Echo3->InitializeFromSpeciesData(DA3, 2);
	}

	// 9. Spawn Training Dummy Target
	World->SpawnActor<AAstrawildTrainingDummy>(AAstrawildTrainingDummy::StaticClass(), Origin + FVector(0, -600, 50), FRotator::ZeroRotator, SpawnParams);
}