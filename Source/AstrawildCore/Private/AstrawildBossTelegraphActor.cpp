#include "AstrawildBossTelegraphActor.h"

#include "AstrawildVfxActor.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildBossTelegraphActor::AAstrawildBossTelegraphActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Flat warning disc (engine cylinder — REPLACE_BEFORE_RELEASE with a decal ring).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = VisualMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderMesh.Object);
    }
    // Cylinder default is 100cm radius x 100cm half-height — flatten to a disc.
    VisualMesh->SetWorldScale3D(FVector(3.5f, 3.5f, 0.04f));

    // FPP-1: element-identity light — the disc itself stays the readable
    // silhouette; the light paints it in the attacker's element color (or hot
    // amber for element-neutral danger). Attenuation covers the blast area.
    TelegraphLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TelegraphLight"));
    TelegraphLight->SetupAttachment(VisualMesh);
    TelegraphLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    TelegraphLight->SetIntensity(0.0f); // Dark until ConfigureTelegraph runs.
    TelegraphLight->SetAttenuationRadius(900.0f);
    TelegraphLight->SetCastShadows(false);
    TelegraphLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.12f, 1.0f));

    bReplicates = true;
}

void AAstrawildBossTelegraphActor::ConfigureTelegraph(const float InBlastRadius, const float InDuration, const EAstrawildElementType Element)
{
    BlastRadius = FMath::Max(50.0f, InBlastRadius);
    TelegraphDuration = FMath::Max(0.1f, InDuration);

    if (TelegraphLight)
    {
        const FLinearColor Tint = Element == EAstrawildElementType::None
            ? FLinearColor(1.0f, 0.42f, 0.10f, 1.0f) // hot warning amber
            : FAstrawildVfxPalette::GetElementTint(Element);
        TelegraphLight->SetLightColor(Tint);
        TelegraphLight->SetAttenuationRadius(FMath::Max(500.0f, BlastRadius * 1.6f));
    }
}

void AAstrawildBossTelegraphActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAstrawildBossTelegraphActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Pulse so the warning reads as "imminent" — grow toward the full radius,
    // then flash in the last quarter of the countdown.
    Elapsed += DeltaTime;
    const float Fraction = FMath::Clamp(Elapsed / FMath::Max(0.1f, TelegraphDuration), 0.0f, 1.0f);
    const float Pulse = Fraction > 0.75f ? (0.9f + 0.1f * FMath::Sin(Elapsed * 40.0f)) : (0.35f + 0.65f * Fraction);
    const float RadiusScale = (BlastRadius / 100.0f) * Pulse;
    VisualMesh->SetWorldScale3D(FVector(RadiusScale, RadiusScale, 0.04f));

    // FPP-1: the light breathes with the disc (steady grow, hard flash at the
    // end) — the danger cadence is visible at combat distance.
    if (TelegraphLight)
    {
        const float LightIntensity = Fraction > 0.75f
            ? (9.0f + 4.0f * FMath::Sin(Elapsed * 40.0f))
            : (2.0f + 5.0f * Fraction);
        TelegraphLight->SetIntensity(FMath::Max(0.0f, LightIntensity));
    }

    if (Elapsed >= TelegraphDuration && GetWorld() && GetLocalRole() == ROLE_Authority)
    {
        GetWorld()->DestroyActor(this);
    }
}
