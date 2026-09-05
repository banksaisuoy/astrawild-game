#include "AstrawildDungeonRoomActor.h"

#include "AstrawildArtPack.h"
#include "AstrawildBossHazardActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResonancePillarActor.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildWaterPlaneActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    /**
     * DP-9: placeholder tint through a dynamic material instance — the exact
     * AAstrawildResourceNode::ApplyVisualTint idiom (the engine basic-shape
     * mesh carries a material at slot 0; skip tinting when it does not).
     */
    void ApplyThemedTint(UStaticMeshComponent* Mesh, const FLinearColor& Tint)
    {
        if (!Mesh || !Mesh->GetStaticMesh())
        {
            return;
        }
        UMaterialInterface* BaseMaterial = Mesh->GetMaterial(0);
        if (!BaseMaterial)
        {
            return;
        }
        if (UMaterialInstanceDynamic* DynMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
        {
            DynMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
        }
    }
}

AAstrawildDungeonRoomActor::AAstrawildDungeonRoomActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f; // Clear-check cadence — cheap.
    bReplicates = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        // Floor plate by default — BuildRoomShell rescales per template.
        VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 0.1f));

        // DP-9: perimeter side walls (engine cubes — same placeholder vocabulary
        // as the floor; REPLACE_BEFORE_RELEASE with authored modular meshes).
        // Hidden until a theme sizes them (unthemed rooms stay byte-identical).
        LeftWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWallMesh"));
        LeftWallMesh->SetupAttachment(RootComponent);
        LeftWallMesh->SetStaticMesh(CubeMesh.Object);
        LeftWallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LeftWallMesh->SetVisibility(false);

        RightWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWallMesh"));
        RightWallMesh->SetupAttachment(RootComponent);
        RightWallMesh->SetStaticMesh(CubeMesh.Object);
        RightWallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RightWallMesh->SetVisibility(false);
    }

    // DP-9: themed accent light — dark until a theme configures it.
    RoomLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RoomLight"));
    RoomLight->SetupAttachment(RootComponent);
    RoomLight->SetIntensity(0.0f);
    RoomLight->SetCastShadows(false);
}

void AAstrawildDungeonRoomActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildDungeonRoomActor, bCleared);
    DOREPLIFETIME(AAstrawildDungeonRoomActor, Template);   // LCP-2: client themed shells
    DOREPLIFETIME(AAstrawildDungeonRoomActor, RoomIndex);  // LCP-2
}

void AAstrawildDungeonRoomActor::OnRep_Template()
{
    // LCP-2: LAN client — the real template arrived (the authority assigned it
    // after spawn); rebuild the themed shell locally. Same guarded path the
    // server's RefreshRoomShell takes, so the double-call never double-builds.
    if (GetLocalRole() != ROLE_Authority)
    {
        RefreshRoomShell();
    }
}

void AAstrawildDungeonRoomActor::BeginPlay()
{
    Super::BeginPlay();
    BuildRoomShell();
}

void AAstrawildDungeonRoomActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // DP-9: room-owned spawnlings die with the room — regeneration destroys
    // rooms through this path, so pillars / pulse tiles / water accents never
    // leak across a Generate() rebuild.
    if (UWorld* World = GetWorld())
    {
        for (const TWeakObjectPtr<AAstrawildResonancePillarActor>& Weak : PuzzlePillars)
        {
            if (AAstrawildResonancePillarActor* Pillar = Weak.Get())
            {
                World->DestroyActor(Pillar);
            }
        }
        PuzzlePillars.Reset();

        for (const TWeakObjectPtr<AAstrawildBossHazardActor>& Weak : HazardTiles)
        {
            if (AAstrawildBossHazardActor* Tile = Weak.Get())
            {
                World->DestroyActor(Tile);
            }
        }
        HazardTiles.Reset();

        if (AAstrawildWaterPlaneActor* Water = WaterFloorAccent.Get())
        {
            World->DestroyActor(Water);
            WaterFloorAccent = nullptr;
        }
    }

    Super::EndPlay(EndPlayReason);
}

// ===========================================================================
// DP-9 — theme resolution + data table (pure statics, automation-tested)
// ===========================================================================

EAstrawildDungeonTheme AAstrawildDungeonRoomActor::ResolveDungeonTheme(const FName DungeonId)
{
    // Identity rides on the STABLE dungeon ids (the save system's mapping key)
    // — the bootstrapper literals themselves stay untouched. Unknown ids fail
    // closed to the unthemed legacy shell.
    if (DungeonId == TEXT("Dungeon_HollowUnderlight"))
    {
        return EAstrawildDungeonTheme::HollowUnderlight;
    }
    if (DungeonId == TEXT("Dungeon_SunkenVault"))
    {
        return EAstrawildDungeonTheme::SunkenVault;
    }
    if (DungeonId == TEXT("Dungeon_EyeOfTheMaelstrom"))
    {
        return EAstrawildDungeonTheme::MaelstromEye;
    }
    return EAstrawildDungeonTheme::None;
}

FAstrawildDungeonThemeProfile AAstrawildDungeonRoomActor::MakeThemeProfile(const EAstrawildDungeonTheme Theme)
{
    FAstrawildDungeonThemeProfile Profile; // defaults = the unthemed legacy shell

    switch (Theme)
    {
    case EAstrawildDungeonTheme::HollowUnderlight:
        // Dark, tight, ash-choked: low oppressive slabs, ember-fungus glow,
        // cliff-shard rubble (the Hollow Approach art vocabulary) and the
        // room-level ash lung — the zone's identity pressed indoors.
        Profile.Theme = Theme;
        Profile.ShellTint = FLinearColor(0.13f, 0.11f, 0.16f, 1.0f);
        Profile.AccentLightTint = FLinearColor(0.55f, 0.32f, 0.16f, 1.0f);
        Profile.AccentLightIntensity = 4.0f;
        Profile.ExtentScale = FVector(0.85f, 0.85f, 1.0f); // tighter, shorter feel
        Profile.SideWallHeight = 260.0f;
        Profile.SideWallThickness = 80.0f;
        Profile.RockBiomeId = TEXT("Zone_HollowApproach");  // cliff shards + granite rubble
        Profile.FloraBiomeId = TEXT("Zone_HollowApproach"); // spore-bush
        Profile.RockCount = 12;
        Profile.FloraCount = 6;
        Profile.DressingScaleMin = 0.7f;
        Profile.DressingScaleMax = 1.3f;
        Profile.Hazard = EAstrawildRoomHazardType::AshLung;
        Profile.HazardPressure = 4.0f; // mild — the zone ash lung is 6
        break;

    case EAstrawildDungeonTheme::SunkenVault:
        // Wide, flooded, reed-lit: broad low halls, mossy ruin-block rubble,
        // glow-reed banks, a water-film floor and the waterlogged slow —
        // wading through a drowned vault.
        Profile.Theme = Theme;
        Profile.ShellTint = FLinearColor(0.16f, 0.28f, 0.33f, 1.0f);
        Profile.AccentLightTint = FLinearColor(0.20f, 0.55f, 0.62f, 1.0f);
        Profile.AccentLightIntensity = 3.5f;
        Profile.ExtentScale = FVector(1.30f, 1.30f, 1.0f); // wide flooded halls
        Profile.SideWallHeight = 190.0f;
        Profile.SideWallThickness = 90.0f;
        Profile.RockBiomeId = TEXT("Zone_VerdantReach");     // moss boulders + granite (ruin blocks)
        Profile.FloraBiomeId = TEXT("Zone_TidebreakerIsles"); // glow-reed banks
        Profile.RockCount = 9;
        Profile.FloraCount = 12;
        Profile.DressingScaleMin = 0.7f;
        Profile.DressingScaleMax = 1.2f;
        Profile.bWaterFloorAccent = true;
        Profile.Hazard = EAstrawildRoomHazardType::Waterlogged;
        Profile.HazardSpeedMultiplier = 0.8f;
        break;

    case EAstrawildDungeonTheme::MaelstromEye:
        // Tall monolith slabs, storm-pulse light, ancient-tech vein accents —
        // the storm's energy saturates the rooms and periodically discharges.
        Profile.Theme = Theme;
        Profile.ShellTint = FLinearColor(0.18f, 0.30f, 0.36f, 1.0f);
        Profile.AccentLightTint = FLinearColor(0.30f, 0.85f, 1.0f, 1.0f);
        Profile.AccentLightIntensity = 6.0f;
        Profile.AccentLightPulseRate = 0.35f; // the storm pulse (cycles/s)
        Profile.ExtentScale = FVector(1.0f, 1.0f, 1.0f);
        Profile.SideWallHeight = 520.0f; // tall monolith shells
        Profile.SideWallThickness = 100.0f;
        Profile.RockBiomeId = TEXT("Zone_Glimmerwood"); // granite, stretched into pillars
        Profile.FloraBiomeId = TEXT("Zone_DuskMarsh");  // glow-reed + spore-bush
        Profile.TechNodeArtId = TEXT("Node_AncientVein"); // ancient-tech vein accent
        Profile.RockCount = 8;
        Profile.FloraCount = 8;
        Profile.TechCount = 5;
        Profile.DressingScaleMin = 0.7f;
        Profile.DressingScaleMax = 1.1f;
        Profile.RockElongation = 2.6f; // rocks read as standing monolith pillars
        Profile.Hazard = EAstrawildRoomHazardType::EnergyPulse;
        Profile.HazardPulseInterval = 9.0f;
        Profile.HazardTileCount = 3;
        Profile.HazardTileRadius = 170.0f;
        Profile.HazardTileDamagePerSecond = 3.0f;
        Profile.HazardTileLifetime = 4.5f;
        break;

    case EAstrawildDungeonTheme::None:
    default:
        break; // unthemed — legacy placeholder shell, no hazard
    }

    return Profile;
}

// ===========================================================================
// DP-9 — resonance-pillar sequence (pure verbs, automation-tested)
// ===========================================================================

AAstrawildDungeonRoomActor::EPillarActivityResult AAstrawildDungeonRoomActor::EvaluatePillarActivation(
    const int32 AttemptedPillarIndex, const int32 ActivatedCount, const int32 TotalPillars)
{
    // Pillars carry their required order position; the attuned pillar must be
    // the NEXT one (AttemptedPillarIndex == ActivatedCount). Anything else —
    // wrong order, stale repeats, degenerate counts — resets the sequence.
    if (TotalPillars <= 0 || ActivatedCount < 0 || ActivatedCount >= TotalPillars)
    {
        return EPillarActivityResult::ResetRequired;
    }
    if (AttemptedPillarIndex != ActivatedCount)
    {
        return EPillarActivityResult::ResetRequired;
    }
    return (ActivatedCount + 1 == TotalPillars) ? EPillarActivityResult::Completed : EPillarActivityResult::Advanced;
}

bool AAstrawildDungeonRoomActor::IsPillarSequenceExpired(const float ElapsedSeconds, const float WindowSeconds)
{
    return WindowSeconds > 0.0f && ElapsedSeconds >= WindowSeconds;
}

// ===========================================================================
// Shell
// ===========================================================================

void AAstrawildDungeonRoomActor::BuildRoomShell()
{
    // Placeholder shell (REPLACE_BEFORE_RELEASE with authored modular meshes):
    // a floor plate scaled to the template extents. Walls arrive with the asset pass.
    if (VisualMesh)
    {
        const FVector Extents = Template.HalfExtents / 50.0f; // Engine cube = 100cm.
        VisualMesh->SetWorldScale3D(FVector(Extents.X, Extents.Y, 0.1f));
        VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
    }

    // DP-9: themed identity builds ONCE (BeginPlay runs with the default
    // template; the generator assigns the real one and calls RefreshRoomShell —
    // the double call must not double-build).
    if (!bThemeApplied && Template.Theme != EAstrawildDungeonTheme::None)
    {
        bThemeApplied = true;
        ApplyThemeShell();
    }
}

void AAstrawildDungeonRoomActor::RefreshRoomShell()
{
    // Audit fix: the generator assigns Template AFTER SpawnActor (BeginPlay already ran
    // with the default template), so the shell must be rebuilt once real extents are set.
    BuildRoomShell();
}

void AAstrawildDungeonRoomActor::ApplyThemeShell()
{
    const FAstrawildDungeonThemeProfile Profile = MakeThemeProfile(Template.Theme);
    const FVector Extents = Template.HalfExtents; // already theme-scaled by the generator

    // Perimeter side walls (±Y). The ±X faces stay OPEN: room-to-room passage
    // runs along the chain axis and the gate actors own the seals there, so
    // themed walls can never trap progression (portal pads sit at ±900cm,
    // outside every wall line).
    const float WallThickness = FMath::Max(30.0f, Profile.SideWallThickness);
    const FVector WallScale((Extents.X * 2.0f + WallThickness) / 100.0f,
        WallThickness / 100.0f,
        Profile.SideWallHeight / 100.0f);
    const float WallZ = 5.0f + Profile.SideWallHeight * 0.5f; // sits on the floor plate top

    UStaticMeshComponent* Walls[2] = { LeftWallMesh, RightWallMesh };
    for (int32 Side = 0; Side < 2; ++Side)
    {
        UStaticMeshComponent* Wall = Walls[Side];
        if (!Wall)
        {
            continue;
        }
        const float SideSign = (Side == 0) ? -1.0f : 1.0f;
        Wall->SetRelativeLocation(FVector(0.0f, SideSign * (Extents.Y + WallThickness * 0.5f), WallZ));
        Wall->SetRelativeScale3D(WallScale);
        const bool bHasWalls = Profile.SideWallHeight > 0.0f;
        Wall->SetVisibility(bHasWalls);
        if (bHasWalls)
        {
            Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Wall->SetCollisionResponseToAllChannels(ECR_Block);
            ApplyThemedTint(Wall, Profile.ShellTint * 0.8f);
        }
    }

    // Floor tint — the ResourceNode MID idiom (placeholder shells read per
    // dungeon with zero authored assets).
    ApplyThemedTint(VisualMesh, Profile.ShellTint);

    // Accent light (the Eye's storm pulse runs on the existing room tick).
    if (RoomLight)
    {
        RoomLight->SetRelativeLocation(FVector(0.0f, 0.0f, Extents.Z * 0.9f + 120.0f));
        RoomLight->SetLightColor(Profile.AccentLightTint);
        RoomLight->SetIntensity(Profile.AccentLightIntensity);
        RoomLight->SetAttenuationRadius(1600.0f);
        RoomLight->SetCastShadows(false);
        CachedLightBaseIntensity = Profile.AccentLightIntensity;
        CachedLightPulseRate = Profile.AccentLightPulseRate;
    }

    BuildRoomDressing(Profile);

    // Sunken Vault flooded-floor accent: the EXISTING water-plane actor, placed
    // room-locally through the additive BuildPlaneAtZ (a wading-height film —
    // the same stylized surface contract as the world ocean).
    if (Profile.bWaterFloorAccent && GetLocalRole() == ROLE_Authority && GetWorld())
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AAstrawildWaterPlaneActor* Water = GetWorld()->SpawnActor<AAstrawildWaterPlaneActor>(
            AAstrawildWaterPlaneActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
        {
            const FVector2D Center(GetActorLocation().X, GetActorLocation().Y);
            const FVector2D Half(Extents.X * 0.92f, Extents.Y * 0.92f);
            // Thin wading film: box top sits ~15cm above the floor plate — a
            // walkable step (never a progression blocker at the gate line),
            // reading as a flooded floor through the vault.
            Water->BuildPlaneAtZ(FBox2D(Center - Half, Center + Half), GetActorLocation().Z + 10.0f, 20.0f);
            WaterFloorAccent = Water;
        }
    }

    // DP-9: puzzle rooms get their resonance pillars (server-side, once).
    if (Template.bRequiresPuzzleSolve)
    {
        SpawnPuzzlePillars();
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d themed (%d): walls %.0fcm, %d rocks, %d flora, %d tech accents, hazard %d."),
        RoomIndex, static_cast<int32>(Template.Theme), Profile.SideWallHeight,
        Profile.RockCount, Profile.FloraCount, Profile.TechCount, static_cast<int32>(Profile.Hazard));
}

void AAstrawildDungeonRoomActor::BuildRoomDressing(const FAstrawildDungeonThemeProfile& Profile)
{
    // Deterministic room-local scatter — the BiomeDressingActor idiom: seeded
    // FRandomStream (room index + theme), ≤6-attempt rejection sampling that
    // keeps the encounter clear-space (center box) clean, built once at shell
    // time — zero per-tick cost. Dressing is VISUAL ONLY (no collision): rooms
    // are combat spaces and blocking scatter would fight AI steering + kiting.
    // Meshes resolve through the EXISTING ArtPack binding tables (no new
    // /Game/ paths — validator check 8 stays byte-clean).
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FRandomStream Stream(1009 + RoomIndex * 31 + static_cast<int32>(Template.Theme) * 977);
    const FVector2D Extents2D(Template.HalfExtents.X, Template.HalfExtents.Y);

    const auto NextPoint = [&Stream, &Extents2D]() -> FVector
    {
        for (int32 Attempt = 0; Attempt < 6; ++Attempt)
        {
            const FVector2D P(Stream.FRandRange(-0.9f, 0.9f), Stream.FRandRange(-0.9f, 0.9f));
            if (FMath::Abs(P.X) > 0.5f || FMath::Abs(P.Y) > 0.5f)
            {
                return FVector(P.X * Extents2D.X, P.Y * Extents2D.Y, 6.0f); // just above the floor plate
            }
        }
        return FVector(0.75f * Extents2D.X, 0.75f * Extents2D.Y, 6.0f);
    };

    const auto BuildISM = [this, &Stream, &NextPoint, &Profile](const TArray<FString>& MeshPaths,
        const int32 Count, const float Elongation, const FName Label)
    {
        if (Count <= 0 || MeshPaths.IsEmpty())
        {
            return;
        }
        for (int32 PathIdx = 0; PathIdx < MeshPaths.Num(); ++PathIdx)
        {
            UStaticMesh* Mesh = Cast<UStaticMesh>(FSoftObjectPath(MeshPaths[PathIdx]).TryLoad());
            if (!Mesh)
            {
                continue; // ArtPack soft-ref contract — fallback is simply fewer props
            }
            UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this,
                *FString::Printf(TEXT("ISM_Dungeon_%s_%d"), *Label.ToString(), PathIdx));
            if (!ISM)
            {
                continue;
            }
            ISM->RegisterComponent();
            ISM->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            ISM->SetCastShadow(false);
            ISM->SetStaticMesh(Mesh);

            for (int32 InstanceIdx = PathIdx; InstanceIdx < Count; InstanceIdx += MeshPaths.Num())
            {
                const FVector Point = NextPoint();
                const float Scale = Stream.FRandRange(Profile.DressingScaleMin, Profile.DressingScaleMax);
                const FRotator Rotation(Stream.FRandRange(-2.5f, 2.5f), Stream.FRandRange(0.0f, 360.0f), Stream.FRandRange(-2.5f, 2.5f));
                ISM->AddInstance(FTransform(Rotation, Point, FVector(Scale, Scale, Scale * Elongation)), false);
            }
        }
    };

    if (const AstrawildArtPack::FBiomeArt* RockArt = AstrawildArtPack::FindBiomeArt(Profile.RockBiomeId))
    {
        BuildISM(RockArt->RockMeshPaths, Profile.RockCount, Profile.RockElongation, TEXT("Rock"));
    }
    if (const AstrawildArtPack::FBiomeArt* FloraArt = AstrawildArtPack::FindBiomeArt(Profile.FloraBiomeId))
    {
        BuildISM(FloraArt->GrassMeshPaths, Profile.FloraCount, 1.0f, TEXT("Flora"));
    }
    if (!Profile.TechNodeArtId.IsNone())
    {
        if (const AstrawildArtPack::FNodeArt* NodeArt = AstrawildArtPack::FindNodeArt(Profile.TechNodeArtId))
        {
            BuildISM({ NodeArt->MeshPath }, Profile.TechCount, 1.0f, TEXT("Tech"));
        }
    }
}

void AAstrawildDungeonRoomActor::SpawnPuzzlePillars()
{
    if (GetLocalRole() != ROLE_Authority || !GetWorld() || !PuzzlePillars.IsEmpty())
    {
        return;
    }

    // Three pillars in a spread triangle, off the guard's corner; the required
    // order is the pillar index (the prompt carries the numeral — the window
    // and the guard are the challenge, not a guessing game).
    const FAstrawildDungeonThemeProfile Profile = MakeThemeProfile(Template.Theme);
    const float BaseZ = Profile.bWaterFloorAccent ? 25.0f : 10.0f; // water-film surface or floor plate top
    const FVector Offsets[3] = {
        FVector(-0.42f * Template.HalfExtents.X, -0.40f * Template.HalfExtents.Y, BaseZ),
        FVector(0.42f * Template.HalfExtents.X, -0.40f * Template.HalfExtents.Y, BaseZ),
        FVector(0.0f, 0.42f * Template.HalfExtents.Y, BaseZ)
    };

    for (int32 PillarIdx = 0; PillarIdx < GetPuzzlePillarCount(); ++PillarIdx)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AAstrawildResonancePillarActor* Pillar = GetWorld()->SpawnActor<AAstrawildResonancePillarActor>(
            AAstrawildResonancePillarActor::StaticClass(), GetActorLocation() + Offsets[PillarIdx],
            FRotator::ZeroRotator, Params))
        {
            Pillar->PillarIndex = PillarIdx;
            Pillar->OwningRoom = this;
            PuzzlePillars.Add(Pillar);
        }
    }

    if (!PuzzlePillars.IsEmpty())
    {
        UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d: %d resonance pillars placed (window %.0fs, wrong order resets)."),
            RoomIndex, PuzzlePillars.Num(), GetPuzzleSequenceWindowSeconds());
    }
}

// ===========================================================================
// Encounter (unchanged behavior)
// ===========================================================================

void AAstrawildDungeonRoomActor::SpawnEncounter(const TArray<FName>& CreatureDefinitionIds)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Audit C-5: boss rooms spawn the phased boss character (previously a plain Echo
    // spawned here and the whole 3-phase boss class was unreachable dead code).
    // Batch 6: the boss now derives its stats from the real species definition
    // (BossDefinitionId was cosmetic before — HP/damage/weakness come from data).
    if (Template.bIsBossRoom)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        const FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
        AAstrawildEchoBossCharacter* Boss = World->SpawnActor<AAstrawildEchoBossCharacter>(
            AAstrawildEchoBossCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
        if (Boss)
        {
            if (CreatureDefinitionIds.Num() > 0)
            {
                if (const UAstrawildEchoDefinition* BossDefinition = Registry->FindEcho(CreatureDefinitionIds[0]))
                {
                    Boss->InitializeFromBossDefinition(BossDefinition);
                }
            }
            Boss->DefeatEventTargetId = BossDefeatEventId; // Batch 8: per-dungeon quest target.
            // DP-5: resolve the per-boss special set from the now-final defeat
            // id BEFORE the FR-7 summon override below — an explicit room
            // override always beats the set's default summons.
            Boss->ApplyBossSpecialSet();
            // Final-audit (AUD-3 loot note): room bosses are rewarded by the room's
            // ClearLootTableId on clear — the species DefeatLoot path is disabled
            // so the Sovereign does not triple-drop its SovereignCore.
            Boss->bGrantSpeciesDefeatLoot = false;
            // Final Run (FR-7): phase-2 summon override — the Sovereign calls Eye
            // Sentinels instead of the class-default Gloomfangs.
            if (!BossSummonSpeciesId.IsNone())
            {
                Boss->SummonSpeciesId = BossSummonSpeciesId;
            }
            BossCreature = Boss;
            UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon boss room %d: phased boss spawned."), RoomIndex);
        }
        return;
    }

    // Combat/elite/puzzle rooms cycle the creature pool.
    const FName SpeciesId = CreatureDefinitionIds.Num() > 0
        ? CreatureDefinitionIds[RoomIndex % CreatureDefinitionIds.Num()]
        : NAME_None;

    UAstrawildEchoDefinition* Definition = Registry->FindEcho(SpeciesId);
    if (!Definition)
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Dungeon room %d: unknown species %s."), RoomIndex, *SpeciesId.ToString());
        return;
    }

    const TArray<FVector>& Offsets = Template.CreatureSpawnOffsets.IsEmpty()
        ? TArray<FVector>{ FVector(200.0f, 0.0f, 120.0f) }
        : Template.CreatureSpawnOffsets;

    for (const FVector& Offset : Offsets)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Creature = World->SpawnActor<AAstrawildEchoCharacter>(
            AAstrawildEchoCharacter::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Params);
        if (Creature)
        {
            Creature->InitializeFromDefinition(Definition);
            EncounterCreatures.Add(Creature);
        }
    }
}

bool AAstrawildDungeonRoomActor::IsEncounterDefeated() const
{
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : EncounterCreatures)
    {
        const AAstrawildEchoCharacter* Creature = Weak.Get();
        if (Creature && !Creature->IsDefeated())
        {
            return false;
        }
    }
    return true;
}

// ===========================================================================
// Tick: light pulse + room hazard + puzzle window + clear check
// ===========================================================================

void AAstrawildDungeonRoomActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // DP-9: the Eye's accent light pulses on the room's existing tick cadence
    // (one intensity write per room per 0.5s — negligible; runs before any
    // authority gate so it animates wherever the themed shell exists).
    LightPhase += DeltaTime;
    if (RoomLight && CachedLightPulseRate > 0.0f && CachedLightBaseIntensity > 0.0f)
    {
        const float Wave = 0.5f + 0.5f * FMath::Sin(LightPhase * CachedLightPulseRate * 2.0f * PI);
        RoomLight->SetIntensity(FMath::Lerp(CachedLightBaseIntensity * 0.45f, CachedLightBaseIntensity, Wave));
    }

    // DP-9: room-level hazard identity — server-side, active only while the
    // room is UNCLEARED (the entry room clears at generation, so it is exempt
    // by construction; cleared rooms shed the effect — see MarkCleared).
    if (Template.Theme != EAstrawildDungeonTheme::None && !bCleared)
    {
        ApplyRoomHazard(DeltaTime);
    }

    // DP-9: attunement window countdown — an abandoned half-sequence resets so
    // the puzzle is always retryable (never a stall).
    if (GetLocalRole() == ROLE_Authority && !bCleared && PuzzleActivatedCount > 0 && !bPuzzleSolved)
    {
        PuzzleWindowElapsed += DeltaTime;
        if (IsPillarSequenceExpired(PuzzleWindowElapsed, GetPuzzleSequenceWindowSeconds()))
        {
            UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d: resonance window expired — sequence resets."), RoomIndex);
            ResetPuzzleSequence();
        }
    }

    // Audit C-4: rooms with neither creatures nor a boss have nothing to check — the
    // generator clears them at spawn time. The boss counts toward the clear condition.
    if (GetLocalRole() != ROLE_Authority || bCleared || !HasEncounter())
    {
        return;
    }

    // Room clears when every encounter creature AND the boss (if any) are defeated
    // (directive §23; audit C-4 — the entry-room early-out previously stalled the whole
    // dungeon at RoomsCleared == N-1).
    EncounterCreatures.RemoveAll([](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return !Weak.IsValid(); });
    const bool bBossDown = !BossCreature.IsValid() || (BossCreature.IsValid() && BossCreature->IsDefeated());

    // DP-9: puzzle rooms ALSO need the resonance sequence solved before they
    // clear (the gate unseals through the existing room-clear path). Fail-open
    // when pillars never spawned — progression can never stall on a spawn miss.
    const bool bPuzzlePending = !PuzzlePillars.IsEmpty() && !bPuzzleSolved;
    if (!bPuzzlePending && IsEncounterDefeated() && bBossDown)
    {
        MarkCleared();
    }
}

void AAstrawildDungeonRoomActor::ApplyRoomHazard(const float DeltaTime)
{
    if (GetLocalRole() != ROLE_Authority || bCleared)
    {
        return;
    }

    const FAstrawildDungeonThemeProfile Profile = MakeThemeProfile(Template.Theme);
    switch (Profile.Hazard)
    {
    case EAstrawildRoomHazardType::AshLung:
    {
        // Mild, non-lethal: regen suppression refreshed by the 0.5s room tick
        // while the player stands inside the uncleared room. The survival tick
        // consumes it CLAMPED at zero net regen (the DP-7 zone-verb contract).
        FAstrawildStatusEffect AshLung;
        AshLung.StatusId = GetRoomAshLungStatusId();
        AshLung.RemainingSeconds = 1.6f;
        AshLung.StaminaRegenPenaltyPerSecond = FMath::Max(0.0f, Profile.HazardPressure);
        ApplyStatusToPlayersInside(AshLung);
        break;
    }

    case EAstrawildRoomHazardType::Waterlogged:
    {
        // Drowned-vault wading: the existing Chill/Shock SpeedMultiplier verb.
        FAstrawildStatusEffect Waterlogged;
        Waterlogged.StatusId = GetRoomWaterloggedStatusId();
        Waterlogged.RemainingSeconds = 1.6f;
        Waterlogged.SpeedMultiplier = FMath::Clamp(Profile.HazardSpeedMultiplier, 0.2f, 1.0f);
        ApplyStatusToPlayersInside(Waterlogged);
        break;
    }

    case EAstrawildRoomHazardType::EnergyPulse:
    {
        // Periodic energy discharges in specific tiles — the existing
        // AAstrawildBossHazardActor arena-hazard pattern (armor-mitigated,
        // server-authoritative, dissipates on its own lifetime).
        HazardPulseElapsed += DeltaTime;
        HazardTiles.RemoveAll([](const TWeakObjectPtr<AAstrawildBossHazardActor>& Weak) { return !Weak.IsValid(); });
        if (HazardPulseElapsed >= FMath::Max(1.0f, Profile.HazardPulseInterval))
        {
            HazardPulseElapsed = 0.0f;
            SpawnHazardPulseTiles(Profile);
        }
        break;
    }

    default:
        break;
    }
}

void AAstrawildDungeonRoomActor::SpawnHazardPulseTiles(const FAstrawildDungeonThemeProfile& Profile)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Deterministic tile ring: the pattern rotates one third of a turn per
    // pulse, so specific tiles fire periodically but never from the same spots.
    const int32 TileCount = FMath::Clamp(Profile.HazardTileCount, 0, 4);
    const float BaseAngle = static_cast<float>(HazardPulseCounter) * (PI * 2.0f / 3.0f);
    ++HazardPulseCounter;

    for (int32 TileIdx = 0; TileIdx < TileCount; ++TileIdx)
    {
        const float Angle = BaseAngle + static_cast<float>(TileIdx) * (PI * 2.0f / static_cast<float>(FMath::Max(1, TileCount)));
        const FVector Offset(FMath::Cos(Angle) * Template.HalfExtents.X * 0.55f,
            FMath::Sin(Angle) * Template.HalfExtents.Y * 0.55f,
            40.0f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (AAstrawildBossHazardActor* Tile = World->SpawnActor<AAstrawildBossHazardActor>(
            AAstrawildBossHazardActor::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Params))
        {
            Tile->DamagePerSecond = FMath::Max(0.0f, Profile.HazardTileDamagePerSecond);
            Tile->HazardRadius = FMath::Max(50.0f, Profile.HazardTileRadius);
            Tile->LifetimeSeconds = FMath::Max(1.0f, Profile.HazardTileLifetime);
            HazardTiles.Add(Tile);
        }
    }
}

void AAstrawildDungeonRoomActor::ApplyStatusToPlayersInside(const FAstrawildStatusEffect& Effect)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
        if (!Player || !Player->IsAlive() || !IsPlayerInsideRoom(Player))
        {
            continue;
        }
        if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
        {
            Survival->AddStatusEffect(Effect);
        }
    }
}

void AAstrawildDungeonRoomActor::RemoveStatusFromPlayersInside(const FName StatusId)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
        if (!Player || !Player->IsAlive() || !IsPlayerInsideRoom(Player))
        {
            continue;
        }
        if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
        {
            Survival->RemoveStatusEffect(StatusId);
        }
    }
}

bool AAstrawildDungeonRoomActor::IsPlayerInsideRoom(const AAstrawildPlayerCharacter* Player) const
{
    if (!Player)
    {
        return false;
    }
    const FVector Delta = Player->GetActorLocation() - GetActorLocation();
    return FMath::Abs(Delta.X) <= Template.HalfExtents.X &&
           FMath::Abs(Delta.Y) <= Template.HalfExtents.Y &&
           FMath::Abs(Delta.Z) <= Template.HalfExtents.Z + 350.0f;
}

// ===========================================================================
// Resonance-pillar sequence (server state machine over the pure verbs)
// ===========================================================================

void AAstrawildDungeonRoomActor::NotifyPillarInteracted(const int32 PillarIndex)
{
    if (GetLocalRole() != ROLE_Authority || bCleared || bPuzzleSolved || PuzzlePillars.IsEmpty())
    {
        return;
    }

    const int32 TotalPillars = PuzzlePillars.Num();
    AAstrawildResonancePillarActor* Pillar = nullptr;
    for (const TWeakObjectPtr<AAstrawildResonancePillarActor>& Weak : PuzzlePillars)
    {
        AAstrawildResonancePillarActor* Candidate = Weak.Get();
        if (Candidate && Candidate->PillarIndex == PillarIndex)
        {
            Pillar = Candidate;
            break;
        }
    }
    if (!Pillar)
    {
        return;
    }

    switch (EvaluatePillarActivation(PillarIndex, PuzzleActivatedCount, TotalPillars))
    {
    case EPillarActivityResult::Advanced:
        Pillar->SetActivated(true);
        ++PuzzleActivatedCount; // the window started on the first attunement and keeps running
        UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d: pillar %d attuned (%d/%d)."),
            RoomIndex, PillarIndex, PuzzleActivatedCount, TotalPillars);
        break;

    case EPillarActivityResult::Completed:
        Pillar->SetActivated(true);
        PuzzleActivatedCount = TotalPillars;
        bPuzzleSolved = true;
        PuzzleWindowElapsed = 0.0f;
        UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d: resonance sequence solved — clear the guard and the gate unseals."), RoomIndex);
        break;

    case EPillarActivityResult::ResetRequired:
    default:
        UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d: wrong attunement order — the sequence resets."), RoomIndex);
        ResetPuzzleSequence();
        break;
    }
}

void AAstrawildDungeonRoomActor::ResetPuzzleSequence()
{
    for (const TWeakObjectPtr<AAstrawildResonancePillarActor>& Weak : PuzzlePillars)
    {
        if (AAstrawildResonancePillarActor* Pillar = Weak.Get())
        {
            Pillar->SetActivated(false);
        }
    }
    PuzzleActivatedCount = 0;
    PuzzleWindowElapsed = 0.0f;
}

// ===========================================================================
// Clear + restore
// ===========================================================================

void AAstrawildDungeonRoomActor::MarkCleared()
{
    if (bCleared || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    bCleared = true;

    // DP-9: cleared rooms shed their hazard identity — statuses removed from
    // anyone still inside, pulse tiles dissipated, puzzle pillars locked lit.
    ClearRoomHazardIdentity();

    GrantClearReward();
    OnRoomCleared.Broadcast(this, RoomIndex);
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d cleared."), RoomIndex);
}

void AAstrawildDungeonRoomActor::ClearRoomHazardIdentity()
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    const FAstrawildDungeonThemeProfile Profile = MakeThemeProfile(Template.Theme);
    if (Profile.Hazard == EAstrawildRoomHazardType::AshLung)
    {
        RemoveStatusFromPlayersInside(GetRoomAshLungStatusId());
    }
    else if (Profile.Hazard == EAstrawildRoomHazardType::Waterlogged)
    {
        RemoveStatusFromPlayersInside(GetRoomWaterloggedStatusId());
    }

    if (UWorld* World = GetWorld())
    {
        for (const TWeakObjectPtr<AAstrawildBossHazardActor>& Weak : HazardTiles)
        {
            if (AAstrawildBossHazardActor* Tile = Weak.Get())
            {
                World->DestroyActor(Tile);
            }
        }
    }
    HazardTiles.Reset();
    HazardPulseElapsed = 0.0f;

    // Puzzle pillars lock in their solved-lit state (room furniture now).
    for (const TWeakObjectPtr<AAstrawildResonancePillarActor>& Weak : PuzzlePillars)
    {
        if (AAstrawildResonancePillarActor* Pillar = Weak.Get())
        {
            Pillar->SetActivated(true);
        }
    }
    if (!PuzzlePillars.IsEmpty())
    {
        PuzzleActivatedCount = PuzzlePillars.Num();
        bPuzzleSolved = true;
    }
    PuzzleWindowElapsed = 0.0f;
}

void AAstrawildDungeonRoomActor::RestoreClearedState()
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Already cleared (e.g. the entry room clears at generation time) — nothing to do.
    if (bCleared)
    {
        return;
    }

    // Silent teardown: Destroy() bypasses the defeat pipeline entirely, so no
    // HostileDefeated events, no ecosystem notifications, no loot — all of that
    // already happened when the room legitimately cleared before the save.
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : EncounterCreatures)
    {
        if (AAstrawildEchoCharacter* Creature = Weak.Get())
        {
            Creature->Destroy();
        }
    }
    EncounterCreatures.Reset();

    if (AAstrawildEchoBossCharacter* Boss = BossCreature.Get())
    {
        Boss->Destroy();
        BossCreature = nullptr;
    }

    bCleared = true;
    // DP-9: a restored room was cleared before the save — its hazard identity
    // is shed and its puzzle (if any) reads solved.
    ClearRoomHazardIdentity();
    // NOTE: no OnRoomCleared broadcast — the generator counts restored rooms itself
    // (ApplySavedState) so completion rewards never double-fire on load.
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d restored as cleared from save (encounter despawned)."), RoomIndex);
}

void AAstrawildDungeonRoomActor::GrantClearReward()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Reward the first player with research points + event.
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_HostileDefeated, Player, TEXT("DungeonRoomCleared"), 1, GetActorLocation());
            }

            // Wave 3: rooms carrying a loot table grant it on clear (boss rooms hold the boss table).
            if (!Template.ClearLootTableId.IsNone() && Player->InventoryComponent)
            {
                if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
                {
                    if (const UAstrawildLootTableDefinition* LootTable = Registry->FindLootTable(Template.ClearLootTableId))
                    {
                        for (const FAstrawildItemStack& Drop : LootTable->GuaranteedDrops)
                        {
                            Player->InventoryComponent->AddItem(Drop.ItemId, Drop.Quantity);
                        }
                        if (LootTable->BonusRollChance > 0.0f && FMath::FRand() < LootTable->BonusRollChance && LootTable->GuaranteedDrops.Num() > 0)
                        {
                            const FAstrawildItemStack& Bonus = LootTable->GuaranteedDrops[FMath::RandRange(0, LootTable->GuaranteedDrops.Num() - 1)];
                            Player->InventoryComponent->AddItem(Bonus.ItemId, Bonus.Quantity);
                        }
                        UE_LOG(LogAstrawildEconomy, Log, TEXT("Dungeon room %d loot granted to first player (%s)."), RoomIndex, *Template.ClearLootTableId.ToString());
                    }
                }
            }
        }
    }
}
