#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildGuildData.h"
#include "AstrawildGuildSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildTerritoryCapturedSignature, FGameplayTag, GuildTag, FGameplayTag, TotemTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAstrawildGuildBuffChangedSignature, FGameplayTag, GuildTag, FGameplayTag, BuffTag, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildArenaMatchStartedSignature, FGameplayTag, ArenaTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildArenaMatchFinishedSignature, FGameplayTag, ArenaTag, int32, WinningTeamIndex);

UCLASS()
class ASTRAWILDCORE_API UAstrawildGuildSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildGuildSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Guild|Rules", meta=(ClampMin="0.01"))
    float ArenaTickIntervalSeconds = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Guild|Rules", meta=(ClampMin="1"))
    int32 ArenaTeamSize = 4;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Guild|State")
    FGameplayTag ActiveArenaTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Guild|State")
    FAstrawildGuildArenaMatchState ArenaMatchState;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Guild|Events")
    FOnAstrawildTerritoryCapturedSignature OnTerritoryCaptured;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Guild|Events")
    FOnAstrawildGuildBuffChangedSignature OnGuildBuffChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Guild|Events")
    FOnAstrawildArenaMatchStartedSignature OnArenaMatchStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Guild|Events")
    FOnAstrawildArenaMatchFinishedSignature OnArenaMatchFinished;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Authority")
    bool RegisterGuild(const FGameplayTag& GuildTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Authority")
    bool RegisterBuffNode(const FAstrawildGuildBuffNode& BuffNode);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Authority")
    bool SetGuildBuffLevel(const FGameplayTag& GuildTag, const FGameplayTag& BuffTag, int32 NewLevel);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Guild")
    int32 GetGuildBuffLevel(const FGameplayTag& GuildTag, const FGameplayTag& BuffTag) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Authority")
    bool RegisterTerritoryTotem(const FAstrawildGuildTerritory& Territory);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Authority")
    bool CaptureTerritory(const FGameplayTag& GuildTag, const FGameplayTag& TotemTag, AActor* Instigator);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Guild")
    bool GetTerritory(const FGameplayTag& TotemTag, FAstrawildGuildTerritory& OutTerritory) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Arena")
    bool RegisterArenaTeam(const FGameplayTag& TeamTag, const TArray<AActor*>& Members);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Arena")
    bool StartArenaMatch(const FGameplayTag& ArenaTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Arena")
    bool AddArenaScore(const FGameplayTag& TeamTag, int32 ScoreDelta);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild|Arena")
    void EndArenaMatch(int32 WinningTeamIndex = -1);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Guild|Arena")
    bool GetArenaTeamState(const FGameplayTag& TeamTag, FAstrawildGuildArenaTeam& OutTeam) const;

private:
    TSet<FGameplayTag> RegisteredGuilds;
    TMap<FGameplayTag, FAstrawildGuildBuffNode> BuffNodes;
    TMap<FGameplayTag, TMap<FGameplayTag, int32>> GuildBuffLevels;
    TMap<FGameplayTag, FAstrawildGuildTerritory> Territories;
    TMap<FGameplayTag, FAstrawildGuildArenaTeam> ArenaTeams;
    FTimerHandle ArenaTimerHandle;

    bool HasAuthorityForGuild() const;
    void HandleArenaTick();
    void RemoveInvalidArenaMembers();
};
