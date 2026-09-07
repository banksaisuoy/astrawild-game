#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildAssetFallback.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Basic engine geometry the fallback layer can substitute for a missing asset
 * (directive [2].1). Engine basic shapes always exist — they ship with the
 * runtime — so the game can never crash on a missing project mesh.
 */
UENUM(BlueprintType)
enum class EAstrawildFallbackShape : uint8
{
    Cube UMETA(DisplayName="Cube"),
    Sphere UMETA(DisplayName="Sphere"),
    Cylinder UMETA(DisplayName="Cylinder"),
    Cone UMETA(DisplayName="Cone")
};

/**
 * SCP Phase 2 — AssetFallbackManager (directive [2].1 crash-proof spec).
 *
 * Central place for every runtime asset load: when a StaticMesh fails to
 * resolve (missing file, LFS pointer not fetched, cooked package stripped),
 * the manager hands back an engine basic shape instead of null so callers
 * keep rendering geometry and the game keeps running. Every substitution is
 * reported to the error reporter (on-disk trail) and counted.
 *
 * The repo's procedural-first art direction (PMC silhouettes) remains the
 * primary path; this layer hardens the secondary asset-driven paths
 * (skiff mesh, held weapons, environment scatter, future content packs).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildAssetFallbackLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Engine path for a fallback shape (always resolvable at runtime). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Assets")
    static FString GetFallbackShapePath(EAstrawildFallbackShape Shape);

    /**
     * Resolve a mesh with fallback: returns Path's mesh when it loads,
     * otherwise the engine basic shape for FallbackShape (never null).
     * Reports the substitution through the error reporter once per context id.
     */
    static UStaticMesh* ResolveMeshWithFallback(const FSoftObjectPath& Path,
        EAstrawildFallbackShape FallbackShape, FName ContextId);

    /**
     * Install a resolved mesh onto a component (null-safe no-op when the
     * component is null). Returns the mesh actually applied.
     */
    static UStaticMesh* ApplyMeshToComponent(UStaticMeshComponent* Component,
        UStaticMesh* Mesh);

    /** Substitutions served since process start (diagnostics HUD hook). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Assets")
    static int32 GetFallbackCount();

    /** Successful direct resolutions since process start. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Assets")
    static int32 GetDirectResolveCount();

private:
    static int32 FallbackCount;
    static int32 DirectResolveCount;
};
