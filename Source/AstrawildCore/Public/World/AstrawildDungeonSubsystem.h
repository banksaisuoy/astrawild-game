#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AstrawildDungeonData.h"
#include "AstrawildDungeonSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDungeonStartedSignature, FName, DungeonId, AActor*, Starter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonCompletedSignature, FName, DungeonId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDungeonFailedSignature, FName, DungeonId, const FText&, FailureReason);

UCLASS()
class ASTRAWILDCORE_API UAstrawildDungeonSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UDataTable> DungeonTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    FName ActiveDungeonId = NAME_None;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Dungeon")
    TSet<TWeakObjectPtr<AActor>> Participants;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon|Events")
    FOnDungeonStartedSignature OnDungeonStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon|Events")
    FOnDungeonCompletedSignature OnDungeonCompleted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon|Events")
    FOnDungeonFailedSignature OnDungeonFailed;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    bool StartDungeon(FName DungeonId, AActor* Starter);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    bool JoinActiveDungeon(AActor* Participant);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    bool RegisterBossDefeated(FName DungeonId, AActor* DefeatedBoss);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void StopDungeon();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    bool IsDungeonActive() const { return !ActiveDungeonId.IsNone(); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    float GetRemainingTimeSeconds() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    int32 GetParticipantCount() const;

private:
    double ActiveDungeonStartTimeSeconds = 0.0;
    const FAstrawildDungeonRow* FindDungeonRow(FName DungeonId) const;
    bool CheckRequiredKey(const FAstrawildDungeonRow& Row, AActor* Starter) const;
};
