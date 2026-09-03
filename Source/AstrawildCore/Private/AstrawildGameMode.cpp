#include "AstrawildGameMode.h"

#include "AstrawildCheatManager.h"
#include "AstrawildCore.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildWorldBootstrapper.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildGameMode::AAstrawildGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    // Server-authoritative session classes (directive §28).
    DefaultPawnClass = AAstrawildPlayerCharacter::StaticClass();
    GameStateClass = AAstrawildGameState::StaticClass();
    PlayerControllerClass = AAstrawildPlayerController::StaticClass();
}

void AAstrawildGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    // Procedural world (zero-asset playability, directive §50).
    UWorld* World = GetWorld();
    if (World)
    {
        // Prevent duplicate bootstrapper (reuse existing instance if already in level)
        for (TActorIterator<AAstrawildWorldBootstrapper> It(World); It; ++It)
        {
            Bootstrapper = *It;
            break;
        }
        if (!Bootstrapper)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Bootstrapper = World->SpawnActor<AAstrawildWorldBootstrapper>(
                AAstrawildWorldBootstrapper::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
        }

        // Prevent duplicate PlayerStart anchor
        bool bHasPlayerStart = false;
        for (TActorIterator<APlayerStart> It(World); It; ++It)
        {
            bHasPlayerStart = true;
            break;
        }
        if (!bHasPlayerStart)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 150.0f), FRotator::ZeroRotator, Params);
        }
    }

    // Audit C-2: free root technologies (e.g. BasicCrafting) are granted every session
    // so crafting gates open from the start — previously NO legitimate unlock path existed.
    if (World && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->GrantStartingTechnologies();
        }
    }

    // Audit H-3: optional "continue" path — load the newest save (auto vs manual) on boot.
    if (bAutoLoadLatestOnBeginPlay && World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            SaveSubsystem->LoadLatest(World);
        }
    }

    // Autosave cadence (directive §27).
    if (AutosaveIntervalSeconds > 0.0f && World)
    {
        World->GetTimerManager().SetTimer(AutosaveTimerHandle, this, &AAstrawildGameMode::HandleAutosave, AutosaveIntervalSeconds, true);
    }

    UE_LOG(LogAstrawild, Log, TEXT("ASTRAWILD game mode online (Dawn Fields bootstrapper %s)."), Bootstrapper ? TEXT("spawned") : TEXT("FAILED"));
}

AActor* AAstrawildGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // Prefer a placed PlayerStart; the bootstrapper guarantees a fallback exists.
    if (AActor* Start = Super::ChoosePlayerStart_Implementation(Player))
    {
        return Start;
    }
    return Bootstrapper.Get();
}

void AAstrawildGameMode::RequestPlayerRespawn(AController* Controller, const float DelaySeconds)
{
    UWorld* World = GetWorld();
    if (!World || !Controller)
    {
        return;
    }

    // Audit C-6 (final run): the previous SetTimerForNextTick lambda captured `this` raw;
    // binding through FTimerDelegate::CreateUObject keeps the timer UObject-safe.
    FTimerHandle RespawnHandle;
    World->GetTimerManager().SetTimer(
        RespawnHandle,
        FTimerDelegate::CreateUObject(this, &AAstrawildGameMode::RespawnPlayer, Controller),
        FMath::Max(0.5f, DelaySeconds),
        false);
}

void AAstrawildGameMode::RespawnPlayer(AController* Controller)
{
    UWorld* World = GetWorld();
    if (!World || !Controller)
    {
        return;
    }

    if (APawn* OldPawn = Controller->GetPawn())
    {
        OldPawn->Destroy();
    }

    RestartPlayer(Controller);

    if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(Controller->GetPawn()))
    {
        // Final-audit H-1 (AUD-4): respawn used to hard-teleport to the WORLD ORIGIN
        // (0,0,150) — the 4-zone map corner, far from the camp, with a bogus Z.
        // The camp PlayerStart that RestartPlayer just selected was immediately
        // overridden. Respawn now lands at the camp center on the terrain (the
        // same deterministic point a fresh game uses).
        FVector RespawnLocation(0.0f, 0.0f, 150.0f);
        if (AAstrawildWorldBootstrapper* Bootstrap = Bootstrapper.Get())
        {
            const FVector2D CampXY = AAstrawildWorldBootstrapper::GetCampCenterXY();
            RespawnLocation = FVector(CampXY.X, CampXY.Y, Bootstrap->GroundZ(CampXY) + 120.0f);
        }
        Player->HandleRespawn(FTransform(RespawnLocation));
        UE_LOG(LogAstrawildCombat, Log, TEXT("Player respawned at the camp."));
    }
}

void AAstrawildGameMode::HandleAutosave()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (UAstrawildSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
    {
        SaveSubsystem->SaveWorld(World, TEXT("ASTRAWILD_Auto"));
    }
}
