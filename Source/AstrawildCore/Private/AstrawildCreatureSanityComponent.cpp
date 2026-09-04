#include "AstrawildCreatureSanityComponent.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildTimeSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAstrawildCreatureSanityComponent::UAstrawildCreatureSanityComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;
    SetIsReplicatedByDefault(true);
}

void UAstrawildCreatureSanityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildCreatureSanityComponent, Sanity);
    DOREPLIFETIME(UAstrawildCreatureSanityComponent, IllnessId);
}

bool UAstrawildCreatureSanityComponent::IsOwnerServerEcho() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    // Simulation is server-authoritative; pure test objects (no world/net) tick
    // locally so automation contracts can drive the same code.
    const UWorld* World = Owner->GetWorld();
    if (!World)
    {
        return true;
    }
    return Owner->GetLocalRole() == ROLE_Authority;
}

bool UAstrawildCreatureSanityComponent::IsNight() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    const UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>();
    if (!Time)
    {
        return false;
    }
    // GetCurrentMinute() is minute-of-day (0..1439) — hour 22:00..04:59 is night.
    const int32 MinuteOfDay = Time->GetCurrentMinute();
    const int32 Hour = MinuteOfDay / 60;
    return Hour >= 22 || Hour < 5;
}

void UAstrawildCreatureSanityComponent::RefreshComfortProximity()
{
    bNearBed = false;
    bNearHotSpring = false;

    const UWorld* World = GetWorld();
    const AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        return;
    }

    static constexpr float ComfortRadius = 1100.0f;
    static const FName BedId = TEXT("Building_CreatureBed");
    static const FName HotSpringId = TEXT("Building_HotSpring");

    for (TActorIterator<AAstrawildBuildingActor> It(World); It; ++It)
    {
        const AAstrawildBuildingActor* Building = *It;
        if (!IsValid(Building))
        {
            continue;
        }
        if (Building->DefinitionId != BedId && Building->DefinitionId != HotSpringId)
        {
            continue;
        }
        if (FVector::DistSquared(Building->GetActorLocation(), Owner->GetActorLocation()) >
            ComfortRadius * ComfortRadius)
        {
            continue;
        }
        if (Building->DefinitionId == BedId)
        {
            bNearBed = true;
        }
        else
        {
            bNearHotSpring = true;
        }
    }
}

void UAstrawildCreatureSanityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsOwnerServerEcho())
    {
        return;
    }

    // FCR-1-c fix: non-const cast — the Ulcer drain below calls the mutating
    // ApplyDamage pipeline (a const pointer was a module-wide compile blocker).
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(GetOwner());
    if (!IsValid(Echo) || !Echo->bCaptured || Echo->IsDefeated())
    {
        // Wild echoes are stress-free by design; defeated echoes stop simulating.
        return;
    }

    // Combat drain window: 20 seconds after the last damage event.
    if (Echo->CurrentHealth < Echo->GetMaxHealth() * 0.999f)
    {
        SecondsSinceCombat = 0.0f;
    }
    else
    {
        SecondsSinceCombat += DeltaTime;
    }
    const bool bInCombat = SecondsSinceCombat < 20.0f;
    const bool bWorking = Echo->AssignedWorkSite.IsValid();

    ProximityAccumulator += DeltaTime;
    if (ProximityAccumulator >= 1.0f)
    {
        ProximityAccumulator = 0.0f;
        RefreshComfortProximity();
    }

    const bool bNightRest = IsNight() && !bWorking && !bInCombat;
    const float Delta = ComputeSanityDelta(DeltaTime, bWorking, bInCombat, bNearBed, bNearHotSpring, bNightRest);
    Sanity = FMath::Clamp(Sanity + Delta, 0.0f, 100.0f);

    // Illness pressure: risk accrues only while below the threshold and is
    // reset by climbing back above it (recovery is always possible).
    if (Sanity < IllnessThreshold && IllnessId.IsNone())
    {
        LowSanityExposure += DeltaTime;
        const float Risk = ComputeIllnessRisk(Sanity, LowSanityExposure);
        if (Risk >= 1.0f)
        {
            IllnessId = SelectIllness(FMath::FRand());
            LowSanityExposure = 0.0f;
            UE_LOG(LogAstrawildAI, Log, TEXT("Sanity: echo %s contracted %s"),
                *Echo->GetName(), *IllnessId.ToString());
        }
    }
    else if (Sanity >= IllnessThreshold)
    {
        LowSanityExposure = 0.0f;
    }

    // Illness effects: Ulcer drains health until cured. FCR-1-c fix: the rate is
    // per-SECOND — scale by the tick delta (the raw per-tick call doubled the
    // documented drain at the 0.5s component cadence).
    if (IsIll())
    {
        const float Drain = GetHealthDrainPerSecond();
        if (Drain > 0.0f)
        {
            Echo->ApplyDamage(Drain * DeltaTime);
        }
    }
}

float UAstrawildCreatureSanityComponent::ComputeSanityDelta(float DeltaSeconds, bool bWorking,
    bool bInCombat, bool bNearBed, bool bNearHotSpring, bool bNightRest)
{
    // Rates per second (directive Phase 9.2 bands, tuned at a 1.2/min drain):
    // working -0.02/s, combat -0.01/s, bed +0.05/s, hot spring +0.12/s,
    // night rest +0.01/s. Comfort sources do not coexist with drains in the
    // same step (bed/spring positions are away from work sites in practice),
    // but the math allows overlap — comfort always outweighs stress.
    float Delta = 0.0f;
    if (bWorking)
    {
        Delta -= 0.02f;
    }
    if (bInCombat)
    {
        Delta -= 0.01f;
    }
    if (bNearBed)
    {
        Delta += 0.05f;
    }
    if (bNearHotSpring)
    {
        Delta += 0.12f;
    }
    if (bNightRest)
    {
        Delta += 0.01f;
    }
    return Delta * DeltaSeconds;
}

float UAstrawildCreatureSanityComponent::ComputeIllnessRisk(float Sanity, float ExposureSeconds)
{
    // Risk accumulates linearly while below the threshold: at the very bottom
    // (sanity 0) illness is certain after ~5 minutes of exposure; just under
    // the threshold it takes far longer. Clamped to 0..1.
    if (Sanity >= IllnessThreshold || ExposureSeconds <= 0.0f)
    {
        return 0.0f;
    }

    const float Depth = (IllnessThreshold - Sanity) / IllnessThreshold; // 0..1
    const float Risk = (ExposureSeconds / 300.0f) * (0.25f + 0.75f * Depth);
    return FMath::Clamp(Risk, 0.0f, 1.0f);
}

FName UAstrawildCreatureSanityComponent::SelectIllness(float RiskRoll)
{
    // Deterministic bands: Ulcer 40%, SprainedAnkle 35%, Slacker 25%.
    const float Roll = FMath::Clamp(RiskRoll, 0.0f, 0.999f);
    if (Roll < 0.40f)
    {
        return TEXT("Illness_Ulcer");
    }
    if (Roll < 0.75f)
    {
        return TEXT("Illness_SprainedAnkle");
    }
    return TEXT("Illness_Slacker");
}

float UAstrawildCreatureSanityComponent::GetIllnessWorkMultiplier(FName InIllnessId)
{
    if (InIllnessId == TEXT("Illness_Slacker"))
    {
        return 0.3f;
    }
    return 1.0f;
}

float UAstrawildCreatureSanityComponent::GetIllnessSpeedMultiplier(FName InIllnessId)
{
    if (InIllnessId == TEXT("Illness_SprainedAnkle"))
    {
        return 0.75f;
    }
    return 1.0f;
}

float UAstrawildCreatureSanityComponent::GetIllnessHealthDrain(FName InIllnessId)
{
    if (InIllnessId == TEXT("Illness_Ulcer"))
    {
        return 0.05f; // 3 HP/min — dangerous, never instantly lethal.
    }
    return 0.0f;
}

float UAstrawildCreatureSanityComponent::GetWorkOutputMultiplier() const
{
    float Multiplier = 1.0f;

    if (Sanity < DepressedThreshold)
    {
        Multiplier *= 0.6f;
    }
    Multiplier *= GetIllnessWorkMultiplier(IllnessId);

    return Multiplier;
}

float UAstrawildCreatureSanityComponent::GetSpeedMultiplier() const
{
    return GetIllnessSpeedMultiplier(IllnessId);
}

float UAstrawildCreatureSanityComponent::GetHealthDrainPerSecond() const
{
    return GetIllnessHealthDrain(IllnessId);
}

float UAstrawildCreatureSanityComponent::AddSanity(float Amount)
{
    const float Before = Sanity;
    Sanity = FMath::Clamp(Sanity + Amount, 0.0f, 100.0f);
    return Sanity - Before;
}

bool UAstrawildCreatureSanityComponent::ApplyMedicine()
{
    if (IllnessId.IsNone())
    {
        // Medicine still soothes: the +30 sanity applies even when healthy.
        AddSanity(30.0f);
        return false;
    }

    const FName Cured = IllnessId;
    IllnessId = NAME_None;
    LowSanityExposure = 0.0f;
    AddSanity(30.0f);

    UE_LOG(LogAstrawildAI, Log, TEXT("Sanity: cured %s with medicine"), *Cured.ToString());
    return true;
}

void UAstrawildCreatureSanityComponent::ExportForSave(float& OutSanity, FName& OutIllnessId) const
{
    OutSanity = Sanity;
    OutIllnessId = IllnessId;
}

void UAstrawildCreatureSanityComponent::ImportFromSave(float InSanity, FName InIllnessId)
{
    Sanity = FMath::Clamp(InSanity, 0.0f, 100.0f);

    // Sanitize: only the three known illnesses survive a save round-trip.
    static const FName Known[] = { TEXT("Illness_Ulcer"), TEXT("Illness_SprainedAnkle"), TEXT("Illness_Slacker") };
    IllnessId = NAME_None;
    for (const FName Candidate : Known)
    {
        if (InIllnessId == Candidate)
        {
            IllnessId = InIllnessId;
            break;
        }
    }
    LowSanityExposure = 0.0f;
}
