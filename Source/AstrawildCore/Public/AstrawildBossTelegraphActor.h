#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildBossTelegraphActor.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

/**
 * Final production run (PHASE 14 — boss telegraphs): ground warning disc shown
 * before the boss's AoE blast detonates. Purely visual + lifetime — the BOSS owns
 * the countdown and applies the damage when the telegraph expires (single
 * authority path, no damage logic in the visual actor).
 *
 * FPP-1 (presentation pass): the disc carries the attacker's ELEMENT identity —
 * a tinted point light rides the disc (the codebase's element-color language,
 * same as creature ElementGlowLight) so an Ember blast reads differently from a
 * Frost blast at first glance. Call ConfigureTelegraph right after spawning.
 *
 * REPLACE_BEFORE_RELEASE: swap the flat cylinder for a decal/Niagara ring.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBossTelegraphActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildBossTelegraphActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    /** FPP-1: element-identity light (color from FAstrawildVfxPalette). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    TObjectPtr<UPointLightComponent> TelegraphLight;

    /** Visual blast radius the disc should cover (cm) — call before adding to the world. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="50.0"))
    float BlastRadius = 350.0f;

    /** How long the warning shows before detonation (matches the boss timer). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0.1"))
    float TelegraphDuration = 1.5f;

    /**
     * FPP-1: one-shot configuration (radius/duration/element tint). The None
     * element falls back to a hot warning amber — a danger cue that reads even
     * for element-neutral attacks (the melee windup uses it).
     */
    void ConfigureTelegraph(float InBlastRadius, float InDuration, EAstrawildElementType Element);

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    float Elapsed = 0.0f;
};
