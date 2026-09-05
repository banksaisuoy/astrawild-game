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

    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override; // LCP-6
    virtual void BeginPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

    /**
     * LCP-4 (LAN co-op): a joining player restores their per-player block
     * (session cache first, then the latest save) — late join after world
     * progression and mid-session reconnects both land here. Host-authoritative:
     * only the server-side GameMode runs this.
     */
    virtual void PostLogin(AController* NewPlayer) override;

    /** LCP-4: snapshot the leaving player's block into the session cache (reconnect source). */
    virtual void Logout(AController* Exiting) override;

protected:
    void HandleAutosave();

private:
    FTimerHandle AutosaveTimerHandle;

    void RespawnPlayer(AController* Controller);

    // Audit C-1 (final run): TObjectPtr so GC-tracked and .Get() is valid (was a raw pointer
    // used with .Get() — a hard compile error).
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Runtime")
    TObjectPtr<class AAstrawildWorldBootstrapper> Bootstrapper = nullptr;
};
