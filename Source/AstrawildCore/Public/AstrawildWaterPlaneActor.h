#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildWaterPlaneActor.generated.h"

class UProceduralMeshComponent;
class UBoxComponent;

/**
 * Batch 8 — the sea surface (Docs/ASTRAWILD_VILLAGES_SKIFF.md).
 *
 * A thin, walkable water plane at the global sea level (UAstrawildZoneSubsystem::
 * GetSeaLevelZ). Vertex-colored deep blue through the same DebugMeshMaterial
 * path as the terrain tiles; a flattened box provides solid collision so the
 * sea reads as a wading-height stylized surface (real swimming arrives with the
 * swim system batch — see Engine Verification Queue).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildWaterPlaneActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildWaterPlaneActor();

    /** Builds the plane covering the given rect (world XY, cm) at SeaLevelZ. */
    void BuildPlane(const FBox2D& WorldRect);

    /**
     * DP-9 (dungeon depth — additive): same plane at an explicit box-center Z
     * and optional thickness (default = the sea contract's 120cm slab). Room-
     * local placement for the Sunken Vault's flooded-floor accent uses a thin
     * film (the walkable top stays a small step above the room floor — never a
     * progression blocker); the sea-level path above stays byte-identical.
     */
    void BuildPlaneAtZ(const FBox2D& WorldRect, float SurfaceZ, float BoxThicknessCm = 120.0f);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Water", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Water", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UBoxComponent> SurfaceCollision;
};
