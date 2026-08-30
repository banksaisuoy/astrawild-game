#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildUtilityRobotActor.generated.h"

class AAstrawildWorkSiteActor;
class UStaticMeshComponent;
class UPointLightComponent;
class UAstrawildRobotDefinition;

/**
 * Final production run (PHASE 12 — robotics): the Utility Robot worker.
 * Deployed by consuming a robot item (Item_UtilityRobot). It walks itself to
 * the nearest work site without a robot and mans it — steady 0.8x work rate,
 * no needs/food (unlike Echoes), but the site's POWER gate still applies
 * (a robot on an unpowered workstation produces nothing).
 *
 * Production V2 (Master Plan §12): robot CHASSIS SPECIALIZATIONS — mining,
 * farming and defense frames ship as data (UAstrawildRobotDefinition). The
 * specialist rate applies on matching sites; every other site gets the
 * generic rate. Chassis get a role light + tint from the definition
 * (placeholder visuals until Antigravity binds real meshes).
 *
 * The site's own Tick carries the production math; this actor is presence +
 * locomotion + presence state. Save/load: FAstrawildRobotSaveData (schema v4
 * persists RobotDefinitionId).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildUtilityRobotActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildUtilityRobotActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    TObjectPtr<UPointLightComponent> StatusLight;

    // --- Tunables ---

    /** Movement interpolation speed toward the work site. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Robot", meta=(ClampMin="0.1"))
    float MoveInterpSpeed = 1.6f;

    /** Stand-off distance from the site center (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Robot", meta=(ClampMin="50.0"))
    float StandoffDistance = 160.0f;

    // --- Production V2: chassis specialization ---

    /** Robot definition id (specialist chassis); NAME_None = general-purpose frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Robot")
    FName RobotDefinitionId = NAME_None;

    /** Server: resolve stats + visuals from a registered robot definition. */
    void InitializeFromDefinition(UAstrawildRobotDefinition* Definition);

    /** Resolved definition (null for general-purpose robots). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Robot")
    UAstrawildRobotDefinition* GetRobotDefinition() const;

    /** Work rate this robot contributes at the given site type (specialist aware). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Robot")
    float GetWorkRateFor(EAstrawildWorkType SiteWorkType) const;

    /** Fallback contribution for definition-less robots (site's legacy rate). */
    static constexpr float GenericRobotWorkRate = 0.8f;

    virtual void Tick(float DeltaTime) override;

    /** Server: send the robot to a work site (site owns the work-rate math). */
    void AssignToSite(AAstrawildWorkSiteActor* Site);

    /** Server: release the robot from its site (site destroyed / recalled). */
    void ReleaseFromSite();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Robot")
    bool IsAssigned() const { return AssignedSite.IsValid(); }

    /** Site id snapshot for save/load re-linking. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Robot")
    FName GetAssignedSiteId() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Robot")
    void SetOwnerPlayerId(FName InOwnerPlayerId) { OwnerPlayerId = InOwnerPlayerId; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Robot")
    FName GetOwnerPlayerId() const { return OwnerPlayerId; }

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<AAstrawildWorkSiteActor> AssignedSite;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Robot")
    FName OwnerPlayerId = NAME_None;

    /** Idle bob phase (cosmetic — parked robots idle-scan the horizon). */
    float IdlePhase = 0.0f;

    AAstrawildWorkSiteActor* GetAssignedSite() const;
};
