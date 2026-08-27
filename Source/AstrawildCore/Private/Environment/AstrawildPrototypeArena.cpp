// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildPrototypeArena.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Environment/AstrawildInteractableActor.h"
#include "Environment/AstrawildTrainingDummy.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "AstrawildLogChannels.h"

AAstrawildPrototypeArena::AAstrawildPrototypeArena()
	: bAutoGenerateOnBeginPlay(true)
	, ArenaSize(8000.0f)
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

	UE_LOG(LogAstrawild, Log, TEXT("Generating ASTRAWILD 4-Zone Vertical Slice Map at %s"), *Origin.ToString());

	// ==========================================
	// 1. MAIN FOUNDATION TERRAIN (80m x 80m)
	// ==========================================
	SpawnTestPlatform(Origin + FVector(0, 0, -25), FVector(4000, 4000, 25), FRotator::ZeroRotator, FColor(55, 65, 75));

	// ==========================================
	// 2. ZONE 1: CENTRAL DAWN SPIRE (LANDMARK)
	// ==========================================
	// Elevated Dais
	SpawnTestPlatform(Origin + FVector(0, 0, 30), FVector(500, 500, 30), FRotator::ZeroRotator, FColor(140, 150, 160));
	// Towering Spire Monolith Pillar (12m tall landmark)
	SpawnTestPlatform(Origin + FVector(0, 0, 600), FVector(60, 60, 600), FRotator::ZeroRotator, FColor(241, 196, 15));

	// ==========================================
	// 3. ZONE 2: NORTH-WEST RESOURCE GROVE
	// ==========================================
	// Stepped ledges & exploration ramps
	SpawnTestPlatform(Origin + FVector(-1200, 600, 50), FVector(400, 400, 50), FRotator::ZeroRotator, FColor(90, 130, 90));
	SpawnTestPlatform(Origin + FVector(-1600, 1000, 100), FVector(400, 400, 100), FRotator::ZeroRotator, FColor(90, 130, 90));
	// 25-degree ramp connecting low and high grove
	SpawnTestPlatform(Origin + FVector(-1400, 800, 75), FVector(200, 300, 15), FRotator(25.0f, 45.0f, 0), FColor(100, 140, 100));

	// ==========================================
	// 4. ZONE 3: SOUTH-EAST DANGER & COMBAT PIT
	// ==========================================
	// Sunken Combat Arena Base (-100cm depth)
	SpawnTestPlatform(Origin + FVector(1400, -1200, -100), FVector(800, 800, 25), FRotator::ZeroRotator, FColor(120, 60, 60));
	// Surrounding Pit Walls
	SpawnTestPlatform(Origin + FVector(2200, -1200, 50), FVector(30, 800, 150), FRotator::ZeroRotator, FColor(80, 80, 80));
	SpawnTestPlatform(Origin + FVector(1400, -2000, 50), FVector(800, 30, 150), FRotator::ZeroRotator, FColor(80, 80, 80));
	// Entry slope (30 degrees)
	SpawnTestPlatform(Origin + FVector(650, -1200, -25), FVector(300, 200, 15), FRotator(30.0f, 0, 0), FColor(160, 90, 90));

	// ==========================================
	// 5. ZONE 4: NORTH-EAST ELEVATED REST SANCTUARY
	// ==========================================
	// High Sanctuary Plateau (+200cm elevation)
	SpawnTestPlatform(Origin + FVector(1400, 1200, 200), FVector(600, 600, 200), FRotator::ZeroRotator, FColor(80, 120, 160));
	// Approach Stairs (Heights: 50cm, 100cm, 150cm, 200cm)
	SpawnTestPlatform(Origin + FVector(650, 1200, 25), FVector(150, 200, 25), FRotator::ZeroRotator, FColor(100, 130, 160));
	SpawnTestPlatform(Origin + FVector(900, 1200, 75), FVector(150, 200, 75), FRotator::ZeroRotator, FColor(110, 140, 170));
	SpawnTestPlatform(Origin + FVector(1150, 1200, 125), FVector(150, 200, 125), FRotator::ZeroRotator, FColor(120, 150, 180));

	// 6. Spawn all world entities across the 4 zones
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
		MeshComp->SetWorldScale3D(Extent / 50.0f);

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

	// -------------------------------------------------------------
	// ZONE 1: Central Dawn Spire Monolith
	// -------------------------------------------------------------
	AAstrawildInteractableActor* Monolith = World->SpawnActor<AAstrawildInteractableActor>(AAstrawildInteractableActor::StaticClass(), Origin + FVector(0, 0, 70), FRotator::ZeroRotator, SpawnParams);
	if (Monolith)
	{
		Monolith->PromptText = FText::FromString(TEXT("[E] Attune to Ancient Dawn Spire"));
		Monolith->DetailedDescription = FText::FromString(TEXT("The First Dawn harmonics resonate deeply within your spirit, bestowing 3 Astra Resonators."));
		Monolith->RewardItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);
		Monolith->RewardItemQuantity = 3;
	}

	// -------------------------------------------------------------
	// ZONE 2: North-West Resource Grove (Wood + Mining + Astra Shards)
	// -------------------------------------------------------------
	// Trees
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector TreeLoc = Origin + FVector(-1000.0f - (i * 300.0f), 500.0f + (i * 250.0f), 10.0f);
		AAstrawildHarvestableNode* Tree = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), TreeLoc, FRotator::ZeroRotator, SpawnParams);
		if (Tree)
		{
			Tree->HarvestType = EAstrawildHarvestType::Lumber;
			Tree->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
			Tree->MaxNodeHealth = 100.0f;
		}
	}

	// Ore Nodes
	AAstrawildHarvestableNode* Rock1 = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(-1300, 1100, 110), FRotator::ZeroRotator, SpawnParams);
	if (Rock1)
	{
		Rock1->HarvestType = EAstrawildHarvestType::Mining;
		Rock1->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		Rock1->RareSecondaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		Rock1->RareDropChancePercent = 40;
	}

	AAstrawildHarvestableNode* Rock2 = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(-1700, 700, 110), FRotator::ZeroRotator, SpawnParams);
	if (Rock2)
	{
		Rock2->HarvestType = EAstrawildHarvestType::Mining;
		Rock2->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		Rock2->RareSecondaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		Rock2->RareDropChancePercent = 40;
	}

	// -------------------------------------------------------------
	// ZONE 3: South-East Danger Arena (Combat Trial, Enemies, Dummy)
	// -------------------------------------------------------------
	// Training Dummy
	World->SpawnActor<AAstrawildTrainingDummy>(AAstrawildTrainingDummy::StaticClass(), Origin + FVector(1400, -1200, -75), FRotator::ZeroRotator, SpawnParams);

	// Wild Pyrelite (Solar / Exploration)
	AAstrawildEchoBase* Pyrelite = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(1100, -1500, -75), FRotator::ZeroRotator, SpawnParams);
	if (Pyrelite)
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
		DA1->PlaceholderTint = FColor(230, 126, 34);
		Pyrelite->InitializeFromSpeciesData(DA1, 2);
	}

	// Wild Thornback (Geo / Combat)
	AAstrawildEchoBase* Thornback = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(1700, -900, -75), FRotator::ZeroRotator, SpawnParams);
	if (Thornback)
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
		DA2->PlaceholderTint = FColor(46, 204, 113);
		Thornback->InitializeFromSpeciesData(DA2, 3);
	}

	// -------------------------------------------------------------
	// ZONE 4: North-East Rest Sanctuary (Campfire, Bed, Bench)
	// -------------------------------------------------------------
	// Rest Campfire
	AAstrawildBuildingPiece* Campfire = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1300, 1100, 420), FRotator::ZeroRotator, SpawnParams);
	if (Campfire)
	{
		Campfire->BuildingType = EAstrawildBuildingType::Campfire;
		Campfire->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.Campfire"), false);
	}

	// Rest Bed
	AAstrawildBuildingPiece* Bed = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1500, 1300, 420), FRotator::ZeroRotator, SpawnParams);
	if (Bed)
	{
		Bed->BuildingType = EAstrawildBuildingType::RestBed;
		Bed->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.RestBed"), false);
	}

	// Crafting Bench
	AAstrawildBuildingPiece* Bench = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1200, 1350, 420), FRotator::ZeroRotator, SpawnParams);
	if (Bench)
	{
		Bench->BuildingType = EAstrawildBuildingType::CraftingBench;
		Bench->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.CraftingBench"), false);
	}

	// Wild Aquavine (Torrent / Base Utility Companion nearby)
	AAstrawildEchoBase* Aquavine = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), Origin + FVector(1400, 1500, 420), FRotator::ZeroRotator, SpawnParams);
	if (Aquavine)
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
		DA3->PlaceholderTint = FColor(52, 152, 219);
		Aquavine->InitializeFromSpeciesData(DA3, 2);
	}
}