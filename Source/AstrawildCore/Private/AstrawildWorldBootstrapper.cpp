#include "AstrawildWorldBootstrapper.h"

#include "AstrawildBiomeDressingActor.h"
#include "AstrawildCraftingStationActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDungeonGeneratorActor.h"
#include "AstrawildDungeonPortalActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPOIMarkerActor.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildResourceNode.h"
#include "AstrawildRestPoint.h"
#include "AstrawildSkiffActor.h"
#include "AstrawildTerrainTileActor.h"
#include "AstrawildVillageActor.h"
#include "AstrawildWaterPlaneActor.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildWorkSiteActor.h"
#include "AstrawildZoneSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"

namespace
{
    // Engine basic shape asset paths (zero-asset world).
    static constexpr const TCHAR* const ShapeCube = TEXT("/Engine/BasicShapes/Cube.Cube");
    static constexpr const TCHAR* const ShapeSphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
    static constexpr const TCHAR* const ShapeCylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    static constexpr const TCHAR* const ShapeCone = TEXT("/Engine/BasicShapes/Cone.Cone");

    // Per-zone wildlife table (Batch 7 + Batch 8): species placement that makes
    // the twelve regions read differently — herd critters near camp, element
    // lines at home. Batch 8 rows pull from the generated bestiary
    // (Docs/ASTRAWILD_BESTIARY_CODEX.md) — signature species per new zone.
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
        // --- Batch 8 zones: bestiary signature lines ---
        { EAstrawildZone::AzureShallows, TEXT("Echo_Brinefin"), 2 },
        { EAstrawildZone::AzureShallows, TEXT("Echo_Saltcrest"), 2 },
        { EAstrawildZone::AzureShallows, TEXT("Echo_Undertowray"), 1 },
        { EAstrawildZone::TidebreakerIsles, TEXT("Echo_Wavecrest"), 2 },
        { EAstrawildZone::TidebreakerIsles, TEXT("Echo_Mistwing"), 2 },
        { EAstrawildZone::TidebreakerIsles, TEXT("Echo_Voidwing"), 1 },
        { EAstrawildZone::SunscarDesert, TEXT("Echo_Sunhide"), 2 },
        { EAstrawildZone::SunscarDesert, TEXT("Echo_Glimmerhornet"), 2 },
        { EAstrawildZone::SunscarDesert, TEXT("Echo_Pyreblaze"), 1 },
        { EAstrawildZone::SunscarDesert, TEXT("Echo_Pistongolem"), 1 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Echo_Sunhorn"), 2 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Echo_Magmawing"), 1 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Echo_Geargolem"), 1 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Echo_Frostblaze"), 1 },
        { EAstrawildZone::VerdantReach, TEXT("Echo_Verdantbloom"), 2 },
        { EAstrawildZone::VerdantReach, TEXT("Echo_Fernthorn"), 2 },
        { EAstrawildZone::VerdantReach, TEXT("Echo_Ghostshade"), 1 },
        { EAstrawildZone::VerdantReach, TEXT("Echo_Sunpaw"), 1 },
        { EAstrawildZone::PearlseaReef, TEXT("Echo_Coralray"), 2 },
        { EAstrawildZone::PearlseaReef, TEXT("Echo_Pearlcrest"), 2 },
        { EAstrawildZone::PearlseaReef, TEXT("Echo_Abyssjelly"), 1 },
        { EAstrawildZone::PearlseaReef, TEXT("Echo_Embershade"), 1 },
        // --- Production V2: the role-differentiated production roster ---
        { EAstrawildZone::DawnFields, TEXT("Echo_Terraquill"), 2 },
        { EAstrawildZone::DuskMarsh, TEXT("Echo_Mistmender"), 1 },
        { EAstrawildZone::EmberRidge, TEXT("Echo_Cindermule"), 1 },
        { EAstrawildZone::Glimmerwood, TEXT("Echo_Voltpylon"), 1 },
        { EAstrawildZone::VerdantReach, TEXT("Echo_Bastionbeetle"), 1 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Echo_Deepdelver"), 1 },
    };

    // Per-zone resource nodes: signature NODE DEFINITIONS per region
    // (Production V2 — deterministic node identity, Master Plan §1 P0 fix).
    struct FZoneResourceRow
    {
        EAstrawildZone Zone;
        FName NodeId;
        int32 Count;
    };

    const FZoneResourceRow ZoneResources[] = {
        { EAstrawildZone::DuskMarsh, TEXT("Node_Sunfiber"), 8 },
        { EAstrawildZone::DuskMarsh, TEXT("Node_Dawnwood"), 5 },
        { EAstrawildZone::Glimmerwood, TEXT("Node_Dawnwood"), 6 },
        { EAstrawildZone::Glimmerwood, TEXT("Node_DawnCrystal"), 5 },
        { EAstrawildZone::EmberRidge, TEXT("Node_Fieldstone"), 7 },
        { EAstrawildZone::EmberRidge, TEXT("Node_EmberAsh"), 5 },
        { EAstrawildZone::EmberRidge, TEXT("Node_DawnCrystal"), 4 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Node_Fieldstone"), 6 },
        { EAstrawildZone::FrostveilExpanse, TEXT("Node_DawnCrystal"), 3 },
        { EAstrawildZone::HollowApproach, TEXT("Node_Fieldstone"), 4 },
        { EAstrawildZone::HollowApproach, TEXT("Node_DawnCrystal"), 4 },
        { EAstrawildZone::HollowApproach, TEXT("Node_AncientVein"), 2 },
        // --- Batch 8 zones: signature materials ---
        { EAstrawildZone::SunscarDesert, TEXT("Node_DuneGlass"), 6 },
        { EAstrawildZone::SunscarDesert, TEXT("Node_Fieldstone"), 4 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Node_StormSilver"), 5 },
        { EAstrawildZone::StormcrestHighlands, TEXT("Node_Fieldstone"), 4 },
        { EAstrawildZone::VerdantReach, TEXT("Node_Sunfiber"), 6 },
        { EAstrawildZone::VerdantReach, TEXT("Node_Dawnwood"), 4 },
        { EAstrawildZone::AzureShallows, TEXT("Node_SeaPearl"), 4 },
        { EAstrawildZone::AzureShallows, TEXT("Node_Fieldstone"), 2 },
        { EAstrawildZone::TidebreakerIsles, TEXT("Node_SeaPearl"), 3 },
        { EAstrawildZone::TidebreakerIsles, TEXT("Node_Dawnwood"), 3 },
        { EAstrawildZone::PearlseaReef, TEXT("Node_CoralShard"), 5 },
        { EAstrawildZone::PearlseaReef, TEXT("Node_SeaPearl"), 2 },
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
    // Dawn Fields zone center — resolved from the zone table so the camp follows
    // the grid (Batch 8 moved Dawn Fields to the middle-west cell).
    if (const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_DawnFields")))
    {
        return Zone->GetCenter();
    }
    return FVector2D(0.0f, -40000.0f);
}

FVector2D AAstrawildWorldBootstrapper::GetDungeonCenterXY()
{
    // Hollow Approach zone center — the wilds before the Underlight gate.
    if (const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_HollowApproach")))
    {
        return Zone->GetCenter();
    }
    return FVector2D(80000.0f, -40000.0f);
}

FVector2D AAstrawildWorldBootstrapper::GetSunkenVaultCenterXY()
{
    // Batch 8: the Sunken Vault hides deep in the Tidebreaker Isles, offset from
    // the Driftwood Landing so the portal walk reads as a journey.
    if (const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_TidebreakerIsles")))
    {
        return Zone->GetCenter() + FVector2D(-14000.0f, 12000.0f);
    }
    return FVector2D(-120000.0f, -80000.0f);
}

FVector2D AAstrawildWorldBootstrapper::GetDriftwoodLandingXY()
{
    // Batch 8: the fishing hamlet sits on the Isles' biggest island (dry-spot
    // search runs at spawn; this is the search anchor).
    if (const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_TidebreakerIsles")))
    {
        return Zone->GetCenter() + FVector2D(8000.0f, -6000.0f);
    }
    return FVector2D(-112000.0f, -86000.0f);
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

    // Batch 8 — the Grand Expanse: sea, living villages, aircraft.
    SpawnWaterPlanes();
    SpawnPOIMarkers();
    SpawnVillages();
    SpawnSkiffs();

    if (bBuildLandmarks)
    {
        BuildZoneLandmarks();
    }

    // Production V2 Batch 2: deterministic biome dressing (trees/rocks/grass)
    // runs LAST — every gameplay actor above feeds its exclusion bubble so the
    // scatter never buries the camp, villages, dungeons, POIs or skiff pads.
    if (bBuildBiomeDressing)
    {
        SpawnBiomeDressing();
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
            if (UDirectionalLightComponent* DirLight = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
            {
                DirLight->SetIntensity(65000.0f);
                DirLight->SetLightColor(FLinearColor(1.0f, 0.95f, 0.88f));
                DirLight->bAtmosphereSunLight = true;
                DirLight->AtmosphereSunLightIndex = 0;
                DirLight->CastShadows = true;
            }
            else if (ULightComponent* LightComponent = SunLight->GetLightComponent())
            {
                LightComponent->SetIntensity(65000.0f);
            }
        }
    }

    // Sky light for ambient fill (cached — the atmosphere pass drives intensity).
    if (!SkyLightActor)
    {
        SkyLightActor = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0, 0, 3000), FRotator::ZeroRotator, Params);
        if (SkyLightActor)
        {
            if (USkyLightComponent* SkyComponent = SkyLightActor->GetLightComponent())
            {
                SkyComponent->SetMobility(EComponentMobility::Movable);
                SkyComponent->SetIntensity(3.5f);
                SkyComponent->bRealTimeCapture = true;
                SkyComponent->SetLightColor(FLinearColor(0.85f, 0.92f, 1.0f));
            }
        }
    }

    World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, Params);

    // Exponential height fog (cached — the atmosphere pass colors it every tick;
    // previously spawned with defaults and never touched again).
    if (!HeightFogActor)
    {
        HeightFogActor = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, Params);
        if (HeightFogActor)
        {
            if (UExponentialHeightFogComponent* FogComponent = HeightFogActor->GetComponent())
            {
                FogComponent->SetMobility(EComponentMobility::Movable);
                FogComponent->SetFogDensity(0.00012f);
                FogComponent->SetFogHeightFalloff(0.22f);
                FogComponent->FogInscatteringLuminance = FLinearColor(0.70f, 0.76f, 0.84f);
                FogComponent->SetStartDistance(1500.0f);
            }
        }
    }

    // Production V2 Batch 2: unbound post-process volume — a restrained,
    // production-directed grade (bloom + vignette + gentle contrast/saturation
    // + exposure clamps that keep night playable and noon unblown).
    if (!PostProcessVolume)
    {
        PostProcessVolume = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector(0, 0, 0), FRotator::ZeroRotator, Params);
        if (PostProcessVolume)
        {
            PostProcessVolume->bUnbound = true;
            PostProcessVolume->bEnabled = true;
            PostProcessVolume->Priority = 1;

            FPostProcessSettings& Settings = PostProcessVolume->Settings;
            Settings.bOverride_BloomIntensity = true;
            Settings.BloomIntensity = 0.6f;
            Settings.bOverride_VignetteIntensity = true;
            Settings.VignetteIntensity = 0.35f;
            Settings.bOverride_ColorSaturation = true;
            Settings.ColorSaturation = FVector4(1.05f, 1.05f, 1.05f, 1.05f);
            Settings.bOverride_ColorContrast = true;
            Settings.ColorContrast = FVector4(0.96f, 0.96f, 0.96f, 0.96f);
            Settings.bOverride_AutoExposureMinBrightness = true;
            Settings.AutoExposureMinBrightness = 0.06f;
            Settings.bOverride_AutoExposureMaxBrightness = true;
            Settings.AutoExposureMaxBrightness = 3.2f;
            Settings.bOverride_AutoExposureBias = true;
            Settings.AutoExposureBias = 0.3f;
        }
    }
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

    // --- Dawn Fields: the classic camp ring (legacy knobs, directive §21).
    //     Production V2: nodes now reference DEFINITIONS — identity, quantities,
    //     respawn and rarity-shape visuals resolve deterministically on BeginPlay.
    const FVector2D CampXY = GetCampCenterXY();
    const FName NodeIds[3] = { TEXT("Node_Dawnwood"), TEXT("Node_Fieldstone"), TEXT("Node_Sunfiber") };

    for (int32 i = 0; i < ResourceNodeCount; ++i)
    {
        const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
        const float Radius = RandomStream.FRandRange(1600.0f, 6800.0f);
        const FVector2D Point(CampXY.X + Radius * FMath::Cos(Angle), CampXY.Y + Radius * FMath::Sin(Angle));

        AAstrawildResourceNode* Node = World->SpawnActor<AAstrawildResourceNode>(AAstrawildResourceNode::StaticClass(),
            FVector(Point.X, Point.Y, GroundZ(Point) + 100.0f), FRotator::ZeroRotator, Params);
        if (Node)
        {
            Node->NodeDefinitionId = NodeIds[i % 3];
        }
    }

    // --- Outer zones: signature nodes per region ---
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
                Node->NodeDefinitionId = Row.NodeId;
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
    // Production V2 (Master Plan §7): work sites spawn from DEFINITIONS — the
    // Build→Power→Assign→Work→Consume→Produce loop is data-driven. Camp sites
    // keep their historical placements; new sites (Ridge rig, Camp kitchen) place
    // from their definition's zone + offset.
    if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
    {
        struct FSitePlacement { FName SiteId; FVector Location; };
        const FSitePlacement Placements[] =
        {
            { TEXT("Site_CampGathering"), CampLocation(-CampRadius, 0.0f) },
            { TEXT("Site_CampFarm"), CampLocation(-CampRadius * 0.7f, CampRadius * 0.7f) },
        };
        for (const FSitePlacement& Placement : Placements)
        {
            if (UAstrawildWorkSiteDefinition* SiteDef = Registry->FindWorkSite(Placement.SiteId))
            {
                if (AAstrawildWorkSiteActor* Site = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), Placement.Location, FRotator::ZeroRotator, Params))
                {
                    Site->InitializeFromDefinition(SiteDef);
                }
            }
        }

        // Definition-placed sites outside the camp (zone center + offset).
        for (UAstrawildWorkSiteDefinition* SiteDef : Registry->GetAllWorkSiteDefinitions())
        {
            if (SiteDef->Zone == EAstrawildZone::DawnFields)
            {
                continue; // Camp sites placed above at their historical spots.
            }
            const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(SiteDef->Zone);
            if (!ZoneDesc)
            {
                continue;
            }
            const FVector2D Center = ZoneDesc->GetCenter();
            const FVector2D SiteXY(Center.X + SiteDef->OffsetFromZoneCenter.X, Center.Y + SiteDef->OffsetFromZoneCenter.Y);
            if (AAstrawildWorkSiteActor* Site = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), FVector(SiteXY.X, SiteXY.Y, GroundZ(SiteXY) + 80.0f), FRotator::ZeroRotator, Params))
            {
                Site->InitializeFromDefinition(SiteDef);
            }
        }
    }

    // NPCs: Batch 8 — the full village rosters now spawn through SpawnVillages()
    // (Dawnstead around this camp + Driftwood Landing on the isles); the old
    // inline warden/trader pair was folded into the Dawnstead roster.

    // First dungeon (directive §21/§23): the Hollow Underlight, at the heart of
    // the Hollow Approach — the ash wilds east of the Dawn Fields.
    // Batch 8: dry-spot search — the Approach has pools below sea level now.
    const FVector DungeonLocation = FindDrySpotNear(GetDungeonCenterXY(), 14000.0f) + FVector(0.0f, 0.0f, 300.0f);
    if (AAstrawildDungeonGeneratorActor* Dungeon = World->SpawnActor<AAstrawildDungeonGeneratorActor>(
        AAstrawildDungeonGeneratorActor::StaticClass(), DungeonLocation, FRotator::ZeroRotator, Params))
    {
        Dungeon->DungeonId = TEXT("Dungeon_HollowUnderlight"); // Batch 6: stable id for the save system.
        Dungeon->RoomCount = 5;
        Dungeon->BossDefinitionId = TEXT("Echo_Gloomfang");
        Dungeon->BossDefeatEventId = TEXT("Creature_UnderlightWarden"); // Batch 8: explicit per-dungeon quest target.
        // Generate() runs in BeginPlay on the server.
    }

    // Batch 6 — Item C: portal pair. The entrance pad sits at the Hollow Approach
    // border, walkable from the camp; the exit pad waits beside the dungeon's
    // entry room. Both publish Event.LocationReached so ReachLocation objectives fire.
    // Batch 8: coordinates follow the new 4x3 grid (camp = Dawn Fields center-west).
    const FVector2D EntranceXY(10000.0f, 0.0f);
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

    // --- Batch 8: the Sunken Vault — dungeon #2 in the Tidebreaker Isles. ---
    // The Dawnfang sea-dragon (bestiary Dragon/Serpent/Large) wards the vault;
    // its defeat publishes Creature_VaultColossus for "The Sunken Vault" quest.
    const FVector VaultLocation = FindDrySpotNear(GetSunkenVaultCenterXY(), 12000.0f) + FVector(0.0f, 0.0f, 300.0f);
    if (AAstrawildDungeonGeneratorActor* Vault = World->SpawnActor<AAstrawildDungeonGeneratorActor>(
        AAstrawildDungeonGeneratorActor::StaticClass(), VaultLocation, FRotator::ZeroRotator, Params))
    {
        Vault->DungeonId = TEXT("Dungeon_SunkenVault");
        Vault->RoomCount = 4;
        Vault->BossDefinitionId = TEXT("Echo_Dawnfang");
        Vault->BossDefeatEventId = TEXT("Creature_VaultColossus");
        Vault->CreaturePoolIds = { TEXT("Echo_Wavecrest"), TEXT("Echo_Lagoonfin"), TEXT("Echo_Saltray") };
        Vault->RewardTechnologyId = NAME_None; // research points only — the Ancient era stays Underlight-exclusive.
        Vault->DungeonCompletionResearchPoints = 20;
    }

    // Vault portal pair: entrance at the Driftwood Landing dock, exit beside the vault entry.
    const FVector DriftwoodLocation = FindDrySpotNear(GetDriftwoodLandingXY(), 10000.0f);
    if (AAstrawildDungeonPortalActor* VaultEntrance = World->SpawnActor<AAstrawildDungeonPortalActor>(
        AAstrawildDungeonPortalActor::StaticClass(), DriftwoodLocation + FVector(0.0f, 2600.0f, 100.0f), FRotator::ZeroRotator, Params))
    {
        VaultEntrance->PortalId = TEXT("Location_SunkenVault");
        VaultEntrance->PromptText = FText::FromString(TEXT("Descend into the Sunken Vault [E]"));
        VaultEntrance->Destination = VaultLocation + FVector(0.0f, 0.0f, 150.0f);
    }
    if (AAstrawildDungeonPortalActor* VaultExit = World->SpawnActor<AAstrawildDungeonPortalActor>(
        AAstrawildDungeonPortalActor::StaticClass(), VaultLocation + FVector(0.0f, 900.0f, 0.0f), FRotator::ZeroRotator, Params))
    {
        VaultExit->PortalId = TEXT("Location_DriftwoodLanding");
        VaultExit->PromptText = FText::FromString(TEXT("Surface at Driftwood Landing [E]"));
        VaultExit->Destination = DriftwoodLocation + FVector(0.0f, 2200.0f, 0.0f);
    }

    // Batch 8 — survey marker: charting Driftwood Landing completes the
    // ReachLocation objective of "Wings over the Vale" (publish-only pad).
    if (AAstrawildDungeonPortalActor* LandingMarker = World->SpawnActor<AAstrawildDungeonPortalActor>(
        AAstrawildDungeonPortalActor::StaticClass(), DriftwoodLocation + FVector(1200.0f, 0.0f, 100.0f), FRotator::ZeroRotator, Params))
    {
        LandingMarker->PortalId = TEXT("Location_DriftwoodLanding");
        LandingMarker->PromptText = FText::FromString(TEXT("Chart Driftwood Landing [E]"));
        LandingMarker->bPublishOnly = true;
    }
}

// ---------------------------------------------------------------------------
// Batch 8 — the Grand Expanse: sea, living villages, aircraft.
// ---------------------------------------------------------------------------

FVector AAstrawildWorldBootstrapper::FindDrySpotNear(const FVector2D& Center, const float SearchRadius) const
{
    // Sample a 5x5 grid inside the search radius; return the highest ground —
    // islands stay dry, dungeon floors land on ridge tops.
    FVector Best(Center.X, Center.Y, GroundZ(Center));
    float BestZ = Best.Z;
    for (int32 StepX = -2; StepX <= 2; ++StepX)
    {
        for (int32 StepY = -2; StepY <= 2; ++StepY)
        {
            const FVector2D Sample(
                Center.X + StepX * SearchRadius * 0.25f,
                Center.Y + StepY * SearchRadius * 0.25f);
            const float Z = GroundZ(Sample);
            if (Z > BestZ)
            {
                BestZ = Z;
                Best = FVector(Sample.X, Sample.Y, Z);
            }
        }
    }
    return Best;
}

void AAstrawildWorldBootstrapper::SpawnPOIMarkers()
{
    // Production V2 (Master Plan §5/§31): one marker per registered POI
    // definition, placed from the definition's zone + offset. The POI subsystem
    // tracks discovery (radius sweep + rewards + save v4).
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (UAstrawildPOIDefinition* POI : Registry->GetAllPOIs())
    {
        if (!POI)
        {
            continue;
        }
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(POI->Zone);
        if (!ZoneDesc)
        {
            continue;
        }
        const FVector2D Center = ZoneDesc->GetCenter();
        const FVector2D PoiXY(Center.X + POI->OffsetFromZoneCenter.X, Center.Y + POI->OffsetFromZoneCenter.Y);
        if (AAstrawildPOIMarkerActor* Marker = World->SpawnActor<AAstrawildPOIMarkerActor>(
            AAstrawildPOIMarkerActor::StaticClass(), FVector(PoiXY.X, PoiXY.Y, GroundZ(PoiXY) + 130.0f), FRotator::ZeroRotator, Params))
        {
            Marker->InitializeFromDefinition(POI);
        }
    }

    UE_LOG(LogAstrawild, Log, TEXT("POI markers placed from registry definitions."));
}

void AAstrawildWorldBootstrapper::SpawnWaterPlanes()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // One plane per sea zone, padded 8km past the rect to swallow the blend band.
    const FName SeaZoneIds[] = { TEXT("Zone_AzureShallows"), TEXT("Zone_TidebreakerIsles"), TEXT("Zone_PearlseaReef") };
    for (const FName ZoneId : SeaZoneIds)
    {
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZoneById(ZoneId);
        if (!ZoneDesc)
        {
            continue;
        }

        const FBox2D Rect(
            ZoneDesc->Bounds.Min - FVector2D(8000.0f, 8000.0f),
            ZoneDesc->Bounds.Max + FVector2D(8000.0f, 8000.0f));

        if (AAstrawildWaterPlaneActor* Water = World->SpawnActor<AAstrawildWaterPlaneActor>(
            AAstrawildWaterPlaneActor::StaticClass(), FVector(Rect.GetCenter(), 0.0f), FRotator::ZeroRotator, Params))
        {
            Water->BuildPlane(Rect);
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnVillages()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // --- Dawnstead: the main village around the camp (Dawn Fields center). ---
    const FVector2D CampXY = GetCampCenterXY();
    AAstrawildVillageActor* Dawnstead = World->SpawnActor<AAstrawildVillageActor>(
        AAstrawildVillageActor::StaticClass(), FVector(CampXY.X + 2800.0f, CampXY.Y + 2800.0f, GroundZ(CampXY + FVector2D(2800.0f, 2800.0f))), FRotator::ZeroRotator, Params);
    if (Dawnstead)
    {
        Dawnstead->VillageId = TEXT("Village_Dawnstead");
        Dawnstead->VillageName = FText::FromString(TEXT("Dawnstead"));
        Dawnstead->HutCount = 7;
        Dawnstead->VillageRadius = 1800.0f;

        // Roster: warden (quest), two traders, blacksmith, elder, two guards, farmer.
        const FName DawnsteadRoster[] = {
            TEXT("NPC_WardenMaren"), TEXT("NPC_VendorTam"), TEXT("NPC_HerbalistWren"),
            TEXT("NPC_BlacksmithBorin"), TEXT("NPC_ElderRowan"), TEXT("NPC_GuardSela"),
            TEXT("NPC_GuardBram"), TEXT("NPC_FarmerJori")
        };
        int32 Slot = 0;
        for (const FName NpcId : DawnsteadRoster)
        {
            const float Angle = 2.0f * PI * (Slot + 0.5f) / 8;
            const FVector SpawnPoint = Dawnstead->GetActorLocation() + FVector(
                FMath::Cos(Angle) * 2400.0f, FMath::Sin(Angle) * 2400.0f, 100.0f);
            if (AAstrawildNPCCharacter* Npc = World->SpawnActor<AAstrawildNPCCharacter>(
                AAstrawildNPCCharacter::StaticClass(), SpawnPoint, FRotator::ZeroRotator, Params))
            {
                Npc->NpcDefinition = Registry->FindNPCDefinition(NpcId);
                Npc->RefreshAppearanceFromDefinition(); // BeginPlay ran before the assignment.
                Npc->SetHomeVillage(Dawnstead);
            }
            ++Slot;
        }
    }

    // --- Driftwood Landing: the island fishing hamlet (Tidebreaker Isles). ---
    const FVector DriftwoodLocation = FindDrySpotNear(GetDriftwoodLandingXY(), 10000.0f);
    AAstrawildVillageActor* Driftwood = World->SpawnActor<AAstrawildVillageActor>(
        AAstrawildVillageActor::StaticClass(), DriftwoodLocation, FRotator::ZeroRotator, Params);
    if (Driftwood)
    {
        Driftwood->VillageId = TEXT("Village_DriftwoodLanding");
        Driftwood->VillageName = FText::FromString(TEXT("Driftwood Landing"));
        Driftwood->HutCount = 3;
        Driftwood->VillageRadius = 1200.0f;
        Driftwood->bCoastal = true;

        // Roster: skiff warden (quest), fisher-trader, old salt.
        const FName DriftwoodRoster[] = { TEXT("NPC_SkiffWardenKael"), TEXT("NPC_FisherNima"), TEXT("NPC_OldSaltPerry") };
        int32 Slot = 0;
        for (const FName NpcId : DriftwoodRoster)
        {
            const float Angle = 2.0f * PI * (Slot + 0.5f) / 3;
            const FVector SpawnPoint = DriftwoodLocation + FVector(
                FMath::Cos(Angle) * 1700.0f, FMath::Sin(Angle) * 1700.0f, 100.0f);
            if (AAstrawildNPCCharacter* Npc = World->SpawnActor<AAstrawildNPCCharacter>(
                AAstrawildNPCCharacter::StaticClass(), SpawnPoint, FRotator::ZeroRotator, Params))
            {
                Npc->NpcDefinition = Registry->FindNPCDefinition(NpcId);
                Npc->RefreshAppearanceFromDefinition(); // BeginPlay ran before the assignment.
                Npc->SetHomeVillage(Driftwood);
            }
            ++Slot;
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnSkiffs()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Dawnstead pad: beside the camp, pointed east toward the Hollow Approach.
    const FVector2D CampXY = GetCampCenterXY();
    const FVector2D PadXY = CampXY + FVector2D(-2800.0f, 3600.0f);
    if (AAstrawildSkiffActor* Skiff = World->SpawnActor<AAstrawildSkiffActor>(
        AAstrawildSkiffActor::StaticClass(), FVector(PadXY.X, PadXY.Y, GroundZ(PadXY) + 400.0f), FRotator(0.0f, 0.0f, 0.0f), Params))
    {
        Skiff->SkiffId = TEXT("Skiff_Dawnstead");
    }

    // Driftwood pad: at the landing, pointed out to sea.
    const FVector DriftwoodLocation = FindDrySpotNear(GetDriftwoodLandingXY(), 10000.0f);
    if (AAstrawildSkiffActor* Skiff = World->SpawnActor<AAstrawildSkiffActor>(
        AAstrawildSkiffActor::StaticClass(), DriftwoodLocation + FVector(-2200.0f, -1500.0f, 400.0f), FRotator(0.0f, -35.0f, 0.0f), Params))
    {
        Skiff->SkiffId = TEXT("Skiff_Driftwood");
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

FAstrawildAtmosphereSample AAstrawildWorldBootstrapper::EvalAtmosphereRamp(const float SunAlpha, const bool bIsNight,
    const float VisibilityMultiplier)
{
    // Pure keyframe ramp — no engine types, no world; automation-tested.
    FAstrawildAtmosphereSample Sample;
    const float Alpha = FMath::Clamp(SunAlpha, 0.0f, 1.0f);
    const float Vis = FMath::Clamp(VisibilityMultiplier, 0.1f, 2.0f);

    if (bIsNight)
    {
        // Cool moonlight over a near-black sky.
        Sample.SunColor = FLinearColor(0.55f, 0.65f, 0.90f, 1.0f);
        Sample.FogColor = FLinearColor(0.05f, 0.07f, 0.13f, 1.0f);
        Sample.FogDensity = 0.00022f;
        Sample.SkyLightIntensity = 0.30f;
    }
    else
    {
        // Two-segment day ramp: dawn gold -> neutral noon -> ember dusk.
        const FLinearColor DawnSun(1.00f, 0.72f, 0.45f, 1.0f);
        const FLinearColor NoonSun(1.00f, 0.98f, 0.92f, 1.0f);
        const FLinearColor DuskSun(1.00f, 0.62f, 0.42f, 1.0f);
        Sample.SunColor = Alpha < 0.5f
            ? FMath::Lerp(DawnSun, NoonSun, Alpha * 2.0f)
            : FMath::Lerp(NoonSun, DuskSun, (Alpha - 0.5f) * 2.0f);

        const FLinearColor DawnFog(0.88f, 0.70f, 0.55f, 1.0f);
        const FLinearColor NoonFog(0.70f, 0.76f, 0.84f, 1.0f);
        const FLinearColor DuskFog(0.66f, 0.47f, 0.50f, 1.0f);
        Sample.FogColor = Alpha < 0.5f
            ? FMath::Lerp(DawnFog, NoonFog, Alpha * 2.0f)
            : FMath::Lerp(NoonFog, DuskFog, (Alpha - 0.5f) * 2.0f);

        // Air thickens toward the horizon hours.
        const float Horizon = FMath::Abs(Alpha - 0.5f) * 2.0f; // 0 noon, 1 dawn/dusk
        Sample.FogDensity = FMath::Lerp(0.00012f, 0.00017f, Horizon);
        Sample.SkyLightIntensity = FMath::Lerp(1.6f, 1.2f, Horizon);
    }

    // Weather coupling: poor visibility thickens the air and dims the sun
    // (storm/fog states read as heavier atmospherics, not just a HUD label).
    const float VisLoss = FMath::Clamp(1.0f - Vis, 0.0f, 0.9f);
    Sample.FogDensity *= 1.0f + VisLoss * 1.8f;
    Sample.SunIntensityMultiplier = 0.45f + 0.55f * Vis;
    Sample.FogColor = FMath::Lerp(Sample.FogColor,
        FLinearColor(Sample.FogColor.R * 0.55f + 0.05f, Sample.FogColor.G * 0.55f + 0.05f,
            Sample.FogColor.B * 0.55f + 0.06f, 1.0f),
        VisLoss * 0.6f);

    return Sample;
}

void AAstrawildWorldBootstrapper::UpdateAtmosphere()
{
    if (!bEnableAtmosphere)
    {
        return;
    }

    AAstrawildGameState* GameState = GetGameState();
    if (!GameState)
    {
        return;
    }

    float VisibilityMultiplier = 1.0f;
    if (UWorld* World = GetWorld())
    {
        if (const UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            VisibilityMultiplier = Weather->GetVisibilityMultiplier();
        }
    }

    const FAstrawildAtmosphereSample Sample = EvalAtmosphereRamp(
        GameState->GetSunCycleAlpha(), GameState->IsNight(), VisibilityMultiplier);

    if (SunLight)
    {
        if (ULightComponent* LightComponent = SunLight->GetLightComponent())
        {
            LightComponent->SetLightColor(Sample.SunColor);
            // Base curve x weather dim — recomputed from scratch every tick so
            // the multiplier NEVER compounds across frames.
            const float BaseIntensity = EvalSunBaseIntensity(GameState->GetSunCycleAlpha(), GameState->IsNight());
            LightComponent->SetIntensity(BaseIntensity * Sample.SunIntensityMultiplier);
        }
    }

    if (SkyLightActor)
    {
        if (USkyLightComponent* SkyComponent = SkyLightActor->GetLightComponent())
        {
            SkyComponent->SetIntensity(Sample.SkyLightIntensity);
        }
    }

    if (HeightFogActor)
    {
        if (UExponentialHeightFogComponent* FogComponent = HeightFogActor->GetComponent())
        {
            FogComponent->SetFogDensity(Sample.FogDensity);
            FogComponent->FogInscatteringLuminance = Sample.FogColor;
        }
    }
}

void AAstrawildWorldBootstrapper::SpawnBiomeDressing()
{
    // Production V2 Batch 2 — the first runtime consumer of the biome
    // definitions: every zone scatters its dressing budget deterministically.
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World)
    {
        return;
    }

    // Exclusion bubbles from every placed gameplay actor so dressing never
    // buries the camp, villages, dungeons, portals, POIs or skiff pads.
    TArray<FVector2D> ExclusionCenters;
    TArray<float> ExclusionRadii;
    const auto AddExclusion = [&ExclusionCenters, &ExclusionRadii](const FVector& Location, const float Radius)
    {
        ExclusionCenters.Add(FVector2D(Location.X, Location.Y));
        ExclusionRadii.Add(Radius);
    };

    AddExclusion(FVector(GetCampCenterXY(), 0.0f), 3000.0f); // camp + rest point + stations + work sites

    for (TActorIterator<AAstrawildVillageActor> It(World); It; ++It)
    {
        AddExclusion(It->GetActorLocation(), 2800.0f);
    }
    for (TActorIterator<AAstrawildDungeonGeneratorActor> It(World); It; ++It)
    {
        AddExclusion(It->GetActorLocation(), 2600.0f);
    }
    for (TActorIterator<AAstrawildDungeonPortalActor> It(World); It; ++It)
    {
        AddExclusion(It->GetActorLocation(), 1400.0f);
    }
    for (TActorIterator<AAstrawildPOIMarkerActor> It(World); It; ++It)
    {
        AddExclusion(It->GetActorLocation(), 1600.0f);
    }
    for (TActorIterator<AAstrawildSkiffActor> It(World); It; ++It)
    {
        AddExclusion(It->GetActorLocation(), 1800.0f);
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    int32 DressedZones = 0;
    for (const FAstrawildZoneDescriptor& ZoneDesc : UAstrawildZoneSubsystem::GetAllZones())
    {
        UAstrawildBiomeDefinition* BiomeDef = Registry ? Registry->FindBiome(ZoneDesc.ZoneId) : nullptr;
        AAstrawildBiomeDressingActor* Dressing = World->SpawnActor<AAstrawildBiomeDressingActor>(
            AAstrawildBiomeDressingActor::StaticClass(), FVector(ZoneDesc.GetCenter(), 0.0f), FRotator::ZeroRotator, Params);
        if (Dressing)
        {
            Dressing->BuildDressing(ZoneDesc, BiomeDef, WorldSeedCached, ExclusionCenters, ExclusionRadii);
            ++DressedZones;
        }
    }

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Biome dressing placed for %d/%d zones (%d exclusion bubbles)."),
        DressedZones, UAstrawildZoneSubsystem::GetZoneCount(), ExclusionCenters.Num());
}

float AAstrawildWorldBootstrapper::EvalSunBaseIntensity(const float SunAlpha, const bool bIsNight)
{
    // Dim to moonlight levels at night (directive §13) — the shared base curve
    // both UpdateSunRotation and the atmosphere pass apply before weather dimming.
    return bIsNight ? 0.4f : FMath::Lerp(0.8f, 9.0f, FMath::Clamp(SunAlpha, 0.0f, 1.0f));
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
        LightComponent->SetIntensity(EvalSunBaseIntensity(SunAlpha, GameState->IsNight()));
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
        UpdateAtmosphere();
        UpdateFlickerLights(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
    }
}
