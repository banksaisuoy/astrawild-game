#include "AstrawildWorldBootstrapper.h"

#include "AstrawildCraftingStationActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDungeonGeneratorActor.h"
#include "AstrawildDungeonPortalActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildResourceNode.h"
#include "AstrawildRestPoint.h"
#include "AstrawildWorkSiteActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"

// Engine light classes for the runtime lighting rig.
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyAtmosphere.h"

AAstrawildWorldBootstrapper::AAstrawildWorldBootstrapper()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f; // Sun tracking only — cheap.

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AAstrawildGameState* AAstrawildWorldBootstrapper::GetGameState() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<AAstrawildGameState>() : nullptr;
}

void AAstrawildWorldBootstrapper::BeginPlay()
{
    Super::BeginPlay();

    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Deterministic seed from the replicated game state.
    if (AAstrawildGameState* GameState = GetGameState())
    {
        const int32 Seed = GameState->WorldSeed;
        GameState->SetWorldSeed(Seed);
        RandomStream = FRandomStream(Seed);
    }
    else
    {
        RandomStream = FRandomStream(1337);
    }

    BuildLighting();
    BuildGround();
    ScatterResourceNodes();
    SpawnWildEchoes();
    SpawnHostiles();
    SpawnPointsOfInterest();

    // Audit C-3: kick the navmesh build — tiles generate around navigation invokers
    // (player + Echoes; see DefaultEngine.ini) so pathfinding works from frame one.
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        NavSys->Build();
        UE_LOG(LogAstrawildWorld, Log, TEXT("Runtime navmesh build requested (invoker-driven generation)."));
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Dawn Fields bootstrapped: %d nodes, %d wild Echoes, %d hostiles."),
        ResourceNodeCount, WildEchoCount, HostileCount);
}

void AAstrawildWorldBootstrapper::BuildLighting()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (!SunLight)
    {
        SunLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0, 0, 4000), FRotator(-50.0f, 30.0f, 0.0f), Params);
        if (SunLight)
        {
            SunLight->SetMobility(EComponentMobility::Movable);
            if (ULightComponent* LightComponent = SunLight->GetLightComponent())
            {
                LightComponent->SetIntensity(8.0f);
            }
        }
    }

    // Sky light for ambient fill.
    ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0, 0, 3000), FRotator::ZeroRotator, Params);
    if (Sky)
    {
        Sky->SetMobility(EComponentMobility::Movable);
        if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
        {
            SkyComponent->SetIntensity(1.5f);
        }
    }

    World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, Params);
    World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, Params);
}

void AAstrawildWorldBootstrapper::BuildGround()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Large ground plane from the engine basic shape (placeholder — REPLACE_BEFORE_RELEASE
    // with proper World Partition landscape, directive §21/§22).
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float PlaneScale = ArenaSize / 100.0f; // Engine plane is 100x100m at scale 1.
    AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
    if (Ground)
    {
        Ground->SetMobility(EComponentMobility::Static);
        if (UStaticMeshComponent* Mesh = Ground->GetStaticMeshComponent())
        {
            UStaticMesh* LoadedPlane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
            if (LoadedPlane)
            {
                Mesh->SetStaticMesh(LoadedPlane);
            }
            Mesh->SetWorldScale3D(FVector(PlaneScale, PlaneScale, 1.0f));
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        }
    }

    // Fallback player start so spawning always works.
    World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params);
}

void AAstrawildWorldBootstrapper::ScatterResourceNodes()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FName ResourceIds[3] = { TEXT("Item_Wood"), TEXT("Item_Stone"), TEXT("Item_Fiber") };

    for (int32 i = 0; i < ResourceNodeCount; ++i)
    {
        const FVector Location(
            RandomStream.FRandRange(-ArenaSize * 0.85f, ArenaSize * 0.85f),
            RandomStream.FRandRange(-ArenaSize * 0.85f, ArenaSize * 0.85f),
            100.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildResourceNode* Node = World->SpawnActor<AAstrawildResourceNode>(AAstrawildResourceNode::StaticClass(), Location, FRotator::ZeroRotator, Params);
        if (Node)
        {
            Node->ResourceItemId = ResourceIds[i % 3];
            Node->ResourceQuantityPerHarvest = 2;
            Node->RemainingQuantity = 3;
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnWildEchoes()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry)
    {
        return;
    }

    // Day-active friendly species (directive §21 first Echo encounters + wave-2 Sprigling herds).
    const FName Species[4] = { TEXT("Echo_Lumewisp"), TEXT("Echo_Stonehide"), TEXT("Echo_Duskmoth"), TEXT("Echo_Sprigling") };

    for (int32 i = 0; i < WildEchoCount; ++i)
    {
        UAstrawildEchoDefinition* Definition = Registry->FindEcho(Species[i % 4]);
        if (!Definition)
        {
            continue;
        }

        const FVector Location(
            RandomStream.FRandRange(-ArenaSize * 0.8f, ArenaSize * 0.8f),
            RandomStream.FRandRange(-ArenaSize * 0.8f, ArenaSize * 0.8f),
            150.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
        if (Echo)
        {
            Echo->InitializeFromDefinition(Definition);
        }
    }

    // Batch 5 — the Ancient-rare: exactly ONE Auroraling per world, placed far
    // from the spawn camp so encountering it feels earned (hardest capture 0.95).
    if (UAstrawildEchoDefinition* Aurora = Registry->FindEcho(TEXT("Echo_Auroraling")))
    {
        const FVector AuroraLocation(
            RandomStream.FRandRange(-ArenaSize, ArenaSize),
            RandomStream.FRandRange(-ArenaSize, ArenaSize),
            150.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(), AuroraLocation, FRotator::ZeroRotator, Params))
        {
            Echo->InitializeFromDefinition(Aurora);
            UE_LOG(LogAstrawild, Log, TEXT("Ancient-rare Auroraling seeded at %s."), *AuroraLocation.ToCompactString());
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnHostiles()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry)
    {
        return;
    }

    UAstrawildEchoDefinition* Gloomfang = Registry->FindEcho(TEXT("Echo_Gloomfang"));
    UAstrawildEchoDefinition* Emberfang = Registry->FindEcho(TEXT("Echo_Emberfang"));
    UAstrawildEchoDefinition* Rimefang = Registry->FindEcho(TEXT("Echo_Rimefang")); // Batch 5 — Frost line.
    if (!Gloomfang)
    {
        return;
    }

    for (int32 i = 0; i < HostileCount; ++i)
    {
        // Rotate night stalker / ember predator / frost stalker (content waves 2 + 5).
        UAstrawildEchoDefinition* HostileDef = Gloomfang;
        if (i % 3 == 1)
        {
            HostileDef = Emberfang ? Emberfang : Gloomfang;
        }
        else if (i % 3 == 2)
        {
            HostileDef = Rimefang ? Rimefang : Gloomfang;
        }
        const FVector Location(
            RandomStream.FRandRange(-ArenaSize * 0.9f, ArenaSize * 0.9f),
            RandomStream.FRandRange(-ArenaSize * 0.9f, ArenaSize * 0.9f),
            150.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
        if (Echo)
        {
            Echo->InitializeFromDefinition(HostileDef);
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnPointsOfInterest()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Starting camp around spawn (directive §21 starting camp + first base location).
    const float CampRadius = 900.0f;

    // Rest point — revive anchor.
    AAstrawildRestPoint* Rest = World->SpawnActor<AAstrawildRestPoint>(AAstrawildRestPoint::StaticClass(), FVector(CampRadius, 0.0f, 100.0f), FRotator::ZeroRotator, Params);

    // Crafting stations: workbench + campfire.
    AAstrawildCraftingStationActor* Workbench = World->SpawnActor<AAstrawildCraftingStationActor>(AAstrawildCraftingStationActor::StaticClass(), FVector(0.0f, CampRadius, 100.0f), FRotator::ZeroRotator, Params);
    if (Workbench)
    {
        Workbench->StationId = TEXT("Station_Workbench");
    }

    AAstrawildCraftingStationActor* Campfire = World->SpawnActor<AAstrawildCraftingStationActor>(AAstrawildCraftingStationActor::StaticClass(), FVector(0.0f, -CampRadius, 100.0f), FRotator::ZeroRotator, Params);
    if (Campfire)
    {
        Campfire->StationId = TEXT("Station_Campfire");
    }

    // Work sites for captured Echoes (directive §18): gathering + farming.
    AAstrawildWorkSiteActor* GatheringSite = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), FVector(-CampRadius, 0.0f, 100.0f), FRotator::ZeroRotator, Params);
    if (GatheringSite)
    {
        GatheringSite->WorkType = EAstrawildWorkType::Gathering;
        GatheringSite->OutputItemId = TEXT("Item_Fiber");
        GatheringSite->SecondsPerOutput = 10.0f;
        GatheringSite->bRequiresPower = false;
    }

    // NPCs (wave 3, directive §26): the warden offering the first quest + a trader.
    UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
    if (Registry)
    {
        if (AAstrawildNPCCharacter* Warden = World->SpawnActor<AAstrawildNPCCharacter>(AAstrawildNPCCharacter::StaticClass(), FVector(CampRadius * 0.7f, -CampRadius * 0.7f, 100.0f), FRotator::ZeroRotator, Params))
        {
            Warden->NpcDefinition = Registry->FindNPCDefinition(TEXT("NPC_WardenMaren"));
        }
        if (AAstrawildNPCCharacter* Trader = World->SpawnActor<AAstrawildNPCCharacter>(AAstrawildNPCCharacter::StaticClass(), FVector(-CampRadius * 0.7f, -CampRadius * 0.7f, 100.0f), FRotator::ZeroRotator, Params))
        {
            Trader->NpcDefinition = Registry->FindNPCDefinition(TEXT("NPC_VendorTam"));
        }
    }

    AAstrawildWorkSiteActor* FarmSite = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), FVector(-CampRadius * 0.7f, CampRadius * 0.7f, 100.0f), FRotator::ZeroRotator, Params);
    if (FarmSite)
    {
        FarmSite->WorkType = EAstrawildWorkType::Farming;
        FarmSite->OutputItemId = TEXT("Item_Berry");
        FarmSite->SecondsPerOutput = 14.0f;
        FarmSite->bRequiresPower = false;
    }

    // First dungeon (directive §21/§23): the Hollow Underlight — placed beyond the eastern wilds.
    const FVector DungeonLocation(ArenaSize * 1.4f, 0.0f, 100.0f);
    AAstrawildDungeonGeneratorActor* Dungeon = World->SpawnActor<AAstrawildDungeonGeneratorActor>(
        AAstrawildDungeonGeneratorActor::StaticClass(), DungeonLocation, FRotator::ZeroRotator, Params);
    if (Dungeon)
    {
        Dungeon->DungeonId = TEXT("Dungeon_HollowUnderlight"); // Batch 6: stable id for the save system.
        Dungeon->RoomCount = 5;
        Dungeon->BossDefinitionId = TEXT("Echo_Gloomfang");
        // Generate() runs in BeginPlay on the server.
    }

    // Batch 6 — Item C: portal pair. The entrance pad sits at the wilds' edge
    // (walkable from camp); the exit pad waits beside the dungeon's entry room.
    // Both publish Event.LocationReached so ReachLocation objectives can fire.
    const FVector EntranceLocation(ArenaSize * 1.05f, 0.0f, 100.0f);
    if (AAstrawildDungeonPortalActor* Entrance = World->SpawnActor<AAstrawildDungeonPortalActor>(
        AAstrawildDungeonPortalActor::StaticClass(), EntranceLocation, FRotator::ZeroRotator, Params))
    {
        Entrance->PortalId = TEXT("Location_HollowUnderlight");
        Entrance->PromptText = FText::FromString(TEXT("Enter the Hollow Underlight [E]"));
        Entrance->Destination = DungeonLocation + FVector(0.0f, 0.0f, 150.0f);
    }
    if (AAstrawildDungeonPortalActor* Exit = World->SpawnActor<AAstrawildDungeonPortalActor>(
        AAstrawildDungeonPortalActor::StaticClass(), DungeonLocation + FVector(0.0f, 900.0f, 0.0f), FRotator::ZeroRotator, Params))
    {
        Exit->PortalId = TEXT("Location_DawnCamp");
        Exit->PromptText = FText::FromString(TEXT("Return to the Dawn Camp [E]"));
        Exit->Destination = EntranceLocation + FVector(-300.0f, 0.0f, 0.0f);
    }
}

void AAstrawildWorldBootstrapper::UpdateSunRotation()
{
    AAstrawildGameState* GameState = GetGameState();
    if (!SunLight || !GameState)
    {
        return;
    }

    // Sun arc: 06:00 sunrise (pitch -5) to 19:00 sunset (pitch -175) — moon-ish at night.
    const float SunAlpha = GameState->GetSunCycleAlpha();
    const float Pitch = FMath::Lerp(-5.0f, -175.0f, SunAlpha);
    SunLight->SetActorRotation(FRotator(Pitch, 30.0f, 0.0f));

    if (ULightComponent* LightComponent = SunLight->GetLightComponent())
    {
        // Dim to moonlight levels at night (directive §13).
        const float Intensity = GameState->IsNight() ? 0.4f : FMath::Lerp(0.8f, 9.0f, SunAlpha);
        LightComponent->SetIntensity(Intensity);
    }
}

void AAstrawildWorldBootstrapper::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (GetLocalRole() == ROLE_Authority || GetNetMode() == NM_Standalone)
    {
        UpdateSunRotation();
    }
}
