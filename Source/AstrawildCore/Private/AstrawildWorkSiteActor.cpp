#include "AstrawildWorkSiteActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPowerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildWorkSiteActor::AAstrawildWorkSiteActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 0.4f));
    }
}

void AAstrawildWorkSiteActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildWorkSiteActor, StoredOutput);
}

void AAstrawildWorkSiteActor::BeginPlay()
{
    Super::BeginPlay();
}

UAstrawildPowerSubsystem* AAstrawildWorkSiteActor::GetPower() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildPowerSubsystem>() : nullptr;
}

bool AAstrawildWorkSiteActor::IsPowered() const
{
    if (!bRequiresPower)
    {
        return true;
    }
    const UAstrawildPowerSubsystem* Power = GetPower();
    return Power && Power->IsLocationPowered(GetActorLocation());
}

void AAstrawildWorkSiteActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority || OutputItemId.IsNone())
    {
        return;
    }

    // Drop stale workers.
    Workers.RemoveAll([](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return !Weak.IsValid(); });
    if (Workers.IsEmpty())
    {
        return;
    }

    const bool bPowered = IsPowered();
    const float PowerMultiplier = bPowered ? 1.5f : (bRequiresPower ? 0.0f : 1.0f);
    if (PowerMultiplier <= 0.0f)
    {
        return;
    }

    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : Workers)
    {
        const AAstrawildEchoCharacter* Echo = Weak.Get();
        if (!Echo || !IsValid(Echo->EchoDefinition) || Echo->IsDefeated())
        {
            continue;
        }

        // Affinity: species-specific (directive §18) or 0.5 baseline when unlisted.
        float Affinity = 0.5f;
        for (const FAstrawildWorkAffinity& WA : Echo->EchoDefinition->WorkAffinities)
        {
            if (WA.WorkType == WorkType)
            {
                Affinity = WA.Affinity;
                break;
            }
        }

        // Personality + needs modifiers (directive §5/§18).
        const float PersonalityMult = Echo->GetWorkSpeedMultiplier();
        const float MoodMult = FMath::Lerp(0.5f, 1.0f, Echo->Needs.Mood / 100.0f);
        const float EnergyMult = FMath::Lerp(0.5f, 1.0f, Echo->Needs.Energy / 100.0f);

        WorkAccumulator += DeltaTime * Affinity * PersonalityMult * MoodMult * EnergyMult * PowerMultiplier;

        // Working consumes energy.
        // (Mutating Needs on a const-free path: Workers holds non-const weak pointers.)
        if (AAstrawildEchoCharacter* MutableEcho = Weak.Get())
        {
            MutableEcho->Needs.Energy = FMath::Max(0.0f, MutableEcho->Needs.Energy - DeltaTime * 0.5f);
        }
    }

    while (WorkAccumulator >= SecondsPerOutput)
    {
        WorkAccumulator -= SecondsPerOutput;
        StoredOutput += 1;
        OnWorkProduced.Broadcast(this, OutputItemId, StoredOutput);
    }
}

int32 AAstrawildWorkSiteActor::CollectOutput()
{
    const int32 Collected = StoredOutput;
    StoredOutput = 0;
    return Collected;
}

bool AAstrawildWorkSiteActor::AssignWorker(AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo) || !Echo->bCaptured)
    {
        return false;
    }

    Echo->AssignedWorkSite = this;
    Echo->IssueCommand(EAstrawildEchoCommand::Work);

    if (!Workers.ContainsByPredicate([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return Weak.Get() == Echo; }))
    {
        Workers.Add(Echo);
    }
    return true;
}

void AAstrawildWorkSiteActor::RemoveWorker(AAstrawildEchoCharacter* Echo)
{
    Workers.RemoveAll([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return Weak.Get() == Echo; });
    if (IsValid(Echo))
    {
        Echo->AssignedWorkSite = nullptr;
    }
}
