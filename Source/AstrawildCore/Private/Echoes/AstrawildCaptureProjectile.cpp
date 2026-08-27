// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildCaptureProjectile.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildCaptureComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AstrawildLogChannels.h"

AAstrawildCaptureProjectile::AAstrawildCaptureProjectile()
	: ResonatorPower(1.0f)
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	RootComponent = CollisionSphere;
	CollisionSphere->InitSphereRadius(18.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionSphere->OnComponentHit.AddDynamic(this, &AAstrawildCaptureProjectile::HandleHit);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 1600.0f;
	ProjectileMovement->MaxSpeed = 2200.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.6f;

	InitialLifeSpan = 6.0f;
}

void AAstrawildCaptureProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void AAstrawildCaptureProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	AActor* InstigatorActor = GetInstigator();
	AAstrawildEchoBase* HitEcho = Cast<AAstrawildEchoBase>(OtherActor);

	if (HitEcho && InstigatorActor)
	{
		UAstrawildCaptureComponent* CaptureComp = InstigatorActor->FindComponentByClass<UAstrawildCaptureComponent>();
		if (CaptureComp)
		{
			int32 OutShakes = 0;
			CaptureComp->AttemptCapture(HitEcho, ResonatorPower, OutShakes);
		}
	}
	else
	{
		UE_LOG(LogAstrawildEcho, Log, TEXT("Capture projectile hit world geometry: %s"), OtherActor ? *OtherActor->GetName() : TEXT("None"));
	}

	Destroy();
}