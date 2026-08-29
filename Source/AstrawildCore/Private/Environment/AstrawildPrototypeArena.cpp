// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/AstrawildPrototypeArena.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Environment/AstrawildInteractableActor.h"
#include "Environment/AstrawildTrainingDummy.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Echoes/AstrawildAlphaEcho.h"
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

	if (bAutoGenerateOnBeginPlay && HasAuthority())
	{
		GenerateTestArena();
	}
}

void AAstrawildPrototypeArena::GenerateTestArena()
{
	if (bHasGeneratedArena)
	{
		return;
	}
	bHasGeneratedArena = true;

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
		MeshComp->SetMobility(EComponentMobility::Movable);
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
			Tree->NodeUniqueId = FGuid(0x51000001u + static_cast<uint32>(i), 0x41535452u, 0x41574F4Fu, 0x445F5653u);
			Tree->HarvestType = EAstrawildHarvestType::Lumber;
			Tree->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
			Tree->MinYieldPerHit = 2;
			Tree->MaxYieldPerHit = 4;
			Tree->MaxNodeHealth = 100.0f;
			Tree->RespawnDurationSeconds = 45.0f;
		}
	}

	// Ore Nodes
	AAstrawildHarvestableNode* Rock1 = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(-1300, 1100, 110), FRotator::ZeroRotator, SpawnParams);
	if (Rock1)
	{
		Rock1->NodeUniqueId = FGuid(0x52000001u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Rock1->HarvestType = EAstrawildHarvestType::Mining;
		Rock1->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		Rock1->RareSecondaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		Rock1->MinYieldPerHit = 2;
		Rock1->MaxYieldPerHit = 3;
		Rock1->RareDropChancePercent = 40;
	}

	AAstrawildHarvestableNode* Rock2 = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), Origin + FVector(-1700, 700, 110), FRotator::ZeroRotator, SpawnParams);
	if (Rock2)
	{
		Rock2->NodeUniqueId = FGuid(0x52000002u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Rock2->HarvestType = EAstrawildHarvestType::Mining;
		Rock2->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		Rock2->RareSecondaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		Rock2->MinYieldPerHit = 2;
		Rock2->MaxYieldPerHit = 3;
		Rock2->RareDropChancePercent = 40;
	}

	// Dawn Fiber for the Rest Bed recipe and early survival bootstrap.
	for (int32 i = 0; i < 3; ++i)
	{
		const FVector FiberLoc = Origin + FVector(-900.0f + (i * 350.0f), 1050.0f - (i * 180.0f), 20.0f);
		AAstrawildHarvestableNode* Fiber = World->SpawnActor<AAstrawildHarvestableNode>(AAstrawildHarvestableNode::StaticClass(), FiberLoc, FRotator::ZeroRotator, SpawnParams);
		if (Fiber)
		{
			Fiber->NodeUniqueId = FGuid(0x53000001u + static_cast<uint32>(i), 0x41535452u, 0x41574F4Fu, 0x445F5653u);
			Fiber->HarvestType = EAstrawildHarvestType::Foraging;
			Fiber->PrimaryResourceTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.DawnFiber"), false);
			Fiber->MinYieldPerHit = 2;
			Fiber->MaxYieldPerHit = 4;
			Fiber->MaxNodeHealth = 60.0f;
			Fiber->RespawnDurationSeconds = 30.0f;
		}
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

	// Alpha Echo encounter for the compact Danger Pit loop.
	AAstrawildAlphaEcho* SolarixAlpha = World->SpawnActor<AAstrawildAlphaEcho>(AAstrawildAlphaEcho::StaticClass(), Origin + FVector(1400, -1200, -75), FRotator::ZeroRotator, SpawnParams);
	if (SolarixAlpha)
	{
		UAstrawildEchoDataAsset* AlphaData = NewObject<UAstrawildEchoDataAsset>(SolarixAlpha, TEXT("VS_SolarixAlphaData"));
		AlphaData->SpeciesTag = FGameplayTag::RequestGameplayTag(FName("Echo.SolarixAlpha"), false);
		AlphaData->SpeciesName = FText::FromString(TEXT("Solarix Alpha"));
		AlphaData->SpeciesTitle = FText::FromString(TEXT("The Dawn Pressure"));
		AlphaData->LoreDescription = FText::FromString(TEXT("A territorial Echo shaped by unstable dawn harmonics."));
		AlphaData->ElementalAffinity = EAstrawildElement::Solar;
		AlphaData->Role = EAstrawildEchoRole::Combat;
		AlphaData->BaseMaxHealth = 1800.0f;
		AlphaData->BaseAttackPower = 95.0f;
		AlphaData->BaseDefensePower = 60.0f;
		AlphaData->BaseWalkSpeed = 260.0f;
		AlphaData->BaseRunSpeed = 520.0f;
		AlphaData->CaptureDifficultyModifier = 5.0f;
		AlphaData->PlaceholderTint = FColor(245, 145, 45);

		auto AddAlphaAbility = [AlphaData](const TCHAR* TagName, const TCHAR* DisplayName, float Damage, float Cooldown, float Range, float Radius)
		{
			FAstrawildEchoAbility Ability;
			Ability.AbilityTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
			Ability.AbilityName = FText::FromString(DisplayName);
			Ability.Element = EAstrawildElement::Solar;
			Ability.BaseDamage = Damage;
			Ability.CooldownSeconds = Cooldown;
			Ability.CastRange = Range;
			Ability.Radius = Radius;
			AlphaData->InnateAbilities.Add(Ability);
		};
		AddAlphaAbility(TEXT("Ability.SolarClaw"), TEXT("Solar Claw"), 90.0f, 4.0f, 350.0f, 180.0f);
		AddAlphaAbility(TEXT("Ability.EmberLine"), TEXT("Ember Line"), 110.0f, 5.0f, 800.0f, 120.0f);
		AddAlphaAbility(TEXT("Ability.DawnRoar"), TEXT("Dawn Roar"), 75.0f, 6.0f, 500.0f, 420.0f);
		AddAlphaAbility(TEXT("Ability.SolarNova"), TEXT("Solar Nova"), 150.0f, 9.0f, 700.0f, 520.0f);
		AddAlphaAbility(TEXT("Ability.FlareRing"), TEXT("Flare Ring"), 120.0f, 7.0f, 650.0f, 480.0f);
		AddAlphaAbility(TEXT("Ability.AshenRush"), TEXT("Ashen Rush"), 135.0f, 8.0f, 1000.0f, 220.0f);

		SolarixAlpha->InitializeFromSpeciesData(AlphaData, 5);
		SolarixAlpha->PhaseTwoHealthThreshold = 0.5f;
		auto MakeAlphaPattern = [](const TCHAR* PatternName, const TCHAR* DisplayName, int32 AbilityIndex, float Telegraph, float Cooldown, float Multiplier)
		{
			FAstrawildBossAttackPattern Pattern;
			Pattern.PatternId = FName(PatternName);
			Pattern.DisplayName = FText::FromString(DisplayName);
			Pattern.SpeciesAbilityIndex = AbilityIndex;
			Pattern.Element = EAstrawildElement::Solar;
			Pattern.TelegraphDuration = Telegraph;
			Pattern.Cooldown = Cooldown;
			Pattern.DamageMultiplier = Multiplier;
			return Pattern;
		};
		SolarixAlpha->PhaseOnePatterns.Add(MakeAlphaPattern(TEXT("SolarClaw"), TEXT("Solar Claw"), 0, 0.45f, 4.0f, 1.0f));
		SolarixAlpha->PhaseOnePatterns.Add(MakeAlphaPattern(TEXT("EmberLine"), TEXT("Ember Line"), 1, 0.80f, 5.0f, 1.0f));
		SolarixAlpha->PhaseOnePatterns.Add(MakeAlphaPattern(TEXT("DawnRoar"), TEXT("Dawn Roar"), 2, 1.10f, 6.0f, 1.0f));
		SolarixAlpha->PhaseTwoPatterns.Add(MakeAlphaPattern(TEXT("SolarNova"), TEXT("Solar Nova"), 3, 1.25f, 9.0f, 1.5f));
		SolarixAlpha->PhaseTwoPatterns.Add(MakeAlphaPattern(TEXT("FlareRing"), TEXT("Flare Ring"), 4, 0.90f, 7.0f, 1.2f));
		SolarixAlpha->PhaseTwoPatterns.Add(MakeAlphaPattern(TEXT("AshenRush"), TEXT("Ashen Rush"), 5, 0.65f, 8.0f, 1.35f));
		SolarixAlpha->PhaseTwoPatterns.Add(MakeAlphaPattern(TEXT("DawnRoar"), TEXT("Dawn Roar"), 2, 1.10f, 6.0f, 1.0f));
		SolarixAlpha->StartEncounter();
	}

	// -------------------------------------------------------------
	// ZONE 4: North-East Rest Sanctuary (Campfire, Bed, Bench)
	// -------------------------------------------------------------
	// Rest Campfire
	AAstrawildBuildingPiece* Campfire = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1300, 1100, 420), FRotator::ZeroRotator, SpawnParams);
	if (Campfire)
	{
		Campfire->BuildingUniqueId = FGuid(0x61000001u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Campfire->BuildingType = EAstrawildBuildingType::Campfire;
		Campfire->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.Campfire"), false);
	}

	// Rest Bed
	AAstrawildBuildingPiece* Bed = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1500, 1300, 420), FRotator::ZeroRotator, SpawnParams);
	if (Bed)
	{
		Bed->BuildingUniqueId = FGuid(0x61000002u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Bed->BuildingType = EAstrawildBuildingType::RestBed;
		Bed->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.RestBed"), false);
	}

	// Crafting Bench
	AAstrawildBuildingPiece* Bench = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1200, 1350, 420), FRotator::ZeroRotator, SpawnParams);
	if (Bench)
	{
		Bench->BuildingUniqueId = FGuid(0x61000003u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Bench->BuildingType = EAstrawildBuildingType::CraftingBench;
		Bench->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.CraftingBench"), false);
	}

	AAstrawildBuildingPiece* Chest = World->SpawnActor<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass(), Origin + FVector(1750, 1250, 420), FRotator::ZeroRotator, SpawnParams);
	if (Chest)
	{
		Chest->BuildingUniqueId = FGuid(0x61000004u, 0x41535452u, 0x41574F4Fu, 0x445F5653u);
		Chest->BuildingType = EAstrawildBuildingType::StorageChest;
		Chest->BuildingTag = FGameplayTag::RequestGameplayTag(FName("Building.StorageChest"), false);
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

	AAstrawildInteractableActor* Spring = World->SpawnActor<AAstrawildInteractableActor>(AAstrawildInteractableActor::StaticClass(), Origin + FVector(1700, 1450, 420), FRotator::ZeroRotator, SpawnParams);
	if (Spring)
	{
		Spring->PromptText = FText::FromString(TEXT("[E] Collect and drink clean spring water"));
		Spring->DetailedDescription = FText::FromString(TEXT("Aquavine keeps a clear Torrent spring flowing beside the sanctuary."));
		Spring->bIsOneTimeOnly = false;
		Spring->QuestTargetTag = FGameplayTag::RequestGameplayTag(FName("Location.AquavineSpring"), false);
		Spring->QuestObjectiveType = EAstrawildQuestObjectiveType::Discover;
		Spring->RewardItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Water"), false);
		Spring->RewardItemQuantity = 5;
		Spring->ThirstRestoredOnInteract = 35.0f;
	}
}