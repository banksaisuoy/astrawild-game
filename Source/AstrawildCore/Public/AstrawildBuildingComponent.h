#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildBuildingComponent.generated.h"

class AAstrawildBuildingActor;
class UAstrawildBuildingDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildBuildingPlaced, FName, DefinitionId, AAstrawildBuildingActor*, Building);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildPlacementModeChanged, bool, bIsPlacing);

/**
 * Player-side base building placement (directive §16): placement preview, grid snap,
 * rotation, collision validation and server-authoritative placement. Consumes the
 * required item from the player inventory.
 *
 * Controls (runtime defaults): B = toggle placement mode / cycle piece, N = rotate,
 * LMB = confirm (while placing).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildBuildingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildBuildingComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Building")
    FAstrawildBuildingPlaced OnBuildingPlaced;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Building")
    FAstrawildPlacementModeChanged OnPlacementModeChanged;

    /** Placement reach in cm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Building", meta=(ClampMin="100.0"))
    float PlacementReach = 600.0f;

    /** Grid snap size (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Building", meta=(ClampMin="50.0"))
    float SnapGridSize = 200.0f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void TogglePlacementMode();

    /** Cycle to the next unlocked building definition. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void CycleBuildingDefinition();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void RotatePreview(float Degrees);

    /** Confirm placement at the current preview location (called from attack input). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void ConfirmPlacement();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void CancelPlacement();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    bool IsPlacing() const { return bPlacementMode && PreviewActor != nullptr; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    FName GetCurrentDefinitionId() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    bool IsPlacementValid() const { return bPlacementValid; }

protected:
    virtual void BeginPlay() override;

private:
    bool bPlacementMode = false;
    bool bPlacementValid = false;
    int32 CurrentDefinitionIndex = 0;
    float PreviewYaw = 0.0f;

    UPROPERTY()
    TObjectPtr<AAstrawildBuildingActor> PreviewActor;

    TArray<FName> CachedUnlockedIds;

    void RebuildUnlockedList();
    void UpdatePreview();
    bool ValidatePlacementLocation(const FVector& Location, float GridSize) const;
    FVector ComputeSnappedLocation() const;
    class AAstrawildPlayerCharacter* GetPlayer() const;
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class UAstrawildResearchSubsystem* GetResearch() const;

    UFUNCTION(Server, Reliable)
    void ServerPlaceBuilding(FName DefinitionId, FVector_NetQuantize Location, float Yaw);
};
