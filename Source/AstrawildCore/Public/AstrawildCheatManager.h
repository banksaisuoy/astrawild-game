#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "AstrawildCheatManager.generated.h"

/**
 * Developer/debug tools (directive §41): console cheats for testing every system.
 * Active only in non-shipping builds (engine strips CheatManager by default in Shipping).
 * Console examples: `AW.SpawnEcho Echo_Lumewisp`, `AW.GiveItem Item_Wood 50`, `AW.SetTime 22 0`.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildCheatManager : public UCheatManager
{
    GENERATED_BODY()

public:
    UFUNCTION(Exec)
    void SpawnEcho(FName EchoDefinitionId);

    UFUNCTION(Exec)
    void GiveItem(FName ItemId, int32 Quantity);

    /** Batch 4 — M-11: buy a ware from the nearest vendor NPC (within 6 m) in its
     *  currency, e.g. `AW.BuyItem Item_Bandage 2`. Routes through the same
     *  server-authoritative API a future shop UMG screen will use. */
    UFUNCTION(Exec)
    void BuyItem(FName ItemId, int32 Quantity);

    /** Batch 4 — M-11: sell a priced item to the nearest vendor NPC for half its
     *  buy price (floor 1), e.g. `AW.SellItem Item_Berry 5`. */
    UFUNCTION(Exec)
    void SellItem(FName ItemId, int32 Quantity);

    /** Wave 3: equip an owned equipment item by id (weapon or shield routing). */
    UFUNCTION(Exec)
    void EquipItem(FName ItemId);

    UFUNCTION(Exec)
    void SetTime(int32 Hour, int32 Minute);

    UFUNCTION(Exec)
    void SetWeather(FName WeatherName);

    UFUNCTION(Exec)
    void God();

    UFUNCTION(Exec)
    void HealAll();

    UFUNCTION(Exec)
    void ResearchPoints(int32 Amount);

    UFUNCTION(Exec)
    void UnlockTech(FName TechId);

    UFUNCTION(Exec)
    void SaveNow();

    UFUNCTION(Exec)
    void LoadNow();

    UFUNCTION(Exec)
    void CaptureAll();

    UFUNCTION(Exec)
    void TeleportForward(float Distance);

private:
    class AAstrawildPlayerCharacter* GetPlayer() const;
};
