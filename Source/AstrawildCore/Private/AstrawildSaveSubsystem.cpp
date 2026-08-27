#include "AstrawildSaveSubsystem.h"

#include "AstrawildCore.h"
#include "Kismet/GameplayStatics.h"

bool UAstrawildSaveSubsystem::SaveSnapshot(const TArray<FAstrawildItemStack>& Inventory, const TArray<FAstrawildEchoInstanceSaveData>& EchoRoster, const TArray<FAstrawildRestPointSaveData>& RestPoints, const FGuid& ActiveRestPointId, const FString& SlotName, const int32 UserIndex)
{
    if (SlotName.IsEmpty())
    {
        UE_LOG(LogAstrawild, Error, TEXT("SaveSnapshot rejected: SlotName is empty."));
        return false;
    }

    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstrawildSaveGame::StaticClass()));
    if (!IsValid(SaveGame))
    {
        UE_LOG(LogAstrawild, Error, TEXT("SaveSnapshot failed: could not create SaveGame object."));
        return false;
    }

    SaveGame->SaveSchemaVersion = CurrentSchemaVersion;
    SaveGame->SavedAtUtc = FDateTime::UtcNow();
    SaveGame->PlayerInventory = Inventory;
    SaveGame->EchoRoster = EchoRoster;
    SaveGame->RestPoints = RestPoints;
    SaveGame->ActiveRestPointId = ActiveRestPointId;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
    if (!bSaved)
    {
        UE_LOG(LogAstrawild, Error, TEXT("SaveSnapshot failed for slot %s."), *SlotName);
    }
    return bSaved;
}

bool UAstrawildSaveSubsystem::LoadSnapshot(TArray<FAstrawildItemStack>& OutInventory, TArray<FAstrawildEchoInstanceSaveData>& OutEchoRoster, TArray<FAstrawildRestPointSaveData>& OutRestPoints, FGuid& OutActiveRestPointId, const FString& SlotName, const int32 UserIndex)
{
    OutInventory.Reset();
    OutEchoRoster.Reset();
    OutRestPoints.Reset();
    OutActiveRestPointId.Invalidate();

    if (SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return false;
    }

    USaveGame* LoadedObject = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(LoadedObject);
    if (!IsValid(SaveGame))
    {
        UE_LOG(LogAstrawild, Error, TEXT("LoadSnapshot failed: invalid SaveGame object in slot %s."), *SlotName);
        return false;
    }

    if (SaveGame->SaveSchemaVersion <= 0 || SaveGame->SaveSchemaVersion > CurrentSchemaVersion)
    {
        UE_LOG(LogAstrawild, Error, TEXT("LoadSnapshot rejected: unsupported schema version %d in slot %s."), SaveGame->SaveSchemaVersion, *SlotName);
        return false;
    }

    OutInventory = SaveGame->PlayerInventory;
    OutEchoRoster = SaveGame->EchoRoster;
    OutRestPoints = SaveGame->RestPoints;
    OutActiveRestPointId = SaveGame->ActiveRestPointId;
    return true;
}

bool UAstrawildSaveSubsystem::DoesSaveExist(const FString& SlotName, const int32 UserIndex) const
{
    return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UAstrawildSaveSubsystem::DeleteSave(const FString& SlotName, const int32 UserIndex)
{
    if (!DoesSaveExist(SlotName, UserIndex))
    {
        return true;
    }

    const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
    if (!bDeleted)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("DeleteSave failed for slot %s."), *SlotName);
    }
    return bDeleted;
}
