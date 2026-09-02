// Copyright Astrawild Team. All Rights Reserved.

#include "Components/AstrawildProceduralEchoMesh.h"
#include "AstrawildEchoDataAsset.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "Runtime/Engine/Classes/SkeletalMeshTypes.h"
#include "Runtime/Engine/Classes/SkeletalMesh.h"

UAstrawildProceduralEchoMesh::UAstrawildProceduralEchoMesh()
	: CurrentBodyPlan(EAstrawildBodyPlan::Quadruped)
	, CurrentElement(EAstrawildElement::None)
	, CurrentTint(FLinearColor::White)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCastShadow(true);
}

void UAstrawildProceduralEchoMesh::GenerateProceduralMesh(const FSoftClassPath& SpeciesDataClass)
{
	if (!SpeciesDataClass.IsValid())
	{
		UE_LOG(LogAstrawild, Warning, TEXT("Invalid SpeciesDataClass for procedural mesh generation"));
		return;
	}

	UClass* DataClass = SpeciesDataClass.TryLoadClass<UAstrawildEchoDataAsset>();
	if (!DataClass)
	{
		UE_LOG(LogAstrawild, Error, TEXT("Failed to load Echo DataAsset: %s"), *SpeciesDataClass.ToString());
		return;
	}

	UAstrawildEchoDataAsset* EchoData = Cast<UAstrawildEchoDataAsset>(DataClass->GetDefaultObject());
	if (!EchoData)
	{
		UE_LOG(LogAstrawild, Error, TEXT("Invalid Echo DataAsset: %s"), *SpeciesDataClass.ToString());
		return;
	}

	SetBodyPlan(EchoData->BodyPlan);
	ApplyElementalVisuals(EchoData->Element, EchoData->PrimaryTint);
}

void UAstrawildProceduralEchoMesh::SetBodyPlan(EAstrawildBodyPlan NewBodyPlan)
{
	CurrentBodyPlan = NewBodyPlan;

	switch (NewBodyPlan)
	{
	case EAstrawildBodyPlan::Quadruped:
		GenerateQuadrupedMesh();
		break;
	case EAstrawildBodyPlan::Biped:
		GenerateBipedMesh();
		break;
	case EAstrawildBodyPlan::Serpent:
		GenerateSerpentMesh();
		break;
	case EAstrawildBodyPlan::Floating:
		GenerateFloatingMesh();
		break;
	case EAstrawildBodyPlan::Insectoid:
		GenerateInsectoidMesh();
		break;
	case EAstrawildBodyPlan::Avian:
		GenerateAvianMesh();
		break;
	case EAstrawildBodyPlan::Crystalline:
		GenerateCrystallineMesh();
		break;
	case EAstrawildBodyPlan::Amorphous:
		GenerateAmorphousMesh();
		break;
	default:
		GenerateQuadrupedMesh(); // Fallback
		break;
	}
}

void UAstrawildProceduralEchoMesh::ApplyElementalVisuals(EAstrawildElement Element, FLinearColor Tint)
{
	CurrentElement = Element;
	CurrentTint = Tint;

	// Apply material parameters if materials are assigned
	for (int32 i = 0; i < GetNumMaterials(); i++)
	{
		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(GetMaterial(i));
		if (MID)
		{
			MID->SetVectorParameterValue(FName("PrimaryTint"), Tint);
			
			FLinearColor GlowColor = FLinearColor::Black;
			float GlowIntensity = 0.0f;
			
			switch (Element)
			{
			case EAstrawildElement::Ember:
				GlowColor = FLinearColor(1.0f, 0.3f, 0.0f);
				GlowIntensity = 1.0f;
				break;
			case EAstrawildElement::Frost:
				GlowColor = FLinearColor(0.0f, 0.5f, 1.0f);
				GlowIntensity = 0.8f;
				break;
			case EAstrawildElement::Pulse:
				GlowColor = FLinearColor(0.0f, 1.0f, 0.5f);
				GlowIntensity = 1.2f;
				break;
			case EAstrawildElement::Toxic:
				GlowColor = FLinearColor(0.3f, 1.0f, 0.0f);
				GlowIntensity = 0.9f;
				break;
			case EAstrawildElement::Solar:
				GlowColor = FLinearColor(1.0f, 0.9f, 0.0f);
				GlowIntensity = 1.5f;
				break;
			case EAstrawildElement::Lunar:
				GlowColor = FLinearColor(0.5f, 0.3f, 1.0f);
				GlowIntensity = 0.7f;
				break;
			default:
				break;
			}

			MID->SetVectorParameterValue(FName("ElementalGlow"), GlowColor);
			MID->SetScalarParameterValue(FName("GlowIntensity"), GlowIntensity);
		}
	}
}

void UAstrawildProceduralEchoMesh::CreateBasicSkeleton()
{
	// Create basic bone hierarchy for runtime animation
	// This is a simplified version - full implementation would use USkeleton class
	
	BoneHierarchy.Empty();
	
	// Root bone
	BoneHierarchy.Add(FBoneReference(FName("root")));
	
	// Add body-specific bones based on current body plan
	switch (CurrentBodyPlan)
	{
	case EAstrawildBodyPlan::Quadruped:
		BoneHierarchy.Add(FBoneReference(FName("spine_01")));
		BoneHierarchy.Add(FBoneReference(FName("spine_02")));
		BoneHierarchy.Add(FBoneReference(FName("neck")));
		BoneHierarchy.Add(FBoneReference(FName("head")));
		BoneHierarchy.Add(FBoneReference(FName("leg_front_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_front_r")));
		BoneHierarchy.Add(FBoneReference(FName("leg_back_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_back_r")));
		BoneHierarchy.Add(FBoneReference(FName("tail_01")));
		BoneHierarchy.Add(FBoneReference(FName("tail_02")));
		break;
		
	case EAstrawildBodyPlan::Biped:
		BoneHierarchy.Add(FBoneReference(FName("pelvis")));
		BoneHierarchy.Add(FBoneReference(FName("spine_01")));
		BoneHierarchy.Add(FBoneReference(FName("spine_02")));
		BoneHierarchy.Add(FBoneReference(FName("neck")));
		BoneHierarchy.Add(FBoneReference(FName("head")));
		BoneHierarchy.Add(FBoneReference(FName("arm_l")));
		BoneHierarchy.Add(FBoneReference(FName("arm_r")));
		BoneHierarchy.Add(FBoneReference(FName("leg_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_r")));
		break;
		
	case EAstrawildBodyPlan::Serpent:
		for (int32 i = 0; i < 12; i++)
		{
			BoneHierarchy.Add(FBoneReference(FName(*FString::Printf(TEXT("spine_%02d"), i))));
		}
		BoneHierarchy.Add(FBoneReference(FName("head")));
		break;
		
	case EAstrawildBodyPlan::Floating:
		BoneHierarchy.Add(FBoneReference(FName("core")));
		BoneHierarchy.Add(FBoneReference(FName("orb_l")));
		BoneHierarchy.Add(FBoneReference(FName("orb_r")));
		BoneHierarchy.Add(FBoneReference(FName("orb_top")));
		break;
		
	case EAstrawildBodyPlan::Insectoid:
		BoneHierarchy.Add(FBoneReference(FName("thorax")));
		BoneHierarchy.Add(FBoneReference(FName("abdomen")));
		BoneHierarchy.Add(FBoneReference(FName("head")));
		BoneHierarchy.Add(FBoneReference(FName("leg_01_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_01_r")));
		BoneHierarchy.Add(FBoneReference(FName("leg_02_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_02_r")));
		BoneHierarchy.Add(FBoneReference(FName("leg_03_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_03_r")));
		BoneHierarchy.Add(FBoneReference(FName("wing_l")));
		BoneHierarchy.Add(FBoneReference(FName("wing_r")));
		break;
		
	case EAstrawildBodyPlan::Avian:
		BoneHierarchy.Add(FBoneReference(FName("body")));
		BoneHierarchy.Add(FBoneReference(FName("neck")));
		BoneHierarchy.Add(FBoneReference(FName("head")));
		BoneHierarchy.Add(FBoneReference(FName("wing_l")));
		BoneHierarchy.Add(FBoneReference(FName("wing_r")));
		BoneHierarchy.Add(FBoneReference(FName("leg_l")));
		BoneHierarchy.Add(FBoneReference(FName("leg_r")));
		BoneHierarchy.Add(FBoneReference(FName("tail")));
		break;
		
	case EAstrawildBodyPlan::Crystalline:
		BoneHierarchy.Add(FBoneReference(FName("core")));
		BoneHierarchy.Add(FBoneReference(FName("crystal_01")));
		BoneHierarchy.Add(FBoneReference(FName("crystal_02")));
		BoneHierarchy.Add(FBoneReference(FName("crystal_03")));
		BoneHierarchy.Add(FBoneReference(FName("crystal_04")));
		break;
		
	case EAstrawildBodyPlan::Amorphous:
		BoneHierarchy.Add(FBoneReference(FName("center")));
		for (int32 i = 0; i < 8; i++)
		{
			BoneHierarchy.Add(FBoneReference(FName(*FString::Printf(TEXT("pseudopod_%02d"), i))));
		}
		break;
	}
}

void UAstrawildProceduralEchoMesh::BuildMeshSection(const TArray<FVector>& Vertices, const TArray<int32>& Indices, int32 SectionID)
{
	// Implementation would create mesh sections from vertices and indices
	// This is a placeholder for the actual mesh generation logic
	
	if (Vertices.Num() == 0 || Indices.Num() == 0)
	{
		return;
	}

	// In a full implementation, this would:
	// 1. Create FStaticSkinnedVertex data
	// 2. Build mesh sections
	// 3. Assign materials
	// 4. Update collision
	
	UE_LOG(LogAstrawild, Verbose, TEXT("Building mesh section %d with %d vertices and %d indices"), 
		SectionID, Vertices.Num(), Indices.Num());
}

void UAstrawildProceduralEchoMesh::GenerateQuadrupedMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate simple capsule-based body with 4 legs
	// Body
	const FVector BodyCenter(0, 0, 50);
	const float BodyRadius = 30.0f;
	const float BodyHeight = 80.0f;
	
	// Simplified vertex generation for prototype
	// Full implementation would generate proper quad-based mesh
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateBipedMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate humanoid-shaped mesh
	// Head, torso, arms, legs
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateSerpentMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate segmented snake-like body
	// Multiple cylinder segments
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateFloatingMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate central core with orbiting elements
	// Sphere with smaller spheres around it
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateInsectoidMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate insect body: thorax, abdomen, 6 legs, wings
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateAvianMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate bird-like body with wings and tail
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateCrystallineMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate crystalline structure with sharp edges
	// Icosahedron-based geometry
	
	BuildMeshSection(Vertices, Indices, 0);
}

void UAstrawildProceduralEchoMesh::GenerateAmorphousMesh()
{
	CreateBasicSkeleton();
	
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	
	// Generate blob-like shape with pseudo-pods
	// Metaballs or noise-displaced sphere
	
	BuildMeshSection(Vertices, Indices, 0);
}
