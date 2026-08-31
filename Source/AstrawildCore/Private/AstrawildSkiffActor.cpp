#include "AstrawildSkiffActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildTerrainTileActor.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildSkiffActor::AAstrawildSkiffActor()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicatingMovement(true);

    // Hull collision — swept movement stops against terrain and buildings.
    HullCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HullCollision"));
    HullCollision->InitBoxExtent(FVector(160.0f, 100.0f, 40.0f));
    HullCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HullCollision->SetCollisionResponseToAllChannels(ECR_Block);
    HullCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // pilot rides attached
    RootComponent = HullCollision;

    BuildSkiffVisuals();
}

void AAstrawildSkiffActor::BuildSkiffVisuals()
{
    // Hull: flattened cube.
    HullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HullMesh"));
    HullMesh->SetupAttachment(HullCollision);
    HullMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        HullMesh->SetStaticMesh(CubeMesh.Object);
    }
    HullMesh->SetRelativeScale3D(FVector(3.0f, 1.9f, 0.55f));
    HullMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));

    // Nose: forward cone.
    NoseCone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NoseCone"));
    NoseCone->SetupAttachment(HullCollision);
    NoseCone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeMesh.Succeeded())
    {
        NoseCone->SetStaticMesh(ConeMesh.Object);
    }
    NoseCone->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    NoseCone->SetRelativeLocation(FVector(210.0f, 0.0f, 10.0f));
    NoseCone->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.8f));

    // Tail fin: vertical wedge.
    TailFin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TailFin"));
    TailFin->SetupAttachment(HullCollision);
    TailFin->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (CubeMesh.Succeeded())
    {
        TailFin->SetStaticMesh(CubeMesh.Object);
    }
    TailFin->SetRelativeLocation(FVector(-140.0f, 0.0f, 70.0f));
    TailFin->SetRelativeScale3D(FVector(0.8f, 0.25f, 1.1f));

    // Pontoons: two side cylinders (visual detail + boarding steps).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    for (int32 Side = 0; Side < 2; ++Side)
    {
        UStaticMeshComponent* Pontoon = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Pontoon%d"), Side));
        Pontoon->SetupAttachment(HullCollision);
        Pontoon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (CylinderMesh.Succeeded())
        {
            Pontoon->SetStaticMesh(CylinderMesh.Object);
        }
        Pontoon->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        Pontoon->SetRelativeLocation(FVector(0.0f, Side == 0 ? 115.0f : -115.0f, -30.0f));
        Pontoon->SetRelativeScale3D(FVector(0.5f, 0.5f, 3.0f));
    }

    // Bow running light.
    BowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BowLight"));
    BowLight->SetupAttachment(HullCollision);
    BowLight->SetRelativeLocation(FVector(230.0f, 0.0f, 60.0f));
    BowLight->SetIntensity(2.0f);
    BowLight->SetAttenuationRadius(1200.0f);
    BowLight->SetLightColor(FLinearColor(0.4f, 0.85f, 1.0f));
}

void AAstrawildSkiffActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildSkiffActor, Pilot);
}

void AAstrawildSkiffActor::BeginPlay()
{
    Super::BeginPlay();

    if (GetLocalRole() == ROLE_Authority)
    {
        ParkedGroundZ = ProbeGroundZ();
        // Park on the terrain (spawn happens before navmesh/light passes).
        SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, ParkedGroundZ + MinHoverHeight));
    }
}

void AAstrawildSkiffActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
}

float AAstrawildSkiffActor::ProbeGroundZ() const
{
    const FVector2D XY(GetActorLocation().X, GetActorLocation().Y);
    return AAstrawildTerrainTileActor::EvalWorldHeight(XY, 1337);
}

FText AAstrawildSkiffActor::GetInteractionPrompt_Implementation() const
{
    return Pilot ? FText::FromString(TEXT("Dawn Skiff (piloted)")) : FText::FromString(TEXT("Board the Dawn Skiff [E]"));
}

void AAstrawildSkiffActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    if (GetLocalRole() != ROLE_Authority)
    {
        return; // H-9 multiplayer batch: same inline policy as portals/shop.
    }

    if (Pilot)
    {
        if (Pilot == Player)
        {
            DismountPilot();
        }
        return;
    }

    if (FVector::Dist(GetActorLocation(), Player->GetActorLocation()) > BoardRange)
    {
        return;
    }

    MountPilot(Player);
}

void AAstrawildSkiffActor::MountPilot(AAstrawildPlayerCharacter* Player)
{
    if (!Player || Pilot)
    {
        return;
    }

    Pilot = Player;
    PilotForwardAxis = PilotTurnAxis = PilotVerticalAxis = 0.0f;
    bPilotBoosting = false;

    // Seat the pilot on the hull: attached, physics movement off, capsule
    // collision off so the rider never fights the skiff's sweeps.
    Player->AttachToComponent(HullCollision, FAttachmentTransformRules::SnapToTargetIncludingScale, NAME_None);
    Player->SetActorRelativeLocation(FVector(-30.0f, 0.0f, 90.0f));
    if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
    {
        Movement->DisableMovement();
        Movement->StopMovementImmediately();
    }
    Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Player->SetPilotedSkiff(this);

    if (AAstrawildPlayerController* Controller = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        Controller->Notify(FText::FromString(TEXT("Boarded the Dawn Skiff — WASD fly, SPACE/CTRL altitude, SHIFT boost, E dismount")));
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Skiff %s boarded by %s."), *SkiffId.ToString(), *Player->GetName());
}

void AAstrawildSkiffActor::DismountPilot()
{
    AAstrawildPlayerCharacter* Player = Pilot;
    if (!Player)
    {
        return;
    }

    Pilot = nullptr;
    PilotForwardAxis = PilotTurnAxis = PilotVerticalAxis = 0.0f;
    bPilotBoosting = false;

    // Dismount beside the hull, back on foot.
    const FVector ExitLocation = GetActorLocation() + GetActorRightVector() * 220.0f + FVector(0.0f, 0.0f, 80.0f);
    Player->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Player->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Player->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    if (UCharacterMovementComponent* Movement = Player->GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
    }
    Player->SetPilotedSkiff(nullptr);

    if (AAstrawildPlayerController* Controller = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        Controller->Notify(FText::FromString(TEXT("Dismounted. The skiff will wait here.")));
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Skiff %s dismounted."), *SkiffId.ToString());
}

void AAstrawildSkiffActor::ReceivePilotMove(const float ForwardAxis, const float TurnAxis)
{
    PilotForwardAxis = FMath::Clamp(ForwardAxis, -1.0f, 1.0f);
    PilotTurnAxis = FMath::Clamp(TurnAxis, -1.0f, 1.0f);
}

void AAstrawildSkiffActor::ReceivePilotVertical(const float VerticalAxis)
{
    PilotVerticalAxis = FMath::Clamp(VerticalAxis, -1.0f, 1.0f);
}

void AAstrawildSkiffActor::ReceivePilotBoost(const bool bBoosting)
{
    bPilotBoosting = bBoosting;
}

FVector AAstrawildSkiffActor::ComputeSkiffVelocity(const FVector& Forward, const float ForwardAxis, const float VerticalAxis,
    const bool bBoosting, const float CruiseSpeed, const float BoostSpeed, const float VerticalSpeed)
{
    const float ForwardClamped = FMath::Clamp(ForwardAxis, -1.0f, 1.0f);
    const float VerticalClamped = FMath::Clamp(VerticalAxis, -1.0f, 1.0f);
    const float HorizontalSpeed = bBoosting ? BoostSpeed : CruiseSpeed;
    return Forward.GetSafeNormal() * (ForwardClamped * HorizontalSpeed) + FVector(0.0f, 0.0f, VerticalClamped * VerticalSpeed);
}

void AAstrawildSkiffActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    if (!Pilot || !IsValid(Pilot))
    {
        // Parked: zero input, nothing to simulate (cheap).
        return;
    }

    // --- Yaw ---
    if (!FMath::IsNearlyZero(PilotTurnAxis))
    {
        AddActorWorldRotation(FRotator(0.0f, PilotTurnAxis * TurnRateDegPerSecond * DeltaTime, 0.0f));
    }

    // --- Velocity (forward thrust + vertical) with altitude clamps ---
    const FVector Velocity = ComputeSkiffVelocity(
        GetActorForwardVector(), PilotForwardAxis, PilotVerticalAxis,
        bPilotBoosting, CruiseSpeed, BoostSpeed, VerticalSpeed);

    FVector Delta = Velocity * DeltaTime;

    // Altitude clamps against terrain floor and flight ceiling.
    const float GroundZ = ProbeGroundZ();
    const float MinZ = GroundZ + MinHoverHeight;
    const float MaxZ = GroundZ + MaxAltitudeAboveGround;
    const float CurrentZ = GetActorLocation().Z;
    if (CurrentZ + Delta.Z < MinZ)
    {
        Delta.Z = MinZ - CurrentZ;
    }
    else if (CurrentZ + Delta.Z > MaxZ)
    {
        Delta.Z = MaxZ - CurrentZ;
    }

    if (!Delta.IsNearlyZero())
    {
        FHitResult Hit;
        AddActorWorldOffset(Delta, true, &Hit);
        if (Hit.IsValidBlockingHit())
        {
            // Terrain/obstacle contact: kill vertical component so the skiff
            // slides along surfaces instead of tunneling (sweep handles the rest).
            PilotVerticalAxis = 0.0f;
        }
    }
}
