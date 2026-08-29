#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildWorldBootstrapper.generated.h"

class UStaticMeshComponent;
class ADirectionalLight;
class ASkyLight;

/**
 * Zero-asset world bootstrapper (directive §21/§50): on the server, builds the
 * Dawn Fields vertical-slice arena entirely from C++ — lighting rig, ground,
 * resource nodes, wild Echoes, the first hostile, rest point, crafting stations
 * and work sites. Everything uses engine basic shapes so the project plays
 * immediately after compile, with no .umap/.uasset content.
 *
 * Deterministic: spawns derive from the GameState WorldSeed.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildWorldBootstrapper : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildWorldBootstrapper();

    /** Arena half-size in cm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World", meta=(ClampMin="1000.0"))
    float ArenaSize = 8000.0f;

    /** Resource nodes scattered around. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 ResourceNodeCount = 26;

    /** Wild Echo population. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 WildEchoCount = 9;

    /** Hostile Gloomfangs (night threat, directive §21). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 HostileCount = 2;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    void BuildLighting();
    void BuildGround();
    void ScatterResourceNodes();
    void SpawnWildEchoes();
    void SpawnHostiles();
    void SpawnPointsOfInterest();

    FRandomStream RandomStream;

    UPROPERTY()
    TObjectPtr<ADirectionalLight> SunLight;

    void UpdateSunRotation();
    class AAstrawildGameState* GetGameState() const;
};
