#include "AstrawildWorldBootstrapper.h"

#include "AstrawildCraftingStationActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDungeonGeneratorActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildResourceNode.h"
#include "AstrawildRestPoint.h"
#include "AstrawildWorkSiteActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

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
    if (!Gloomfang)
    {
        return;
    }

    for (int32 i = 0; i < HostileCount; ++i)
    {
        // Alternate night stalker and ember predator (content wave 2).
        UAstrawildEchoDefinition* HostileDef = (Emberfang && (i % 2 == 1)) ? Emberfang : Gloomfang;
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

    AAstrawildWorkSiteActor* FarmSite = World->SpawnActor<AAstrawildWorkSiteActor>(AAstrawildWorkSiteActor::StaticClass(), FVector(-CampRadius * 0.7f, CampRadius * 0.7f, 100.0f), FRotator::ZeroRotator, Params);
    if (FarmSite)
    {
        FarmSite->WorkType = EAstrawildWorkType::Farming;
        FarmSite->OutputItemId = TEXT("Item_Berry");
        FarmSite->SecondsPerOutput = 14.0f;
        FarmSite->bRequiresPower = false;
    }

    // First dungeon (directive §21/§23): the Hollow Underlight — placed beyond the eastern wilds.
    AAstrawildDungeonGeneratorActor* Dungeon = World->SpawnActor<AAstrawildDungeonGeneratorActor>(
        AAstrawildDungeonGeneratorActor::StaticClass(), FVector(ArenaSize * 1.4f, 0.0f, 100.0f), FRotator::ZeroRotator, Params);
    if (Dungeon)
    {
        Dungeon->RoomCount = 5;
        Dungeon->BossDefinitionId = TEXT("Echo_Gloomfang");
        // Generate() runs in BeginPlay on the server.
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
