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

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Water", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Water", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UBoxComponent> SurfaceCollision;
};
