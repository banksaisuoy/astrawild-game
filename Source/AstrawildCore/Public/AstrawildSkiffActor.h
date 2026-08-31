#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildSkiffActor.generated.h"

class AAstrawildPlayerCharacter;
class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * Batch 8 — the Dawn Skiff: ASTRAWILD's first aircraft (Docs/ASTRAWILD_VILLAGES_SKIFF.md).
 *
 * A boardable resonance skiff (zero-asset silhouette: hull + nose cone +
 * pontoons + tail fin + running lights). Interaction mounts the pilot:
 *  - WASD        — thrust forward/back + yaw turn
 *  - SPACE / CTRL — climb / descend
 *  - SHIFT       — resonance boost
 *  - E           — dismount
 * Mouse look keeps orbiting the camera (third-person free look while flying).
 *
 * Movement is server-authoritative (single-player/listen-server inline policy,
 * same as the dungeon portals — dedicated-client RPCs arrive with the H-9
 * multiplayer batch). Altitude clamps between a ground-probe floor and the
 * flight ceiling; sweeps stop the hull on terrain contact.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildSkiffActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildSkiffActor();

    /** Stable skiff id (Skiff_Dawnstead / Skiff_Driftwood). */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Skiff")
    FName SkiffId = NAME_None;

    // --- Flight tunables ---

    /** Cruise speed (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="100.0"))
    float CruiseSpeed = 1400.0f;

    /** Boost speed (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="200.0"))
    float BoostSpeed = 2600.0f;

    /** Yaw rate (degrees/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="10.0"))
    float TurnRateDegPerSecond = 70.0f;

    /** Climb/descend rate (cm/s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="50.0"))
    float VerticalSpeed = 700.0f;

    /** Hard flight ceiling above terrain (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="1000.0"))
    float MaxAltitudeAboveGround = 12000.0f;

    /** Minimum hover height above terrain (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff|Flight", meta=(ClampMin="50.0"))
    float MinHoverHeight = 220.0f;

    /** Mount range (cm) — generous so boarding from the pontoons feels good. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Skiff", meta=(ClampMin="150.0"))
    float BoardRange = 420.0f;

    /** Current pilot (replicated; null = parked). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Skiff")
    bool IsPiloted() const { return Pilot != nullptr; }

    // --- Pilot input sinks (called by the player character's input handlers) ---

    /** WASD: X = right turn axis (A/D), Y = forward thrust (W/S). */
    void ReceivePilotMove(float ForwardAxis, float TurnAxis);

    /** SPACE held → +1, released → 0; CTRL held → -1. */
    void ReceivePilotVertical(float VerticalAxis);

    /** SHIFT held → boost. */
    void ReceivePilotBoost(bool bBoosting);

    /** E while piloting → dismount (also Interact when standing next to it). */
    void DismountPilot();

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

    /**
     * Pure flight-math helper (unit-tested): clamped desired velocity for one
     * tick. Exposed static so the automation suite can pin the ceiling/floor
     * behaviour without a world.
     */
    static FVector ComputeSkiffVelocity(const FVector& Forward, float ForwardAxis, float VerticalAxis,
        bool bBoosting, float CruiseSpeed, float BoostSpeed, float VerticalSpeed);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void OnConstruction(const FTransform& Transform) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Skiff", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UBoxComponent> HullCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Skiff", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> HullMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Skiff", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> NoseCone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Skiff", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UStaticMeshComponent> TailFin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Skiff", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UPointLightComponent> BowLight;

    UPROPERTY(Replicated)
    TObjectPtr<AAstrawildPlayerCharacter> Pilot;

    float PilotForwardAxis = 0.0f;
    float PilotTurnAxis = 0.0f;
    float PilotVerticalAxis = 0.0f;
    bool bPilotBoosting = false;

    /** Landing legs rest height above terrain when parked (Set at spawn). */
    float ParkedGroundZ = 0.0f;

    float CurrentBankRoll = 0.0f;
    float CurrentPitchTilt = 0.0f;
    float HoverBobTime = 0.0f;

    void MountPilot(AAstrawildPlayerCharacter* Player);
    float ProbeGroundZ() const;
    void BuildSkiffVisuals();
};
