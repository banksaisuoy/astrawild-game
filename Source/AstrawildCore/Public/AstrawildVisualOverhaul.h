#pragma once

#include "CoreMinimal.h"
#include "AstrawildTypes.h"
#include "GameFramework/Actor.h"
#include "AstrawildVisualOverhaul.generated.h"

class UProceduralMeshComponent;
class APostProcessVolume;

/**
 * VO-1R — Visual Overhaul (REBUILD) pure contract layer.
 *
 * Art direction: Palworld / Animal Crossing — a friendly, saturated, bloom-soft
 * world. The original VO-1 session's binary payload was lost with its sandbox;
 * this rebuild keeps everything in the repo's zero-asset procedural language
 * (PMC geometry + post-process settings + light rig bias) so the whole layer
 * survives compile with no .uasset dependency and stays automation-testable.
 *
 * Five pillars (each has a matching world-free test):
 *   1. Toon post-process grade (FToonGrade — applied by the director volume).
 *   2. Anime atmosphere bias (FAnimeLightBias — applied by the bootstrapper's
 *      UpdateAtmosphere on top of the day/night ramp; single writer, no fight).
 *   3. Flower fields (EvalFlowerBudget + EvalPetalTint — consumed by the biome
 *      dressing actor's new flower section).
 *   4. Kawaii Echo amplification (FKawaiiScales — the VIS-001 cute band gets a
 *      bigger head/eye read plus blush dots; cool/strange bands stay authored).
 *   5. Ambient motes (EvalMoteTransform — golden pollen by day / teal fireflies
 *      by night, orbiting the local player in a bounded volume).
 *
 * Everything in the namespace is PURE (world-free, deterministic) except the
 * global enabled flag, which the bootstrapper seeds from its replicated
 * bEnableVisualOverhaul property at BeginPlay (game thread only).
 */
namespace AstrawildVisualOverhaul
{
    /** Toon/cel post-process grade (pure data — automation-tested). */
    struct ASTRAWILDCORE_API FToonGrade
    {
        /** Soft anime glow — brighter than the restrained baseline 0.6. */
        float BloomIntensity = 1.05f;
        /** Only bright petals/skies bloom; terrain stays crisp. */
        float BloomThreshold = 0.85f;
        FVector4 ColorSaturation = FVector4(1.22f, 1.22f, 1.22f, 1.22f);
        FVector4 ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.04f);
        /** Midtone lift — the "painted highlight" cel read. */
        FVector4 ColorGamma = FVector4(0.92f, 0.92f, 0.92f, 0.92f);
        float VignetteIntensity = 0.18f;
        float AutoExposureMinBrightness = 0.08f;
        float AutoExposureMaxBrightness = 3.6f;
        float AutoExposureBias = 0.35f;
    };

    /**
     * Anime atmosphere bias (pure data — automation-tested). Applied by the
     * bootstrapper AFTER the day/night ramp so the sun/sky/fog stay authored
     * while reading warmer, flatter and pastel-bright — the anime ambient look.
     */
    struct ASTRAWILDCORE_API FAnimeLightBias
    {
        /** Sun color lerps toward this warm tone by SunWarmthAmount. */
        FLinearColor SunWarmColor = FLinearColor(1.0f, 0.90f, 0.72f, 1.0f);
        float SunWarmthAmount = 0.35f;
        /** Sun slightly softer; ambient carries the scene (flat anime fill). */
        float SunIntensityScale = 0.92f;
        float SkyIntensityScale = 1.28f;
        /** Crisper far read — anime backgrounds stay legible. */
        float FogDensityScale = 0.75f;
        FLinearColor FogPastelColor = FLinearColor(0.78f, 0.85f, 0.95f, 1.0f);
        float FogPastelAmount = 0.40f;
    };

    /** Kawaii amplifier for the VIS-001 cute band (pure data — automation-tested). */
    struct ASTRAWILDCORE_API FKawaiiScales
    {
        /** Neutral defaults (1/1/0) — EvalKawaiiScales() returns the amplified profile. */
        float Head = 1.0f;
        /** Eye radius multiplier (dark eye pair reads even chunkier). */
        float Eyes = 1.0f;
        /** Blush dot radius as a fraction of the mutated head radius. */
        float Blush = 0.0f;
    };

    /** The toon grade (single source of truth — the director volume applies it). */
    ASTRAWILDCORE_API FToonGrade EvalToonGrade();

    /** The anime atmosphere bias (single source of truth — the bootstrapper applies it). */
    ASTRAWILDCORE_API FAnimeLightBias EvalAnimeLightBias();

    /** The kawaii amplifier (single source of truth — Echo bodies bake with it). */
    ASTRAWILDCORE_API FKawaiiScales EvalKawaiiScales();

    /**
     * Per-zone flower budget (pure — automation-tested). Mirrors the dressing
     * profile table via GetDressingProfile(Zone).FlowerCount so the budget has
     * exactly one authored location; the meadow zones read as flower fields.
     */
    ASTRAWILDCORE_API int32 EvalFlowerBudget(EAstrawildZone Zone);

    /**
     * Deterministic petal tint per zone/seed (pure — automation-tested): each
     * zone carries a 3-entry base palette; the seed picks an entry and jitters
     * value/saturation so a field never reads as one flat color.
     */
    ASTRAWILDCORE_API FLinearColor EvalPetalTint(EAstrawildZone Zone, int32 FlowerSeed);

    /**
     * Deterministic mote orbit (pure — automation-tested): bounded sinusoidal
     * drift around Anchor. Returns false on invalid input; the same seed +
     * time + anchor ALWAYS produces the same position (client/server identical
     * cosmetics, replay-safe).
     */
    ASTRAWILDCORE_API bool EvalMoteTransform(int32 MoteSeed, float TimeSeconds, const FVector& Anchor,
        FVector& OutPosition);

    /** Mote tint: warm pollen by day, teal firefly glow by night. */
    ASTRAWILDCORE_API FLinearColor EvalMoteTint(bool bIsNight);

    /** Max motes the director ever spawns (perf ceiling — automation-tested). */
    static constexpr int32 MaxMoteCount = 96;

    /**
     * Global enabled flag (game thread only). Seeded by the bootstrapper from
     * its replicated bEnableVisualOverhaul property; read by the Echo body
     * builder. Tests that flip it MUST save/restore the previous value.
     */
    ASTRAWILDCORE_API bool IsVisualOverhaulEnabled();
    ASTRAWILDCORE_API void SetVisualOverhaulEnabled(bool bEnabled);
}

/**
 * VO-1R — runtime director actor: owns the toon post-process volume (priority
 * above the bootstrapper's restrained baseline grade) and the ambient mote
 * layer that orbits the local player. Spawned once per machine by the world
 * bootstrapper's BuildLighting (authority AND cosmetic client paths), so no
 * gameplay actor ever replicates it — zero bandwidth, like every cosmetic
 * layer in this project. Dedicated servers skip the mote rebuild (no local
 * pawn to orbit, no renderer to feed).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildVisualOverhaulDirector : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildVisualOverhaulDirector();

    /** Ambient mote budget (clamped to MaxMoteCount). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|VisualOverhaul",
        meta=(ClampMin="0", ClampMax="96"))
    int32 MoteCount = 48;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Mote orbit radius ceiling around the player (cm — matches the pure contract). */
    static float GetMoteOrbitRadius() { return 1400.0f; }

private:
    /** Creates the unbound priority-10 PP volume with the toon grade. */
    void ApplyToonGrade();

    /** Rebuilds the mote octahedron section around the (smoothed) anchor. */
    void RebuildMotes(float TimeSeconds, const FVector& Anchor, bool bIsNight);

    /** Anchor smoothing speed (1/seconds) — motes glide, never snap. */
    static constexpr float AnchorSmoothingSpeed = 6.0f;

    UPROPERTY()
    TObjectPtr<APostProcessVolume> ToonVolume;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VisualOverhaul")
    TObjectPtr<UProceduralMeshComponent> MoteMesh;

    float ElapsedSeconds = 0.0f;
    FVector SmoothedAnchor = FVector::ZeroVector;
    bool bHasValidAnchor = false;
};
