#include "AstrawildWorkSiteActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildPowerSubsystem.h"
#include "AstrawildUtilityRobotActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildWorkSiteActor::AAstrawildWorkSiteActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 0.4f));
    }
}

void AAstrawildWorkSiteActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildWorkSiteActor, StoredOutput);
    DOREPLIFETIME(AAstrawildWorkSiteActor, InputBuffer);
}

void AAstrawildWorkSiteActor::BeginPlay()
{
    Super::BeginPlay();
}

UAstrawildPowerSubsystem* AAstrawildWorkSiteActor::GetPower() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildPowerSubsystem>() : nullptr;
}

bool AAstrawildWorkSiteActor::IsPowered() const
{
    if (!bRequiresPower)
    {
        return true;
    }
    const UAstrawildPowerSubsystem* Power = GetPower();
    return Power && Power->IsLocationPowered(GetActorLocation());
}

void AAstrawildWorkSiteActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetLocalRole() != ROLE_Authority || OutputItemId.IsNone())
    {
        return;
    }

    // Drop stale workers.
    Workers.RemoveAll([](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return !Weak.IsValid(); });

    // Final production run: a utility robot mans the site on its own (PHASE 12 —
    // the automation loop no longer strictly requires captured Echoes).
    const bool bRobotPresent = AssignedRobot.IsValid();
    if (Workers.IsEmpty() && !bRobotPresent)
    {
        return;
    }

    const bool bPowered = IsPowered();
    const float PowerMultiplier = bPowered ? 1.5f : (bRequiresPower ? 0.0f : 1.0f);
    if (PowerMultiplier <= 0.0f)
    {
        return;
    }

    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : Workers)
    {
        AAstrawildEchoCharacter* Echo = Weak.Get();
        if (!Echo || !IsValid(Echo->EchoDefinition) || Echo->IsDefeated())
        {
            continue;
        }

        // Final-audit M-9: workers produce only while AT the site (2× the work
        // range — ExecuteWork paths them to the site, so present workers are the
        // norm). Previously an echo "worked" from across the map, and an echo that
        // refused the Work command (obedience roll) still produced at full rate.
        if (FVector::Dist(Echo->GetActorLocation(), GetActorLocation()) > (WorkRange * 2.0f))
        {
            continue;
        }

        // Affinity: species-specific (directive §18) or 0.5 baseline when unlisted.
        float Affinity = 0.5f;
        for (const FAstrawildWorkAffinity& WA : Echo->EchoDefinition->WorkAffinities)
        {
            if (WA.WorkType == WorkType)
            {
                Affinity = WA.Affinity;
                break;
            }
        }

        // Personality + needs modifiers (directive §5/§18).
        const float PersonalityMult = Echo->GetWorkSpeedMultiplier();
        const float MoodMult = FMath::Lerp(0.5f, 1.0f, Echo->Needs.Mood / 100.0f);
        const float EnergyMult = FMath::Lerp(0.5f, 1.0f, Echo->Needs.Energy / 100.0f);

        // SCP Phase 9: sanity band + illness penalties (Depressed x0.6,
        // Slacker x0.3) — base care now has a mechanical payoff.
        const float SanityMult = Echo->SanityComponent ? Echo->SanityComponent->GetWorkOutputMultiplier() : 1.0f;

        WorkAccumulator += DeltaTime * Affinity * PersonalityMult * MoodMult * EnergyMult * SanityMult * PowerMultiplier;

        // Working consumes energy.
        Echo->Needs.Energy = FMath::Max(0.0f, Echo->Needs.Energy - DeltaTime * 0.5f);
    }

    // Final production run + Production V2: robots work at a flat rate (no needs
    // decay — that is their entire niche — but the power gate above still applies).
    // Specialist chassis (mining/farming/defense) resolve their per-site rate from
    // their robot definition; general-purpose frames keep the site's legacy rate.
    if (const AAstrawildUtilityRobotActor* Robot = AssignedRobot.Get())
    {
        const float RobotRate = Robot->GetWorkRateFor(WorkType);
        WorkAccumulator += DeltaTime * RobotRate * PowerMultiplier;
    }

    while (WorkAccumulator >= SecondsPerOutput)
    {
        // Production V2 (Master Plan §7): consume→produce. Definition-driven sites
        // burn their inputs per cycle — an empty buffer stalls the accumulator at
        // the threshold so work resumes the instant inputs arrive (no free cycles).
        if (!ConsumeCycleInputs())
        {
            WorkAccumulator = SecondsPerOutput;
            break;
        }
        WorkAccumulator -= SecondsPerOutput;
        StoredOutput += FMath::Max(1, OutputQuantity);
        OnWorkProduced.Broadcast(this, OutputItemId, StoredOutput);
    }
}

bool AAstrawildWorkSiteActor::ConsumeCycleInputs()
{
    if (InputItems.IsEmpty())
    {
        return true; // Harvest-from-the-land site: no inputs needed.
    }
    for (const FAstrawildItemStack& Required : InputItems)
    {
        const int32 Have = BufferQuantity(Required.ItemId);
        if (Have < Required.Quantity)
        {
            return false;
        }
    }
    for (const FAstrawildItemStack& Required : InputItems)
    {
        RemoveBufferedQuantity(Required.ItemId, Required.Quantity);
    }
    return true;
}

int32 AAstrawildWorkSiteActor::BufferQuantity(const FName ItemId) const
{
    for (const FAstrawildItemStack& Stack : InputBuffer)
    {
        if (Stack.ItemId == ItemId)
        {
            return Stack.Quantity;
        }
    }
    return 0;
}

void AAstrawildWorkSiteActor::RemoveBufferedQuantity(const FName ItemId, const int32 Quantity)
{
    for (FAstrawildItemStack& Stack : InputBuffer)
    {
        if (Stack.ItemId == ItemId)
        {
            Stack.Quantity = FMath::Max(0, Stack.Quantity - Quantity);
        }
    }
    InputBuffer.RemoveAll([](const FAstrawildItemStack& Stack) { return Stack.Quantity <= 0; });
}

int32 AAstrawildWorkSiteActor::GetBufferedCycleCount() const
{
    if (InputItems.IsEmpty())
    {
        return 0;
    }
    int32 Cycles = TNumericLimits<int32>::Max();
    for (const FAstrawildItemStack& Required : InputItems)
    {
        const int32 Have = BufferQuantity(Required.ItemId);
        const int32 Needed = FMath::Max(1, Required.Quantity);
        Cycles = FMath::Min(Cycles, Have / Needed);
    }
    return Cycles == TNumericLimits<int32>::Max() ? 0 : Cycles;
}

int32 AAstrawildWorkSiteActor::DepositInputsFromInventory(UAstrawildInventoryComponent* Inventory)
{
    // Server: transfer as many full-cycle input sets as the player carries (max
    // 10 cycles staged) — silent add on removal keeps quest credit semantics clean.
    if (!Inventory || InputItems.IsEmpty())
    {
        return 0;
    }
    const int32 MaxStagedCycles = 10;
    const int32 CurrentCycles = GetBufferedCycleCount();
    int32 DepositedCycles = 0;
    for (int32 Cycle = CurrentCycles; Cycle < MaxStagedCycles; ++Cycle)
    {
        bool bCanAfford = true;
        for (const FAstrawildItemStack& Required : InputItems)
        {
            const int32 Needed = Required.Quantity - BufferQuantity(Required.ItemId);
            if (Inventory->GetQuantity(Required.ItemId) < Needed)
            {
                bCanAfford = false;
                break;
            }
        }
        if (!bCanAfford)
        {
            break;
        }
        for (const FAstrawildItemStack& Required : InputItems)
        {
            const int32 Needed = Required.Quantity - BufferQuantity(Required.ItemId);
            if (Needed > 0)
            {
                Inventory->RemoveItem(Required.ItemId, Needed);
                AddBufferedQuantity(Required.ItemId, Needed);
            }
        }
        ++DepositedCycles;
    }
    return DepositedCycles;
}

void AAstrawildWorkSiteActor::AddBufferedQuantity(const FName ItemId, const int32 Quantity)
{
    for (FAstrawildItemStack& Stack : InputBuffer)
    {
        if (Stack.ItemId == ItemId)
        {
            Stack.Quantity += Quantity;
            return;
        }
    }
    FAstrawildItemStack NewStack;
    NewStack.ItemId = ItemId;
    NewStack.Quantity = Quantity;
    InputBuffer.Add(NewStack);
}

bool AAstrawildWorkSiteActor::InitializeFromDefinition(UAstrawildWorkSiteDefinition* Definition)
{
    // Production V2: definitions are the single source of truth for site stats
    // (Master Plan §7 — Build→Power→Assign→Work→Consume→Produce is data-driven).
    if (!IsValid(Definition) || Definition->SiteId.IsNone() || Definition->OutputItemId.IsNone())
    {
        return false;
    }
    SiteId = Definition->SiteId;
    WorkType = Definition->WorkType;
    OutputItemId = Definition->OutputItemId;
    OutputQuantity = FMath::Max(1, Definition->OutputQuantity);
    InputItems = Definition->InputItems;
    SecondsPerOutput = FMath::Max(1.0f, Definition->SecondsPerOutput);
    bRequiresPower = Definition->bRequiresPower;
    return true;
}

int32 AAstrawildWorkSiteActor::CollectOutput()
{
    const int32 Collected = StoredOutput;
    StoredOutput = 0;
    return Collected;
}

bool AAstrawildWorkSiteActor::AssignWorker(AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo) || !Echo->bCaptured)
    {
        return false;
    }

    Echo->AssignedWorkSite = this;
    Echo->IssueCommand(EAstrawildEchoCommand::Work);

    if (!Workers.ContainsByPredicate([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return Weak.Get() == Echo; }))
    {
        Workers.Add(Echo);
    }
    return true;
}

void AAstrawildWorkSiteActor::RemoveWorker(AAstrawildEchoCharacter* Echo)
{
    Workers.RemoveAll([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak) { return Weak.Get() == Echo; });
    if (IsValid(Echo))
    {
        Echo->AssignedWorkSite = nullptr;
    }
}

bool AAstrawildWorkSiteActor::AssignRobot(AAstrawildUtilityRobotActor* Robot)
{
    if (!IsValid(Robot) || AssignedRobot.IsValid())
    {
        return false;
    }
    AssignedRobot = Robot;
    return true;
}

void AAstrawildWorkSiteActor::RemoveRobot(AAstrawildUtilityRobotActor* Robot)
{
    if (AssignedRobot.Get() == Robot)
    {
        AssignedRobot = nullptr;
    }
}

TArray<FGuid> AAstrawildWorkSiteActor::GetAssignedEchoInstanceIds() const
{
    TArray<FGuid> Out;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : Workers)
    {
        if (const AAstrawildEchoCharacter* Echo = Weak.Get())
        {
            Out.Add(Echo->InstanceId);
        }
    }
    return Out;
}

FAstrawildWorkSiteSaveData AAstrawildWorkSiteActor::ExportForSave() const
{
    FAstrawildWorkSiteSaveData Data;
    Data.SiteId = SiteId;
    Data.WorkType = WorkType;
    Data.OutputItemId = OutputItemId;
    Data.Transform = GetActorTransform();
    Data.StoredOutput = StoredOutput;
    Data.AssignedEchoInstanceIds = GetAssignedEchoInstanceIds();
    Data.bHasRobot = AssignedRobot.IsValid();
    // Production V2 (schema v4): buffer + per-cycle output persist across saves.
    Data.InputBuffer = InputBuffer;
    Data.OutputQuantity = FMath::Max(1, OutputQuantity);
    return Data;
}

void AAstrawildWorkSiteActor::ImportFromSave(const FAstrawildWorkSiteSaveData& Data)
{
    // Identity/output restore; assignments are re-linked by the save subsystem
    // once the roster and robots are respawned (documented load order).
    if (!Data.SiteId.IsNone())
    {
        SiteId = Data.SiteId;
    }
    WorkType = Data.WorkType;
    OutputItemId = Data.OutputItemId;
    StoredOutput = FMath::Max(0, Data.StoredOutput);
    WorkAccumulator = 0.0f;
    // Production V2 (schema v4): buffer + per-cycle output restore.
    InputBuffer = Data.InputBuffer;
    OutputQuantity = FMath::Max(1, Data.OutputQuantity);
}

void AAstrawildWorkSiteActor::CreditOfflineProduction(float OfflineSeconds)
{
    // Directive Phase 8.3: capped at 48 hours, half rate, and each credited
    // cycle still burns its real inputs — an empty buffer stalls exactly like
    // the live loop (no offline item minting).
    if (OfflineSeconds <= 0.0f || OutputItemId.IsNone() || SecondsPerOutput <= 0.0f)
    {
        return;
    }

    // FCR-1-d fix (H-d4): the offline credit must respect the SAME gates as the
    // live loop — staffing (echoes or a robot) and power. The old check minted
    // items for unstaffed, unpowered sites while the player was away (~8.6k
    // fiber / 6.1k berries / 9.6k stone per 48h load with zero workers). A site
    // that was left running keeps its saved worker/robot assignment; a dead
    // site produces nothing. Power cannot be "live" while away — the saved
    // battery state is the honest proxy, so powered sites require the flag.
    const bool bStaffed = !GetAssignedEchoInstanceIds().IsEmpty() || HasRobot();
    const bool bPowerOk = !bRequiresPower || IsPowered();
    if (!bStaffed || !bPowerOk)
    {
        return;
    }

    const float EffectiveSeconds = FMath::Min(OfflineSeconds, 48.0f * 3600.0f) * 0.5f;
    int32 CreditedCycles = 0;

    // Simulate whole cycles against the live input buffer.
    float Accumulated = EffectiveSeconds;
    while (Accumulated >= SecondsPerOutput)
    {
        if (!ConsumeCycleInputs())
        {
            // Inputs exhausted mid-window — production stalls, matching the
            // live-loop contract (the accumulator pins at the threshold).
            break;
        }
        Accumulated -= SecondsPerOutput;
        ++CreditedCycles;
    }

    if (CreditedCycles > 0)
    {
        StoredOutput += CreditedCycles * FMath::Max(1, OutputQuantity);
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Offline production: site %s credited %d cycles (%d x %s)."),
            *SiteId.ToString(), CreditedCycles, CreditedCycles * FMath::Max(1, OutputQuantity), *OutputItemId.ToString());
    }
}

FText AAstrawildWorkSiteActor::GetInteractionPrompt_Implementation() const
{
    // Audit C-7: dynamic prompt — collect when output waits, assign otherwise.
    if (StoredOutput > 0)
    {
        return FText::FromString(FString::Printf(TEXT("Collect %d x %s [E]"),
            StoredOutput, *OutputItemId.ToString()));
    }
    if (!Workers.IsEmpty() || AssignedRobot.IsValid())
    {
        return FText::FromString(FString::Printf(TEXT("%s — working (%s) [E]"),
            *UEnum::GetDisplayValueAsText(WorkType).ToString(), *OutputItemId.ToString()));
    }
    return FText::FromString(FString::Printf(TEXT("Assign idle Echo to %s [E]"),
        *UEnum::GetDisplayValueAsText(WorkType).ToString()));
}

void AAstrawildWorkSiteActor::Interact_Implementation(AActor* InteractingActor)
{
    // Audit C-7: the automation loop entry point — collect output, or assign the nearest
    // idle captured Echo owned by this player. Server-authoritative like every mutation.
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    UWorld* World = GetWorld();
    if (!Player || !World || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    auto Notify = [World](const FText& Message)
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
            {
                AstrawildPC->Notify(Message);
            }
        }
    };

    // 1) Collect accumulated output first.
    if (StoredOutput > 0)
    {
        const int32 Collected = CollectOutput();
        if (Collected > 0 && Player->InventoryComponent)
        {
            if (Player->InventoryComponent->AddItem(OutputItemId, Collected))
            {
                if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
                {
                    EventBus->PublishEvent(TAG_Astrawild_Event_ItemCollected, Player, OutputItemId, Collected, GetActorLocation());
                }
                Notify(FText::FromString(FString::Printf(TEXT("Collected %d x %s"),
                    Collected, *OutputItemId.ToString())));
            }
            else
            {
                // Weight gate refused — keep the output stored instead of destroying it.
                StoredOutput = Collected;
                Notify(FText::FromString(TEXT("Too heavy to collect — lighten your pack first.")));
            }
        }
        return;
    }

    // 1.5) Production V2: definition-driven sites take inputs BEFORE worker
    // assignment — E stages every full input cycle the player carries (max 10).
    if (RequiresInputs() && Player->InventoryComponent)
    {
        const int32 Deposited = DepositInputsFromInventory(Player->InventoryComponent);
        if (Deposited > 0)
        {
            Notify(FText::FromString(FString::Printf(TEXT("Staged %d production cycle%s (%d buffered)."),
                Deposited, Deposited == 1 ? TEXT("") : TEXT("s"), GetBufferedCycleCount())));
            return;
        }
        if (GetBufferedCycleCount() <= 0)
        {
            Notify(FText::FromString(FString::Printf(TEXT("Needs inputs per cycle: %s — bring some and press E."),
                *FormatInputRequirements())));
            return;
        }
    }

    // 2) Assign the nearest idle captured Echo of this player.
    const FName OwnerId = Player->GetFName();
    AAstrawildEchoCharacter* Best = nullptr;
    float BestDistance = 4000.0f;
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || !Echo->bCaptured || Echo->IsDefeated() || Echo->AssignedWorkSite.IsValid())
        {
            continue;
        }
        if (!OwnerId.IsNone() && Echo->OwnerPlayerId != OwnerId)
        {
            continue;
        }
        const float Distance = FVector::Dist(GetActorLocation(), Echo->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Echo;
        }
    }

    if (Best)
    {
        if (AssignWorker(Best))
        {
            Notify(FText::FromString(FString::Printf(TEXT("%s assigned to %s."),
                *Best->GetName(), *UEnum::GetDisplayValueAsText(WorkType).ToString())));
        }
    }
    else
    {
        Notify(FText::FromString(TEXT("No idle Echo nearby — capture one and command it to Work (C).")));
    }
}

FString AAstrawildWorkSiteActor::FormatInputRequirements() const
{
    // "2x Item_RawMeat + 1x Item_Berry" — the prompt contract for input sites.
    FString Out;
    for (const FAstrawildItemStack& Required : InputItems)
    {
        if (!Out.IsEmpty())
        {
            Out += TEXT(" + ");
        }
        Out += FString::Printf(TEXT("%dx %s"), FMath::Max(1, Required.Quantity), *Required.ItemId.ToString());
    }
    return Out;
}
