#include "AstrawildCraftingStationActor.h"

#include "AstrawildCore.h"
#include "AstrawildCraftingComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildCraftingStationActor::AAstrawildCraftingStationActor()
{
    PrimaryActorTick.bCanEverTick = false;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(0.8f, 0.8f, 0.6f));
    }
}

void AAstrawildCraftingStationActor::BeginPlay()
{
    Super::BeginPlay();
}

FText AAstrawildCraftingStationActor::GetInteractionPrompt_Implementation() const
{
    return FText::FromString(FString::Printf(TEXT("Craft at %s [E]"), *StationId.ToString()));
}

void AAstrawildCraftingStationActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    UAstrawildCraftingComponent* Crafting = Player ? Player->CraftingComponent : nullptr;
    const UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;

    if (!Crafting || !Registry)
    {
        return;
    }

    // Craft the first station recipe whose gates pass (vertical-slice behavior).
    // NOTE: GetAllRecipes() returns TArray<UAstrawildRecipeDefinition*> (audit C-1 —
    // the previous TPair iteration could not compile).
    for (const UAstrawildRecipeDefinition* Recipe : Registry->GetAllRecipes())
    {
        if (!Recipe || Recipe->RequiredStationId != StationId)
        {
            continue;
        }

        if (Crafting->CanCraft(Recipe))
        {
            if (Crafting->CraftRecipe(Recipe))
            {
                UE_LOG(LogAstrawildEconomy, Log, TEXT("%s crafted %s at %s."),
                    *Player->GetName(), *Recipe->RecipeId.ToString(), *StationId.ToString());
                return;
            }
        }
    }

    UE_LOG(LogAstrawildEconomy, Verbose, TEXT("No craftable recipe at %s right now."), *StationId.ToString());
}
