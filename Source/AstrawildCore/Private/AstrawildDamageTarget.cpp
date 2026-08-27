#include "AstrawildDamageTarget.h"

#include "AstrawildCore.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildDamageTarget::AAstrawildDamageTarget()
{
    PrimaryActorTick.bCanEverTick = false;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
    VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(0.8f, 0.8f, 1.2f));
    }
}

void AAstrawildDamageTarget::BeginPlay()
{
    Super::BeginPlay();
    ResetTarget();
}

bool AAstrawildDamageTarget::ApplyDamage(const float DamageAmount)
{
    if (DamageAmount <= 0.0f || IsDefeated())
    {
        return false;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
    OnDamageReceived.Broadcast(this, DamageAmount, CurrentHealth);
    if (IsDefeated())
    {
        OnDefeated.Broadcast(this);
    }
    return true;
}

void AAstrawildDamageTarget::ResetTarget()
{
    MaxHealth = FMath::Max(1.0f, MaxHealth);
    CurrentHealth = MaxHealth;
}
