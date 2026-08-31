#include "AstrawildZoneSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
    // Zone cell size: every zone is an 800m x 800m square, 4 columns x 3 rows
    // (Batch 8 "The Grand Expanse" — was 3x2 in Batch 7).
    constexpr float ZoneHalfSize = 40000.0f;
    constexpr float ColumnX[4] = { -120000.0f, -40000.0f, 40000.0f, 120000.0f };
    constexpr float RowY[3] = { 80000.0f, 0.0f, -80000.0f };

    // Smooth blend distance for the zone weight field (~60m past each rect): sharp,
    // dramatic biome fronts while the height field stays seam-continuous.
    constexpr float ZoneBlendDistance = 6000.0f; // ~60m: dramatic zone fronts (Palworld-style biome edges), seams stay continuous.

    FAstrawildZoneDescriptor MakeZone(const EAstrawildZone Zone, const TCHAR* Id, const TCHAR* Name,
        const TCHAR* Subtitle, const float CenterX, const float CenterY, const FLinearColor& Tint,
        const FLinearColor& LightColor, const float Base, const float Amplitude, const float Ridge,
        const int32 Threat)
    {
        FAstrawildZoneDescriptor Desc;
        Desc.Zone = Zone;
        Desc.ZoneId = Id;
        Desc.DisplayName = FText::FromString(Name);
        Desc.Subtitle = FText::FromString(Subtitle);
        Desc.Bounds = FBox2D(
            FVector2D(CenterX - ZoneHalfSize, CenterY - ZoneHalfSize),
            FVector2D(CenterX + ZoneHalfSize, CenterY + ZoneHalfSize));
        Desc.GroundTint = Tint;
        Desc.AmbientLightColor = LightColor;
        Desc.BaseHeight = Base;
        Desc.HeightAmplitude = Amplitude;
        Desc.RidgeBlend = Ridge;
        Desc.ThreatLevel = Threat;
        return Desc;
    }
}

const TArray<FAstrawildZoneDescriptor>& UAstrawildZoneSubsystem::GetAllZones()
{
    static TArray<FAstrawildZoneDescriptor> Zones;
    if (Zones.IsEmpty())
    {
        // Grid (Batch 8, 4x3):
        //   North row: Frostveil / Glimmerwood / Ember Ridge / Sunscar Desert
        //   Middle:    Dusk Marsh / Dawn Fields / Hollow Approach / Azure Shallows
        //   South row: Tidebreaker Isles / Stormcrest / Verdant Reach / Pearlsea Reef
        // Sea zones (floor below SeaLevelZ): Azure Shallows, Tidebreaker Isles,
        // Pearlsea Reef — the runtime water planes cover those rects.

        // North row.
        Zones.Add(MakeZone(EAstrawildZone::FrostveilExpanse, TEXT("Zone_Frostveil"), TEXT("Frostveil Expanse"),
            TEXT("Wind-scoured snowfields above the treeline."),
            ColumnX[0], RowY[0], FLinearColor(0.68f, 0.78f, 0.92f), FLinearColor(0.55f, 0.75f, 1.0f),
            900.0f, 2200.0f, 0.8f, 3));

        Zones.Add(MakeZone(EAstrawildZone::Glimmerwood, TEXT("Zone_Glimmerwood"), TEXT("Glimmerwood"),
            TEXT("Crystal spires hum where the First Dawn fell."),
            ColumnX[1], RowY[0], FLinearColor(0.71f, 0.55f, 1.0f), FLinearColor(0.70f, 0.50f, 1.0f),
            300.0f, 900.0f, 0.15f, 2));

        Zones.Add(MakeZone(EAstrawildZone::EmberRidge, TEXT("Zone_EmberRidge"), TEXT("Ember Ridge"),
            TEXT("Obsidian spires over a sleeping caldera."),
            ColumnX[2], RowY[0], FLinearColor(0.82f, 0.45f, 0.30f), FLinearColor(1.0f, 0.50f, 0.20f),
            500.0f, 2600.0f, 0.9f, 3));

        Zones.Add(MakeZone(EAstrawildZone::SunscarDesert, TEXT("Zone_Sunscar"), TEXT("Sunscar Desert"),
            TEXT("Dune seas where the sun was buried."),
            ColumnX[3], RowY[0], FLinearColor(0.87f, 0.78f, 0.55f), FLinearColor(1.0f, 0.85f, 0.50f),
            150.0f, 900.0f, 0.25f, 3));

        // Middle row.
        Zones.Add(MakeZone(EAstrawildZone::DuskMarsh, TEXT("Zone_DuskMarsh"), TEXT("Dusk Marsh"),
            TEXT("Muck pools and reeds that remember the flood."),
            ColumnX[0], RowY[1], FLinearColor(0.37f, 0.55f, 0.43f), FLinearColor(0.30f, 0.90f, 0.70f),
            -60.0f, 260.0f, 0.0f, 2));

        Zones.Add(MakeZone(EAstrawildZone::DawnFields, TEXT("Zone_DawnFields"), TEXT("Dawn Fields"),
            TEXT("Home. Soft hills, first light, Dawnstead village."),
            ColumnX[1], RowY[1], FLinearColor(0.61f, 0.80f, 0.42f), FLinearColor(1.0f, 0.90f, 0.60f),
            220.0f, 520.0f, 0.0f, 1));

        Zones.Add(MakeZone(EAstrawildZone::HollowApproach, TEXT("Zone_HollowApproach"), TEXT("Hollow Approach"),
            TEXT("Ash-choked wilds before the Underlight gate."),
            ColumnX[2], RowY[1], FLinearColor(0.54f, 0.50f, 0.57f), FLinearColor(0.90f, 0.40f, 0.35f),
            260.0f, 1300.0f, 0.5f, 4));

        Zones.Add(MakeZone(EAstrawildZone::AzureShallows, TEXT("Zone_AzureShallows"), TEXT("Azure Shallows"),
            TEXT("A patient sea hiding its drowned bells."),
            ColumnX[3], RowY[1], FLinearColor(0.35f, 0.62f, 0.72f), FLinearColor(0.30f, 0.80f, 1.0f),
            -1400.0f, 700.0f, 0.05f, 2));

        // South row.
        Zones.Add(MakeZone(EAstrawildZone::TidebreakerIsles, TEXT("Zone_TidebreakerIsles"), TEXT("Tidebreaker Isles"),
            TEXT("Broken crowns of a drowned mountain range."),
            ColumnX[0], RowY[2], FLinearColor(0.62f, 0.70f, 0.66f), FLinearColor(0.40f, 0.95f, 0.85f),
            -1600.0f, 3400.0f, 0.55f, 3));

        Zones.Add(MakeZone(EAstrawildZone::StormcrestHighlands, TEXT("Zone_Stormcrest"), TEXT("Stormcrest Highlands"),
            TEXT("Thunder herds graze above the cloudline."),
            ColumnX[1], RowY[2], FLinearColor(0.55f, 0.52f, 0.48f), FLinearColor(0.85f, 0.90f, 1.0f),
            600.0f, 3200.0f, 0.85f, 3));

        Zones.Add(MakeZone(EAstrawildZone::VerdantReach, TEXT("Zone_VerdantReach"), TEXT("Verdant Reach"),
            TEXT("A jungle that grows toward yesterday."),
            ColumnX[2], RowY[2], FLinearColor(0.34f, 0.66f, 0.36f), FLinearColor(0.55f, 1.0f, 0.45f),
            350.0f, 1100.0f, 0.1f, 2));

        Zones.Add(MakeZone(EAstrawildZone::PearlseaReef, TEXT("Zone_PearlseaReef"), TEXT("Pearlsea Reef"),
            TEXT("Coral cathedrals the tide never forgets."),
            ColumnX[3], RowY[2], FLinearColor(0.42f, 0.72f, 0.80f), FLinearColor(0.55f, 0.95f, 1.0f),
            -1500.0f, 2400.0f, 0.35f, 4));
    }
    return Zones;
}

const FAstrawildZoneDescriptor* UAstrawildZoneSubsystem::FindZone(const EAstrawildZone Zone)
{
    for (const FAstrawildZoneDescriptor& Desc : GetAllZones())
    {
        if (Desc.Zone == Zone)
        {
            return &Desc;
        }
    }
    return nullptr;
}

const FAstrawildZoneDescriptor* UAstrawildZoneSubsystem::FindZoneById(const FName ZoneId)
{
    for (const FAstrawildZoneDescriptor& Desc : GetAllZones())
    {
        if (Desc.ZoneId == ZoneId)
        {
            return &Desc;
        }
    }
    return nullptr;
}

EAstrawildZone UAstrawildZoneSubsystem::GetZoneAt(const FVector& WorldLocation)
{
    const FVector2D Point(WorldLocation.X, WorldLocation.Y);
    for (const FAstrawildZoneDescriptor& Desc : GetAllZones())
    {
        if (Point.X >= Desc.Bounds.Min.X && Point.X <= Desc.Bounds.Max.X &&
            Point.Y >= Desc.Bounds.Min.Y && Point.Y <= Desc.Bounds.Max.Y)
        {
            return Desc.Zone;
        }
    }
    return EAstrawildZone::None;
}

void UAstrawildZoneSubsystem::ComputeZoneWeights(const FVector2D& Point, float OutWeights[(int32)EAstrawildZone::Count])
{
    const int32 Count = (int32)EAstrawildZone::Count;
    const TArray<FAstrawildZoneDescriptor>& Zones = GetAllZones();

    float RawSum = 0.0f;
    for (int32 i = 0; i < Count; ++i)
    {
        OutWeights[i] = 0.0f;
    }

    for (const FAstrawildZoneDescriptor& Desc : Zones)
    {
        // Signed distance to the zone rect (0 inside).
        const float Dx = FMath::Max(FMath::Max(Desc.Bounds.Min.X - Point.X, Point.X - Desc.Bounds.Max.X), 0.0f);
        const float Dy = FMath::Max(FMath::Max(Desc.Bounds.Min.Y - Point.Y, Point.Y - Desc.Bounds.Max.Y), 0.0f);
        const float Distance = FMath::Sqrt(Dx * Dx + Dy * Dy);

        const float Ratio = Distance / ZoneBlendDistance;
        const float Weight = 1.0f / (1.0f + Ratio * Ratio);

        const int32 Index = (int32)Desc.Zone;
        if (Index >= 0 && Index < Count)
        {
            OutWeights[Index] = Weight;
            RawSum += Weight;
        }
    }

    if (RawSum <= KINDA_SMALL_NUMBER)
    {
        // Far outside the world: fall back to a uniform blend so the math stays defined.
        for (const FAstrawildZoneDescriptor& Desc : Zones)
        {
            OutWeights[(int32)Desc.Zone] = 1.0f / static_cast<float>(Zones.Num());
        }
        return;
    }

    for (int32 i = 0; i < Count; ++i)
    {
        OutWeights[i] /= RawSum;
    }
}

FBox2D UAstrawildZoneSubsystem::GetWorldBounds()
{
    FBox2D Bounds(ForceInit);
    for (const FAstrawildZoneDescriptor& Desc : GetAllZones())
    {
        Bounds += Desc.Bounds;
    }
    return Bounds;
}

bool UAstrawildZoneSubsystem::IsSeaZone(const EAstrawildZone Zone)
{
    const FAstrawildZoneDescriptor* Desc = FindZone(Zone);
    return Desc && (Zone == EAstrawildZone::AzureShallows || Zone == EAstrawildZone::TidebreakerIsles || Zone == EAstrawildZone::PearlseaReef);
}

bool UAstrawildZoneSubsystem::HasDiscoveredZone(const EAstrawildZone Zone) const
{
    return DiscoveredZones.Contains(Zone);
}

int32 UAstrawildZoneSubsystem::GetDiscoveredZoneCount() const
{
    return DiscoveredZones.Num();
}

EAstrawildZone UAstrawildZoneSubsystem::GetTrackedZoneForPawn(const AActor* Pawn) const
{
    if (const EAstrawildZone* Found = TrackedPawns.Find(Pawn))
    {
        return *Found;
    }
    return EAstrawildZone::None;
}

void UAstrawildZoneSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    const UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    SweepAccumulator += DeltaTime;
    if (SweepAccumulator >= 0.5f)
    {
        SweepAccumulator = 0.0f;
        RunSweep();
    }
}

TStatId UAstrawildZoneSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildZoneSubsystem, STATGROUP_Tickables);
}

bool UAstrawildZoneSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAstrawildZoneSubsystem::ExportForSave(TArray<EAstrawildZone>& OutZones) const
{
    OutZones = DiscoveredZones;
}

void UAstrawildZoneSubsystem::ImportFromSave(const TArray<EAstrawildZone>& InZones)
{
    DiscoveredZones = InZones;
    for (const EAstrawildZone Zone : DiscoveredZones)
    {
        const FAstrawildZoneDescriptor* Desc = FindZone(Zone);
        if (Desc)
        {
            OnZoneDiscovered.Broadcast(Zone, *Desc);
        }
    }
}

void UAstrawildZoneSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    UE_LOG(LogAstrawildWorld, Log, TEXT("Zone subsystem online: %d zones across %s (world seed blended into terrain)."),
        GetZoneCount(), *GetWorldBounds().ToString());
}

void UAstrawildZoneSubsystem::RunSweep()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UAstrawildEventBusSubsystem* EventBus = GetEventBus();

    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        AAstrawildPlayerCharacter* Pawn = *It;
        if (!Pawn)
        {
            continue;
        }

        const EAstrawildZone NewZone = GetZoneAt(Pawn->GetActorLocation());
        const EAstrawildZone* KnownZone = TrackedPawns.Find(Pawn);

        if (KnownZone && *KnownZone == NewZone)
        {
            continue; // No transition.
        }

        const EAstrawildZone OldZone = KnownZone ? *KnownZone : EAstrawildZone::None;
        TrackedPawns.Add(Pawn, NewZone);

        if (EventBus)
        {
            if (OldZone != EAstrawildZone::None)
            {
                if (const FAstrawildZoneDescriptor* OldDesc = FindZone(OldZone))
                {
                    EventBus->PublishEvent(TAG_Astrawild_Event_ZoneLeft, Pawn, OldDesc->ZoneId, 0, Pawn->GetActorLocation());
                }
            }
            if (NewZone != EAstrawildZone::None)
            {
                if (const FAstrawildZoneDescriptor* NewDesc = FindZone(NewZone))
                {
                    EventBus->PublishEvent(TAG_Astrawild_Event_ZoneEntered, Pawn, NewDesc->ZoneId, 0, Pawn->GetActorLocation());
                }
            }
        }

        // Discovery (first visit ever, per save).
        if (NewZone != EAstrawildZone::None && !DiscoveredZones.Contains(NewZone))
        {
            DiscoveredZones.Add(NewZone);
            if (const FAstrawildZoneDescriptor* Desc = FindZone(NewZone))
            {
                UE_LOG(LogAstrawildWorld, Log, TEXT("Zone discovered: %s (%d/%d)."), *Desc->DisplayName.ToString(), DiscoveredZones.Num(), GetZoneCount());
                OnZoneDiscovered.Broadcast(NewZone, *Desc);
            }
        }
    }

    // Drop stale pawns (death/disconnect).
    for (auto It = TrackedPawns.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}

UAstrawildEventBusSubsystem* UAstrawildZoneSubsystem::GetEventBus() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildEventBusSubsystem>() : nullptr;
}
