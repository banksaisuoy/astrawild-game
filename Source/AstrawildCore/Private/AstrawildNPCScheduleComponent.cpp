#include "AstrawildNPCScheduleComponent.h"

#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAstrawildNPCScheduleComponent::UAstrawildNPCScheduleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 2.0f;
}

EAstrawildNPCAnchor UAstrawildNPCScheduleComponent::ResolveAnchor(EAstrawildNPCProfession Profession,
    int32 HourOfDay, bool bRaining)
{
    // Night: everyone sleeps (guards keep a patrol skeleton).
    if (HourOfDay >= 22 || HourOfDay < 6)
    {
        return Profession == EAstrawildNPCProfession::Guard
            ? EAstrawildNPCAnchor::Patrol
            : EAstrawildNPCAnchor::Sleep;
    }

    // Rain: everyone but guards shelters (directive §7.2 "หลบฝน" — rain shelter).
    if (bRaining && Profession != EAstrawildNPCProfession::Guard)
    {
        return EAstrawildNPCAnchor::Shelter;
    }

    // Work hours (6-18 for most, traders open 8-20): at the work anchor.
    const bool bWorkHours = Profession == EAstrawildNPCProfession::Trader
        ? (HourOfDay >= 8 && HourOfDay < 20)
        : (HourOfDay >= 6 && HourOfDay < 18);

    if (bWorkHours)
    {
        return Profession == EAstrawildNPCProfession::Guard
            ? EAstrawildNPCAnchor::Patrol
            : EAstrawildNPCAnchor::Work;
    }

    // Evening: home.
    return EAstrawildNPCAnchor::Home;
}

EAstrawildNPCProfession UAstrawildNPCScheduleComponent::ResolveProfession(uint8 NPCRole)
{
    switch (static_cast<EAstrawildNPCRole>(NPCRole))
    {
    case EAstrawildNPCRole::Vendor:
        return EAstrawildNPCProfession::Trader;
    case EAstrawildNPCRole::Guard:
        return EAstrawildNPCProfession::Guard;
    case EAstrawildNPCRole::Elder:
    case EAstrawildNPCRole::QuestGiver:
        return EAstrawildNPCProfession::Smith; // village anchors: forge/workbench duty
    default:
        return EAstrawildNPCProfession::Farmer;
    }
}

bool UAstrawildNPCScheduleComponent::IsServiceOpen(EAstrawildNPCProfession Profession, int32 HourOfDay, bool bRaining)
{
    // Services trade during the work window — rain closes the stall (the
    // directive's shelter rule) but never the guard post.
    const EAstrawildNPCAnchor Anchor = ResolveAnchor(Profession, HourOfDay, bRaining);
    return Anchor == EAstrawildNPCAnchor::Work || Anchor == EAstrawildNPCAnchor::Patrol;
}

bool UAstrawildNPCScheduleComponent::IsAuthority() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    return Owner->HasAuthority();
}

int32 UAstrawildNPCScheduleComponent::GetCurrentHour() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 12;
    }
    const UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>();
    return Time ? (Time->GetCurrentMinute() / 60) : 12;
}

bool UAstrawildNPCScheduleComponent::IsRaining() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    const UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>();
    if (!Weather)
    {
        return false;
    }
    const EAstrawildWeatherState State = Weather->GetCurrentWeather();
    return State == EAstrawildWeatherState::Rain || State == EAstrawildWeatherState::HeavyRain ||
        State == EAstrawildWeatherState::Storm;
}

void UAstrawildNPCScheduleComponent::RefreshProfessionFromDefinition()
{
    // FCR-1-d fix (H-d3): called after NpcDefinition is assigned (and lazily
    // from TickComponent until a definition resolves).
    const AAstrawildNPCCharacter* NPC = Cast<AAstrawildNPCCharacter>(GetOwner());
    if (NPC && IsValid(NPC->NpcDefinition))
    {
        const EAstrawildNPCProfession Resolved = ResolveProfession(static_cast<uint8>(NPC->NpcDefinition->Role));
        if (Resolved != Profession)
        {
            Profession = Resolved;
            // The anchor may change with the profession — re-derive immediately.
            CurrentAnchor = ResolveAnchor(Profession, GetCurrentHour(), IsRaining());
            MoveToAnchor();
        }
    }
}

void UAstrawildNPCScheduleComponent::BeginPlay()
{
    Super::BeginPlay();

    // Profession defaults from the NPC definition role; the home anchor is the
    // spawn vicinity (villages are compact by design).
    if (const AAstrawildNPCCharacter* NPC = Cast<AAstrawildNPCCharacter>(GetOwner()))
    {
        if (IsValid(NPC->NpcDefinition))
        {
            Profession = ResolveProfession(static_cast<uint8>(NPC->NpcDefinition->Role));
        }
        // FCR-1-d fix (H-d2): the guard was INVERTED — with the ZeroVector default
        // the home anchor was NEVER initialized, so every sleep/evening/rain/work
        // destination resolved to the WORLD ORIGIN and all NPCs marched to the
        // map corner. Initialize when unset; designer values are preserved.
        if (HomeLocation.IsZero())
        {
            HomeLocation = NPC->GetActorLocation();
        }
    }
    else if (const AActor* Owner = GetOwner())
    {
        if (HomeLocation.IsZero())
        {
            HomeLocation = Owner->GetActorLocation();
        }
    }

    CurrentAnchor = ResolveAnchor(Profession, GetCurrentHour(), IsRaining());
}

bool UAstrawildNPCScheduleComponent::AreServicesOpenNow() const
{
    return IsServiceOpen(Profession, GetCurrentHour(), IsRaining());
}

void UAstrawildNPCScheduleComponent::MoveToAnchor()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return;
    }

    // Resolve the anchor's destination.
    FVector Destination = HomeLocation;
    switch (CurrentAnchor)
    {
    case EAstrawildNPCAnchor::Work:
        Destination = HomeLocation + WorkLocationOffset;
        break;
    case EAstrawildNPCAnchor::Shelter:
        // Shelter: nearest building interior approximates as home (compact villages).
        Destination = HomeLocation;
        break;
    case EAstrawildNPCAnchor::Patrol:
    {
        // Guards drift on a ring around home (the existing patrol feel).
        const double Angle = FMath::Fmod(static_cast<double>(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0) * 0.1, 2.0 * PI);
        Destination = HomeLocation + FVector(FMath::Cos(Angle) * 600.0f, FMath::Sin(Angle) * 600.0f, 0.0f);
        break;
    }
    case EAstrawildNPCAnchor::Sleep:
    case EAstrawildNPCAnchor::Home:
    default:
        Destination = HomeLocation;
        break;
    }

    if (AAIController* Controller = Cast<AAIController>(Owner->GetController()))
    {
        Controller->MoveToLocation(Destination, 120.0f);
    }
}

void UAstrawildNPCScheduleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsAuthority())
    {
        return;
    }

    // FCR-1-d fix (H-d3) safety net: definitions are assigned AFTER BeginPlay on
    // every spawn path — keep resolving until it sticks (cheap: one IsValid).
    RefreshProfessionFromDefinition();

    const EAstrawildNPCAnchor NewAnchor = ResolveAnchor(Profession, GetCurrentHour(), IsRaining());
    if (NewAnchor != CurrentAnchor)
    {
        UE_LOG(LogAstrawildAI, Log, TEXT("NPC %s schedule: %d -> %d"),
            *GetNameSafe(GetOwner()), static_cast<int32>(CurrentAnchor), static_cast<int32>(NewAnchor));
        CurrentAnchor = NewAnchor;
        MoveToAnchor();
        return;
    }

    // Idle drift: re-issue the anchor movement every ~20s so NPCs that get
    // nudged by combat find their way back (guards patrol on a tighter loop).
    DriftAccumulator += DeltaTime;
    const float DriftPeriod = CurrentAnchor == EAstrawildNPCAnchor::Patrol ? 8.0f : 20.0f;
    if (DriftAccumulator >= DriftPeriod)
    {
        DriftAccumulator = 0.0f;
        MoveToAnchor();
    }
}
