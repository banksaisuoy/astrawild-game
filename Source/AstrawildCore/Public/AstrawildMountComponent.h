#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildMountComponent.generated.h"

class AAstrawildPlayerCharacter;

/**
 * SCP Phase 5 — creature riding (directive [3] Phase 5.3).
 *
 * Mounted on every Echo (the mount contract). A captured, bonded, rideable
 * species carries its owner:
 *  - rider attaches at the documented socket contract (MountSocket /
 *    RiderPelvisSocket / hand grips / stirrups — procedural bodies use the
 *    equivalent offset table until skeletal rigs ship the sockets),
 *  - movement input is forwarded from the rider (the skiff pilot pattern),
 *  - land mounts sprint at 1.25x species speed, avian mounts fly
 *    (SPACE climb / CTRL descend, ceiling-gated),
 *  - riding is transient state (never persisted — remount on load).
 *
 * Eligibility: captured + owner match + Bond >= 25 + rideable family/body/
 * size (Beast/Dragon/Avian/Insectoid quadrupeds/avian plans, Medium+).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildMountComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildMountComponent();

    /** Bond required before a creature accepts a rider (directive trust arc). */
    static constexpr float MountBondGate = 25.0f;

    /** Riding speed multiplier over species base speed. */
    static constexpr float MountSpeedMultiplier = 1.25f;

    /** Vertical speed while piloting a flying mount (cm/s). */
    static constexpr float MountVerticalSpeed = 520.0f;

    // --- Socket contract (directive Phase 2.2) ---
    static FName GetMountSocketName() { return TEXT("MountSocket"); }
    static FName GetRiderPelvisSocketName() { return TEXT("RiderPelvisSocket"); }
    static FName GetLeftHandGripSocketName() { return TEXT("LeftHandGripSocket"); }
    static FName GetRightHandGripSocketName() { return TEXT("RightHandGripSocket"); }
    static FName GetLeftFootStirrupSocketName() { return TEXT("LeftFootStirrupSocket"); }
    static FName GetRightFootStirrupSocketName() { return TEXT("RightFootStirrupSocket"); }

    // --- Species eligibility (pure — automation-tested) ---

    /** True when family/body/size form a rideable silhouette. */
    static bool IsRideableSpecies(EAstrawildEchoFamily Family, EAstrawildBodyPlan BodyPlan, EAstrawildSizeClass SizeClass);

    /** Mount speed: species speed x 1.25 (flying mounts add vertical). */
    static float ComputeMountSpeed(float SpeciesMoveSpeed);

    /** Rider seat offset for a size class (procedural-body contract). */
    static FVector ComputeRiderSeatOffset(EAstrawildSizeClass SizeClass);

    // --- Runtime state ---

    /** True while a rider is seated. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    bool IsMounted() const { return Rider != nullptr; }

    /** The seated rider (null when unmounted). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    AAstrawildPlayerCharacter* GetRider() const { return Rider; }

    /** Can this specific echo be ridden by the given player right now? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    bool CanBeMountedBy(const AAstrawildPlayerCharacter* Player) const;

    /** Seat a rider (server). Returns false when ineligible. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mount")
    bool MountRider(AAstrawildPlayerCharacter* Player);

    /** Eject the rider (server). Always safe to call. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mount")
    void DismountRider();

    /** Rider input forwarding (the skiff pilot pattern). */
    void ReceiveRiderMove(float ForwardAxis, float TurnAxis);
    void ReceiveRiderVertical(float VerticalAxis);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildMountStateChanged, bool, bMounted, const AAstrawildPlayerCharacter*, Rider);

    /** Fired on mount + dismount (HUD + audio hooks). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mount")
    FAstrawildMountStateChanged OnMountStateChanged;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    /** Seated rider (replicated so clients render the seated pose). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mount", meta=(AllowPrivateAccess = "true"), Replicated)
    TObjectPtr<AAstrawildPlayerCharacter> Rider;

    /** Mount movement mode cached at mount time (land vs flying). */
    bool bFlyingMount = false;

    float RiderForwardAxis = 0.0f;
    float RiderTurnAxis = 0.0f;
    float RiderVerticalAxis = 0.0f;

    /** Drives the echo from rider input while mounted (authority). */
    void DriveMountedMovement(float DeltaTime);

    bool IsAuthority() const;
};
