#include "AstrawildRestPoint.h"

#include "AstrawildCore.h"

AAstrawildRestPoint::AAstrawildRestPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    WorldObjectId = FGuid::NewGuid();
}

void AAstrawildRestPoint::BeginPlay()
{
    Super::BeginPlay();

    if (!WorldObjectId.IsValid())
    {
        WorldObjectId = FGuid::NewGuid();
    }
}

void AAstrawildRestPoint::ActivateRestPoint()
{
    if (bActive)
    {
        return;
    }

    bActive = true;
    OnActivated.Broadcast(this);
}

void AAstrawildRestPoint::Interact_Implementation(AActor* InteractingActor)
{
    if (!IsValid(InteractingActor))
    {
        return;
    }
    ActivateRestPoint();
}

FText AAstrawildRestPoint::GetInteractionPrompt_Implementation() const
{
    return NSLOCTEXT("ASTRAWILD", "RestPointPrompt", "เปิดใช้งานจุดพัก");
}

FAstrawildRestPointSaveData AAstrawildRestPoint::ToSaveData() const
{
    FAstrawildRestPointSaveData Data;
    Data.WorldObjectId = WorldObjectId;
    Data.Transform = GetActorTransform();
    Data.bActive = bActive;
    return Data;
}
