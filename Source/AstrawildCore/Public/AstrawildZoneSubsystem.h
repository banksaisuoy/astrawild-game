#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildZoneSubsystem.generated.h"

class AAstrawildPlayerCharacter;
class UAstrawildEventBusSubsystem;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildZoneDiscovered, EAstrawildZone, Zone, const FAstrawildZoneDescriptor&, Descriptor);

/**
 * Static description of one surface zone of the Shattered Vale (Batch 7).
 * Bounds are world-space centimeters on the XY plane and tile the world exactly:
 * X [-120000, 120000] x Y [-80000, 80000] (2.4km x 1.6km).
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildZoneDescriptor
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    EAstrawildZone Zone = EAstrawildZone::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FName ZoneId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FText DisplayName;

    /** One-line flavor shown under the HUD banner. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FText Subtitle;

    /** World-space rectangle in centimeters (XY plane). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FBox2D Bounds = FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);

    /** Editor/vertex tint used by the runtime terrain tiles. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FLinearColor GroundTint = FLinearColor::White;

    /** Signature zone light color (landmark point lights). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    FLinearColor AmbientLightColor = FLinearColor::White;

    /** Terrain base offset in centimeters. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    float BaseHeight = 0.0f;

    /** Terrain amplitude in centimeters. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    float HeightAmplitude = 0.0f;

    /** 0 = smooth rolling hills, 1 = fully ridged mountains. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    float RidgeBlend = 0.0f;

    /** 1 (safe) .. 4 (endgame wilds). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Zone")
    int32 ThreatLevel = 1;

    FVector2D GetCenter() const { return Bounds.GetCenter(); }

    float GetSizeX() const { return Bounds.Max.X - Bounds.Min.X; }

    float GetSizeY() const { return Bounds.Max.Y - Bounds.Min.Y; }
};

/**
 * The Shattered Vale zone registry (Batch 7 — closes gap M-13).
 *
 * - Pure static zone table + lookup: any client can resolve a world position to a
 *   zone without replication (HUD banner).
 * - Server-side sweep: tracks each player pawn's zone, publishes Event.ZoneEntered /
 *   Event.ZoneLeft on the event bus and records zone discovery for the save system.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildZoneSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Ordered zone table (Dawn Fields first — natural reading order for UIs). */
    static const TArray<FAstrawildZoneDescriptor>& GetAllZones();

    static const FAstrawildZoneDescriptor* FindZone(const EAstrawildZone Zone);

    static const FAstrawildZoneDescriptor* FindZoneById(const FName ZoneId);

    /** Rect-containment lookup. Deterministic: first matching rect wins on shared borders. */
    static EAstrawildZone GetZoneAt(const FVector& WorldLocation);

    /**
     * Smooth per-zone weights forming a partition of unity — the terrain height
     * function blends zone height profiles through these weights so adjacent tiles
     * never seam. Blend distance ~60m beyond each zone rect.
     */
    static void ComputeZoneWeights(const FVector2D& Point, float OutWeights[(int32)EAstrawildZone::Count]);

    /** World rect covered by the zone grid (Batch 8: 3.2km x 2.4km). */
    static FBox2D GetWorldBounds();

    /**
     * Batch 8 — global sea level (world Z, centimeters). The three sea zones
     * (Azure Shallows, Tidebreaker Isles, Pearlsea Reef) sit with their floors
     * below this line; the runtime water planes spawn at this height. Kept as a
     * pure static so terrain tint (waterline shading) and the water actors can
     * never disagree.
     */
    static float GetSeaLevelZ() { return -450.0f; }

    /** Batch 8 — true when the zone's lowest possible terrain dips below sea level. */
    static bool IsSeaZone(const EAstrawildZone Zone);

    /** Number of real zones (excludes None/Count). */
    static int32 GetZoneCount() { return (int32)EAstrawildZone::Count - 1; }

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Zone")
    FAstrawildZoneDiscovered OnZoneDiscovered;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Zone")
    bool HasDiscoveredZone(const EAstrawildZone Zone) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Zone")
    int32 GetDiscoveredZoneCount() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Zone")
    TArray<EAstrawildZone> GetDiscoveredZones() const { return DiscoveredZones; }

    /** Zone the given pawn currently stands in (server-side tracked; None if unknown). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Zone")
    EAstrawildZone GetTrackedZoneForPawn(const AActor* Pawn) const;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void ExportForSave(TArray<EAstrawildZone>& OutZones) const;
    void ImportFromSave(const TArray<EAstrawildZone>& InZones);

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    /** Discovered zones (server, persisted through FAstrawildZoneSaveData). */
    TArray<EAstrawildZone> DiscoveredZones;

    /** Last known zone per player pawn. */
    TMap<TWeakObjectPtr<const AActor>, EAstrawildZone> TrackedPawns;

    /** Sweep throttle accumulator (0.5s cadence, mirrors the journal sweep). */
    float SweepAccumulator = 0.5f;

    void RunSweep();

    UAstrawildEventBusSubsystem* GetEventBus() const;
};
