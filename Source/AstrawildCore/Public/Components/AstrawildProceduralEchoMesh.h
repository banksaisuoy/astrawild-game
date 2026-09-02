// Copyright Astrawild Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildProceduralEchoMesh.generated.h"

/**
 * Procedural Mesh Generator for Echo Species
 * Generates basic skeletal meshes at runtime based on Body Plan
 * Eliminates dependency on external 3D models for initial testing
 */
UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildProceduralEchoMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UAstrawildProceduralEchoMesh();

	/** Call this to generate the mesh based on species data */
	UFUNCTION(BlueprintCallable, Category="Echo|Generation")
	void GenerateProceduralMesh(const FSoftClassPath& SpeciesDataClass);

	/** Set the body plan type explicitly */
	UFUNCTION(BlueprintCallable, Category="Echo|Generation")
	void SetBodyPlan(EAstrawildBodyPlan NewBodyPlan);

	/** Update material parameters based on elemental type */
	UFUNCTION(BlueprintCallable, Category="Echo|Visuals")
	void ApplyElementalVisuals(EAstrawildElement Element, FLinearColor Tint);

protected:
	/** Generate geometry for specific body plans */
	void GenerateQuadrupedMesh();
	void GenerateBipedMesh();
	void GenerateSerpentMesh();
	void GenerateFloatingMesh();
	void GenerateInsectoidMesh();
	void GenerateAvianMesh();
	void GenerateCrystallineMesh();
	void GenerateAmorphousMesh();

	/** Helper to create a simple bone hierarchy */
	void CreateBasicSkeleton();

	/** Helper to build mesh section from primitive shapes */
	void BuildMeshSection(const TArray<FVector>& Vertices, const TArray<int32>& Indices, int32 SectionID);

private:
	EAstrawildBodyPlan CurrentBodyPlan;
	EAstrawildElement CurrentElement;
	FLinearColor CurrentTint;
	
	TArray<FBoneReference> BoneHierarchy;
	TArray<FSkeletalMeshLODInfo> LODInfo;
};
