#include "AstrawildCraftingStationActor.h"

#include "Net/UnrealNetwork.h" // LCP-2: DOREPLIFETIME

#include "AstrawildCore.h"
#include "AstrawildCraftingComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildCraftingStationActor::AAstrawildCraftingStationActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // LCP-2: stations replicate so LAN clients see them and route their
    // interact intent (craft screen open request) to the server copy.
    bReplicates = true;
    NetUpdateFrequency = 1.0f;

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

void AAstrawildCraftingStationActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildCraftingStationActor, StationId);
}

FText AAstrawildCraftingStationActor::GetInteractionPrompt_Implementation() const
{
    return FText::FromString(FString::Printf(TEXT("Craft at %s [E]"), *StationId.ToString()));
}

void AAstrawildCraftingStationActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);

    // Final-audit F-02: the crafting station used to auto-craft the FIRST passing
    // recipe with no player agency — while a fully implemented crafting screen
    // (recipe list, gates, timers, cancel) sat unreachable in the widget code.
    // Interact now opens that screen; the player chooses.
    if (Player)
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            if (PC->IsCraftingOpen())
            {
                PC->ToggleCraftingScreen(); // E again = close (fast back-to-game).
            }
            else
            {
                PC->ToggleCraftingScreen();
                if (PC->IsCraftingOpen())
                {
                    UE_LOG(LogAstrawildEconomy, Log, TEXT("%s opened the crafting screen at %s."),
                        *Player->GetName(), *StationId.ToString());
                }
            }
            return;
        }
    }

    // No local player controller (dedicated server path): keep the legacy
    // auto-craft fallback so automation tests / server scripts still function.
    UAstrawildCraftingComponent* Crafting = Player ? Player->CraftingComponent : nullptr;
    const UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;

    if (!Crafting || !Registry)
    {
        return;
    }

    // Craft the first station recipe whose gates pass (server fallback path).
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
