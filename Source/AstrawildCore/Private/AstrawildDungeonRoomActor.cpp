#include "AstrawildDungeonRoomActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildDungeonRoomActor::AAstrawildDungeonRoomActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f; // Clear-check cadence — cheap.
    bReplicates = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        // Floor plate by default — BuildRoomShell rescales per template.
        VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 0.1f));
    }
}

void AAstrawildDungeonRoomActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildDungeonRoomActor, bCleared);
}

void AAstrawildDungeonRoomActor::BeginPlay()
{
    Super::BeginPlay();
    BuildRoomShell();
}

void AAstrawildDungeonRoomActor::BuildRoomShell()
{
    // Placeholder shell (REPLACE_BEFORE_RELEASE with authored modular meshes):
    // a floor plate scaled to the template extents. Walls arrive with the asset pass.
    if (VisualMesh)
    {
        const FVector Extents = Template.HalfExtents / 50.0f; // Engine cube = 100cm.
        VisualMesh->SetWorldScale3D(FVector(Extents.X, Extents.Y, 0.1f));
        VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
    }
}

void AAstrawildDungeonRoomActor::SpawnEncounter(const TArray<FName>& CreatureDefinitionIds)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!World || !Registry || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Boss room spawns the boss definition once; other rooms cycle the pool.
    const FName SpeciesId = Template.bIsBossRoom
        ? (CreatureDefinitionIds.Num() > 0 ? CreatureDefinitionIds[0] : NAME_None)
        : CreatureDefinitionIds[RoomIndex % CreatureDefinitionIds.Num()];

    UAstrawildEchoDefinition* Definition = Registry->FindEcho(SpeciesId);
    if (!Definition)
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Dungeon room %d: unknown species %s."), RoomIndex, *SpeciesId.ToString());
        return;
    }

    const TArray<FVector>& Offsets = Template.CreatureSpawnOffsets.IsEmpty()
        ? TArray<FVector>{ FVector(200.0f, 0.0f, 120.0f) }
        : Template.CreatureSpawnOffsets;

    for (const FVector& Offset : Offsets)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Creature = World->SpawnActor<AAstrawildEchoCharacter>(
            AAstrawildEchoCharacter::StaticClass(), GetActorLocation() + Offset, FRotator::ZeroRotator, Params);
        if (Creature)
        {
            Creature->InitializeFromDefinition(Definition);
            EncounterCreatures.Add(Creature);
        }
    }
}

bool AAstrawildDungeonRoomActor::IsEncounterDefeated() const
{
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : EncounterCreatures)
    {
        const AAstrawildEchoCharacter* Creature = Weak.Get();
        if (Creature && !Creature->IsDefeated())
        {
            return false;
        }
    }
    return true;
}

void AAstrawildDungeonRoomActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority || bCleared || EncounterCreatures.IsEmpty())
    {
        return;
    }

    // Room clears when every encounter creature is defeated (directive §23).
    EncounterCreatures.RemoveAll([](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return !Weak.IsValid(); });
    if (IsEncounterDefeated())
    {
        MarkCleared();
    }
}

void AAstrawildDungeonRoomActor::MarkCleared()
{
    if (bCleared || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    bCleared = true;
    GrantClearReward();
    OnRoomCleared.Broadcast(this, RoomIndex);
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon room %d cleared."), RoomIndex);
}

void AAstrawildDungeonRoomActor::GrantClearReward()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Reward the first player with research points + event.
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_HostileDefeated, Player, TEXT("DungeonRoomCleared"), 1, GetActorLocation());
            }

            // Wave 3: rooms carrying a loot table grant it on clear (boss rooms hold the boss table).
            if (!Template.ClearLootTableId.IsNone() && Player->InventoryComponent)
            {
                if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
                {
                    if (const UAstrawildLootTableDefinition* LootTable = Registry->FindLootTable(Template.ClearLootTableId))
                    {
                        for (const FAstrawildItemStack& Drop : LootTable->GuaranteedDrops)
                        {
                            Player->InventoryComponent->AddItem(Drop.ItemId, Drop.Quantity);
                        }
                        if (LootTable->BonusRollChance > 0.0f && FMath::FRand() < LootTable->BonusRollChance && LootTable->GuaranteedDrops.Num() > 0)
                        {
                            const FAstrawildItemStack& Bonus = LootTable->GuaranteedDrops[FMath::RandRange(0, LootTable->GuaranteedDrops.Num() - 1)];
                            Player->InventoryComponent->AddItem(Bonus.ItemId, Bonus.Quantity);
                        }
                        UE_LOG(LogAstrawildEconomy, Log, TEXT("Dungeon room %d loot granted to first player (%s)."), RoomIndex, *Template.ClearLootTableId.ToString());
                    }
                }
            }
        }
    }
}
