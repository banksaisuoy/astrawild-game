#include "AstrawildVisualOverhaul.h"

#include "AstrawildBiomeDressingActor.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildVfxActor.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"

// ===========================================================================
// Pure contract layer (world-free — automation-tested)
// ===========================================================================

namespace
{
    /** Global enabled flag — game thread only, seeded by the bootstrapper. */
    bool GBVisualOverhaulEnabled = true;

    /**
     * Per-zone petal palettes (VO-1R pillar 3). Three entries each so a seed
     * mod pick always yields visible variety inside one field; Verdant Reach
     * deliberately spans the hue wheel ("rainbow meadow" landmark read).
     */
    const FLinearColor GPetalPalettes[13][3] = {
        // None / Wilderness — quiet meadow whites (budget is 0 anyway).
        { FLinearColor(0.95f, 0.95f, 0.92f, 1.0f), FLinearColor(0.92f, 0.95f, 0.88f, 1.0f), FLinearColor(0.96f, 0.93f, 0.90f, 1.0f) },
        // Dawn Fields — coral / sunrise cream / blush rose.
        { FLinearColor(1.00f, 0.55f, 0.62f, 1.0f), FLinearColor(1.00f, 0.90f, 0.72f, 1.0f), FLinearColor(0.98f, 0.68f, 0.80f, 1.0f) },
        // Dusk Marsh — violet / lilac / teal mist.
        { FLinearColor(0.62f, 0.45f, 0.85f, 1.0f), FLinearColor(0.75f, 0.60f, 0.95f, 1.0f), FLinearColor(0.45f, 0.80f, 0.85f, 1.0f) },
        // Ember Ridge — ember orange / magma red / gold.
        { FLinearColor(1.00f, 0.55f, 0.25f, 1.0f), FLinearColor(0.95f, 0.35f, 0.30f, 1.0f), FLinearColor(1.00f, 0.80f, 0.35f, 1.0f) },
        // Frostveil Expanse — ice blue / white lavender / frost cyan.
        { FLinearColor(0.65f, 0.82f, 0.98f, 1.0f), FLinearColor(0.88f, 0.85f, 0.97f, 1.0f), FLinearColor(0.70f, 0.92f, 0.90f, 1.0f) },
        // Glimmerwood — glow teal / mint / bioluminescent cyan.
        { FLinearColor(0.40f, 0.95f, 0.85f, 1.0f), FLinearColor(0.55f, 0.95f, 0.65f, 1.0f), FLinearColor(0.35f, 0.85f, 1.00f, 1.0f) },
        // Hollow Approach — ash purple / pale bone / dusk violet.
        { FLinearColor(0.50f, 0.42f, 0.60f, 1.0f), FLinearColor(0.80f, 0.75f, 0.70f, 1.0f), FLinearColor(0.55f, 0.50f, 0.65f, 1.0f) },
        // Azure Shallows — aqua / seafoam / sky.
        { FLinearColor(0.45f, 0.90f, 0.95f, 1.0f), FLinearColor(0.65f, 0.95f, 0.85f, 1.0f), FLinearColor(0.55f, 0.80f, 0.98f, 1.0f) },
        // Tidebreaker Isles — coral / hot pink / sun yellow.
        { FLinearColor(1.00f, 0.50f, 0.45f, 1.0f), FLinearColor(1.00f, 0.60f, 0.75f, 1.0f), FLinearColor(1.00f, 0.90f, 0.50f, 1.0f) },
        // Sunscar Desert — magenta bloom / dry gold / dusty rose.
        { FLinearColor(0.95f, 0.50f, 0.80f, 1.0f), FLinearColor(0.95f, 0.80f, 0.45f, 1.0f), FLinearColor(0.90f, 0.65f, 0.55f, 1.0f) },
        // Stormcrest Highlands — thistle / storm blue / silver.
        { FLinearColor(0.70f, 0.65f, 0.90f, 1.0f), FLinearColor(0.55f, 0.65f, 0.90f, 1.0f), FLinearColor(0.80f, 0.85f, 0.95f, 1.0f) },
        // Verdant Reach — poppy / cornflower / marigold (rainbow meadow).
        { FLinearColor(1.00f, 0.45f, 0.40f, 1.0f), FLinearColor(0.50f, 0.60f, 0.95f, 1.0f), FLinearColor(1.00f, 0.80f, 0.30f, 1.0f) },
        // Pearlsea Reef — pearl pink / aqua pearl / shell peach.
        { FLinearColor(1.00f, 0.75f, 0.80f, 1.0f), FLinearColor(0.70f, 0.92f, 0.95f, 1.0f), FLinearColor(1.00f, 0.85f, 0.72f, 1.0f) },
    };
}

// Free-function definitions use qualified names (AstrawildVisualOverhaul::X)
// so header/impl pairing tools see them, matching the lint contract.

AstrawildVisualOverhaul::FToonGrade AstrawildVisualOverhaul::EvalToonGrade()
{
    return FToonGrade();
}

AstrawildVisualOverhaul::FAnimeLightBias AstrawildVisualOverhaul::EvalAnimeLightBias()
{
    return FAnimeLightBias();
}

AstrawildVisualOverhaul::FKawaiiScales AstrawildVisualOverhaul::EvalKawaiiScales()
{
    // The amplified profile — compounds on top of the VIS-001 cute 1.18x
    // head boost (1.18 * 1.12 ~= 1.32 total) and never applies when the
    // overhaul is off (neutral struct defaults stay 1/1/0).
    FKawaiiScales Scales;
    Scales.Head = 1.12f;
    Scales.Eyes = 1.35f;
    Scales.Blush = 0.10f;
    return Scales;
}

int32 AstrawildVisualOverhaul::EvalFlowerBudget(const EAstrawildZone Zone)
{
    // Single source of truth: the dressing profile table owns the budget.
    return AAstrawildBiomeDressingActor::GetDressingProfile(Zone).FlowerCount;
}

FLinearColor AstrawildVisualOverhaul::EvalPetalTint(const EAstrawildZone Zone, const int32 FlowerSeed)
{
    const int32 ZoneIndex = static_cast<int32>(Zone);
    if (ZoneIndex < 0 || ZoneIndex > 12)
    {
        return FLinearColor::White;
    }

    const FLinearColor Base = GPetalPalettes[ZoneIndex][((FlowerSeed % 3) + 3) % 3];

    // Deterministic value/saturation jitter so one field never reads flat.
    FRandomStream Stream(static_cast<int32>(static_cast<uint32>(FlowerSeed) * 2654435761u) ^ (ZoneIndex + 17));
    const float ValueScale = Stream.FRandRange(0.85f, 1.10f);
    const float SaturationScale = Stream.FRandRange(0.90f, 1.15f);

    const float Luma = Base.R * 0.3f + Base.G * 0.59f + Base.B * 0.11f;
    FLinearColor Out(
        Luma + (Base.R - Luma) * SaturationScale,
        Luma + (Base.G - Luma) * SaturationScale,
        Luma + (Base.B - Luma) * SaturationScale,
        1.0f);
    Out *= ValueScale;
    return FLinearColor(
        FMath::Clamp(Out.R, 0.05f, 1.0f),
        FMath::Clamp(Out.G, 0.05f, 1.0f),
        FMath::Clamp(Out.B, 0.05f, 1.0f),
        1.0f);
}

bool AstrawildVisualOverhaul::EvalMoteTransform(const int32 MoteSeed, const float TimeSeconds,
    const FVector& Anchor, FVector& OutPosition)
{
    if (MoteSeed < 0 || TimeSeconds < 0.0f)
    {
        return false;
    }

    FRandomStream Stream(static_cast<int32>(static_cast<uint32>(MoteSeed) * 2246822519u + 1u));
    const float W1 = Stream.FRandRange(0.35f, 0.90f);
    const float W2 = Stream.FRandRange(0.35f, 0.90f);
    const float W3 = Stream.FRandRange(0.50f, 1.40f);
    const float Phase = Stream.FRandRange(0.0f, 2.0f * PI);
    const float Rx = Stream.FRandRange(120.0f, 850.0f);
    const float Ry = Stream.FRandRange(120.0f, 850.0f);
    const float Rz = Stream.FRandRange(40.0f, 220.0f);
    const float Lift = Stream.FRandRange(80.0f, 280.0f);

    OutPosition = Anchor + FVector(
        FMath::Sin(TimeSeconds * W1 + Phase) * Rx,
        FMath::Cos(TimeSeconds * W2 + Phase) * Ry,
        FMath::Sin(TimeSeconds * W3 + Phase * 1.7f) * Rz + Lift);
    return true;
}

FLinearColor AstrawildVisualOverhaul::EvalMoteTint(const bool bIsNight)
{
    // Warm pollen by day, teal firefly glow by night — the two anime
    // "living air" reads that every zone shares.
    return bIsNight
        ? FLinearColor(0.45f, 0.85f, 1.00f, 1.0f)
        : FLinearColor(1.00f, 0.85f, 0.45f, 1.0f);
}

bool AstrawildVisualOverhaul::IsVisualOverhaulEnabled()
{
    return GBVisualOverhaulEnabled;
}

void AstrawildVisualOverhaul::SetVisualOverhaulEnabled(const bool bEnabled)
{
    GBVisualOverhaulEnabled = bEnabled;
}

// ===========================================================================
// Director actor (toon grade volume + ambient mote layer)
// ===========================================================================

AAstrawildVisualOverhaulDirector::AAstrawildVisualOverhaulDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    MoteMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MoteMesh"));
    SetRootComponent(MoteMesh);
    MoteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MoteMesh->SetAbsolute(true, true, true);
}

void AAstrawildVisualOverhaulDirector::BeginPlay()
{
    Super::BeginPlay();

    ApplyToonGrade();

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("VO-1R visual overhaul director online: toon grade applied, %d ambient motes."),
        FMath::Clamp(MoteCount, 0, AstrawildVisualOverhaul::MaxMoteCount));
}

void AAstrawildVisualOverhaulDirector::ApplyToonGrade()
{
    UWorld* World = GetWorld();
    if (!World || !ToonVolume)
    {
        if (!World)
        {
            return;
        }
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ToonVolume = World->SpawnActor<APostProcessVolume>(
            APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
    }

    if (!ToonVolume)
    {
        return;
    }

    // Priority 10 sits above the bootstrapper's restrained baseline volume
    // (priority 1) — the toon grade wins for every field it overrides.
    ToonVolume->bUnbound = true;
    ToonVolume->bEnabled = true;
    ToonVolume->Priority = 10;

    const AstrawildVisualOverhaul::FToonGrade Grade = AstrawildVisualOverhaul::EvalToonGrade();
    FPostProcessSettings& Settings = ToonVolume->Settings;
    Settings.bOverride_BloomIntensity = true;
    Settings.BloomIntensity = Grade.BloomIntensity;
    Settings.bOverride_BloomThreshold = true;
    Settings.BloomThreshold = Grade.BloomThreshold;
    Settings.bOverride_ColorSaturation = true;
    Settings.ColorSaturation = Grade.ColorSaturation;
    Settings.bOverride_ColorContrast = true;
    Settings.ColorContrast = Grade.ColorContrast;
    Settings.bOverride_ColorGamma = true;
    Settings.ColorGamma = Grade.ColorGamma;
    Settings.bOverride_VignetteIntensity = true;
    Settings.VignetteIntensity = Grade.VignetteIntensity;
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = Grade.AutoExposureMinBrightness;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = Grade.AutoExposureMaxBrightness;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = Grade.AutoExposureBias;
}

void AAstrawildVisualOverhaulDirector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetNetMode() == NM_DedicatedServer)
    {
        return; // no local pawn to orbit, no renderer to feed
    }

    ElapsedSeconds += DeltaTime;

    // Anchor = local player pawn; smoothed so the mote cloud glides with the
    // player instead of snapping on possession/teleport.
    FVector TargetAnchor = SmoothedAnchor;
    if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        if (const APawn* Pawn = PC->GetPawn())
        {
            TargetAnchor = Pawn->GetActorLocation();
        }
    }

    if (!bHasValidAnchor)
    {
        SmoothedAnchor = TargetAnchor;
        bHasValidAnchor = true;
    }
    else
    {
        SmoothedAnchor = FMath::VInterpTo(SmoothedAnchor, TargetAnchor, DeltaTime, AnchorSmoothingSpeed);
    }

    bool bIsNight = false;
    if (const AAstrawildGameState* GameState = GetWorld() ? Cast<AAstrawildGameState>(GetWorld()->GetGameState()) : nullptr)
    {
        bIsNight = GameState->IsNight();
    }

    RebuildMotes(ElapsedSeconds, SmoothedAnchor, bIsNight);
}

void AAstrawildVisualOverhaulDirector::RebuildMotes(const float TimeSeconds, const FVector& Anchor, const bool bIsNight)
{
    if (!MoteMesh)
    {
        return;
    }

    const int32 Count = FMath::Clamp(MoteCount, 0, AstrawildVisualOverhaul::MaxMoteCount);
    if (Count <= 0)
    {
        MoteMesh->ClearAllMeshSections();
        return;
    }

    const FLinearColor Tint = AstrawildVisualOverhaul::EvalMoteTint(bIsNight);
    const FColor MoteColor = Tint.ToFColor(true);

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;
    Vertices.Reserve(Count * 6);
    Triangles.Reserve(Count * 24);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        FVector Position;
        if (!AstrawildVisualOverhaul::EvalMoteTransform(0x5A17 + Index * 7919, TimeSeconds, Anchor, Position))
        {
            continue;
        }

        // One small octahedron reads from every side at gameplay distance —
        // no billboarding needed, no camera-facing tick cost.
        const float Radius = 7.0f + 4.0f * (Index % 3); // subtle size variety
        const int32 Base = Vertices.Num();
        Vertices.Append({
            Position + FVector(0, 0, Radius),
            Position + FVector(0, 0, -Radius),
            Position + FVector(Radius, 0, 0),
            Position + FVector(-Radius, 0, 0),
            Position + FVector(0, Radius, 0),
            Position + FVector(0, -Radius, 0)
        });
        for (int32 VertexIndex = 0; VertexIndex < 6; ++VertexIndex)
        {
            Normals.Add((Vertices[Base + VertexIndex] - Position).GetSafeNormal());
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(MoteColor);
        }
        Triangles.Append({ Base, Base + 2, Base + 4, Base, Base + 4, Base + 3,
                           Base, Base + 3, Base + 5, Base, Base + 5, Base + 2,
                           Base + 1, Base + 4, Base + 2, Base + 1, Base + 3, Base + 4,
                           Base + 1, Base + 2, Base + 5, Base + 1, Base + 5, Base + 3 });
    }

    if (Vertices.Num() > 0)
    {
        MoteMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors,
            TArray<FProcMeshTangent>(), false);

        if (UMaterial* Material = AAstrawildBeamVfxActor::LoadVertexColorMaterial())
        {
            MoteMesh->SetMaterial(0, Material);
        }
    }
}
