#include "AstrawildEchoBossCharacter.h"

#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildEchoBossCharacter::AAstrawildEchoBossCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(true);

    GetCapsuleComponent()->InitCapsuleSize(80.0f, 160.0f);
    GetCharacterMovement()->MaxWalkSpeed = 380.0f;

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    // Boss silhouette: oversized cone (engine basic shape — REPLACE_BEFORE_RELEASE).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(ConeMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(2.4f, 2.4f, 1.6f));
        PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    }
}

void AAstrawildEchoBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, CurrentHealth);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, CurrentPhase);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, bEnraged);
}

void AAstrawildEchoBossCharacter::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

float AAstrawildEchoBossCharacter::GetHealthFraction() const
{
    return FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
}

float AAstrawildEchoBossCharacter::GetAttackDamage() const
{
    float Damage = BaseDamage;
    if (bEnraged)
    {
        Damage *= EnrageDamageMultiplier;
    }
    else if (CurrentPhase == 2)
    {
        Damage *= 1.15f;
    }
    return Damage;
}

AAstrawildPlayerCharacter* AAstrawildEchoBossCharacter::FindNearestPlayer() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    AAstrawildPlayerCharacter* Best = nullptr;
    float BestDistance = 4000.0f;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
        if (!Player || !Player->IsAlive())
        {
            continue;
        }
        const float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Player;
        }
    }
    return Best;
}

void AAstrawildEchoBossCharacter::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority || IsDefeated())
    {
        return;
    }

    // --- Enrage timer (directive §24): fights can't be stalled forever. ---
    EnrageElapsed += DeltaTime;
    if (!bEnraged && EnrageElapsed >= EnrageTimerSeconds)
    {
        bEnraged = true;
        TransitionToPhase(3);
        UE_LOG(LogAstrawildCombat, Log, TEXT("Boss ENRAGED by timer at %.0f%% health."), GetHealthFraction() * 100.0f);
    }

    // --- Phase transitions by health (server-authoritative). ---
    const float Fraction = GetHealthFraction();
    if (CurrentPhase == 1 && Fraction <= 0.66f)
    {
        TransitionToPhase(2);
    }
    else if (CurrentPhase == 2 && Fraction <= 0.33f)
    {
        TransitionToPhase(3);
    }

    ExecuteAttack(DeltaTime);
}

void AAstrawildEchoBossCharacter::TransitionToPhase(const int32 NewPhase)
{
    if (CurrentPhase == NewPhase || NewPhase < 1)
    {
        return;
    }

    const float FractionAtTransition = GetHealthFraction();
    CurrentPhase = NewPhase;
    OnPhaseChanged.Broadcast(NewPhase, FractionAtTransition);

    // Phase behavior changes (directive §24 — pattern change, not just HP):
    if (NewPhase == 2)
    {
        GetCharacterMovement()->MaxWalkSpeed = 440.0f;   // Faster pursuit.
        AttackCooldownSeconds = 1.6f;                     // Faster swings.
        SpawnSummons();
    }
    else if (NewPhase == 3)
    {
        GetCharacterMovement()->MaxWalkSpeed = 560.0f;   // Enrage speed.
        AttackCooldownSeconds = 1.0f;                     // Enrage tempo.
    }

    UE_LOG(LogAstrawildCombat, Log, TEXT("Boss phase -> %d (%.0f%% HP)."), NewPhase, FractionAtTransition * 100.0f);
}

void AAstrawildEchoBossCharacter::SpawnSummons()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry || bPhase2SummonsSpawned)
    {
        return;
    }

    UAstrawildEchoDefinition* Definition = Registry->FindEcho(SummonSpeciesId);
    if (!Definition)
    {
        return;
    }

    bPhase2SummonsSpawned = true;
    for (int32 i = 0; i < SummonCount; ++i)
    {
        const float Angle = (PI * 2.0f * i) / FMath::Max(1, SummonCount);
        const FVector Offset(FMath::Cos(Angle) * 350.0f, FMath::Sin(Angle) * 350.0f, 120.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Add = World->SpawnActor<AAstrawildEchoCharacter>(
            AAstrawildEchoCharacter::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Params);
        if (Add)
        {
            Add->InitializeFromDefinition(Definition);
            Summons.Add(Add);
        }
    }
    UE_LOG(LogAstrawildCombat, Log, TEXT("Boss summoned %d adds."), SummonCount);
}

void AAstrawildEchoBossCharacter::ExecuteAttack(const float DeltaTime)
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = FindNearestPlayer();
    if (!World || !Player)
    {
        return;
    }

    const float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

    // Movement: pursue the player (the AI controller hookup arrives with the asset pass —
    // direct movement keeps the encounter fully playable server-side meanwhile).
    if (Distance > 260.0f)
    {
        const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
        AddMovementInput(Direction, 1.0f);
    }
    else if (World->GetTimeSeconds() - LastAttackTime >= AttackCooldownSeconds)
    {
        LastAttackTime = World->GetTimeSeconds();

        // Telegraphed hit: the swing lands after the cooldown gate (timing tuning in-engine).
        // Audit C-5: routed through the player's combat component so dodge i-frames and
        // block mitigation apply (consistent with Echo attacks — previously bypassed).
        if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
        {
            const float RawDamage = GetAttackDamage();
            const float Mitigated = Player->CombatComponent
                ? Player->CombatComponent->GetMitigatedIncomingDamage(RawDamage)
                : RawDamage;
            Survival->ApplyDamage(Mitigated);
            UE_LOG(LogAstrawildCombat, Verbose, TEXT("Boss hit player for %.1f (phase %d%s)."),
                Mitigated, CurrentPhase, bEnraged ? TEXT(" ENRAGED") : TEXT(""));
        }
    }
}

float AAstrawildEchoBossCharacter::ApplyBossDamage(const float DamageAmount)
{
    if (GetLocalRole() != ROLE_Authority || DamageAmount <= 0.0f || IsDefeated())
    {
        return 0.0f;
    }

    const float Applied = FMath::Min(CurrentHealth, DamageAmount);
    CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

    if (IsDefeated())
    {
        // Clean up summons so the arena is safe on victory.
        for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : Summons)
        {
            if (AAstrawildEchoCharacter* Add = Weak.Get())
            {
                Add->ApplyDamage(999999.0f);
            }
        }
        OnBossDefeated.Broadcast(this);
        UE_LOG(LogAstrawildCombat, Log, TEXT("Boss DEFEATED."));
    }

    return Applied;
}
