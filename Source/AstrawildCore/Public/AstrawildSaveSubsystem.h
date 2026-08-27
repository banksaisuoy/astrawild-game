#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "AstrawildTypes.h"
#include "AstrawildSaveSubsystem.generated.h"

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 SaveSchemaVersion = 1;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FDateTime SavedAtUtc;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildItemStack> PlayerInventory;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildEchoInstanceSaveData> EchoRoster;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildRestPointSaveData> RestPoints;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid ActiveRestPointId;
};

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool SaveSnapshot(const TArray<FAstrawildItemStack>& Inventory, const TArray<FAstrawildEchoInstanceSaveData>& EchoRoster, const TArray<FAstrawildRestPointSaveData>& RestPoints, const FGuid& ActiveRestPointId, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool LoadSnapshot(TArray<FAstrawildItemStack>& OutInventory, TArray<FAstrawildEchoInstanceSaveData>& OutEchoRoster, TArray<FAstrawildRestPointSaveData>& OutRestPoints, FGuid& OutActiveRestPointId, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Save")
    bool DoesSaveExist(const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool DeleteSave(const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Save")
    int32 GetCurrentSchemaVersion() const { return CurrentSchemaVersion; }

private:
    static constexpr int32 CurrentSchemaVersion = 1;
};
