#include "AstrawildResonancePillarActor.h"

#include "AstrawildDungeonRoomActor.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    /** Dormant stone vs attuned resonance tints (placeholder shells read at gameplay distance). */
    const FLinearColor DormantPillarTint(0.35f, 0.32f, 0.38f, 1.0f);
    const FLinearColor AttunedPillarTint(0.30f, 0.85f, 1.0f, 1.0f);
}

AAstrawildResonancePillarActor::AAstrawildResonancePillarActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Tall resonance stone (engine cylinder — the same placeholder vocabulary
    // as the gate pillars; REPLACE_BEFORE_RELEASE). Actor origin sits at the
    // pillar BASE so room placement is a plain floor-level offset.
    PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
    RootComponent = PillarMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        PillarMesh->SetStaticMesh(CylinderMesh.Object);
        // 240cm tall, slim: traceable + solid (the interaction trace needs collision).
        PillarMesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 2.4f));
        PillarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f)); // engine cylinder is centered — lift to sit on the base
        PillarMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PillarMesh->SetCollisionResponseToAllChannels(ECR_Block);
    }
}

void AAstrawildResonancePillarActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildResonancePillarActor, bActivated);
}

FText AAstrawildResonancePillarActor::GetInteractionPrompt_Implementation() const
{
    // The numeral carries the required order — the time window and the guard
    // are the challenge, not a blind guessing game.
    const TCHAR* Numerals[9] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX") };
    const int32 SafeIndex = FMath::Clamp(PillarIndex, 0, 8);
    if (bActivated)
    {
        return FText::FromString(FString::Printf(TEXT("The pillar hums in resonance (%s)"), Numerals[SafeIndex]));
    }
    return FText::FromString(FString::Printf(TEXT("Attune resonance pillar %s [E]"), Numerals[SafeIndex]));
}

void AAstrawildResonancePillarActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    if (GetLocalRole() != ROLE_Authority)
    {
        // Dedicated-client routing arrives with the H-9 multiplayer batch (same
        // policy as the dungeon portals — single-player/listen-server inline).
        return;
    }

    // The owning room owns the sequence state machine (pure verbs, tested).
    if (AAstrawildDungeonRoomActor* Room = OwningRoom.Get())
    {
        Room->NotifyPillarInteracted(PillarIndex);
    }
}

void AAstrawildResonancePillarActor::SetActivated(const bool bNewActivated)
{
    if (GetLocalRole() != ROLE_Authority || bActivated == bNewActivated)
    {
        return;
    }

    bActivated = bNewActivated;
    ApplyActivationVisual(); // server-side now; clients re-apply in OnRep
}

void AAstrawildResonancePillarActor::OnRep_bActivated()
{
    ApplyActivationVisual();
}

void AAstrawildResonancePillarActor::ApplyActivationVisual()
{
    if (!PillarMesh || !PillarMesh->GetStaticMesh())
    {
        return;
    }
    if (!PillarMesh->GetMaterial(0))
    {
        return; // ResourceNode tint idiom — skip when the mesh carries no material
    }

    if (UMaterialInstanceDynamic* DynMaterial = PillarMesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        DynMaterial->SetVectorParameterValue(TEXT("Color"), bActivated ? AttunedPillarTint : DormantPillarTint);
    }
}
