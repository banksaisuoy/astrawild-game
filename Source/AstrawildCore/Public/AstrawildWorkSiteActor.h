#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildInteractable.h"
#include "AstrawildWorkSiteActor.generated.h"

class AAstrawildWorkSiteActor;
class AAstrawildEchoCharacter;
class AAstrawildUtilityRobotActor;
class UStaticMeshComponent;
class UAstrawildWorkSiteDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAstrawildWorkProduced, AAstrawildWorkSiteActor*, Site, FName, ItemId, int32, Quantity);

/**
 * A base work site (directive §18): Echoes assigned here produce items over time.
 * Rate scales with species work affinity, personality and needs. Sites powered by
 * the power grid (workstations) produce faster.
 *
 * Audit C-7: now interactable — E collects stored output; with the output empty it
 * assigns the nearest idle captured Echo (the automation loop is reachable in play).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildWorkSiteActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildWorkSiteActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Work")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Work")
    FAstrawildWorkProduced OnWorkProduced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    EAstrawildWorkType WorkType = EAstrawildWorkType::Gathering;

    /** Working range for assigned Echoes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work", meta=(ClampMin="100.0"))
    float WorkRange = 250.0f;

    /** Item produced by this site. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    FName OutputItemId = NAME_None;

    /** Seconds of effective work per produced item (base). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work", meta=(ClampMin="0.1"))
    float SecondsPerOutput = 12.0f;

    /** Requires power from the grid to run at full speed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    bool bRequiresPower = false;

    /**
     * Final production run: stable site id — bootstrapper sites use fixed ids
     * ("Site_CampGathering"...), the save system re-links workers by it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    FName SiteId = NAME_None;

    /** Fixed work rate contributed by a manned utility robot (no needs, power-gated). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work", meta=(ClampMin="0.0", ClampMax="2.0"))
    float RobotWorkRate = 0.8f;

    // --- Production V2 (Master Plan §7): data-driven Consume→Produce loop ---

    /** Inputs consumed per production cycle (empty = harvest-from-the-land site). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work|Production")
    TArray<FAstrawildItemStack> InputItems;

    /** Output produced per completed cycle (definition default 1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work|Production", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;

    /** Server: resolve every stat from a registered work-site definition. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    bool InitializeFromDefinition(UAstrawildWorkSiteDefinition* Definition);

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Items accumulated and waiting for pickup. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    int32 GetStoredOutput() const { return StoredOutput; }

    /** Player collects the accumulated output. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    int32 CollectOutput();

    /** Attach an Echo worker (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    bool AssignWorker(AAstrawildEchoCharacter* Echo);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    void RemoveWorker(AAstrawildEchoCharacter* Echo);

    // --- Final production run: utility robot worker (PHASE 12 robotics) ---

    /** Attach a utility robot (server — one robot per site). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    bool AssignRobot(AAstrawildUtilityRobotActor* Robot);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    void RemoveRobot(AAstrawildUtilityRobotActor* Robot);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    bool HasRobot() const { return AssignedRobot.IsValid(); }

    // --- Production V2: input buffer queries ---

    /** True when this site consumes inputs per cycle (definition-driven sites). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    bool RequiresInputs() const { return !InputItems.IsEmpty(); }

    /** Staged input buffer (server state, saved in schema v4). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    const TArray<FAstrawildItemStack>& GetInputBuffer() const { return InputBuffer; }

    /** How many full cycles the current buffer covers (0 when inputs missing). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    int32 GetBufferedCycleCount() const;

    /** Roster instance ids of the Echoes currently assigned (save snapshot). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    TArray<FGuid> GetAssignedEchoInstanceIds() const;

    /** Save/restore: output level + worker identity (assignment re-linked by the save subsystem). */
    FAstrawildWorkSiteSaveData ExportForSave() const;
    void ImportFromSave(const FAstrawildWorkSiteSaveData& Data);

    /** IAstrawildInteractable (audit C-7): collect output / assign nearest idle Echo. */
    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

    /** True when at least one worker is currently assigned. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    bool HasAnyWorkers() const { return !Workers.IsEmpty(); }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Replicated)
    int32 StoredOutput = 0;

    /** Staged inputs for upcoming cycles (Production V2 consume→produce loop). */
    UPROPERTY(Replicated)
    TArray<FAstrawildItemStack> InputBuffer;

    float WorkAccumulator = 0.0f;

    TArray<TWeakObjectPtr<AAstrawildEchoCharacter>> Workers;

    /** Final production run: robot presence — the site Tick adds its flat rate. */
    TWeakObjectPtr<AAstrawildUtilityRobotActor> AssignedRobot;

    bool IsPowered() const;
    class UAstrawildPowerSubsystem* GetPower() const;

    /** Consume one cycle's inputs from the buffer (false when insufficient → stall). */
    bool ConsumeCycleInputs();

    /** Deposit matching inputs from a player inventory into the buffer (server). */
    int32 DepositInputsFromInventory(class UAstrawildInventoryComponent* Inventory);

    /** Buffer helpers (server state math — deterministic, unit-testable shape). */
    FString FormatInputRequirements() const;
    int32 BufferQuantity(FName ItemId) const;
    void RemoveBufferedQuantity(FName ItemId, int32 Quantity);
    void AddBufferedQuantity(FName ItemId, int32 Quantity);
};
