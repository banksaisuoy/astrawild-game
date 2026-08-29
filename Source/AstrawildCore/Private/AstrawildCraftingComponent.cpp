#include "AstrawildCraftingComponent.h"

#include "AstrawildCore.h"
#include "AstrawildCraftingStationActor.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildResearchSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UAstrawildCraftingComponent::UAstrawildCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildCraftingComponent::BeginPlay()
{
    Super::BeginPlay();
}

UAstrawildInventoryComponent* UAstrawildCraftingComponent::GetInventory() const
{
    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
}

UAstrawildItemRegistrySubsystem* UAstrawildCraftingComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

UAstrawildResearchSubsystem* UAstrawildCraftingComponent::GetResearch() const
{
    const UWorld* World = GetWorld();
    return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;
}

bool UAstrawildCraftingComponent::IsNearStation(const FName StationId) const
{
    if (StationId.IsNone())
    {
        return true;
    }

    const AActor* Owner = GetOwner();
    const UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        return false;
    }

    for (TActorIterator<AAstrawildCraftingStationActor> It(World); It; ++It)
    {
        const AAstrawildCraftingStationActor* Station = *It;
        if (Station && Station->StationId == StationId &&
            FVector::Dist(Station->GetActorLocation(), Owner->GetActorLocation()) <= Station->UseRadius)
        {
            return true;
        }
    }
    return false;
}

bool UAstrawildCraftingComponent::CanCraftIgnoringStation(const UAstrawildRecipeDefinition* Recipe) const
{
    if (!IsValid(Recipe) || Recipe->RecipeId.IsNone())
    {
        return false;
    }

    // Technology gate (directive §19).
    if (!Recipe->RequiredTechId.IsNone())
    {
        const UAstrawildResearchSubsystem* Research = GetResearch();
        if (!Research || !Research->IsTechUnlocked(Recipe->RequiredTechId))
        {
            return false;
        }
    }

    // Ingredient availability (directive §15).
    const UAstrawildInventoryComponent* Inventory = GetInventory();
    if (!Inventory)
    {
        return false;
    }
    for (const FAstrawildItemStack& Ingredient : Recipe->Ingredients)
    {
        if (!Inventory->HasItem(Ingredient.ItemId, Ingredient.Quantity))
        {
            return false;
        }
    }
    return true;
}

bool UAstrawildCraftingComponent::CanCraft(const UAstrawildRecipeDefinition* Recipe) const
{
    if (!CanCraftIgnoringStation(Recipe))
    {
        return false;
    }

    // Station proximity gate.
    return IsNearStation(Recipe->RequiredStationId);
}

bool UAstrawildCraftingComponent::CraftRecipe(const UAstrawildRecipeDefinition* Recipe)
{
    if (GetOwnerRole() != ROLE_Authority || !CanCraft(Recipe) || IsCrafting())
    {
        return false;
    }

    UAstrawildInventoryComponent* Inventory = GetInventory();
    if (!Inventory || !Inventory->ConsumeItems(Recipe->Ingredients))
    {
        return false;
    }

    if (Recipe->CraftDurationSeconds <= 0.0f)
    {
        // Instant craft.
        for (const FAstrawildItemStack& Output : Recipe->Outputs)
        {
            Inventory->AddItem(Output.ItemId, Output.Quantity);
        }
        OnCraftCompleted.Broadcast(Recipe->RecipeId, true);
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_RecipeCrafted, GetOwner(), Recipe->RecipeId, 1, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
            }
        }
        return true;
    }

    // Timed craft queue (directive §15 craft time).
    ActiveRecipeId = Recipe->RecipeId;
    CraftTimeTotal = Recipe->CraftDurationSeconds;
    CraftTimeRemaining = CraftTimeTotal;
    PendingOutputs = Recipe->Outputs;
    return true;
}

bool UAstrawildCraftingComponent::CraftByRecipeId(const FName RecipeId)
{
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildRecipeDefinition* Recipe = Registry ? Registry->FindRecipe(RecipeId) : nullptr;
    return Recipe ? CraftRecipe(Recipe) : false;
}

void UAstrawildCraftingComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwnerRole() != ROLE_Authority || !IsCrafting())
    {
        return;
    }

    CraftTimeRemaining -= DeltaTime;
    OnCraftProgress.Broadcast(ActiveRecipeId, 1.0f - FMath::Clamp(CraftTimeRemaining / FMath::Max(0.01f, CraftTimeTotal), 0.0f, 1.0f));

    if (CraftTimeRemaining <= 0.0f)
    {
        CompleteActiveCraft();
    }
}

void UAstrawildCraftingComponent::CompleteActiveCraft()
{
    UAstrawildInventoryComponent* Inventory = GetInventory();
    const FName CompletedRecipe = ActiveRecipeId;

    if (Inventory)
    {
        for (const FAstrawildItemStack& Output : PendingOutputs)
        {
            Inventory->AddItem(Output.ItemId, Output.Quantity);
        }
    }

    ActiveRecipeId = NAME_None;
    PendingOutputs.Reset();
    CraftTimeRemaining = 0.0f;
    CraftTimeTotal = 0.0f;

    OnCraftCompleted.Broadcast(CompletedRecipe, true);
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Craft completed: %s."), *CompletedRecipe.ToString());

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_RecipeCrafted, GetOwner(), CompletedRecipe, 1, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
        }
    }
}
