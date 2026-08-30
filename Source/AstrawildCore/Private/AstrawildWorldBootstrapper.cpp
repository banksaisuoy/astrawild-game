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
#include "AstrawildTerrainTileActor.h"
#include "AstrawildWorkSiteActor.h"
#include "AstrawildZoneSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"

// Engine light classes for the runtime lighting rig.
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/SkyAtmosphere.h"
#include "Engine/SkyLight.h"

namespace
{
    // Engine basic shape asset paths (zero-asset world).
    const TCHAR* ShapeCube = TEXT("/Engine/BasicShapes/Cube.Cube");
    const TCHAR* ShapeSphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    const TCHAR* ShapeCylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    const TCHAR* ShapeCone = TEXT("/Engine/BasicShapes/Cone.Cone");

    // Per-zone wildlife table (Batch 7): species placement that makes the six
    // regions read differently — herd critters near camp, element lines at home.
    struct FZoneWildlifeRow
    {
        EAstrawildZone Zone;
        FName SpeciesId;
        int32 Count;
    };

    const FZoneWildlifeRow ZoneWildlife[] = {
        { EAstrawildZone::DuskMarsh, TEXT("Echo_Duskmoth"), 3 },
        { EAstrawildZone::DuskMarsh, TEXT("Echo_Sprigling"), 2 },
        { EAstrawildZone::Glimmerwood, TEXT("Echo_Voltmaw"), 1 },
        { EAstrawildZone::Glimmerwood, TEXT("Echo_Sprigling"), 2 },
        { EAstrawildZone::EmberRidge, TEXT("Echo_Emberfang"), 2 },
        { EAstrawildZone::EmberRidge, TEXT("Echo_Stonehide"), 1 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Echo_Rimefang"), 2 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Echo_Stonehide"), 1 },
        { EAstrawildZone::HollowApproach, TEXT("Echo_Gloomfang"), 2 },
    };

    // Per-zone resource nodes: signature materials per region.
    struct FZoneResourceRow
    {
        EAstrawildZone Zone;
        FName ItemId;
        int32 Count;
    };

    const FZoneResourceRow ZoneResources[] = {
        { EAstrawildZone::DuskMarsh, TEXT("Item_Fiber"), 8 },
        { EAstrawildZone::DuskMarsh, TEXT("Item_Wood"), 5 },
        { EAstrawildZone::Glimmerwood, TEXT("Item_Wood"), 6 },
        { EAstrawildZone::Glimmerwood, TEXT("Item_CrystalShard"), 5 },
        { EAstrawildZone::EmberRidge, TEXT("Item_Stone"), 7 },
        { EAstrawildZone::EmberRidge, TEXT("Item_EmberAsh"), 5 },
        { EAstrawildZone::EmberRidge, TEXT("Item_CrystalShard"), 4 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Item_Stone"), 6 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Item_CrystalShard"), 3 },
        { EAstrawildZone::HollowApproach, TEXT("Item_Stone"), 4 },
        { EAstrawildZone::HollowApproach, TEXT("Item_CrystalShard"), 4 },
    };
}

AAstrawildWorldBootstrapper::AAstrawildWorldBootstrapper()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.25f; // Sun tracking + light flicker — cheap.

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

AAstrawildGameState* AAstrawildWorldBootstrapper::GetGameState() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<AAstrawildGameState>() : nullptr;
}

FVector2D AAstrawildWorldBootstrapper::GetCampCenterXY()
{
    // Dawn Fields zone center (see UAstrawildZoneSubsystem zone table).
    return FVector2D(0.0f, -40000.0f);
}

FVector2D AAstrawildWorldBootstrapper::GetDungeonCenterXY()
{
    // Hollow Approach zone center — the wilds before the Underlight gate.
    return FVector2D(80000.0f, -40000.0f);
}

float AAstrawildWorldBootstrapper::GroundZ(const FVector2D& WorldXY) const
{
    return AAstrawildTerrainTileActor::EvalWorldHeight(WorldXY, WorldSeedCached);
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
        WorldSeedCached = Seed;
    }
    else
    {
        RandomStream = FRandomStream(1337);
        WorldSeedCached = 1337;
    }

    BuildLighting();

    if (bBuildTerrain)
    {
        BuildTerrain();
    }

    ScatterResourceNodes();

    if (bPopulateWildlife)
    {
        SpawnWildEchoes();
        SpawnHostiles();
    }

    SpawnPointsOfInterest();

    if (bBuildLandmarks)
    {
        BuildZoneLandmarks();
    }

    // Audit C-3: kick the navmesh build — tiles generate around navigation invokers
    // (player + Echoes; see DefaultEngine.ini) so pathfinding works from frame one.
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        NavSys->Build();
        UE_LOG(LogAstrawildWorld, Log, TEXT("Runtime navmesh build requested (invoker-driven generation)."));
    }

    const int32 ZoneCount = UAstrawildZoneSubsystem::GetZoneCount();
    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Shattered Vale bootstrapped: %d zones, %d camp nodes, %d camp Echoes, %d camp hostiles, %d zone wildlife rows, %d zone resource rows."),
        ZoneCount, ResourceNodeCount, WildEchoCount, HostileCount,
        static_cast<int32>(sizeof(ZoneWildlife) / sizeof(ZoneWildlife[0])),
        static_cast<int32>(sizeof(ZoneResources) / sizeof(ZoneResources[0])));
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

void AAstrawildWorldBootstrapper::BuildTerrain()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (const FAstrawildZoneDescriptor& ZoneDesc : UAstrawildZoneSubsystem::GetAllZones())
    {
        const FVector TileOrigin(ZoneDesc.Bounds.Min.X, ZoneDesc.Bounds.Min.Y, 0.0f);
        AAstrawildTerrainTileActor* Tile = World->SpawnActor<AAstrawildTerrainTileActor>(
            AAstrawildTerrainTileActor::StaticClass(), TileOrigin, FRotator::ZeroRotator, Params);
        if (Tile)
        {
            Tile->BuildTile(ZoneDesc, WorldSeedCached, TerrainResolution);
        }
    }

    // Fallback player start at the camp so spawning always works.
    const FVector2D CampXY = GetCampCenterXY();
    World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),
        FVector(CampXY.X, CampXY.Y, GroundZ(CampXY) + 120.0f), FRotator::ZeroRotator, Params);
}

AStaticMeshActor* AAstrawildWorldBootstrapper::SpawnShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, Params);
    if (Actor)
    {
        Actor->SetMobility(EComponentMobility::Movable);
        if (UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent())
        {
            UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
            if (LoadedMesh)
            {
                Mesh->SetStaticMesh(LoadedMesh);
            }
            Mesh->SetWorldScale3D(Scale);
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        }
    }
    return Actor;
}

void AAstrawildWorldBootstrapper::SpawnZoneLight(const FVector& Location, const FLinearColor& Color, const float Intensity, const float AttenuationRadius, const bool bFlicker)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (!Light)
    {
        return;
    }

    Light->SetMobility(EComponentMobility::Movable);
    if (ULightComponent* LightComponent = Light->GetLightComponent())
    {
        LightComponent->SetLightColor(Color);
        LightComponent->SetIntensity(Intensity);
    }
    if (UPointLightComponent* PointComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
    {
        PointComponent->SetAttenuationRadius(AttenuationRadius);
    }

    if (bFlicker)
    {
        FlickerLights.Add(Light);
        FlickerBaseIntensity.Add(Intensity);
        FlickerPhase.Add(RandomStream.FRandRange(0.0f, 2.0f * PI));
    }
}

FVector AAstrawildWorldBootstrapper::RandomPointInZone(const FAstrawildZoneDescriptor& Zone, const float MinDistanceToCenter)
{
    const FVector2D Center = Zone.GetCenter();
    for (int32 Attempt = 0; Attempt < 8; ++Attempt)
    {
        const FVector2D Point(
            RandomStream.FRandRange(Zone.Bounds.Min.X + 4000.0f, Zone.Bounds.Max.X - 4000.0f),
            RandomStream.FRandRange(Zone.Bounds.Min.Y + 4000.0f, Zone.Bounds.Max.Y - 4000.0f));
        if (FVector2D::Distance(Point, Center) >= MinDistanceToCenter)
        {
            return FVector(Point.X, Point.Y, GroundZ(Point));
        }
    }
    return FVector(Center.X + MinDistanceToCenter, Center.Y, GroundZ(Center));
}

void AAstrawildWorldBootstrapper::ScatterResourceNodes()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // --- Dawn Fields: the classic camp ring (legacy knobs, directive §21) ---
    const FVector2D CampXY = GetCampCenterXY();
    const FName ResourceIds[3] = { TEXT("Item_Wood"), TEXT("Item_Stone"), TEXT("Item_Fiber") };

    for (int32 i = 0; i < ResourceNodeCount; ++i)
    {
        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius = RandomStream.FRandRange(1600.0f, 6800.0f);
        const FVector2D Point(CampXY.X + Radius * FMath::Cos(Angle), CampXY.Y + Radius * FMath::Sin(Angle));

        AAstrawildResourceNode* Node = World->SpawnActor<AAstrawildResourceNode>(AAstrawildResourceNode::StaticClass(),
            FVector(Point.X, Point.Y, GroundZ(Point) + 100.0f), FRotator::ZeroRotator, Params);
        if (Node)
        {
            Node->ResourceItemId = ResourceIds[i % 3];
            Node->ResourceQuantityPerHarvest = 2;
            Node->RemainingQuantity = 3;
        }
    }

    // --- Outer zones: signature materials per region ---
    for (const FZoneResourceRow& Row : ZoneResources)
    {
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(Row.Zone);
        if (!ZoneDesc)
        {
            continue;
        }

        for (int32 i = 0; i < Row.Count; ++i)
        {
            const FVector Point = RandomPointInZone(*ZoneDesc, 2500.0f);
            AAstrawildResourceNode* Node = World->SpawnActor<AAstrawildResourceNode>(AAstrawildResourceNode::StaticClass(),
                FVector(Point.X, Point.Y, Point.Z + 100.0f), FRotator::ZeroRotator, Params);
            if (Node)
            {
                Node->ResourceItemId = Row.ItemId;
                Node->ResourceQuantityPerHarvest = 2;
                Node->RemainingQuantity = 3;
            }
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

    // --- Dawn Fields: day-active friendly species around camp (legacy knob) ---
    const FVector2D CampXY = GetCampCenterXY();
    const FName Species[4] = { TEXT("Echo_Lumewisp"), TEXT("Echo_Stonehide"), TEXT("Echo_Duskmoth"), TEXT("Echo_Sprigling") };

    for (int32 i = 0; i < WildEchoCount; ++i)
    {
        UAstrawildEchoDefinition* Definition = Registry->FindEcho(Species[i % 4]);
        if (!Definition)
        {
            continue;
        }

        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius = RandomStream.FRandRange(2400.0f, 9000.0f);
        const FVector2D Point(CampXY.X + Radius * FMath::Cos(Angle), CampXY.Y + Radius * FMath::Sin(Angle));

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(),
            FVector(Point.X, Point.Y, GroundZ(Point) + 150.0f), FRotator::ZeroRotator, Params);
        if (Echo)
        {
            Echo->InitializeFromDefinition(Definition);
        }
    }

    // --- Outer zones: species lines in their home regions ---
    for (const FZoneWildlifeRow& Row : ZoneWildlife)
    {
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(Row.Zone);
        UAstrawildEchoDefinition* Definition = ZoneDesc ? Registry->FindEcho(Row.SpeciesId) : nullptr;
        if (!Definition)
        {
            continue;
        }

        for (int32 i = 0; i < Row.Count; ++i)
        {
            const FVector Point = RandomPointInZone(*ZoneDesc, 3000.0f);

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(),
                FVector(Point.X, Point.Y, Point.Z + 150.0f), FRotator::ZeroRotator, Params);
            if (Echo)
            {
                Echo->InitializeFromDefinition(Definition);
            }
        }
    }

    // Batch 5 — the Ancient-rare: exactly ONE Auroraling per world, deep in the
    // Glimmerwood (the zone farthest thematically from the camp), so the
    // encounter feels earned (hardest capture 0.95).
    if (const FAstrawildZoneDescriptor* Glimmerwood = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_Glimmerwood")))
    {
        if (UAstrawildEchoDefinition* Aurora = Registry->FindEcho(TEXT("Echo_Auroraling")))
        {
            const FVector Point = RandomPointInZone(*Glimmerwood, 20000.0f);
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(),
                FVector(Point.X, Point.Y, Point.Z + 150.0f), FRotator::ZeroRotator, Params))
            {
                Echo->InitializeFromDefinition(Aurora);
                UE_LOG(LogAstrawild, Log, TEXT("Ancient-rare Auroraling seeded in the Glimmerwood at %s."), *Point.ToCompactString());
            }
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
    if (!Gloomfang)
    {
        return;
    }

    // Dawn Fields: the night threat still patrols the home wilds (legacy knob).
    const FVector2D CampXY = GetCampCenterXY();
    for (int32 i = 0; i < HostileCount; ++i)
    {
        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius = RandomStream.FRandRange(4200.0f, 9000.0f);
        const FVector2D Point(CampXY.X + Radius * FMath::Cos(Angle), CampXY.Y + Radius * FMath::Sin(Angle));

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(),
            FVector(Point.X, Point.Y, GroundZ(Point) + 150.0f), FRotator::ZeroRotator, Params);
        if (Echo)
        {
            Echo->InitializeFromDefinition(Gloomfang);
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

    // Starting camp at the Dawn Fields center (directive §21 starting camp).
    const FVector2D CampXY = GetCampCenterXY();
    const float CampRadius = 900.0f;
    const float CampZ = GroundZ(CampXY);
    const auto CampLocation = [CampXY, CampZ](const float OffsetX, const float OffsetY) -> FVector
    {
        return FVector(CampXY.X + OffsetX, CampXY.Y + OffsetY, CampZ + 100.0f);
    };

    // Rest point — revive anchor.
    World->SpawnActor<AAstrawildRestPoint>(AAstrawildRestPoint::StaticClass(), CampLocation(CampRadius, 0.0f), FRotator::ZeroRotator, Params);

    // Crafting stations: workbench + campfire.
    if (AAstrawildCraftingStationActor* Workbench = World->SpawnActor<AAstrawildCraftingStationActor>(AAstrawildCraftingStationActor::StaticClass(), CampLocation(0.0f, CampRadius), FRotator::ZeroRotator, Params))
    {
        Workbench->StationId = TEXT("Station_Workbench");
    }

    if (AAstrawildCraftingStationActor* Campfire = World->SpawnActor<AAstrawildCraftingStationActor>(AAstrawildCraftingStationActor::StaticClass(), CampLocation(0.0f, -CampRadius), FRotator::ZeroRotator, Params))
    {
        Campfire->StationId = TEXT("Station_Campfire");
    }

    // Work sites for captured Echoes (directive §18): gathering + farming.
    // Final production run: stable SiteIds so save/load can re-link workers.
    if (AAstrawildWorkSiteActor* GatheringSite = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), CampLocation(-CampRadius, 0.0f), FRotator::ZeroRotator, Params))
    {
        GatheringSite->SiteId = TEXT("Site_CampGathering");
        GatheringSite->WorkType = EAstrawildWorkType::Gathering;
        GatheringSite->OutputItemId = TEXT("Item_Fiber");
        GatheringSite->SecondsPerOutput = 10.0f;
        GatheringSite->bRequiresPower = false;
    }

    if (AAstrawildWorkSiteActor* FarmSite = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), CampLocation(-CampRadius * 0.7f, CampRadius * 0.7f), FRotator::ZeroRotator, Params))
    {
        FarmSite->SiteId = TEXT("Site_CampFarm");
        FarmSite->WorkType = EAstrawildWorkType::Farming;
        FarmSite->OutputItemId = TEXT("Item_Berry");
        FarmSite->SecondsPerOutput = 14.0f;
        FarmSite->bRequiresPower = false;
    }

    // NPCs (wave 3, directive §26): the warden offering the first quest + a trader.
    UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
    if (Registry)
    {
        if (AAstrawildNPCCharacter* Warden = World->SpawnActor<AAstrawildNPCCharacter>(AAstrawildNPCCharacter::StaticClass(), CampLocation(CampRadius * 0.7f, -CampRadius * 0.7f), FRotator::ZeroRotator, Params))
        {
            Warden->NpcDefinition = Registry->FindNPCDefinition(TEXT("NPC_WardenMaren"));
        }
        if (AAstrawildNPCCharacter* Trader = World->SpawnActor<AAstrawildNPCCharacter>(AAstrawildNPCCharacter::StaticClass(), CampLocation(-CampRadius * 0.7f, -CampRadius * 0.7f), FRotator::ZeroRotator, Params))
        {
            Trader->NpcDefinition = Registry->FindNPCDefinition(TEXT("NPC_VendorTam"));
        }
    }

    // First dungeon (directive §21/§23): the Hollow Underlight, at the heart of
    // the Hollow Approach — the ash wilds east of the Dawn Fields.
    const FVector2D DungeonXY = GetDungeonCenterXY();
    const FVector DungeonLocation(DungeonXY.X, DungeonXY.Y, GroundZ(DungeonXY) + 300.0f);
    if (AAstrawildDungeonGeneratorActor* Dungeon = World->SpawnActor<AAstrawildDungeonGeneratorActor>(
        AAstrawildDungeonGeneratorActor::StaticClass(), DungeonLocation, FRotator::ZeroRotator, Params))
    {
        Dungeon->DungeonId = TEXT("Dungeon_HollowUnderlight"); // Batch 6: stable id for the save system.
        Dungeon->RoomCount = 5;
        Dungeon->BossDefinitionId = TEXT("Echo_Gloomfang");
        // Generate() runs in BeginPlay on the server.
    }

    // Batch 6 — Item C: portal pair. The entrance pad sits at the Hollow Approach
    // border, walkable from the camp; the exit pad waits beside the dungeon's
    // entry room. Both publish Event.LocationReached so ReachLocation objectives fire.
    const FVector2D EntranceXY(52000.0f, -40000.0f);
    const FVector EntranceLocation(EntranceXY.X, EntranceXY.Y, GroundZ(EntranceXY) + 100.0f);
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

void AAstrawildWorldBootstrapper::BuildZoneLandmarks()
{
    // Signature silhouettes + tinted light per zone. Shapes use engine basics;
    // color identity comes from the point lights (zero-asset constraint §50).
    using EZone = EAstrawildZone;

    for (const FAstrawildZoneDescriptor& ZoneDesc : UAstrawildZoneSubsystem::GetAllZones())
    {
        const float MinDist = 4500.0f;

        switch (ZoneDesc.Zone)
        {
        case EZone::DawnFields:
        {
            // Dawnwood groves + glowcap clusters around the camp.
            for (int32 i = 0; i < 10; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, 12000.0f);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 175), FVector(1.2, 1.2, 3.5), FRotator::ZeroRotator);
                SpawnShape(ShapeCone, Point + FVector(0, 0, 550), FVector(4.2, 4.2, 5.2), FRotator::ZeroRotator);
            }
            for (int32 i = 0; i < 3; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, 9000.0f);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 60), FVector(0.5, 0.5, 1.2), FRotator::ZeroRotator);
                SpawnShape(ShapeSphere, Point + FVector(0, 0, 140), FVector(1.6, 1.6, 1.0), FRotator::ZeroRotator);
                SpawnZoneLight(Point + FVector(0, 0, 220), FLinearColor(0.3f, 0.95f, 0.7f), 9000.0f, 1800.0f, true);
            }
            break;
        }

        case EZone::DuskMarsh:
        {
            // Dead trees, muck pools and flickering marsh wisps.
            for (int32 i = 0; i < 7; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                const FRotator Lean(RandomStream.FRandRange(-8.0f, 8.0f), RandomStream.FRandRange(0.0f, 360.0f), 0.0f);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 300), FVector(0.9, 0.9, 6.0), Lean);
                SpawnShape(ShapeCube, Point + FVector(180, 0, 520), FVector(0.6, 0.6, 3.0), FRotator(35.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f));
            }
            for (int32 i = 0; i < 5; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                const float PoolScale = RandomStream.FRandRange(3.5f, 5.5f);
                SpawnShape(ShapeSphere, Point + FVector(0, 0, 15), FVector(PoolScale, PoolScale, 0.4f), FRotator::ZeroRotator);
                SpawnZoneLight(Point + FVector(0, 0, 350), ZoneDesc.AmbientLightColor, 14000.0f, 2600.0f, true);
            }
            for (int32 i = 0; i < 8; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 90), FVector(0.25, 0.25, 1.8), FRotator::ZeroRotator);
                SpawnShape(ShapeCylinder, Point + FVector(60, 30, 70), FVector(0.2, 0.2, 1.4), FRotator(0, 0, 12.0f));
            }
            break;
        }

        case EZone::EmberRidge:
        {
            // Obsidian spires, lava mounds with flickering ember light, ember rocks.
            for (int32 i = 0; i < 6; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                const float Height = RandomStream.FRandRange(8.0f, 14.0f);
                SpawnShape(ShapeCube, Point + FVector(0, 0, Height * 50.0f), FVector(RandomStream.FRandRange(1.5f, 2.6f), RandomStream.FRandRange(1.5f, 2.6f), Height),
                    FRotator(RandomStream.FRandRange(-4.0f, 4.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-3.0f, 3.0f)));
            }
            for (int32 i = 0; i < 4; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnShape(ShapeSphere, Point + FVector(0, 0, 40), FVector(3.6, 3.6, 1.1f), FRotator::ZeroRotator);
                SpawnZoneLight(Point + FVector(0, 0, 260), ZoneDesc.AmbientLightColor, 28000.0f, 3200.0f, true);
            }
            for (int32 i = 0; i < 6; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnShape(ShapeCube, Point + FVector(0, 0, 60), FVector(0.9, 0.9, 0.9),
                    FRotator(RandomStream.FRandRange(-25.0f, 25.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-25.0f, 25.0f)));
            }
            break;
        }

        case EZone::FrostveilExpanse:
        {
            // Ice pillars with cold rim light + snow drifts.
            for (int32 i = 0; i < 7; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                const float Height = RandomStream.FRandRange(6.0f, 12.0f);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, Height * 50.0f), FVector(RandomStream.FRandRange(1.0f, 1.8f), RandomStream.FRandRange(1.0f, 1.8f), Height),
                    FRotator(RandomStream.FRandRange(-3.0f, 3.0f), RandomStream.FRandRange(0.0f, 360.0f), 0.0f));
            }
            for (int32 i = 0; i < 4; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnZoneLight(Point + FVector(0, 0, 400), ZoneDesc.AmbientLightColor, 18000.0f, 3000.0f, false);
            }
            for (int32 i = 0; i < 4; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnShape(ShapeSphere, Point + FVector(0, 0, 30), FVector(4.0, 4.0, 0.8f), FRotator::ZeroRotator);
            }
            break;
        }

        case EZone::Glimmerwood:
        {
            // Crystal spires with violet light + glimmer trees.
            for (int32 i = 0; i < 9; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                const float Height = RandomStream.FRandRange(5.0f, 10.0f);
                SpawnShape(ShapeCone, Point + FVector(0, 0, Height * 50.0f), FVector(RandomStream.FRandRange(1.0f, 2.0f), RandomStream.FRandRange(1.0f, 2.0f), Height),
                    FRotator(RandomStream.FRandRange(-6.0f, 6.0f), RandomStream.FRandRange(0.0f, 360.0f), 0.0f));
                if (i % 2 == 0)
                {
                    SpawnZoneLight(Point + FVector(0, 0, 320), ZoneDesc.AmbientLightColor, 20000.0f, 2800.0f, true);
                }
            }
            for (int32 i = 0; i < 5; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, MinDist);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 175), FVector(1.0, 1.0, 3.5), FRotator::ZeroRotator);
                SpawnShape(ShapeCone, Point + FVector(0, 0, 520), FVector(3.6, 3.6, 4.6), FRotator::ZeroRotator);
            }
            break;
        }

        case EZone::HollowApproach:
        {
            // Ash spires, charred trees and dim blood-ash light over the wilds.
            for (int32 i = 0; i < 5; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, 8000.0f);
                const float Height = RandomStream.FRandRange(6.0f, 11.0f);
                SpawnShape(ShapeCube, Point + FVector(0, 0, Height * 50.0f), FVector(RandomStream.FRandRange(1.2f, 2.2f), RandomStream.FRandRange(1.2f, 2.2f), Height),
                    FRotator(RandomStream.FRandRange(-6.0f, 6.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-5.0f, 5.0f)));
            }
            for (int32 i = 0; i < 4; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, 8000.0f);
                SpawnShape(ShapeCylinder, Point + FVector(0, 0, 260), FVector(1.0, 1.0, 5.2), FRotator(RandomStream.FRandRange(-10.0f, 10.0f), RandomStream.FRandRange(0.0f, 360.0f), 0.0f));
            }
            for (int32 i = 0; i < 2; ++i)
            {
                const FVector Point = RandomPointInZone(ZoneDesc, 12000.0f);
                SpawnZoneLight(Point + FVector(0, 0, 500), ZoneDesc.AmbientLightColor, 16000.0f, 3400.0f, false);
            }
            break;
        }

        default:
            break;
        }
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

void AAstrawildWorldBootstrapper::UpdateFlickerLights(const float TimeSeconds)
{
    for (int32 i = 0; i < FlickerLights.Num() && i < FlickerBaseIntensity.Num(); ++i)
    {
        APointLight* Light = FlickerLights[i];
        if (!Light)
        {
            continue;
        }

        const float Phase = FlickerPhase.IsValidIndex(i) ? FlickerPhase[i] : 0.0f;
        const float Pulse = 0.5f + 0.5f * FMath::Sin(TimeSeconds * 4.5f + Phase);
        const float Noise = 0.5f + 0.5f * FMath::Sin(TimeSeconds * 11.3f + Phase * 2.7f);
        const float Multiplier = 0.72f + 0.28f * (0.65f * Pulse + 0.35f * Noise);

        if (ULightComponent* LightComponent = Light->GetLightComponent())
        {
            LightComponent->SetIntensity(FlickerBaseIntensity[i] * Multiplier);
        }
    }
}

void AAstrawildWorldBootstrapper::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (GetLocalRole() == ROLE_Authority || GetNetMode() == NM_Standalone)
    {
        UpdateSunRotation();
        UpdateFlickerLights(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
    }
}
