#include "AstrawildNPCCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildNPCCharacter::AAstrawildNPCCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(40.0f, 90.0f);

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Capsule.Capsule"));
    if (CapsuleMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(CapsuleMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.4f, 0.4f, 0.9f));
    }
}

void AAstrawildNPCCharacter::BeginPlay()
{
    Super::BeginPlay();
}

FText AAstrawildNPCCharacter::GetInteractionPrompt_Implementation() const
{
    if (NpcDefinition)
    {
        return FText::FromString(FString::Printf(TEXT("Talk to %s [E]"), *NpcDefinition->DisplayName.ToString()));
    }
    return FText::FromString(TEXT("Talk [E]"));
}

void AAstrawildNPCCharacter::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    // Offer the quest attached to this NPC (directive §25/§26).
    if (NpcDefinition && !NpcDefinition->OfferedQuestId.IsNone())
    {
        if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
        {
            if (UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
            {
                Quests->StartQuest(NpcDefinition->OfferedQuestId);
                UE_LOG(LogAstrawild, Log, TEXT("NPC offered quest %s."), *NpcDefinition->OfferedQuestId.ToString());
            }
        }
    }
}
