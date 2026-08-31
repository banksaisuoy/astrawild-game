#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildVfxActor.generated.h"

class UPointLightComponent;
class UProceduralMeshComponent;
class UMaterial;

/**
 * Shared color vocabulary for every runtime placeholder effect (Production V2
 * Batch 2 — Visual Vertical Slice, Master Plan §21/§31).
 *
 * One palette drives beam tints, projectile cores, Echo rarity rings, held
 * weapon colors and scanner pulses so the whole game speaks the same visual
 * language. Pure statics — deterministic, world-free, automation-testable.
 */
struct ASTRAWILDCORE_API FAstrawildVfxPalette
{
    /** Element identity color (weapons, projectile cores, Echo element glow). */
    static FLinearColor GetElementTint(EAstrawildElementType Element);

    /** Rarity ladder color (Echo rings, loot feedback). */
    static FLinearColor GetRarityTint(EAstrawildRarity Rarity);

    /** Weapon family identity color (held weapon mesh, muzzle flash). */
    static FLinearColor GetWeaponFamilyTint(EAstrawildWeaponFamily Family);

    /** Scanner tier pulse color by scanner ITEM id (Field/Array/Oracle). */
    static FLinearColor GetScannerTint(FName ScannerItemId);
};

/**
 * Runtime placeholder beam / arc-chain / muzzle-flash actor (Master Plan §8
 * combat readability + §21 priority VFX).
 *
 * The Beam and ArcChain weapon archetypes previously resolved damage with NO
 * visual at all (documented gap in Batch 1 §7). This actor draws a bright
 * vertex-colored stretched prism between muzzle and impact, with an optional
 * jagged multi-hop lightning path for Arc Chain weapons, plus a short-lived
 * point light so every shot reads at gameplay distance.
 *
 * Antigravity contract: the weapon definition's MuzzleVfxId / TrailVfxId /
 * ImpactVfxId remain the authoritative Niagara binding surface
 * (NS_AW_Weap_<id>) — this placeholder is replaced by the real systems in
 * the art pass; the damage pipeline is untouched.
 *
 * Cosmetic only: spawned where the attack executes (server / listen server /
 * standalone). Dedicated-MP replication is deferred to the H-12 RPC pass.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBeamVfxActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildBeamVfxActor();

    /**
     * Draw one bright beam from Start to End. Thin stretched prism + midpoint
     * light; thickness collapses over LifetimeSeconds, then the actor destroys
     * itself. Returns nullptr on invalid input or non-game worlds.
     */
    static AAstrawildBeamVfxActor* SpawnBeam(UWorld* World, const FVector& Start, const FVector& End,
        const FLinearColor& Tint, float Thickness = 14.0f, float LifetimeSeconds = 0.16f);

    /**
     * Draw a jagged lightning path through the hop chain (muzzle → first hit →
     * chain targets). Each hop splits into jittered sub-segments. One actor for
     * the whole shot; hop points with no chain just draw a straight bolt.
     */
    static AAstrawildBeamVfxActor* SpawnArcChain(UWorld* World, const TArray<FVector>& HopPoints,
        const FLinearColor& Tint, float LifetimeSeconds = 0.24f);

    /** Brief muzzle flash: small bright core + light at the firing position. */
    static AAstrawildBeamVfxActor* SpawnMuzzleFlash(UWorld* World, const FVector& Location, const FVector& Direction,
        const FLinearColor& Tint, float LifetimeSeconds = 0.09f);

    /** Shared vertex-color material loader (DebugMeshMaterial — same path as terrain/Echo bodies). */
    static UMaterial* LoadVertexColorMaterial();

    /**
     * Pure geometry helper (automation-tested): the transform that centers and
     * orients a box between two points. Returns false when the points coincide.
     */
    static bool ComputeBeamTransform(const FVector& Start, const FVector& End, FVector& OutCenter,
        FRotator& OutRotation, float& OutLength);

    /**
     * Pure arc helper (automation-tested): splits A→B into SubSegmentCount + 1
     * waypoints with deterministic perpendicular jitter (FRandomStream seed) —
     * the lightning zig-zag shape. Always includes A and B.
     */
    static void ComputeArcJitter(int32 Seed, const FVector& A, const FVector& B, int32 SubSegmentCount,
        float JitterAmplitude, TArray<FVector>& OutWaypoints);

    virtual void Tick(float DeltaTime) override;

private:
    /** Builds the beam prism geometry (unit thickness, length baked on X). */
    void BuildBeamGeometry(const TArray<FVector>& InWaypoints, const FLinearColor& Tint, bool bJagged);

    /** Muzzle flash core: two-cone octahedron along the fire direction. */
    void BuildMuzzleGeometry(const FLinearColor& Tint);

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UProceduralMeshComponent> BeamMesh;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UPointLightComponent> BeamLight;

    float ElapsedSeconds = 0.0f;
    float LifetimeSeconds = 0.16f;
};

/**
 * Scanner pulse ring (Master Plan §10 — scanner is a signature tool; hold-V
 * now produces a world-space expanding ring so scanning reads as an action).
 *
 * One flattened annulus (vertex-colored, DebugMeshMaterial) scales outward
 * from the scanning player to the scanner's effective range while a point
 * light fades — the Oracle Scanner pulse visibly travels further than the
 * Field Scanner's, selling the upgrade path. Cosmetic, locally spawned.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildScannerPulseActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildScannerPulseActor();

    /**
     * Spawn an expanding pulse ring at Origin. MaxRadius is the scanner's
     * effective observation range (tint from FAstrawildVfxPalette by tier).
     */
    static AAstrawildScannerPulseActor* SpawnPulse(UWorld* World, const FVector& Origin,
        const FLinearColor& Tint, float MaxRadius = 3500.0f, float DurationSeconds = 0.9f);

    /**
     * Pure geometry helper (automation-tested): builds one annulus ring
     * (Segments quads) at unit radius scale — inner fraction of the outer
     * radius so the ring keeps visible thickness as it scales up.
     */
    static void BuildRingGeometry(int32 Segments, float InnerFraction, const FLinearColor& Tint,
        TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals,
        TArray<FVector2D>& OutUVs, TArray<FColor>& OutColors);

    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UProceduralMeshComponent> RingMesh;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UPointLightComponent> PulseLight;

    float ElapsedSeconds = 0.0f;
    float DurationSeconds = 0.9f;
    float MaxRadius = 3500.0f;
};

/**
 * Capture sequence VFX actor (Master Plan §21/§31): 3 spinning orbital resonance
 * rings around the target Echo that contract inward with a burst of capture light.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildCaptureVfxActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildCaptureVfxActor();

    /**
     * Spawn orbital capture rings at TargetLocation. Contracts over DurationSeconds.
     */
    static AAstrawildCaptureVfxActor* SpawnCaptureVfx(UWorld* World, const FVector& TargetLocation,
        const FLinearColor& Tint, float InitialRadius = 160.0f, float DurationSeconds = 1.0f);

    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UProceduralMeshComponent> CaptureMesh;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|VFX")
    TObjectPtr<UPointLightComponent> CaptureLight;

    float ElapsedSeconds = 0.0f;
    float DurationSeconds = 1.0f;
    float InitialRadius = 160.0f;
    FLinearColor BaseTint = FLinearColor::White;
};
