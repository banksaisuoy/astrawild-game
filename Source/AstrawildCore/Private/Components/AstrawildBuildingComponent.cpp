// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildBuildingComponent.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Components/AstrawildInventoryComponent.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

UAstrawildBuildingComponent::UAstrawildBuildingComponent()
	: bIsBuildModeActive(false)
	, MaxPlacementDistance(800.0f)
	, bEnableGridSnap(true)
	, GridSnapSize(100.0f)
	, CurrentGhostLocation(FVector::ZeroVector)
	, CurrentGhostRotation(FRotator::ZeroRotator)
	, bIsValidPlacementLocation(false)
	, PreviewRotationYaw(0.0f)
	, bIsPlacingPiece(false)
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

	// Draw ghost preview box
	UWorld* World = GetWorld();
	if (World)
	{
		const FColor PreviewColor = bIsValidPlacementLocation ? FColor(46, 204, 113, 160) : FColor(231, 76, 60, 160);
		DrawDebugBox(World, CurrentGhostLocation + FVector(0, 0, 50), FVector(50, 50, 50), CurrentGhostRotation.Quaternion(), PreviewColor, false, -1.0f, 0, 2.0f);
	}
}

void UAstrawildBuildingComponent::EnterBuildMode(TSubclassOf<AAstrawildBuildingPiece> InBuildingClass, const FGameplayTag& InBuildingTag, const TArray<FAstrawildRecipeIngredient>& Cost)
{
	ActiveBuildingClass = InBuildingClass ? InBuildingClass : TSubclassOf<AAstrawildBuildingPiece>(AAstrawildBuildingPiece::StaticClass());
	ActiveBuildingTag = InBuildingTag;
	ActiveBuildingCost = Cost;
	bIsBuildModeActive = true;
	PreviewRotationYaw = 0.0f;
	bIsPlacingPiece = false;

	UE_LOG(LogAstrawild, Log, TEXT("Entered Build Mode for: %s"), *InBuildingTag.ToString());
}

void UAstrawildBuildingComponent::ExitBuildMode()
{
	bIsBuildModeActive = false;
	ActiveBuildingClass = nullptr;
	ActiveBuildingTag = FGameplayTag::EmptyTag;
	ActiveBuildingCost.Empty();
	bIsPlacingPiece = false;

	UE_LOG(LogAstrawild, Log, TEXT("Exited Build Mode."));
}

void UAstrawildBuildingComponent::RotatePreview(float Degrees)
{
	PreviewRotationYaw = FMath::Fmod(PreviewRotationYaw + Degrees, 360.0f);
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

	if (bHit)
	{
		// Slope check: normal Z must be > 0.70 (slope < ~45 deg)
		if (HitResult.ImpactNormal.Z < 0.70f)
		{
			const_cast<UAstrawildBuildingComponent*>(this)->LastPlacementErrorMessage = FText::FromString(TEXT("Cannot place on steep slope!"));
			return false;
		}

		FVector PlacedLocation = HitResult.ImpactPoint;

		// Grid Snapping
		if (bEnableGridSnap && GridSnapSize > 0.0f)
		{
			PlacedLocation.X = FMath::GridSnap(PlacedLocation.X, GridSnapSize);
			PlacedLocation.Y = FMath::GridSnap(PlacedLocation.Y, GridSnapSize);
		}

		FRotator FinalRotation = FRotator(0.0f, CameraRotation.Yaw + PreviewRotationYaw, 0.0f);
		if (bEnableGridSnap)
		{
			FinalRotation.Yaw = FMath::GridSnap(FinalRotation.Yaw, 45.0f);
		}

		// Collision overlap test for clear space
		FCollisionQueryParams OverlapParams;
		OverlapParams.AddIgnoredActor(OwnerActor);
		const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
			PlacedLocation + FVector(0, 0, 50),
			FinalRotation.Quaternion(),
			ECC_WorldDynamic,
			FCollisionShape::MakeBox(FVector(45, 45, 45)),
			OverlapParams
		);

		if (bBlocked)
		{
			const_cast<UAstrawildBuildingComponent*>(this)->LastPlacementErrorMessage = FText::FromString(TEXT("Space is blocked by another object!"));
			return false;
		}

		OutTransform.SetLocation(PlacedLocation);
		OutTransform.SetRotation(FinalRotation.Quaternion());
		OutTransform.SetScale3D(FVector::OneVector);
		return true;
	}

	const_cast<UAstrawildBuildingComponent*>(this)->LastPlacementErrorMessage = FText::FromString(TEXT("Target is out of range!"));
	return false;
}

bool UAstrawildBuildingComponent::PlaceBuilding()
{
	if (!bIsBuildModeActive || !ActiveBuildingClass || !bIsValidPlacementLocation || bIsPlacingPiece)
	{
		return false;
	}

	if (!CanAffordBuilding())
	{
		LastPlacementErrorMessage = FText::FromString(TEXT("Cannot place: Insufficient building materials!"));
		OnBuildingFailed.Broadcast(LastPlacementErrorMessage);
		UE_LOG(LogAstrawild, Warning, TEXT("Cannot place building: Insufficient resources."));
		return false;
	}

	bIsPlacingPiece = true; // Lock against double clicks / double spend

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
		PlacedPiece->DismantleRefund = ActiveBuildingCost;

		if (ActiveBuildingTag.ToString().Contains(TEXT("Campfire")))
		{
			PlacedPiece->BuildingType = EAstrawildBuildingType::Campfire;
		}
		else if (ActiveBuildingTag.ToString().Contains(TEXT("Bed")))
		{
			PlacedPiece->BuildingType = EAstrawildBuildingType::RestBed;
		}
		else if (ActiveBuildingTag.ToString().Contains(TEXT("CraftingBench")))
		{
			PlacedPiece->BuildingType = EAstrawildBuildingType::CraftingBench;
		}

		UE_LOG(LogAstrawild, Log, TEXT("Successfully constructed %s at %s"), *ActiveBuildingTag.ToString(), *CurrentGhostLocation.ToString());
		OnBuildingPlaced.Broadcast(PlacedPiece);
		bIsPlacingPiece = false;
		return true;
	}

	bIsPlacingPiece = false;
	return false;
}