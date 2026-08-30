#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildDungeonGateActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * Dungeon progression gate (Batch 6 — implements the gate forward-declared since
 * the original dungeon slice; directive §23 "gates stay sealed until the previous
 * room clears"): a resonance arch spanning the passage between two rooms.
 *
 * Sealed: blocking collision + crossbar dropped at chest height.
 * Open: collision disabled + crossbar lifted into the lintel (walk through).
 *
 * Server-authoritative; bOpen replicates and OnRep re-applies the state because
 * collision enabled/disabled does not replicate on its own.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDungeonGateActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildDungeonGateActor();

    /** Blocking volume sealing the passage (thin across the chain axis). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UBoxComponent> GateCollision;

    /** Pillars framing the passage (engine basic shapes — REPLACE_BEFORE_RELEASE). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> LeftPillarMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> RightPillarMesh;

    /** The seal bar: chest height while sealed, lifted into the lintel while open. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> CrossbarMesh;

    /** Passage half-width the gate must span (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="100.0"))
    float PassageHalfWidth = 500.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon", Replicated)
    bool bOpen = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server: unseal the passage (idempotent). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void OpenGate();

    /** Server: seal the passage (idempotent). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void SealGate();

private:
    /** Crossbar center height while sealed (chest). */
    static constexpr float SealedCrossbarZ = 60.0f;

    /** Crossbar center height while open (lifted into the lintel). */
    static constexpr float OpenCrossbarZ = 520.0f;

    void ApplyGateState();

    UFUNCTION()
    void OnRep_bOpen();
};
