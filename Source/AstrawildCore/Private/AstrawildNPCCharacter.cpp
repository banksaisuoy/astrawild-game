#include "AstrawildNPCCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildQuestComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    /** Trade range for vendor transactions (interaction reach is 300 cm — this
     *  adds capsule-to-capsule grace so talking and buying feel the same). */
    constexpr float VendorTradeRangeCm = 450.0f;
}

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

    // Batch 5 — Item C: vendor interaction now opens the real shop screen
    // (pure-C++ UMG — wares, prices, balance, buy/sell buttons, close). The
    // toast-only listing from Batch 4 is retired; transactions still run
    // through the same server-authoritative TryPurchase/TrySell pipeline.
    if (NpcDefinition && !NpcDefinition->ShopLootTableId.IsNone() && !NpcDefinition->CurrencyItemId.IsNone())
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            PC->OpenShop(this);
        }
    }
}

int32 AAstrawildNPCCharacter::ComputeVendorSellValue(const int32 VendorPrice)
{
    // Half the buy price, floored at 1 for anything tradeable; 0 stays 0 (not
    // sellable). Buy-low/sell-higher exploits are impossible by construction.
    return VendorPrice > 0 ? FMath::Max(1, VendorPrice / 2) : 0;
}

EAstrawildVendorResult AAstrawildNPCCharacter::TryPurchase(AActor* Purchaser, const FName ItemId, const int32 Quantity)
{
    // Server-authoritative only — single-player/listen-server interact and cheat
    // paths both land here; a future dedicated-client shop UI must route through
    // a Server RPC before calling this.
    if (GetLocalRole() != ROLE_Authority)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(Purchaser);
    UAstrawildInventoryComponent* Inventory = Player ? Player->InventoryComponent : nullptr;
    if (!Player || !Inventory || Quantity < 1 || Quantity > 99)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    if (!NpcDefinition || NpcDefinition->ShopLootTableId.IsNone() || NpcDefinition->CurrencyItemId.IsNone())
    {
        return EAstrawildVendorResult::NotAVendor;
    }

    // Range guard — must be near the vendor (cheat callers are range-checked too).
    if (FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(VendorTradeRangeCm))
    {
        return EAstrawildVendorResult::TooFarAway;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    const UAstrawildLootTableDefinition* Shop = Registry->FindLootTable(NpcDefinition->ShopLootTableId);
    const UAstrawildItemDefinition* WareDef = Registry->FindItem(ItemId);
    const UAstrawildItemDefinition* CurrencyDef = Registry->FindItem(NpcDefinition->CurrencyItemId);
    if (!Shop || !CurrencyDef || !WareDef || WareDef->VendorPrice <= 0)
    {
        return EAstrawildVendorResult::NotAWare;
    }

    // Ware membership — the item must be listed in the vendor's shop table (and
    // the currency itself can never be a ware because its VendorPrice is 0).
    const bool bIsWare = Shop->GuaranteedDrops.ContainsByPredicate(
        [ItemId](const FAstrawildItemStack& Stack) { return Stack.ItemId == ItemId; });
    if (!bIsWare)
    {
        return EAstrawildVendorResult::NotAWare;
    }

    // Funds + weight — validated before anything moves (no partial transactions).
    const int32 TotalCost = WareDef->VendorPrice * Quantity;
    if (!Inventory->HasItem(NpcDefinition->CurrencyItemId, TotalCost))
    {
        return EAstrawildVendorResult::NotEnoughCurrency;
    }
    if (!Inventory->CanAddItem(ItemId, Quantity))
    {
        return EAstrawildVendorResult::TooHeavy;
    }

    // Execute: currency out, ware in (silent adds — the caller notifies once).
    Inventory->RemoveItem(NpcDefinition->CurrencyItemId, TotalCost);
    Inventory->AddItemSilent(ItemId, Quantity);

    UE_LOG(LogAstrawildEconomy, Log,
        TEXT("Vendor %s sold %d x %s to %s for %d %s."),
        *NpcDefinition->NpcId.ToString(), Quantity, *ItemId.ToString(),
        *Player->GetName(), TotalCost, *NpcDefinition->CurrencyItemId.ToString());

    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        const FString Message = FString::Printf(TEXT("Bought %d x %s for %d %s."),
            Quantity, *WareDef->DisplayName.ToString(), TotalCost,
            *CurrencyDef->DisplayName.ToString());
        PC->Notify(FText::FromString(Message));
    }

    return EAstrawildVendorResult::Success;
}

EAstrawildVendorResult AAstrawildNPCCharacter::TrySell(AActor* Seller, const FName ItemId, const int32 Quantity)
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(Seller);
    UAstrawildInventoryComponent* Inventory = Player ? Player->InventoryComponent : nullptr;
    if (!Player || !Inventory || Quantity < 1 || Quantity > 99)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    if (!NpcDefinition || NpcDefinition->CurrencyItemId.IsNone())
    {
        return EAstrawildVendorResult::NotAVendor;
    }

    if (FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(VendorTradeRangeCm))
    {
        return EAstrawildVendorResult::TooFarAway;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    const UAstrawildItemDefinition* WareDef = Registry->FindItem(ItemId);
    const UAstrawildItemDefinition* CurrencyDef = Registry->FindItem(NpcDefinition->CurrencyItemId);
    // Only priced items are sellable — junk with VendorPrice 0 cannot be minted
    // into currency, and the currency itself is never sellable back to the vendor.
    if (!CurrencyDef || !WareDef || WareDef->VendorPrice <= 0 || ItemId == NpcDefinition->CurrencyItemId)
    {
        return EAstrawildVendorResult::NotAWare;
    }

    if (!Inventory->HasItem(ItemId, Quantity))
    {
        return EAstrawildVendorResult::InvalidRequest;
    }

    const int32 TotalValue = ComputeVendorSellValue(WareDef->VendorPrice) * Quantity;
    if (!Inventory->CanAddItem(NpcDefinition->CurrencyItemId, TotalValue))
    {
        return EAstrawildVendorResult::TooHeavy;
    }

    Inventory->RemoveItem(ItemId, Quantity);
    Inventory->AddItemSilent(NpcDefinition->CurrencyItemId, TotalValue);

    UE_LOG(LogAstrawildEconomy, Log,
        TEXT("Vendor %s bought %d x %s from %s for %d %s."),
        *NpcDefinition->NpcId.ToString(), Quantity, *ItemId.ToString(),
        *Player->GetName(), TotalValue, *NpcDefinition->CurrencyItemId.ToString());

    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
    {
        const FString Message = FString::Printf(TEXT("Sold %d x %s for %d %s."),
            Quantity, *WareDef->DisplayName.ToString(), TotalValue,
            *CurrencyDef->DisplayName.ToString());
        PC->Notify(FText::FromString(Message));
    }

    return EAstrawildVendorResult::Success;
}
