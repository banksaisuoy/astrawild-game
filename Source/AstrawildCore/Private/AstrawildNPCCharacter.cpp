#include "AstrawildNPCCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildNPCAIController.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildVillageActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NavigationInvokerComponent.h"
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

    // Batch 8 — living villages: the NPC brain (patrol / guard duty / campfire nights).
    AIControllerClass = AAstrawildNPCAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Navmesh anchor — runtime tiles generate around each villager (audit C-3 pattern).
    UNavigationInvokerComponent* NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
    NavInvoker->SetGenerationRadii(4000.0f, 6000.0f);

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(CylinderMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.4f, 0.4f, 0.9f));
    }

    // Batch 8 — head + role hat silhouette pieces.
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(GetCapsuleComponent());
    HeadMesh->SetCollisionProfileName(TEXT("NoCollision"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereMesh.Object);
        HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 78.0f));
        HeadMesh->SetRelativeScale3D(FVector(0.22f));
    }

    HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
    HatMesh->SetupAttachment(HeadMesh);
    HatMesh->SetCollisionProfileName(TEXT("NoCollision"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeMesh.Succeeded())
    {
        HatMesh->SetStaticMesh(ConeMesh.Object);
        HatMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 26.0f));
        HatMesh->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.22f));
    }

    // Batch 8 — role-colored lantern (subtle, short radius — identity, not a lamp post).
    RoleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RoleLight"));
    RoleLight->SetupAttachment(GetCapsuleComponent());
    RoleLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    RoleLight->SetIntensity(0.8f);
    RoleLight->SetAttenuationRadius(420.0f);
    RoleLight->SetLightColor(FLinearColor(1.0f, 0.85f, 0.6f));
}

void AAstrawildNPCCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Editor-placed NPCs may already carry a definition; runtime spawns refresh
    // explicitly after the bootstrapper assigns NpcDefinition.
    RefreshAppearanceFromDefinition();
}

bool AAstrawildNPCCharacter::IsGuard() const
{
    return IsValid(NpcDefinition) && NpcDefinition->Role == EAstrawildNPCRole::Guard;
}

int32 AAstrawildNPCCharacter::AdvancePatrolIndex()
{
    const int32 Modulus = (HomeVillage && HomeVillage->GetWaypointCount() > 0) ? HomeVillage->GetWaypointCount() : 6;
    PatrolIndex = ((PatrolIndex + 1) % Modulus + Modulus) % Modulus;
    return PatrolIndex;
}

void AAstrawildNPCCharacter::RefreshAppearanceFromDefinition()
{
    if (!IsValid(NpcDefinition))
    {
        return;
    }

    // Role identity: lantern color + silhouette proportions.
    FLinearColor RoleColor(1.0f, 0.85f, 0.6f);
    FVector BodyScale(0.4f, 0.4f, 0.9f);
    switch (NpcDefinition->Role)
    {
    case EAstrawildNPCRole::Guard:
        RoleColor = FLinearColor(0.45f, 0.75f, 1.0f);
        BodyScale = FVector(0.48f, 0.48f, 1.0f); // bulkier
        break;
    case EAstrawildNPCRole::Vendor:
        RoleColor = FLinearColor(1.0f, 0.8f, 0.3f);
        BodyScale = FVector(0.44f, 0.44f, 0.85f);
        break;
    case EAstrawildNPCRole::Elder:
        RoleColor = FLinearColor(0.8f, 0.55f, 1.0f);
        BodyScale = FVector(0.36f, 0.36f, 1.05f); // taller, thinner
        break;
    case EAstrawildNPCRole::QuestGiver:
        RoleColor = FLinearColor(0.4f, 1.0f, 0.7f);
        break;
    default:
        // Blend the definition tint into the lantern so villagers still differ.
        RoleColor = FMath::Lerp(RoleColor, NpcDefinition->PrimaryTint, 0.5f);
        break;
    }

    if (RoleLight)
    {
        RoleLight->SetLightColor(RoleColor);
    }
    if (PlaceholderMesh)
    {
        PlaceholderMesh->SetRelativeScale3D(BodyScale);
    }
    if (NpcDefinition->Role == EAstrawildNPCRole::Guard && HatMesh)
    {
        HatMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.28f)); // guard helm crest
    }
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

    // Batch 8 — conversation bookkeeping: the AI pauses and faces the player.
    if (UWorld* World = GetWorld())
    {
        LastInteractedTime = World->GetTimeSeconds();
    }
    LastInteractedActor = Player;

    // GDP-4: talking builds the relationship (+2, once per in-world day).
    AddAffinity(2.0f);

    // Production V2 Batch 3 — when the NPC has a dialogue tree, the conversation
    // screen takes over the whole interaction: quest offers migrate into choice
    // consequences (StartQuestId) and vendor hand-off happens via bOpenShop, so
    // both legacy paths below are skipped. NPCs without a tree keep the direct
    // quest-toast + shop behavior.
    if (NpcDefinition && !NpcDefinition->DialogueTreeId.IsNone())
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            UAstrawildItemRegistrySubsystem* Registry = GetWorld()
                ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>()
                : nullptr;
            if (Registry && Registry->FindDialogueTree(NpcDefinition->DialogueTreeId))
            {
                AstrawildPC->OpenDialogue(this);
                return;
            }
            UE_LOG(LogAstrawild, Warning, TEXT("NPC %s references unregistered dialogue tree %s."),
                *NpcDefinition->NpcId.ToString(), *NpcDefinition->DialogueTreeId.ToString());
        }
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
    // GDP-4: Confidants get up to 15% off — the relationship literally pays.
    const int32 TotalCost = FMath::Max(1,
        FMath::FloorToInt32(WareDef->VendorPrice * Quantity * (1.0f - GetVendorDiscountFraction())));
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

    // GDP-4: trading deepens the bond (+1, shared once-per-day gate with talking).
    AddAffinity(1.0f);

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

// ===========================================================================
// GDP-4 — NPC affinity
// ===========================================================================

int32 AAstrawildNPCCharacter::GetCurrentWorldDay() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const UAstrawildTimeSubsystem* Time = World->GetSubsystem<UAstrawildTimeSubsystem>())
        {
            return Time->GetCurrentDay();
        }
    }
    return 0;
}

FName AAstrawildNPCCharacter::GetStableNPCId() const
{
    return NpcDefinition ? NpcDefinition->NpcId : NAME_None;
}

int32 AAstrawildNPCCharacter::GetAffinityTier() const
{
    if (Affinity >= 75.0f) return 3; // Confidant
    if (Affinity >= 50.0f) return 2; // Friend
    if (Affinity >= 25.0f) return 1; // Acquaintance
    return 0;                        // Stranger
}

FText AAstrawildNPCCharacter::GetAffinityTierTitle() const
{
    switch (GetAffinityTier())
    {
    case 3: return FText::FromString(TEXT("Confidant"));
    case 2: return FText::FromString(TEXT("Friend"));
    case 1: return FText::FromString(TEXT("Acquaintance"));
    default: return FText::FromString(TEXT("Stranger"));
    }
}

float AAstrawildNPCCharacter::GetVendorDiscountFraction() const
{
    // 5% per tier above Stranger — the relationship literally pays for itself.
    return GetAffinityTier() * 0.05f;
}

void AAstrawildNPCCharacter::AddAffinity(const float Amount)
{
    if (GetLocalRole() != ROLE_Authority || Amount <= 0.0f)
    {
        return;
    }

    // Once-per-in-world-day gate (same cadence as vendor stock restock).
    const int32 Today = GetCurrentWorldDay();
    if (LastAffinityGainDay == Today)
    {
        return;
    }
    LastAffinityGainDay = Today;

    const float Before = Affinity;
    Affinity = FMath::Clamp(Affinity + Amount, 0.0f, 100.0f);

    const int32 TierBefore = FMath::Clamp(static_cast<int32>(Before / 25.0f), 0, 3);
    const int32 TierAfter = GetAffinityTier();
    if (TierAfter > TierBefore)
    {
        UE_LOG(LogAstrawild, Log, TEXT("NPC %s affinity tier up: %s (%.0f)."),
            *GetStableNPCId().ToString(), *GetAffinityTierTitle().ToString(), Affinity);
    }
}
