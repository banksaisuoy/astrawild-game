// Copyright Astrawild. All Rights Reserved.

#include "Materials/AstrawildMaterialBuilder.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "AstrawildBestiaryData.h"

UMaterialInstanceDynamic* UAstrawildMaterialBuilder::CreateSurvivorExosuitMI(UObject* WorldContext, const FString& ParentMaterialPath)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World || !World->IsGameWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid world context for Survivor MI creation"));
		return nullptr;
	}

	UObject* ParentMat = StaticLoadObject(UMaterial::StaticClass(), nullptr, *ParentMaterialPath);
	if (!ParentMat)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load parent material: %s"), *ParentMaterialPath);
		return nullptr;
	}

	UMaterialInstanceDynamic* MatInst = UMaterialInstanceDynamic::Create(Cast<UMaterial>(ParentMat), World);
	if (!MatInst)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Material Instance Dynamic for Survivor"));
		return nullptr;
	}

	// Set Survivor Exosuit default parameters
	MatInst->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.45f, 0.52f, 0.58f, 1.0f)); // Steel blue-gray
	MatInst->SetScalarParameterValue(FName("Roughness"), 0.35f); // Semi-gloss armor
	MatInst->SetScalarParameterValue(FName("Metallic"), 0.85f); // High metal content
	MatInst->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f)); // No glow by default
	MatInst->SetScalarParameterValue(FName("EmissiveIntensity"), 0.0f);

	UE_LOG(LogTemp, Display, TEXT("Created Survivor Exosuit MI from %s"), *ParentMaterialPath);
	return MatInst;
}

UMaterialInstanceDynamic* UAstrawildMaterialBuilder::CreateEchoMI(UObject* WorldContext, const FString& SpeciesName, const FString& ParentMaterialPath)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World || !World->IsGameWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid world context for Echo MI creation"));
		return nullptr;
	}

	UObject* ParentMat = StaticLoadObject(UMaterial::StaticClass(), nullptr, *ParentMaterialPath);
	if (!ParentMat)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load parent material: %s"), *ParentMaterialPath);
		return nullptr;
	}

	UMaterialInstanceDynamic* MatInst = UMaterialInstanceDynamic::Create(Cast<UMaterial>(ParentMat), World);
	if (!MatInst)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Material Instance Dynamic for Echo: %s"), *SpeciesName);
		return nullptr;
	}

	// Get species data
	const FAstrawildEchoSpecies* SpeciesData = FAstrawildBestiaryData::FindSpeciesByName(SpeciesName);
	if (!SpeciesData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Species not found: %s - using defaults"), *SpeciesName);
		ApplyDefaultEchoParameters(MatInst);
		return MatInst;
	}

	// Apply species-specific colors
	FLinearColor BaseColor;
	BaseColor.R = SpeciesData->PrimaryColor.R / 255.0f;
	BaseColor.G = SpeciesData->PrimaryColor.G / 255.0f;
	BaseColor.B = SpeciesData->PrimaryColor.B / 255.0f;
	BaseColor.A = 1.0f;

	FLinearColor SecondaryColor;
	SecondaryColor.R = SpeciesData->SecondaryColor.R / 255.0f;
	SecondaryColor.G = SpeciesData->SecondaryColor.G / 255.0f;
	SecondaryColor.B = SpeciesData->SecondaryColor.B / 255.0f;
	SecondaryColor.A = 1.0f;

	MatInst->SetVectorParameterValue(FName("BaseColor"), BaseColor);
	MatInst->SetVectorParameterValue(FName("SecondaryColor"), SecondaryColor);

	// Apply roughness/metallic based on element
	float Roughness = 0.5f;
	float Metallic = 0.1f;
	GetElementalData(SpeciesData->Element, BaseColor, SecondaryColor, Roughness, Metallic);
	MatInst->SetScalarParameterValue(FName("Roughness"), Roughness);
	MatInst->SetScalarParameterValue(FName("Metallic"), Metallic);

	// Apply elemental glow
	FLinearColor EmissiveColor;
	float EmissiveIntensity = 0.0f;
	if (SpeciesData->Element != EAstrawildElement::None)
	{
		GetElementalData(SpeciesData->Element, BaseColor, EmissiveColor, Roughness, Metallic);
		EmissiveIntensity = 0.8f; // Default glow intensity for elemental echoes
		MatInst->SetVectorParameterValue(FName("EmissiveColor"), EmissiveColor);
		MatInst->SetScalarParameterValue(FName("EmissiveIntensity"), EmissiveIntensity);
	}
	else
	{
		MatInst->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor::Black);
		MatInst->SetScalarParameterValue(FName("EmissiveIntensity"), 0.0f);
	}

	UE_LOG(LogTemp, Display, TEXT("Created Echo MI for %s (Element: %s, Glow: %.2f)"), 
		*SpeciesName, *UEnum::GetValueAsString(SpeciesData->Element), EmissiveIntensity);

	return MatInst;
}

void UAstrawildMaterialBuilder::ApplyElementalParameters(UMaterialInstanceDynamic* MatInst, FName Element)
{
	if (!MatInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Material Instance for elemental application"));
		return;
	}

	FLinearColor BaseColor, EmissiveColor;
	float Roughness, Metallic;
	GetElementalData(Element, BaseColor, EmissiveColor, Roughness, Metallic);

	MatInst->SetScalarParameterValue(FName("Roughness"), Roughness);
	MatInst->SetScalarParameterValue(FName("Metallic"), Metallic);
	MatInst->SetVectorParameterValue(FName("EmissiveColor"), EmissiveColor);
	
	float EmissiveIntensity = (Element != NAME_None) ? 0.8f : 0.0f;
	MatInst->SetScalarParameterValue(FName("EmissiveIntensity"), EmissiveIntensity);

	UE_LOG(LogTemp, Display, TEXT("Applied elemental parameters: %s (Glow: %.2f)"), *Element.ToString(), EmissiveIntensity);
}

void UAstrawildMaterialBuilder::BuildInitialMaterialSet(UObject* WorldContext, const FString& ParentMaterialPath)
{
	UE_LOG(LogTemp, Display, TEXT("Building initial material set from %s"), *ParentMaterialPath);

	// Create Survivor Exosuit
	CreateSurvivorExosuitMI(WorldContext, ParentMaterialPath);

	// Create MIs for first 3 Echoes: Bastionbeetle, Terraquill, Cindermule
	TArray<FString> InitialEchoes = { TEXT("Bastionbeetle"), TEXT("Terraquill"), TEXT("Cindermule") };
	
	for (const FString& EchoName : InitialEchoes)
	{
		CreateEchoMI(WorldContext, EchoName, ParentMaterialPath);
	}

	UE_LOG(LogTemp, Display, TEXT("Initial material set complete: Survivor + 3 Echoes"));
}

void UAstrawildMaterialBuilder::GetElementalData(FName Element, FLinearColor& BaseColor, FLinearColor& Emissive, float& Roughness, float& Metallic)
{
	// Default values
	Roughness = 0.5f;
	Metallic = 0.1f;
	Emissive = FLinearColor::Black;

	// Elemental overrides
	if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Ember))
	{
		Emissive = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f); // Orange-red glow
		Roughness = 0.4f;
		Metallic = 0.2f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Frost))
	{
		Emissive = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f); // Cyan-blue glow
		Roughness = 0.3f; // Icy smooth
		Metallic = 0.0f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Pulse))
	{
		Emissive = FLinearColor(0.8f, 0.0f, 1.0f, 1.0f); // Purple glow
		Roughness = 0.5f;
		Metallic = 0.3f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Toxin))
	{
		Emissive = FLinearColor(0.3f, 1.0f, 0.2f, 1.0f); // Green glow
		Roughness = 0.6f;
		Metallic = 0.1f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Volt))
	{
		Emissive = FLinearColor(1.0f, 0.9f, 0.1f, 1.0f); // Yellow glow
		Roughness = 0.4f;
		Metallic = 0.5f; // Conductive
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Solar))
	{
		Emissive = FLinearColor(1.0f, 0.8f, 0.4f, 1.0f); // Golden glow
		Roughness = 0.3f;
		Metallic = 0.4f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Lunar))
	{
		Emissive = FLinearColor(0.7f, 0.8f, 1.0f, 1.0f); // Pale blue glow
		Roughness = 0.5f;
		Metallic = 0.2f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Abyss))
	{
		Emissive = FLinearColor(0.5f, 0.0f, 0.5f, 1.0f); // Deep purple glow
		Roughness = 0.7f;
		Metallic = 0.1f;
	}
	else if (Element == GET_ENUMERATOR_NAME(EAstrawildElement, Chrono))
	{
		Emissive = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f); // Bright cyan glow
		Roughness = 0.2f; // Very smooth (time-worn)
		Metallic = 0.6f;
	}
}

void UAstrawildMaterialBuilder::ApplyDefaultEchoParameters(UMaterialInstanceDynamic* MatInst)
{
	if (!MatInst) return;

	MatInst->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
	MatInst->SetScalarParameterValue(FName("Roughness"), 0.5f);
	MatInst->SetScalarParameterValue(FName("Metallic"), 0.1f);
	MatInst->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor::Black);
	MatInst->SetScalarParameterValue(FName("EmissiveIntensity"), 0.0f);
}
