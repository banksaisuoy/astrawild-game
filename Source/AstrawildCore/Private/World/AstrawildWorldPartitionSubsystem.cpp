#include "World/AstrawildWorldPartitionSubsystem.h"

#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

int32 UAstrawildWorldPartitionSubsystem::ClampCellCoordinate(const int32 Coordinate)
{
    return FMath::Clamp(Coordinate, 0, CellsPerAxis - 1);
}

FIntPoint UAstrawildWorldPartitionSubsystem::GetCellCoordinates(const FVector WorldLocation)
{
    const float Origin = -static_cast<float>(MapSizeCentimeters) * 0.5f;
    const int32 X = FMath::FloorToInt((WorldLocation.X - Origin) / static_cast<float>(CellSizeCentimeters));
    const int32 Y = FMath::FloorToInt((WorldLocation.Y - Origin) / static_cast<float>(CellSizeCentimeters));
    return FIntPoint(ClampCellCoordinate(X), ClampCellCoordinate(Y));
}

FAstrawildWorldCell UAstrawildWorldPartitionSubsystem::GetCellForLocation(const FVector WorldLocation) const
{
    FAstrawildWorldCell Cell;
    Cell.Coordinates = GetCellCoordinates(WorldLocation);
    Cell.LinearIndex = Cell.Coordinates.Y * CellsPerAxis + Cell.Coordinates.X;

    const float Origin = -static_cast<float>(MapSizeCentimeters) * 0.5f;
    const FVector2D Min(
        Origin + Cell.Coordinates.X * CellSizeCentimeters,
        Origin + Cell.Coordinates.Y * CellSizeCentimeters);
    const FVector2D Max = Min + FVector2D(CellSizeCentimeters, CellSizeCentimeters);
    Cell.Bounds = FBox2D(Min, Max);
    return Cell;
}

bool UAstrawildWorldPartitionSubsystem::GetBiome(const FName BiomeId, FAstrawildBiomeDefinition& OutBiome) const
{
    if (!BiomeTable || BiomeId.IsNone())
    {
        return false;
    }

    const FAstrawildBiomeDefinition* Row = BiomeTable->FindRow<FAstrawildBiomeDefinition>(BiomeId, TEXT("BiomeLookup"));
    if (!Row)
    {
        return false;
    }

    OutBiome = *Row;
    return true;
}

bool UAstrawildWorldPartitionSubsystem::GetBiomeAtLocation(const FVector WorldLocation, FAstrawildBiomeDefinition& OutBiome) const
{
    // The quadrant fallback keeps the world deterministic before authored landscape volumes exist.
    // Landscape/World Partition volumes can replace this lookup later without changing callers.
    FName BiomeId = TEXT("Biome.DawnMeadows");
    if (WorldLocation.X >= 0.0f && WorldLocation.Y < 0.0f)
    {
        BiomeId = TEXT("Biome.SylvanRainforest");
    }
    else if (WorldLocation.X < 0.0f && WorldLocation.Y >= 0.0f)
    {
        BiomeId = TEXT("Biome.ScorchedObsidianCaldera");
    }
    else if (WorldLocation.X >= 0.0f && WorldLocation.Y >= 0.0f)
    {
        BiomeId = TEXT("Biome.GlacialZenith");
    }

    return GetBiome(BiomeId, OutBiome);
}

bool UAstrawildWorldPartitionSubsystem::DiscoverSpire(const FName SpireId)
{
    FAstrawildFastTravelSpire Spire;
    if (!GetSpireData(SpireId, Spire) || DiscoveredSpireIds.Contains(SpireId))
    {
        return false;
    }

    DiscoveredSpireIds.Add(SpireId);
    OnSpireDiscovered.Broadcast(SpireId);
    return true;
}

bool UAstrawildWorldPartitionSubsystem::IsSpireDiscovered(const FName SpireId) const
{
    return DiscoveredSpireIds.Contains(SpireId);
}

bool UAstrawildWorldPartitionSubsystem::GetSpireData(const FName SpireId, FAstrawildFastTravelSpire& OutSpire) const
{
    if (!SpireTable || SpireId.IsNone())
    {
        return false;
    }

    const FAstrawildFastTravelSpire* Row = SpireTable->FindRow<FAstrawildFastTravelSpire>(SpireId, TEXT("SpireLookup"));
    if (!Row)
    {
        return false;
    }

    OutSpire = *Row;
    return true;
}

bool UAstrawildWorldPartitionSubsystem::TryFastTravel(ACharacter* Character, const FName DestinationSpireId)
{
    if (!Character || !IsSpireDiscovered(DestinationSpireId))
    {
        return false;
    }

    if (GetWorld() && GetWorld()->GetNetMode() != NM_Standalone && !Character->HasAuthority())
    {
        return false;
    }

    FAstrawildFastTravelSpire Destination;
    if (!GetSpireData(DestinationSpireId, Destination))
    {
        return false;
    }

    const FTransform SafeTransform = Destination.WorldTransform;
    Character->SetActorLocationAndRotation(SafeTransform.GetLocation(), SafeTransform.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
    return true;
}

TArray<FAstrawildWorldCell> UAstrawildWorldPartitionSubsystem::GetAllWorldCells() const
{
    TArray<FAstrawildWorldCell> Cells;
    Cells.Reserve(TotalCellCount);
    for (int32 Y = 0; Y < CellsPerAxis; ++Y)
    {
        for (int32 X = 0; X < CellsPerAxis; ++X)
        {
            const FVector Location(
                -MapSizeCentimeters * 0.5f + (X + 0.5f) * CellSizeCentimeters,
                -MapSizeCentimeters * 0.5f + (Y + 0.5f) * CellSizeCentimeters,
                0.0f);
            Cells.Add(GetCellForLocation(Location));
        }
    }
    return Cells;
}
