#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildWorldData.h"
#include "AstrawildWorldPartitionSubsystem.generated.h"

class ACharacter;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpireDiscoveredSignature, FName, SpireId);

UCLASS()
class ASTRAWILDCORE_API UAstrawildWorldPartitionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static constexpr int32 MapSizeCentimeters = 409600;
    static constexpr int32 CellSizeCentimeters = 51200;
    static constexpr int32 CellsPerAxis = 8;
    static constexpr int32 TotalCellCount = CellsPerAxis * CellsPerAxis;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    TObjectPtr<UDataTable> BiomeTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    TObjectPtr<UDataTable> SpawnRuleTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    TObjectPtr<UDataTable> SpireTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World")
    TSet<FName> DiscoveredSpireIds;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World|Events")
    FOnSpireDiscoveredSignature OnSpireDiscovered;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World")
    FAstrawildWorldCell GetCellForLocation(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World")
    bool GetBiomeAtLocation(FVector WorldLocation, FAstrawildBiomeDefinition& OutBiome) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World")
    bool GetBiome(FName BiomeId, FAstrawildBiomeDefinition& OutBiome) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Fast Travel")
    bool DiscoverSpire(FName SpireId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Fast Travel")
    void RefreshDefaultSpireDiscovery();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Fast Travel")
    bool DiscoverSpireForCharacter(FName SpireId, ACharacter* Character);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Fast Travel")
    bool IsSpireDiscovered(FName SpireId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Fast Travel")
    bool GetSpireData(FName SpireId, FAstrawildFastTravelSpire& OutSpire) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Fast Travel")
    bool TryFastTravel(ACharacter* Character, FName DestinationSpireId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World")
    TArray<FAstrawildWorldCell> GetAllWorldCells() const;

private:
    static int32 ClampCellCoordinate(int32 Coordinate);
    static FIntPoint GetCellCoordinates(FVector WorldLocation);
};
