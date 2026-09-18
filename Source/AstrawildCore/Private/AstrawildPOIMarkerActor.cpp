#include "AstrawildPOIMarkerActor.h"

#include "Net/UnrealNetwork.h" // LCP-2: DOREPLIFETIME

#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
        /** POI-type silhouettes with zero assets — each archetype reads differently. */
        FLinearColor GetPoiBeaconColor(const EAstrawildPOIType Type)
        {
                switch (Type)
                {
                case EAstrawildPOIType::AncientTech:
                case EAstrawildPOIType::SignalSource:
                        return FLinearColor(0.55f, 0.85f, 1.0f); // cold resonance cyan
                case EAstrawildPOIType::Ruin:
                case EAstrawildPOIType::CaveEntrance:
                        return FLinearColor(1.0f, 0.72f, 0.35f); // warm ember
                case EAstrawildPOIType::Watchtower:
                        return FLinearColor(0.4f, 1.0f, 0.55f); // sentinel green
                case EAstrawildPOIType::Landmark:
                default:
                        return FLinearColor(0.95f, 0.9f, 0.6f); // dawn gold
                }
        }
}

AAstrawildPOIMarkerActor::AAstrawildPOIMarkerActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // LCP-2: POI markers replicate (id-driven visuals) so LAN clients see the
    // world's discovery targets.
    bReplicates = true;
    NetUpdateFrequency = 1.0f;

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
    VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 2.6f));
    }

    BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
    BeaconLight->SetupAttachment(RootComponent);
    BeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 320.0f));
    BeaconLight->SetIntensity(8000.0f);
    BeaconLight->SetAttenuationRadius(1400.0f);
    BeaconLight->SetLightColor(FLinearColor(0.95f, 0.9f, 0.6f));
}

void AAstrawildPOIMarkerActor::BeginPlay()
{
    Super::BeginPlay();

    // LCP-2: clients resolve the beacon visual from the replicated id
    // (InitializeFromDefinition's definition pointer never crosses the wire).
    OnRep_PoiId();
}

void AAstrawildPOIMarkerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildPOIMarkerActor, PoiId);
}

void AAstrawildPOIMarkerActor::OnRep_PoiId()
{
    if (PoiId.IsNone())
    {
        return;
    }

    // Resolve the definition from the LOCAL registry (identical content on
    // every machine) and apply the beacon color.
    const UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry =
        World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (UAstrawildPOIDefinition* Def = Registry ? Registry->FindPOI(PoiId) : nullptr)
    {
        if (BeaconLight)
        {
            BeaconLight->SetLightColor(GetPoiBeaconColor(Def->Type));
        }
    }
}

void AAstrawildPOIMarkerActor::InitializeFromDefinition(UAstrawildPOIDefinition* Definition)
{
    if (!IsValid(Definition))
    {
        return;
    }
    PoiId = Definition->PoiId;
    if (BeaconLight)
    {
        BeaconLight->SetLightColor(GetPoiBeaconColor(Definition->Type));
    }
}

void AAstrawildPOIMarkerActor::MarkDiscovered()
{
    // Dim to a quiet ember — the beacon's job (drawing the eye) is done.
    if (BeaconLight)
    {
        BeaconLight->SetIntensity(2000.0f);
        BeaconLight->SetLightColor(FLinearColor(0.4f, 0.4f, 0.42f));
    }
}

FText AAstrawildPOIMarkerActor::GetInteractionPrompt_Implementation() const
{
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildPOIDefinition* Def = Registry ? Registry->FindPOI(PoiId) : nullptr;
    return Def
        ? FText::FromString(FString::Printf(TEXT("Examine %s [E]"), *Def->DisplayName.ToString()))
        : FText::FromString(TEXT("Examine [E]"));
}

void AAstrawildPOIMarkerActor::Interact_Implementation(AActor* InteractingActor)
{
    // Reading the marker reprints the lore line (discovery itself is radius-based
    // in the POI subsystem; E is the flavour interaction).
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildPOIDefinition* Def = Registry ? Registry->FindPOI(PoiId) : nullptr;
    if (!Def)
    {
        return;
    }
    if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
        {
            AstrawildPC->Notify(Def->LoreLine);
        }
    }
    UE_LOG(LogAstrawild, Verbose, TEXT("POI marker examined: %s"), *Def->PoiId.ToString());
}
