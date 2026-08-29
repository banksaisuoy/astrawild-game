#include "AstrawildEchoCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildEchoCharacter::AAstrawildEchoCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    InstanceId = FGuid::NewGuid();

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.8f));
    }
}

void AAstrawildEchoCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (EchoDefinition)
    {
        InitializeFromDefinition(EchoDefinition, InstanceId);
    }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Echo %s has no EchoDefinition assigned."), *GetName());
    }
}

bool AAstrawildEchoCharacter::InitializeFromDefinition(UAstrawildEchoDefinition* InDefinition, const FGuid& OptionalInstanceId)
{
    if (!IsValid(InDefinition) || InDefinition->DefinitionId.IsNone())
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Echo initialization rejected: invalid definition."));
        return false;
    }

    EchoDefinition = InDefinition;
    CachedStats = InDefinition->BaseStats;
    CurrentHealth = FMath::Max(1.0f, CachedStats.MaxHealth);
    Trust = FMath::Max(0.0f, Trust);
    InstanceId = OptionalInstanceId.IsValid() ? OptionalInstanceId : FGuid::NewGuid();
    GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, CachedStats.MoveSpeed);
    return true;
}

bool AAstrawildEchoCharacter::ApplyDamage(const float DamageAmount)
{
    if (DamageAmount <= 0.0f || IsDefeated())
    {
        return false;
    }

    const float MitigatedDamage = FMath::Max(0.0f, DamageAmount - CachedStats.Defense);
    CurrentHealth = FMath::Max(0.0f, CurrentHealth - MitigatedDamage);
    OnDamaged.Broadcast(this, CurrentHealth);
    if (IsDefeated())
    {
        OnDefeated.Broadcast(this);
    }
    return true;
}

float AAstrawildEchoCharacter::GetHealthFraction() const
{
    const float MaxHealth = FMath::Max(1.0f, CachedStats.MaxHealth);
    return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

float AAstrawildEchoCharacter::ComputeCaptureChance() const
{
    if (IsDefeated() || !IsValid(EchoDefinition))
    {
        return 0.0f;
    }

    const float HealthFraction = GetHealthFraction();
    const float Resilience = FMath::Clamp(CachedStats.CaptureResilience, 0.0f, 1.0f);
    const float WeakenBonus = (1.0f - HealthFraction) * (1.0f - Resilience);
    const float TrustBonus = FMath::Clamp(Trust / 100.0f, 0.0f, 1.0f) * 0.5f;
    return FMath::Clamp(0.05f + WeakenBonus + TrustBonus, 0.05f, 0.95f);
}

bool AAstrawildEchoCharacter::Capture(const float InitialTrust)
{
    if (bCaptured || IsDefeated() || !IsValid(EchoDefinition))
    {
        return false;
    }

    bCaptured = true;
    Trust = FMath::Max(0.0f, InitialTrust) + EchoDefinition->TrustGainOnCapture;
    OnCaptured.Broadcast(this);
    return true;
}

void AAstrawildEchoCharacter::AddTrust(const float Amount)
{
    Trust = FMath::Max(0.0f, Trust + Amount);
}

FAstrawildEchoInstanceSaveData AAstrawildEchoCharacter::ToSaveData() const
{
    FAstrawildEchoInstanceSaveData Data;
    Data.InstanceId = InstanceId;
    Data.DefinitionId = EchoDefinition ? EchoDefinition->DefinitionId : NAME_None;
    Data.Level = FMath::Max(1, Level);
    Data.Trust = Trust;
    Data.Experience = 0;
    Data.bInRoster = bCaptured;
    Data.LastKnownTransform = GetActorTransform();
    return Data;
}
