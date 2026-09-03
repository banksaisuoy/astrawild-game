#include "AstrawildAssetFallback.h"

#include "AstrawildErrorReporter.h"
#include "AstrawildLog.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
    /** Context ids already reported this session — one diagnostic per site. */
    TSet<FName> ReportedContexts;
}

int32 UAstrawildAssetFallbackLibrary::FallbackCount = 0;
int32 UAstrawildAssetFallbackLibrary::DirectResolveCount = 0;

FString UAstrawildAssetFallbackLibrary::GetFallbackShapePath(EAstrawildFallbackShape Shape)
{
    switch (Shape)
    {
    case EAstrawildFallbackShape::Cube:
        return TEXT("/Engine/BasicShapes/Cube.Cube");
    case EAstrawildFallbackShape::Sphere:
        return TEXT("/Engine/BasicShapes/Sphere.Sphere");
    case EAstrawildFallbackShape::Cylinder:
        return TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    case EAstrawildFallbackShape::Cone:
        return TEXT("/Engine/BasicShapes/Cone.Cone");
    default:
        return TEXT("/Engine/BasicShapes/Cube.Cube");
    }
}

UStaticMesh* UAstrawildAssetFallbackLibrary::ResolveMeshWithFallback(const FSoftObjectPath& Path,
    EAstrawildFallbackShape FallbackShape, FName ContextId)
{
    // Try the intended asset first (safe for empty paths — returns null).
    if (UStaticMesh* Mesh = Cast<UStaticMesh>(Path.ResolveObject()))
    {
        ++DirectResolveCount;
        return Mesh;
    }

    if (UStaticMesh* Mesh = Cast<UStaticMesh>(Path.TryLoad()))
    {
        ++DirectResolveCount;
        return Mesh;
    }

    // Asset missing/unloadable: engine basic shapes always ship with the
    // runtime, so gameplay continues with stand-in geometry (directive [2].1).
    const FString FallbackPath = GetFallbackShapePath(FallbackShape);
    UStaticMesh* FallbackMesh = LoadObject<UStaticMesh>(nullptr, *FallbackPath);
    if (!FallbackMesh)
    {
        // Pathological: even the engine shape failed. Report loudly — callers
        // must null-check as usual, but this is an engine install problem.
        UAstrawildErrorReporterLibrary::ReportError(TEXT("AssetFallback"),
            FString::Printf(TEXT("Context %s: engine fallback shape %s failed to load"),
                *ContextId.ToString(), *FallbackPath));
        return nullptr;
    }

    ++FallbackCount;

    // One diagnostic line per context — keeps the report readable when a whole
    // pack is missing (single line, not one per scattered instance).
    if (!ReportedContexts.Contains(ContextId))
    {
        ReportedContexts.Add(ContextId);
        UAstrawildErrorReporterLibrary::ReportWarning(TEXT("AssetFallback"),
            FString::Printf(TEXT("Context %s: asset %s missing — substituted %s"),
                *ContextId.ToString(), *Path.ToString(), *FallbackPath));
    }

    return FallbackMesh;
}

UStaticMesh* UAstrawildAssetFallbackLibrary::ApplyMeshToComponent(UStaticMeshComponent* Component,
    UStaticMesh* Mesh)
{
    if (!Component || !Mesh)
    {
        return nullptr;
    }

    Component->SetStaticMesh(Mesh);
    return Mesh;
}

int32 UAstrawildAssetFallbackLibrary::GetFallbackCount()
{
    return FallbackCount;
}

int32 UAstrawildAssetFallbackLibrary::GetDirectResolveCount()
{
    return DirectResolveCount;
}
