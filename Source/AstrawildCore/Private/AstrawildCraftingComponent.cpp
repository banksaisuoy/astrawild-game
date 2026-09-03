#include "AstrawildCraftingComponent.h"

#include "AstrawildAttributeComponent.h"
#include "AstrawildCore.h"
#include "AstrawildCraftingStationActor.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
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
    if (!Inventory)
    {
        return false;
    }

    // H-11 fix (Production V2): pre-flight the OUTPUT weight BEFORE consuming
    // ingredients — a craft whose results cannot fit never starts, instead of
    // consuming materials and silently losing the outputs at completion.
    if (!Inventory->CanAddItemStacks(Recipe->Outputs))
    {
        UE_LOG(LogAstrawildEconomy, Warning,
            TEXT("Craft refused (outputs would exceed carry weight): %s — free pack space first."),
            *Recipe->RecipeId.ToString());
        return false;
    }

    if (!Inventory->ConsumeItems(Recipe->Ingredients))
    {
        return false;
    }

    // GDP-3: Masterwork (Craft 5+) — 15% chance the craft was so clean the
    // station refunds the full ingredient set on completion. The roll happens
    // at consume time so the outcome is fixed even for timed crafts.
    bool bMasterworkRefund = false;
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (Player->AttributeComponent &&
            Player->AttributeComponent->GetMasterworkRefundChance() > 0.0f &&
            FMath::FRandRange(0.0f, 1.0f) < Player->AttributeComponent->GetMasterworkRefundChance())
        {
            bMasterworkRefund = true;
        }
    }

    if (Recipe->CraftDurationSeconds <= 0.0f)
    {
        // Instant craft.
        for (const FAstrawildItemStack& Output : Recipe->Outputs)
        {
            Inventory->AddItem(Output.ItemId, Output.Quantity);
        }
        if (bMasterworkRefund)
        {
            for (const FAstrawildItemStack& Ingredient : Recipe->Ingredients)
            {
                Inventory->AddItem(Ingredient.ItemId, Ingredient.Quantity);
            }
            UE_LOG(LogAstrawildEconomy, Log, TEXT("Masterwork! Ingredients refunded: %s"), *Recipe->RecipeId.ToString());
        }
        OnCraftCompleted.Broadcast(Recipe->RecipeId, true);
        if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
        {
            if (Player->AttributeComponent)
            {
                Player->AttributeComponent->AddAttributeXP(EAstrawildAttributeType::Craft, 8.0f);
            }
        }
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_RecipeCrafted, GetOwner(), Recipe->RecipeId, 1, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
            }
        }
        return true;
    }

    // Timed craft queue (directive §15 craft time). GDP-3: Craft attribute
    // shaves real seconds off (1 + 4% per level above 1; floor 25% of base).
    float CraftSeconds = Recipe->CraftDurationSeconds;
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (Player->AttributeComponent)
        {
            CraftSeconds = FMath::Max(Recipe->CraftDurationSeconds * 0.25f,
                Recipe->CraftDurationSeconds / Player->AttributeComponent->GetCraftSpeedMultiplier());
        }
    }
    ActiveRecipeId = Recipe->RecipeId;
    CraftTimeTotal = CraftSeconds;
    CraftTimeRemaining = CraftTimeTotal;
    PendingOutputs = Recipe->Outputs;
    bMasterworkPendingRefund = bMasterworkRefund;
    PendingRefundInputs = bMasterworkRefund ? Recipe->Ingredients : TArray<FAstrawildItemStack>();
    OnCraftStarted.Broadcast(Recipe->RecipeId, CraftSeconds);
    return true;
}

// Audit C-2 (final run): server RPC bodies must be named *_Implementation — the previous
// definitions used the declared RPC name, which collides with the UHT-generated thunk
// (C2084) and leaves the virtual _Implementation undefined (LNK2001).
void UAstrawildCraftingComponent::ServerRequestCraft_Implementation(const FName RecipeId)
{
    CraftByRecipeId(RecipeId);
}

void UAstrawildCraftingComponent::ServerRequestCancelCraft_Implementation()
{
    CancelActiveCraft();
}

bool UAstrawildCraftingComponent::CancelActiveCraft()
{
    if (GetOwnerRole() != ROLE_Authority || !IsCrafting())
    {
        return false;
    }

    // H-11 guard: once outputs are (partially) granted and only the held
    // remainder awaits pack space, cancelling would refund the ingredients
    // AND keep granted outputs — free items. Refuse: the craft is done cooking,
    // it just needs space.
    if (bOutputsPendingHandoff)
    {
        UE_LOG(LogAstrawildEconomy, Warning,
            TEXT("Cancel refused (outputs waiting for pack space): %s — free inventory space to finish."),
            *ActiveRecipeId.ToString());
        return false;
    }

    const FName CancelledRecipe = ActiveRecipeId;
    bool bRefunded = false;

    // Refund ingredients from the recipe definition (server-authoritative).
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildRecipeDefinition* Recipe = Registry ? Registry->FindRecipe(CancelledRecipe) : nullptr;
    UAstrawildInventoryComponent* Inventory = GetInventory();
    if (Recipe && Inventory)
    {
        for (const FAstrawildItemStack& Ingredient : Recipe->Ingredients)
        {
            Inventory->AddItem(Ingredient.ItemId, Ingredient.Quantity);
        }
        bRefunded = true;
    }

    ActiveRecipeId = NAME_None;
    PendingOutputs.Reset();
    CraftTimeRemaining = 0.0f;
    CraftTimeTotal = 0.0f;

    OnCraftCancelled.Broadcast(CancelledRecipe, bRefunded);
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Craft cancelled: %s (refunded: %s)."), *CancelledRecipe.ToString(), bRefunded ? TEXT("yes") : TEXT("no"));
    return true;
}

float UAstrawildCraftingComponent::GetCraftingProgress() const
{
    if (!IsCrafting() || CraftTimeTotal <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(1.0f - CraftTimeRemaining / CraftTimeTotal, 0.0f, 1.0f);
}

TArray<UAstrawildRecipeDefinition*> UAstrawildCraftingComponent::GetTechUnlockedRecipes() const
{
    TArray<UAstrawildRecipeDefinition*> Result;

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return Result;
    }

    UAstrawildResearchSubsystem* Research = GetResearch();
    for (UAstrawildRecipeDefinition* Recipe : Registry->GetAllRecipes())
    {
        if (!Recipe)
        {
            continue;
        }
        if (Recipe->RequiredTechId.IsNone() || (Research && Research->IsTechUnlocked(Recipe->RequiredTechId)))
        {
            Result.Add(Recipe);
        }
    }
    return Result;
}

TArray<FName> UAstrawildCraftingComponent::GetNearbyStationIds() const
{
    TArray<FName> Result;

    const AActor* Owner = GetOwner();
    const UWorld* World = GetWorld();
    if (!Owner || !World)
    {
        return Result;
    }

    for (TActorIterator<AAstrawildCraftingStationActor> It(World); It; ++It)
    {
        const AAstrawildCraftingStationActor* Station = *It;
        if (Station && !Station->StationId.IsNone() &&
            FVector::Dist(Station->GetActorLocation(), Owner->GetActorLocation()) <= Station->UseRadius)
        {
            Result.AddUnique(Station->StationId);
        }
    }
    return Result;
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

    // H-11 fix (Production V2): weight can shift while a timed craft runs
    // (equipment swaps mid-craft). If the pack can no longer absorb the
    // outputs, HOLD them and retry every second — outputs are never silently
    // dropped, and the craft completes the moment space frees up.
    if (Inventory)
    {
        // Grant outputs front-to-back; each SUCCESSFUL stack is removed from
        // the pending list so a later retry never double-grants it.
        while (PendingOutputs.Num() > 0)
        {
            const FAstrawildItemStack& Output = PendingOutputs[0];
            if (!Inventory->AddItem(Output.ItemId, Output.Quantity))
            {
                break;
            }
            PendingOutputs.RemoveAt(0);
        }
        if (PendingOutputs.Num() > 0)
        {
            // Remainder held — retry on the existing 1s cadence. The player
            // sees "waiting for pack space" instead of losing items.
            bOutputsPendingHandoff = true;
            CraftTimeRemaining = 1.0f;
            CraftTimeTotal = FMath::Max(CraftTimeTotal, 1.0f);
            UE_LOG(LogAstrawildEconomy, Warning,
                TEXT("Craft outputs held (carry weight full): %s — %d stack(s) pending, will retry until space frees."),
                *CompletedRecipe.ToString(), PendingOutputs.Num());
            return;
        }
    }

    bOutputsPendingHandoff = false;
    ActiveRecipeId = NAME_None;
    PendingOutputs.Reset();

    // GDP-3: Masterwork refund + Craft XP on timed-craft completion.
    if (bMasterworkPendingRefund && Inventory)
    {
        for (const FAstrawildItemStack& Refund : PendingRefundInputs)
        {
            Inventory->AddItem(Refund.ItemId, Refund.Quantity);
        }
        UE_LOG(LogAstrawildEconomy, Log, TEXT("Masterwork! Ingredients refunded: %s"), *CompletedRecipe.ToString());
    }
    bMasterworkPendingRefund = false;
    PendingRefundInputs.Reset();

    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (Player->AttributeComponent)
        {
            Player->AttributeComponent->AddAttributeXP(EAstrawildAttributeType::Craft, 8.0f);
        }
    }

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
