#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AstrawildGameMode.generated.h"

/**
 * ASTRAWILD game mode (directive §1/§11/§28): server-authoritative session owner.
 * Spawns the procedural Dawn Fields bootstrapper so the game is playable straight
 * from compile with zero map assets, and drives respawn + autosave.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAstrawildGameMode();

    /** Seconds before a dead player respawns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Rules", meta=(ClampMin="0.5"))
    float RespawnDelaySeconds = 5.0f;

    /** Autosave interval in seconds (0 disables). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Rules", meta=(ClampMin="0.0"))
    float AutosaveIntervalSeconds = 300.0f;

    /**
     * Audit H-3: when true, the session auto-loads the latest save (autosave wins over
     * manual if newer) at BeginPlay — the "continue game" path. Default OFF so PIE
     * iteration starts fresh; enable in a main-menu GameMode subclass later.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Rules")
    bool bAutoLoadLatestOnBeginPlay = false;

    /** Queue a respawn for a controller (server). */
    void RequestPlayerRespawn(AController* Controller, float DelaySeconds);

    virtual void BeginPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    void HandleAutosave();

private:
    FTimerHandle AutosaveTimerHandle;

    void RespawnPlayer(AController* Controller);
    class AAstrawildWorldBootstrapper* Bootstrapper = nullptr;
};
