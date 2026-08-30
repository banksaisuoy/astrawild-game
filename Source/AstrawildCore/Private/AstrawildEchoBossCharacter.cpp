#include "AstrawildEchoBossCharacter.h"

#include "AstrawildBossHazardActor.h"
#include "AstrawildBossTelegraphActor.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildProjectileActor.h"
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

    // Final production run (PHASE 14): weak-point core — a small chest sphere that
    // exposes periodically (REPLACE_BEFORE_RELEASE: glowing material + pulse).
    WeakPointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeakPointMesh"));
    WeakPointMesh->SetupAttachment(GetCapsuleComponent());
    WeakPointMesh->SetCollisionProfileName(TEXT("NoCollision"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        WeakPointMesh->SetStaticMesh(SphereMesh.Object);
    }
    WeakPointMesh->SetWorldScale3D(FVector(0.9f));
    WeakPointMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
    WeakPointMesh->SetVisibility(false);
}

void AAstrawildEchoBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, CurrentHealth);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, CurrentPhase);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, bEnraged);
    DOREPLIFETIME(AAstrawildEchoBossCharacter, bWeakPointExposed);
}

void AAstrawildEchoBossCharacter::OnRep_bWeakPointExposed()
{
    // Clients mirror the vulnerability window visually.
    if (WeakPointMesh)
    {
        WeakPointMesh->SetVisibility(bWeakPointExposed);
    }
}

void AAstrawildEchoBossCharacter::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
    ArenaCenter = GetActorLocation();

    if (WeakPointMesh)
    {
        WeakPointMesh->SetVisibility(bWeakPointExposed);
    }
}

float AAstrawildEchoBossCharacter::GetHealthFraction() const
{
    return FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
}

float AAstrawildEchoBossCharacter::GetAttackDamage() const
{
    return ComputeBossAttackDamage(BaseDamage, CurrentPhase, bEnraged, EnrageDamageMultiplier);
}

// --- Batch 6: pure statics (shared with the automation tests) ---

float AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(
    const EAstrawildElementType AttackElement,
    const EAstrawildElementType Weakness,
    const EAstrawildElementType OwnElement)
{
    // Same vocabulary as the Echo pipeline (EchoCharacter::ApplyElementalDamage):
    // weakness ×1.5, own-element resist ×0.75, everything else neutral. The None
    // element never triggers either branch.
    if (AttackElement == EAstrawildElementType::None)
    {
        return 1.0f;
    }
    if (AttackElement == Weakness)
    {
        return 1.5f;
    }
    if (AttackElement == OwnElement)
    {
        return 0.75f;
    }
    return 1.0f;
}

int32 AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(const float HealthFraction, const bool bEnraged)
{
    if (bEnraged)
    {
        return 3;
    }
    if (HealthFraction <= 0.33f)
    {
        return 3;
    }
    if (HealthFraction <= 0.66f)
    {
        return 2;
    }
    return 1;
}

float AAstrawildEchoBossCharacter::ComputeBossAttackDamage(
    const float Base, const int32 Phase, const bool bEnraged, const float EnrageMultiplier)
{
    if (bEnraged)
    {
        return Base * FMath::Max(1.0f, EnrageMultiplier);
    }
    if (Phase == 2)
    {
        return Base * 1.15f;
    }
    return Base;
}

// --- Batch 6: definition-driven stats ---

void AAstrawildEchoBossCharacter::InitializeFromBossDefinition(const UAstrawildEchoDefinition* Definition)
{
    if (!Definition)
    {
        return;
    }

    BossSpeciesId = Definition->DefinitionId;
    WeaknessElement = Definition->WeaknessElement;
    BossElement = Definition->Element;

    // Boss scale on top of the species baseline (directive §24 — the PHASE design
    // carries the difficulty; the scale just makes it a boss-sized encounter).
    MaxHealth = FMath::Max(100.0f, Definition->BaseStats.MaxHealth * BossHealthScale);
    BaseDamage = FMath::Max(5.0f, Definition->BaseStats.AttackPower * BossDamageScale);
    CurrentHealth = MaxHealth;

    UE_LOG(LogAstrawildCombat, Log, TEXT("Boss initialized from %s: HP %.0f, ATK %.0f, weakness %d, element %d."),
        *BossSpeciesId.ToString(), MaxHealth, BaseDamage,
        static_cast<int32>(WeaknessElement), static_cast<int32>(BossElement));
}

// --- Batch 6: status effects on the boss ---

void AAstrawildEchoBossCharacter::ApplyBossStatus(const FAstrawildStatusEffect& Effect)
{
    if (GetLocalRole() != ROLE_Authority || Effect.StatusId.IsNone() || IsDefeated())
    {
        return;
    }

    FAstrawildStatusEffect Scaled = Effect;
    Scaled.RemainingSeconds *= FMath::Clamp(BossStatusDurationMultiplier, 0.05f, 1.0f);

    // Refresh an existing stack of the same status instead of stacking (bosses
    // can't be chain-frozen — directive §24 "never solve difficulty with CC").
    for (FAstrawildStatusEffect& Existing : ActiveStatusEffects)
    {
        if (Existing.StatusId == Scaled.StatusId)
        {
            Existing = Scaled;
            RefreshWalkSpeed();
            return;
        }
    }

    ActiveStatusEffects.Add(Scaled);
    RefreshWalkSpeed();
    UE_LOG(LogAstrawildCombat, Log, TEXT("Boss afflicted by %s (%.1fs)."), *Scaled.StatusId.ToString(), Scaled.RemainingSeconds);
}

float AAstrawildEchoBossCharacter::GetStatusSpeedMultiplier() const
{
    float Multiplier = 1.0f;
    for (const FAstrawildStatusEffect& Effect : ActiveStatusEffects)
    {
        Multiplier = FMath::Min(Multiplier, FMath::Max(0.1f, Effect.SpeedMultiplier));
    }
    return Multiplier;
}

void AAstrawildEchoBossCharacter::RefreshWalkSpeed()
{
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = PhaseWalkSpeed * GetStatusSpeedMultiplier();
    }
}

void AAstrawildEchoBossCharacter::TickStatusEffects(const float DeltaTime)
{
    if (ActiveStatusEffects.IsEmpty() || IsDefeated())
    {
        return;
    }

    for (int32 i = ActiveStatusEffects.Num() - 1; i >= 0; --i)
    {
        FAstrawildStatusEffect& Effect = ActiveStatusEffects[i];
        Effect.RemainingSeconds -= DeltaTime;

        if (Effect.DamagePerSecond > 0.0f && !IsDefeated())
        {
            // DoT rides the normal damage pipeline so a burning boss can die to
            // its burn and fire the full defeat event chain.
            ApplyBossDamage(Effect.DamagePerSecond * DeltaTime);
        }

        if (Effect.RemainingSeconds <= 0.0f || IsDefeated())
        {
            ActiveStatusEffects.RemoveAt(i);
        }
    }
    RefreshWalkSpeed();
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

    // --- Phase transitions by health (server-authoritative). Steps one phase at a
    //     time so phase 2 always spawns its adds even when chunked below 33%.
    const int32 TargetPhase = ComputePhaseForHealthFraction(GetHealthFraction(), false);
    if (TargetPhase > CurrentPhase)
    {
        TransitionToPhase(CurrentPhase + 1);
    }

    TickStatusEffects(DeltaTime);
    ExecuteAttack(DeltaTime);

    // Final production run (PHASE 14): specials + weak point + pending blasts.
    TickSpecials(DeltaTime);
    TickWeakPoint(DeltaTime);
    TickPendingBlasts(DeltaTime);
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
        PhaseWalkSpeed = 440.0f;   // Faster pursuit.
        AttackCooldownSeconds = 1.6f;                     // Faster swings.
        SpawnSummons();
    }
    else if (NewPhase == 3)
    {
        PhaseWalkSpeed = 560.0f;   // Enrage speed.
        AttackCooldownSeconds = 1.0f;                     // Enrage tempo.
    }
    RefreshWalkSpeed();

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

            // Batch 3 — Item B: boss hits are heavy by design — always stagger the
            // player when the swing actually lands (dodged hits never reach here).
            if (Player->CombatComponent)
            {
                Player->CombatComponent->ApplyStagger(Player->CombatComponent->PlayerStaggerSeconds);
            }

            UE_LOG(LogAstrawildCombat, Verbose, TEXT("Boss hit player for %.1f (phase %d%s)."),
                Mitigated, CurrentPhase, bEnraged ? TEXT(" ENRAGED") : TEXT(""));
        }
    }
}

float AAstrawildEchoBossCharacter::ApplyElementalBossDamage(const float DamageAmount, const EAstrawildElementType Element)
{
    if (GetLocalRole() != ROLE_Authority || DamageAmount <= 0.0f || IsDefeated())
    {
        return 0.0f;
    }

    // Batch 6: resolve the element against the boss's weakness/own element (same
    // multiplier vocabulary as the Echo pipeline) before the phase pipeline.
    float Damage = DamageAmount * ComputeBossElementalMultiplier(Element, WeaknessElement, BossElement);

    // Final production run (PHASE 14): strikes landed while the weak-point core is
    // exposed hit the vulnerability window (x2 by default) — the skill ceiling.
    if (bWeakPointExposed)
    {
        Damage *= FMath::Max(1.0f, WeakPointDamageMultiplier);
    }

    const float Applied = ApplyBossDamage(Damage);

    if (Applied > 0.0f)
    {
        // Shared element→status factory (Batch 3 vocabulary): Ember burns, Frost
        // chills, Flora poisons, Pulse shocks — bosses just shed them faster.
        const FAstrawildStatusEffect Status = UAstrawildCombatComponent::MakeElementalStatusEffect(Element, DamageAmount);
        if (!Status.StatusId.IsNone())
        {
            ApplyBossStatus(Status);
        }
    }
    return Applied;
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
        // Final production run (PHASE 14): dissolve telegraphs + hazards so the
        // arena is safe the moment the fight ends.
        CleanupEncounterFx();

        // Clean up summons so the arena is safe on victory.
        for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : Summons)
        {
            if (AAstrawildEchoCharacter* Add = Weak.Get())
            {
                Add->ApplyDamage(999999.0f);
            }
        }

        // Batch 6: publish the defeat to the event bus so quests/journal credit the
        // kill (previously OnBossDefeated had no subscribers at all — the delegate
        // broadcast below remains for future UMG/boss-health-bar consumers).
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_HostileDefeated, this, DefeatEventTargetId, 1, GetActorLocation());
            }
        }

        OnBossDefeated.Broadcast(this);
        UE_LOG(LogAstrawildCombat, Log, TEXT("Boss DEFEATED."));
    }

    return Applied;
}

// --- Final production run (PHASE 14): special-attack pipeline ---

void AAstrawildEchoBossCharacter::TickSpecials(const float DeltaTime)
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = FindNearestPlayer();
    if (!World || !Player)
    {
        return;
    }

    const double Now = World->GetTimeSeconds();

    // Special cadence: phase 1 energy bolt; phase 2+ adds the telegraphed AoE slam
    // (and phase 3 halves the special cooldown — enrage pressure).
    if (Now - LastSpecialAttackTime >= SpecialAttackCooldownSeconds * (CurrentPhase >= 3 ? 0.5f : 1.0f))
    {
        LastSpecialAttackTime = Now;

        // Always: energy bolt at the player (dodgeable projectile).
        FireEnergyBolt(Player);

        // Phase 2+: telegraphed AoE at the player's CURRENT position — the ring
        // gives the player the window to sprint out (the whole point of telegraphs).
        if (CurrentPhase >= 2)
        {
            FAstrawildPendingBlast Blast;
            Blast.Location = Player->GetActorLocation();
            Blast.RemainingSeconds = TelegraphDurationSeconds;

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            if (AAstrawildBossTelegraphActor* Ring = World->SpawnActor<AAstrawildBossTelegraphActor>(
                AAstrawildBossTelegraphActor::StaticClass(), Blast.Location, FRotator::ZeroRotator, Params))
            {
                Ring->BlastRadius = SpecialBlastRadius;
                Ring->TelegraphDuration = TelegraphDurationSeconds;
                Blast.Ring = Ring;
            }
            PendingBlasts.Add(Blast);
            UE_LOG(LogAstrawildCombat, Verbose, TEXT("Boss telegraphs a blast at %.0f,%.0f."), Blast.Location.X, Blast.Location.Y);
        }
    }

    // Arena hazards: phase 2+ scatters lingering pools around the arena center.
    if (CurrentPhase >= 2 && Now - LastHazardTime >= HazardIntervalSeconds)
    {
        LastHazardTime = Now;
        SpawnArenaHazard();
    }
}

void AAstrawildEchoBossCharacter::TickWeakPoint(const float DeltaTime)
{
    WeakPointElapsed += DeltaTime;

    if (bWeakPointExposed)
    {
        if (WeakPointElapsed >= WeakPointWindowSeconds)
        {
            bWeakPointExposed = false;
            WeakPointElapsed = 0.0f;
            if (WeakPointMesh)
            {
                WeakPointMesh->SetVisibility(false);
            }
        }
    }
    else if (WeakPointElapsed >= WeakPointPeriodSeconds)
    {
        bWeakPointExposed = true;
        WeakPointElapsed = 0.0f;
        if (WeakPointMesh)
        {
            WeakPointMesh->SetVisibility(true);
        }
        UE_LOG(LogAstrawildCombat, Verbose, TEXT("Boss weak point exposed for %.1fs!"), WeakPointWindowSeconds);
    }
}

void AAstrawildEchoBossCharacter::TickPendingBlasts(const float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float SquaredRadius = SpecialBlastRadius * SpecialBlastRadius;
    for (int32 i = PendingBlasts.Num() - 1; i >= 0; --i)
    {
        FAstrawildPendingBlast& Blast = PendingBlasts[i];
        Blast.RemainingSeconds -= DeltaTime;
        if (Blast.RemainingSeconds > 0.0f)
        {
            continue;
        }

        // Detonate: radial damage to alive players inside (standard mitigation).
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            const APlayerController* PC = It->Get();
            AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
            if (!Player || !Player->IsAlive())
            {
                continue;
            }
            if (FVector::DistSquared(Blast.Location, Player->GetActorLocation()) > SquaredRadius)
            {
                continue;
            }

            if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
            {
                const float Mitigated = Player->CombatComponent
                    ? Player->CombatComponent->GetMitigatedIncomingDamage(SpecialBlastDamage)
                    : SpecialBlastDamage;
                Survival->ApplyDamage(Mitigated);
                if (Player->CombatComponent)
                {
                    Player->CombatComponent->ApplyStagger(Player->CombatComponent->PlayerStaggerSeconds);
                }
            }
        }

        if (AAstrawildBossTelegraphActor* Ring = Blast.Ring.Get())
        {
            World->DestroyActor(Ring);
        }
        PendingBlasts.RemoveAt(i);
    }
}

void AAstrawildEchoBossCharacter::FireEnergyBolt(AAstrawildPlayerCharacter* Target)
{
    UWorld* World = GetWorld();
    if (!World || !Target)
    {
        return;
    }

    const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 60.0f);
    const FVector Direction = (Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f) - Muzzle).GetSafeNormal();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Instigator = this;
    if (AAstrawildProjectileActor* Bolt = World->SpawnActor<AAstrawildProjectileActor>(
        AAstrawildProjectileActor::StaticClass(), Muzzle, Direction.Rotation(), Params))
    {
        Bolt->Launch(Direction, GetAttackDamage() * 0.8f, BossElement, this);
    }
}

void AAstrawildEchoBossCharacter::SpawnArenaHazard()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Random point on the hazard ring around the arena center.
    const float Angle = FMath::FRandRange(0.0f, PI * 2.0f);
    const FVector Location = ArenaCenter + FVector(
        FMath::Cos(Angle) * HazardSpawnRadius,
        FMath::Sin(Angle) * HazardSpawnRadius,
        0.0f);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AAstrawildBossHazardActor* Hazard = World->SpawnActor<AAstrawildBossHazardActor>(
        AAstrawildBossHazardActor::StaticClass(), Location, FRotator::ZeroRotator, Params))
    {
        ActiveHazards.Add(Hazard);
    }
}

void AAstrawildEchoBossCharacter::CleanupEncounterFx()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const FAstrawildPendingBlast& Blast : PendingBlasts)
    {
        if (AAstrawildBossTelegraphActor* Ring = Blast.Ring.Get())
        {
            World->DestroyActor(Ring);
        }
    }
    PendingBlasts.Reset();

    for (const TWeakObjectPtr<AAstrawildBossHazardActor>& Weak : ActiveHazards)
    {
        if (AAstrawildBossHazardActor* Hazard = Weak.Get())
        {
            World->DestroyActor(Hazard);
        }
    }
    ActiveHazards.Reset();

    if (bWeakPointExposed)
    {
        bWeakPointExposed = false;
        if (WeakPointMesh)
        {
            WeakPointMesh->SetVisibility(false);
        }
    }
}
