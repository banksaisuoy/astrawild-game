// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildBuildingComponent.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Components/AstrawildInventoryComponent.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"

UAstrawildBuildingComponent::UAstrawildBuildingComponent()
	: bIsBuildModeActive(false)
	, MaxPlacementDistance(800.0f)
	, CurrentGhostLocation(FVector::ZeroVector)
	, CurrentGhostRotation(FRotator::ZeroRotator)
	, bIsValidPlacementLocation(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildBuildingComponent::BeginPlay()
{
	Super::BeginPlay();
	GetInventory();
}

UAstrawildInventoryComponent* UAstrawildBuildingComponent::GetInventory()
{
	if (!CachedInventory.IsValid() && GetOwner())
	{
		CachedInventory = GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>();
	}
	return CachedInventory.Get();
}

void UAstrawildBuildingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsBuildModeActive)
	{
		return;
	}

	FTransform PreviewTransform;
	bIsValidPlacementLocation = GetPlacementTransform(PreviewTransform);
	CurrentGhostLocation = PreviewTransform.GetLocation();
	CurrentGhostRotation = PreviewTransform.GetRotation().Rotator();
}

void UAstrawildBuildingComponent::EnterBuildMode(TSubclassOf<AAstrawildBuildingPiece> InBuildingClass, const FGameplayTag& InBuildingTag, const TArray<FAstrawildRecipeIngredient>& Cost)
{
	ActiveBuildingClass = InBuildingClass;
	ActiveBuildingTag = InBuildingTag;
	ActiveBuildingCost = Cost;
	bIsBuildModeActive = true;

	UE_LOG(LogAstrawild, Log, TEXT("Entered Build Mode for: %s"), *InBuildingTag.ToString());
}

void UAstrawildBuildingComponent::ExitBuildMode()
{
	bIsBuildModeActive = false;
	ActiveBuildingClass = nullptr;
	ActiveBuildingTag = FGameplayTag::EmptyTag;
	ActiveBuildingCost.Empty();

	UE_LOG(LogAstrawild, Log, TEXT("Exited Build Mode."));
}

bool UAstrawildBuildingComponent::CanAffordBuilding() const
{
	UAstrawildInventoryComponent* Inv = const_cast<UAstrawildBuildingComponent*>(this)->GetInventory();
	if (!Inv)
	{
		return false;
	}

	for (const FAstrawildRecipeIngredient& Ingredient : ActiveBuildingCost)
	{
		if (!Inv->HasItem(Ingredient.ItemTag, Ingredient.Quantity))
		{
			return false;
		}
	}

	return true;
}

bool UAstrawildBuildingComponent::GetPlacementTransform(FTransform& OutTransform) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !GetWorld())
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;

	ACharacter* CharOwner = Cast<ACharacter>(OwnerActor);
	if (CharOwner && CharOwner->GetController())
	{
		CharOwner->GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);
	}
	else
	{
		CameraLocation = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
		CameraRotation = OwnerActor->GetActorRotation();
	}

	const FVector TraceStart = CameraLocation;
	const FVector TraceEnd = TraceStart + (CameraRotation.Vector() * MaxPlacementDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

	if (bHit && HitResult.ImpactNormal.Z > 0.6f) // Ensure walkable/flat enough surface
	{
		OutTransform.SetLocation(HitResult.ImpactPoint);
		OutTransform.SetRotation(FRotator(0.0f, CameraRotation.Yaw, 0.0f).Quaternion());
		OutTransform.SetScale3D(FVector::OneVector);
		return true;
	}

	// Fallback to in front of player
	const FVector FallbackLoc = OwnerActor->GetActorLocation() + (OwnerActor->GetActorForwardVector() * 300.0f);
	OutTransform.SetLocation(FallbackLoc);
	OutTransform.SetRotation(OwnerActor->GetActorRotation().Quaternion());
	OutTransform.SetScale3D(FVector::OneVector);
	return false;
}

bool UAstrawildBuildingComponent::PlaceBuilding()
{
	if (!bIsBuildModeActive || !ActiveBuildingClass || !bIsValidPlacementLocation)
	{
		return false;
	}

	if (!CanAffordBuilding())
	{
		UE_LOG(LogAstrawild, Warning, TEXT("Cannot place building: Insufficient resources."));
		return false;
	}

	UAstrawildInventoryComponent* Inv = GetInventory();
	if (Inv)
	{
		for (const FAstrawildRecipeIngredient& Ingredient : ActiveBuildingCost)
		{
			Inv->RemoveItem(Ingredient.ItemTag, Ingredient.Quantity);
		}
	}

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(CurrentGhostLocation);
	SpawnTransform.SetRotation(CurrentGhostRotation.Quaternion());
	SpawnTransform.SetScale3D(FVector::OneVector);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAstrawildBuildingPiece* PlacedPiece = GetWorld()->SpawnActor<AAstrawildBuildingPiece>(ActiveBuildingClass, SpawnTransform, SpawnParams);
	if (PlacedPiece)
	{
		PlacedPiece->BuildingTag = ActiveBuildingTag;
		UE_LOG(LogAstrawild, Log, TEXT("Successfully placed building piece: %s at %s"), *ActiveBuildingTag.ToString(), *CurrentGhostLocation.ToString());
		OnBuildingPlaced.Broadcast(PlacedPiece);
		return true;
	}

	return false;
}