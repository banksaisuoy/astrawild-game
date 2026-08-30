#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildVillageActor.generated.h"

class AStaticMeshActor;

/**
 * Batch 8 — living villages (Docs/ASTRAWILD_VILLAGES_SKIFF.md).
 *
 * A village is: a procedural hamlet (hut ring + palisade + campfire + lamp
 * posts — engine basic shapes, zero assets) plus the waypoint circuit its NPCs
 * patrol. The WorldBootstrapper spawns one AAstrawildVillageActor per
 * settlement, spawns the NPC roster around it and links each NPC through
 * AAstrawildNPCCharacter::SetHomeVillage.
 *
 * Waypoints: a deterministic ring of posts just outside the hut circle; the
 * campfire is the night gather point (see AAstrawildNPCAIController).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildVillageActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildVillageActor();

    /** Stable settlement id (Village_Dawnstead / Village_DriftwoodLanding). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Village")
    FName VillageId = NAME_None;

    /** Display name used in HUD toasts. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Village")
    FText VillageName;

    /** Huts in the ring (visual density). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Village", meta=(ClampMin="1", ClampMax="12"))
    int32 HutCount = 6;

    /** Ring radius (cm). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Village", meta=(ClampMin="1000.0"))
    float VillageRadius = 1600.0f;

    /** True → dock planks instead of a palisade (fishing hamlet flavour). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Village")
    bool bCoastal = false;

    /** Night gather point (the campfire). */
    FVector GetCampfireLocation() const { return CampfireLocation; }

    int32 GetWaypointCount() const { return Waypoints.Num(); }

    /** Deterministic waypoint access; index wraps modulo count. */
    FVector GetWaypoint(const int32 Index) const;

    virtual void BeginPlay() override;

protected:
    /** Server: builds the hut ring, campfire, lamp posts and waypoints. */
    void BuildVillage();

private:
    TArray<FVector> Waypoints;
    FVector CampfireLocation = FVector::ZeroVector;

    AStaticMeshActor* SpawnShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, const FRotator& Rotation);
    void SpawnVillageLight(const FVector& Location, const FLinearColor& Color, const float Intensity);
};
