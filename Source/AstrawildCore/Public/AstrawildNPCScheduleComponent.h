#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildNPCScheduleComponent.generated.h"

/**
 * SCP Phase 7 — NPC daily schedules (directive [3] Phase 7.2).
 *
 * Every villager lives a day: work hours at their profession anchor, evening
 * at home, night curfew. Rain sends everyone but guards to shelter. The
 * component drives the NPC's AI controller with MoveToLocation anchors and
 * gates services (the trader's shop only trades during work hours).
 */
UENUM(BlueprintType)
enum class EAstrawildNPCProfession : uint8
{
    Smith UMETA(DisplayName="Smith"),
    Farmer UMETA(DisplayName="Farmer"),
    Trader UMETA(DisplayName="Trader"),
    Guard UMETA(DisplayName="Guard")
};

/** Where the NPC should be right now. */
UENUM(BlueprintType)
enum class EAstrawildNPCAnchor : uint8
{
    Work UMETA(DisplayName="Work"),
    Home UMETA(DisplayName="Home"),
    Shelter UMETA(DisplayName="Shelter (rain)"),
    Patrol UMETA(DisplayName="Patrol (guard)"),
    Sleep UMETA(DisplayName="Sleep")
};

UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildNPCScheduleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildNPCScheduleComponent();

    // --- Static contracts (automation-tested) ---

    /** Resolve the anchor for an hour + weather (directive 4 professions). */
    static EAstrawildNPCAnchor ResolveAnchor(EAstrawildNPCProfession Profession, int32 HourOfDay, bool bRaining);

    /** Profession derived from the NPC role (Vendor->Trader, Guard->Guard...). */
    static EAstrawildNPCProfession ResolveProfession(uint8 NPCRole);

    /** True while the NPC provides services (work window, not raining). */
    static bool IsServiceOpen(EAstrawildNPCProfession Profession, int32 HourOfDay, bool bRaining);

    // --- Runtime state ---

    /** Assigned profession (defaults by role on begin play). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Schedule")
    EAstrawildNPCProfession Profession = EAstrawildNPCProfession::Farmer;

    /** True while services are open right now (shop gating). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Schedule")
    bool AreServicesOpenNow() const;

    /** Current anchor (HUD + AI gate). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Schedule")
    EAstrawildNPCAnchor GetCurrentAnchor() const { return CurrentAnchor; }

    /** Home anchor location (the spawn vicinity — villages are compact). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Schedule")
    FVector HomeLocation = FVector::ZeroVector;

    /** Work anchor offset from home (per profession drift). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Schedule")
    FVector WorkLocationOffset = FVector(400.0f, 0.0f, 0.0f);

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    EAstrawildNPCAnchor CurrentAnchor = EAstrawildNPCAnchor::Home;

    /** Anchor drift cadence (guards patrol, others amble around work). */
    float DriftAccumulator = 0.0f;

    bool IsAuthority() const;
    int32 GetCurrentHour() const;
    bool IsRaining() const;
    void MoveToAnchor();
};
