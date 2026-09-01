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
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Bootstrapper = World->SpawnActor<AAstrawildWorldBootstrapper>(
            AAstrawildWorldBootstrapper::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

        // Immediate PlayerStart anchor so frame-0 pawn possession places player safely on ground
        World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 150.0f), FRotator::ZeroRotator, Params);
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
        if (AAstrawildWorldBootstrapper* Bootstrap = Bootstrapper.Get())
        {
            Player->HandleRespawn(FTransform(FVector(0.0f, 0.0f, 150.0f)));
        }
        UE_LOG(LogAstrawildCombat, Log, TEXT("Player respawned."));
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
