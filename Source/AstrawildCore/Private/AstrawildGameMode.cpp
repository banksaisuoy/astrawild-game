#include "AstrawildGameMode.h"

#include "AstrawildCheatManager.h"
#include "AstrawildCore.h"
#include "AstrawildGameState.h"
#include "AstrawildEchoRosterSubsystem.h" // PCR-2: PostLogin roster mirror push
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildWorldBootstrapper.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h" // LCP-6: HasOption
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

void AAstrawildGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);

    // LCP-6: the LAN host travel URL carries "autoload" so the rehosted listen
    // world continues from the save written before the ServerTravel (the H-3
    // "continue game" machinery — normally off for PIE iteration).
    if (UGameplayStatics::HasOption(Options, TEXT("autoload")))
    {
        bAutoLoadLatestOnBeginPlay = true;
        UE_LOG(LogAstrawildNetwork, Log, TEXT("LCP-6: autoload requested via travel options."));
    }
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
            const bool bLoaded = SaveSubsystem->LoadLatest(World);
            // CGC-4 (save UX trinity): a fresh machine (no slots at all) stays
            // silent — nothing was lost. But a slot EXISTING and failing to
            // read (corrupt/foreign) deserves an honest line: queue it for the
            // first player controller (flushed once in PostLogin — BeginPlay
            // has no players to notify yet).
            if (!bLoaded && (SaveSubsystem->DoesSaveExist(TEXT("ASTRAWILD_Main"))
                || SaveSubsystem->DoesSaveExist(TEXT("ASTRAWILD_Auto"))))
            {
                PendingBootNotice = FText::FromString(
                    TEXT("Your expedition record could not be read — a fresh expedition of the Vale begins."));
                UE_LOG(LogAstrawild, Warning, TEXT("Boot load failed with slots present — fresh world, notice queued."));
            }
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
        // CGC-4 (save UX trinity): the autosave was completely silent — a
        // player had no confirmation the 5-minute timer was doing anything.
        // Toast every connected player through the PC iterator (the
        // EchoCharacter cry-radius idiom); NotifyPlayer routes to the owning
        // screen on every machine (host-local or remote client RPC).
        if (SaveSubsystem->SaveWorld(World, TEXT("ASTRAWILD_Auto")))
        {
            const FText Notice = FText::FromString(TEXT("Autosaved."));
            for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
            {
                if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(It->Get()))
                {
                    PC->NotifyPlayer(Notice);
                }
            }
        }
    }
}


void AAstrawildGameMode::PostLogin(AController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // CGC-4 (save UX trinity): flush the queued boot load-failure notice to
    // the FIRST player controller (the host in every shipped configuration) —
    // once, regardless of who joined first or how many logins follow.
    if (HasAuthority() && !PendingBootNotice.IsEmpty() && !bBootNoticeDelivered)
    {
        if (AAstrawildPlayerController* NoticePC = Cast<AAstrawildPlayerController>(NewPlayer))
        {
            NoticePC->NotifyPlayer(PendingBootNotice, EAstrawildToastCue::Warning);
            bBootNoticeDelivered = true;
        }
    }

    // LCP-4: late join / reconnect restore. Runs on the LISTEN HOST only (the
    // GameMode exists solely on the server). The first player (host) restored
    // through LoadWorld's legacy block; everyone else restores here.
    if (!HasAuthority())
    {
        return;
    }
    APlayerController* PC = Cast<APlayerController>(NewPlayer);
    if (!PC || PC == GetWorld()->GetFirstPlayerController())
    {
        return; // host + non-player controllers ( spectator/dev ) skip
    }
    UWorld* World = GetWorld();
    if (!World || !World->GetGameInstance())
    {
        return;
    }
    if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
    {
        SaveSubsystem->TryRestoreLateJoinPlayer(PC);
    }

    // PCR-2: the joining player's roster mirror starts empty — push their
    // slice now so the roster screen is live from the first frame (later
    // updates ride the mutation-time pushes).
    if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
    {
        Roster->PushRosterMirrors();
    }
}

void AAstrawildGameMode::Logout(AController* Exiting)
{
    // LCP-4: snapshot before the controller + pawn go away so a reconnect
    // within the session restores exactly (inventory/roster/quests/position).
    if (HasAuthority())
    {
        if (APlayerController* PC = Cast<APlayerController>(Exiting))
        {
            if (UWorld* World = GetWorld(); World && World->GetGameInstance())
            {
                if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
                {
                    SaveSubsystem->SnapshotPlayerForSession(PC);
                }
            }
        }
    }
    Super::Logout(Exiting);
}
