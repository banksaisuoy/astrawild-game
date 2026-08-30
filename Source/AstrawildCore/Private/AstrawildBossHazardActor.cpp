#include "AstrawildBossHazardActor.h"

#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildBossHazardActor::AAstrawildBossHazardActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Placeholder hazard body (REPLACE_BEFORE_RELEASE — Niagara field).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = VisualMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereMesh.Object);
    }
    VisualMesh->SetWorldScale3D(FVector(2.2f, 2.2f, 0.5f));

    bReplicates = true;
}

void AAstrawildBossHazardActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAstrawildBossHazardActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() == ROLE_Authority)
    {
        ApplyHazardDamage(DeltaTime);

        Elapsed += DeltaTime;
        if (Elapsed >= LifetimeSeconds && GetWorld())
        {
            GetWorld()->DestroyActor(this);
        }
    }
}

void AAstrawildBossHazardActor::ApplyHazardDamage(const float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    DamageAccumulator += DamagePerSecond * DeltaSeconds;
    if (DamageAccumulator < 1.0f)
    {
        return; // Whole-number ticks only — no fractional spam.
    }

    const int32 Damage = static_cast<int32>(DamageAccumulator);
    DamageAccumulator -= Damage;

    const float SquaredRadius = HazardRadius * HazardRadius;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        AAstrawildPlayerCharacter* Player = PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
        if (!Player || !Player->IsAlive())
        {
            continue;
        }
        if (FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > SquaredRadius)
        {
            continue;
        }

        // Standard damage pipeline: armor still matters, i-frame dodges still avoid.
        if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
        {
            const float Mitigated = Player->CombatComponent
                ? Player->CombatComponent->GetMitigatedIncomingDamage(static_cast<float>(Damage))
                : static_cast<float>(Damage);
            Survival->ApplyDamage(Mitigated);
        }
    }
}
