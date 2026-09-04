#include "AstrawildEchoCharacter.h"

#include "AstrawildAbilityLibrary.h"
#include "AstrawildCore.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildCreatureSanityComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildGeneticsLibrary.h"
#include "AstrawildMountComponent.h"
#include "AstrawildEchoAIController.h"
#include "AstrawildEcosystemSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildProjectileActor.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildVfxActor.h"
#include "AstrawildWorkSiteActor.h"
#include "AstrawildZoneSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationInvokerComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------------
// Batch 8 — procedural body construction (The Grand Menagerie).
// Vertex-colored ProceduralMesh sections + DebugMeshMaterial (same guaranteed
// color-rendering path as the terrain tiles). Zero art assets required.
// ---------------------------------------------------------------------------
namespace
{
    struct FAstrawildBodyPart
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colors;
    };

    void PushQuad(FAstrawildBodyPart& Part, const int32 A, const int32 B, const int32 C, const int32 D)
    {
        Part.Triangles.Append({ A, B, C, A, C, D });
    }

    void AddBoxPart(FAstrawildBodyPart& Part, const FVector& Center, const FVector& HalfSize, const FColor& Color)
    {
        const int32 Base = Part.Vertices.Num();
        const FVector Corners[8] = {
            Center + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y,  HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y,  HalfSize.Z),
        };
        for (const FVector& Corner : Corners)
        {
            Part.Vertices.Add(Corner);
            Part.Normals.Add((Corner - Center).GetSafeNormal());
            Part.UVs.Add(FVector2D(0.0f, 0.0f));
            Part.Colors.Add(Color);
        }
        PushQuad(Part, Base + 0, Base + 1, Base + 2, Base + 3); // bottom
        PushQuad(Part, Base + 4, Base + 7, Base + 6, Base + 5); // top
        PushQuad(Part, Base + 0, Base + 4, Base + 5, Base + 1); // front
        PushQuad(Part, Base + 3, Base + 2, Base + 6, Base + 7); // back
        PushQuad(Part, Base + 1, Base + 5, Base + 6, Base + 2); // right
        PushQuad(Part, Base + 0, Base + 3, Base + 7, Base + 4); // left
    }

    void AddSpherePart(FAstrawildBodyPart& Part, const FVector& Center, const float Radius, const FColor& Color, const int32 Segments = 10)
    {
        const int32 Rings = FMath::Max(4, Segments);
        const int32 Slices = FMath::Max(4, Segments);
        const int32 Base = Part.Vertices.Num();

        for (int32 Ring = 0; Ring <= Rings; ++Ring)
        {
            const float Phi = PI * static_cast<float>(Ring) / static_cast<float>(Rings);
            for (int32 Slice = 0; Slice <= Slices; ++Slice)
            {
                const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
                const FVector Normal(
                    FMath::Sin(Phi) * FMath::Cos(Theta),
                    FMath::Sin(Phi) * FMath::Sin(Theta),
                    FMath::Cos(Phi));
                Part.Vertices.Add(Center + Normal * Radius);
                Part.Normals.Add(Normal);
                Part.UVs.Add(FVector2D(static_cast<float>(Slice) / Slices, static_cast<float>(Ring) / Rings));
                Part.Colors.Add(Color);
            }
        }

        const auto VertexIndex = [Slices](const int32 Ring, const int32 Slice) -> int32
        {
            return Ring * (Slices + 1) + Slice;
        };

        for (int32 Ring = 0; Ring < Rings; ++Ring)
        {
            for (int32 Slice = 0; Slice < Slices; ++Slice)
            {
                const int32 A = Base + VertexIndex(Ring, Slice);
                const int32 B = Base + VertexIndex(Ring + 1, Slice);
                const int32 C = Base + VertexIndex(Ring + 1, Slice + 1);
                const int32 D = Base + VertexIndex(Ring, Slice + 1);
                Part.Triangles.Append({ A, B, C, A, C, D });
            }
        }
    }

    void AddConePart(FAstrawildBodyPart& Part, const FVector& BaseCenter, const FVector& Tip, const float BaseRadius, const FColor& Color, const int32 Segments = 8)
    {
        const int32 Slices = FMath::Max(3, Segments);
        const FVector Axis = (Tip - BaseCenter).GetSafeNormal();
        const FVector AnyPerp = FMath::Abs(Axis.Z) < 0.95f ? FVector::UpVector : FVector::ForwardVector;
        const FVector Perp1 = FVector::CrossProduct(Axis, AnyPerp).GetSafeNormal();
        const FVector Perp2 = FVector::CrossProduct(Axis, Perp1).GetSafeNormal();

        const int32 Base = Part.Vertices.Num();
        // Base ring.
        for (int32 Slice = 0; Slice < Slices; ++Slice)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
            const FVector Dir = Perp1 * FMath::Cos(Theta) + Perp2 * FMath::Sin(Theta);
            Part.Vertices.Add(BaseCenter + Dir * BaseRadius);
            Part.Normals.Add(-Axis);
            Part.UVs.Add(FVector2D(0.0f, 0.0f));
            Part.Colors.Add(Color);
        }
        // Tip.
        const int32 TipIndex = Part.Vertices.Num();
        Part.Vertices.Add(Tip);
        Part.Normals.Add(Axis);
        Part.UVs.Add(FVector2D(0.5f, 0.5f));
        Part.Colors.Add(Color);
        // Base cap center.
        const int32 CapIndex = Part.Vertices.Num();
        Part.Vertices.Add(BaseCenter);
        Part.Normals.Add(-Axis);
        Part.UVs.Add(FVector2D(0.5f, 0.5f));
        Part.Colors.Add(Color);

        for (int32 Slice = 0; Slice < Slices; ++Slice)
        {
            const int32 A = Base + Slice;
            const int32 B = Base + (Slice + 1) % Slices;
            Part.Triangles.Append({ A, TipIndex, B });
            Part.Triangles.Append({ CapIndex, B, A });
        }
    }

    void AddCylinderPart(FAstrawildBodyPart& Part, const FVector& BottomCenter, const FVector& TopCenter, const float Radius, const FColor& Color, const int32 Segments = 8)
    {
        const int32 Slices = FMath::Max(3, Segments);
        const FVector Axis = (TopCenter - BottomCenter).GetSafeNormal();
        const FVector AnyPerp = FMath::Abs(Axis.Z) < 0.95f ? FVector::UpVector : FVector::ForwardVector;
        const FVector Perp1 = FVector::CrossProduct(Axis, AnyPerp).GetSafeNormal();
        const FVector Perp2 = FVector::CrossProduct(Axis, Perp1).GetSafeNormal();

        const int32 Base = Part.Vertices.Num();
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const FVector RingCenter = Side == 0 ? BottomCenter : TopCenter;
            for (int32 Slice = 0; Slice < Slices; ++Slice)
            {
                const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
                const FVector Dir = Perp1 * FMath::Cos(Theta) + Perp2 * FMath::Sin(Theta);
                Part.Vertices.Add(RingCenter + Dir * Radius);
                Part.Normals.Add(Dir);
                Part.UVs.Add(FVector2D(0.0f, 0.0f));
                Part.Colors.Add(Color);
            }
        }
        for (int32 Slice = 0; Slice < Slices; ++Slice)
        {
            const int32 A0 = Base + Slice;
            const int32 B0 = Base + (Slice + 1) % Slices;
            const int32 A1 = Base + Slices + Slice;
            const int32 B1 = Base + Slices + (Slice + 1) % Slices;
            Part.Triangles.Append({ A0, B0, B1, A0, B1, A1 });
        }
    }

    float BodyScaleForSize(const EAstrawildSizeClass SizeClass)
    {
        switch (SizeClass)
        {
        case EAstrawildSizeClass::Tiny:   return 0.45f;
        case EAstrawildSizeClass::Small:  return 0.7f;
        case EAstrawildSizeClass::Large:  return 1.4f;
        case EAstrawildSizeClass::Huge:   return 1.9f;
        default:                          return 1.0f;
        }
    }
}

void AAstrawildEchoCharacter::BuildProceduralBody()
{
    if (!IsValid(EchoDefinition))
    {
        return;
    }

    if (!BodyMesh)
    {
        BodyMesh = NewObject<UProceduralMeshComponent>(this, TEXT("BodyMesh"));
        BodyMesh->SetupAttachment(GetCapsuleComponent());
        BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BodyMesh->SetRelativeLocation(FVector::ZeroVector);
        BodyMesh->RegisterComponent();
    }

    BodyMesh->ClearAllMeshSections();

    const FColor Primary = EchoDefinition->PrimaryTint.ToFColor(true);
    const FColor Secondary = EchoDefinition->SecondaryTint.ToFColor(true);
    const FColor Accent(
        FMath::Min(255, Primary.R + 60),
        FMath::Min(255, Primary.G + 60),
        FMath::Min(255, Primary.B + 60),
        255);
    const float S = BodyScaleForSize(EchoDefinition->SizeClass);

    FAstrawildBodyPart Body;

    switch (EchoDefinition->BodyPlan)
    {
    case EAstrawildBodyPlan::Quadruped:
    {
        // Torso + head + four legs + tail cone.
        AddSpherePart(Body, FVector(0, 0, 55 * S), 42 * S, Primary, 8);
        AddSpherePart(Body, FVector(52 * S, 0, 78 * S), 24 * S, Secondary, 8);
        AddConePart(Body, FVector(52 * S, 0, 78 * S), FVector(80 * S, 0, 92 * S), 9 * S, Accent, 6); // snout/horn
        const float LegX = 30 * S;
        const float LegY = 26 * S;
        for (const FVector2D Corner : { FVector2D(LegX, LegY), FVector2D(LegX, -LegY), FVector2D(-LegX, LegY), FVector2D(-LegX, -LegY) })
        {
            AddCylinderPart(Body, FVector(Corner.X, Corner.Y, 40 * S), FVector(Corner.X, Corner.Y, 2 * S), 9 * S, Secondary, 6);
        }
        AddConePart(Body, FVector(-44 * S, 0, 62 * S), FVector(-92 * S, 0, 74 * S), 10 * S, Secondary, 6);
        break;
    }
    case EAstrawildBodyPlan::Biped:
    {
        AddSpherePart(Body, FVector(0, 0, 72 * S), 38 * S, Primary, 8);
        AddSpherePart(Body, FVector(0, 0, 122 * S), 22 * S, Secondary, 8);
        AddConePart(Body, FVector(0, 0, 138 * S), FVector(0, 0, 172 * S), 10 * S, Accent, 6); // crest
        AddCylinderPart(Body, FVector(16 * S, 0, 58 * S), FVector(30 * S, 0, 96 * S), 8 * S, Secondary, 6); // arms
        AddCylinderPart(Body, FVector(-16 * S, 0, 58 * S), FVector(-30 * S, 0, 96 * S), 8 * S, Secondary, 6);
        AddCylinderPart(Body, FVector(14 * S, 0, 36 * S), FVector(14 * S, 0, 2 * S), 10 * S, Secondary, 6); // legs
        AddCylinderPart(Body, FVector(-14 * S, 0, 36 * S), FVector(-14 * S, 0, 2 * S), 10 * S, Secondary, 6);
        break;
    }
    case EAstrawildBodyPlan::Serpent:
    {
        // Rising S-curve of segments + wedge head.
        const float SegX[6] = { -80, -44, -12, 20, 48, 66 };
        const float SegZ[6] = { 6, 26, 48, 68, 82, 90 };
        const float SegR[6] = { 9, 15, 21, 25, 22, 17 };
        for (int32 i = 0; i < 6; ++i)
        {
            AddSpherePart(Body, FVector(SegX[i] * S, 0, SegZ[i] * S), SegR[i] * S, i % 2 == 0 ? Primary : Secondary, 8);
        }
        AddConePart(Body, FVector(66 * S, 0, 96 * S), FVector(112 * S, 0, 108 * S), 14 * S, Accent, 6); // head wedge
        break;
    }
    case EAstrawildBodyPlan::Floating:
    {
        AddSpherePart(Body, FVector(0, 0, 95 * S), 34 * S, Primary, 10);
        AddSpherePart(Body, FVector(30 * S, 22 * S, 108 * S), 12 * S, Secondary, 6);
        AddSpherePart(Body, FVector(-28 * S, 24 * S, 88 * S), 10 * S, Secondary, 6);
        AddSpherePart(Body, FVector(-24 * S, -26 * S, 112 * S), 11 * S, Secondary, 6);
        AddConePart(Body, FVector(0, 0, 62 * S), FVector(0, 0, 18 * S), 20 * S, Accent, 6); // energy tail
        break;
    }
    case EAstrawildBodyPlan::Insectoid:
    {
        AddSpherePart(Body, FVector(34 * S, 0, 55 * S), 17 * S, Secondary, 8); // head
        AddSpherePart(Body, FVector(6 * S, 0, 52 * S), 24 * S, Primary, 8); // thorax
        AddSpherePart(Body, FVector(-34 * S, 0, 48 * S), 28 * S, Primary, 8); // abdomen
        AddCylinderPart(Body, FVector(34 * S, 10 * S, 66 * S), FVector(50 * S, 16 * S, 92 * S), 3 * S, Secondary, 5); // antennae
        AddCylinderPart(Body, FVector(34 * S, -10 * S, 66 * S), FVector(50 * S, -16 * S, 92 * S), 3 * S, Secondary, 5);
        const float LegX[4] = { 24, 8, -16, -34 };
        for (int32 i = 0; i < 4; ++i)
        {
            AddCylinderPart(Body, FVector(LegX[i] * S, 14 * S, 44 * S), FVector(LegX[i] * S, 30 * S, 4 * S), 4 * S, Secondary, 5);
            AddCylinderPart(Body, FVector(LegX[i] * S, -14 * S, 44 * S), FVector(LegX[i] * S, -30 * S, 4 * S), 4 * S, Secondary, 5);
        }
        break;
    }
    case EAstrawildBodyPlan::Avian:
    {
        AddSpherePart(Body, FVector(0, 0, 62 * S), 30 * S, Primary, 8); // keeled body
        AddSpherePart(Body, FVector(30 * S, 0, 86 * S), 16 * S, Primary, 8); // head
        AddConePart(Body, FVector(42 * S, 0, 84 * S), FVector(64 * S, 0, 88 * S), 6 * S, Accent, 5); // beak
        // Folded wings (flattened boxes).
        AddBoxPart(Body, FVector(-6 * S, 34 * S, 70 * S), FVector(26 * S, 6 * S, 20 * S), Secondary);
        AddBoxPart(Body, FVector(-6 * S, -34 * S, 70 * S), FVector(26 * S, 6 * S, 20 * S), Secondary);
        AddBoxPart(Body, FVector(-34 * S, 0, 58 * S), FVector(14 * S, 16 * S, 4 * S), Secondary); // tail fan
        AddCylinderPart(Body, FVector(6 * S, 8 * S, 34 * S), FVector(8 * S, 8 * S, 6 * S), 4 * S, Secondary, 5); // legs
        AddCylinderPart(Body, FVector(6 * S, -8 * S, 34 * S), FVector(8 * S, -8 * S, 6 * S), 4 * S, Secondary, 5);
        break;
    }
    case EAstrawildBodyPlan::Crystalline:
    {
        // Faceted shard crown: big center + orbiting shards.
        AddConePart(Body, FVector(0, 0, 4 * S), FVector(0, 0, 120 * S), 34 * S, Primary, 4);
        AddConePart(Body, FVector(28 * S, 0, 4 * S), FVector(34 * S, 0, 78 * S), 14 * S, Secondary, 4);
        AddConePart(Body, FVector(-26 * S, 10 * S, 4 * S), FVector(-32 * S, 14 * S, 64 * S), 12 * S, Secondary, 4);
        AddConePart(Body, FVector(-20 * S, -18 * S, 4 * S), FVector(-24 * S, -24 * S, 52 * S), 10 * S, Secondary, 4);
        break;
    }
    case EAstrawildBodyPlan::Amorphous:
    {
        AddSpherePart(Body, FVector(0, 0, 48 * S), 34 * S, Primary, 8);
        AddSpherePart(Body, FVector(24 * S, 14 * S, 62 * S), 22 * S, Primary, 8);
        AddSpherePart(Body, FVector(-22 * S, 18 * S, 54 * S), 18 * S, Primary, 8);
        AddSpherePart(Body, FVector(-14 * S, -22 * S, 66 * S), 20 * S, Primary, 8);
        AddSpherePart(Body, FVector(18 * S, -20 * S, 44 * S), 16 * S, Primary, 8);
        AddSpherePart(Body, FVector(0, 0, 58 * S), 12 * S, Accent, 8); // inner glow core
        break;
    }
    default:
    {
        AddSpherePart(Body, FVector(0, 0, 55 * S), 36 * S, Primary, 8);
        break;
    }
    }

    if (Body.Vertices.Num() > 0)
    {
        BodyMesh->CreateMeshSection(0, Body.Vertices, Body.Triangles, Body.Normals, Body.UVs, Body.Colors, TArray<FProcMeshTangent>(), false);

        // Same guaranteed vertex-color material path as the terrain tiles.
        UMaterial* BodyMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
        if (BodyMaterial)
        {
            BodyMesh->SetMaterial(0, BodyMaterial);
        }

        // Hide the legacy placeholder sphere — the silhouette takes over.
        if (PlaceholderMesh)
        {
            PlaceholderMesh->SetVisibility(false);
            PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        // Scale the capsule so Huge creatures actually feel huge and Tiny ones
        // can hide in the grass (gameplay collision matches the silhouette).
        const float CapsuleRadius = 34.0f * S;
        const float CapsuleHalfHeight = FMath::Max(CapsuleRadius + 10.0f, 60.0f * S);
        GetCapsuleComponent()->SetCapsuleSize(CapsuleRadius, CapsuleHalfHeight);
    }

    // Production V2 Batch 2: rarity ring + element glow identity.
    ApplyVisualIdentity();

    UE_LOG(LogAstrawildAI, Verbose, TEXT("Built procedural body for %s (plan %d, size %d)."),
        *EchoDefinition->DefinitionId.ToString(),
        static_cast<int32>(EchoDefinition->BodyPlan),
        static_cast<int32>(EchoDefinition->SizeClass));
}

void AAstrawildEchoCharacter::ApplyVisualIdentity()
{
    // Rarity ring (Rare+ only — Common/Uncommon stay clean to avoid noise): a
    // flattened annulus at the feet in the rarity color, appended as BodyMesh
    // section 1 on the shared vertex-color material.
    if (BodyMesh && EchoDefinition && EchoDefinition->Rarity >= EAstrawildRarity::Rare)
    {
        const FLinearColor RarityTint = FAstrawildVfxPalette::GetRarityTint(EchoDefinition->Rarity);
        const FColor RingColor = FLinearColor(
            FMath::Clamp(RarityTint.R * 0.6f + 0.05f, 0.0f, 1.0f),
            FMath::Clamp(RarityTint.G * 0.6f + 0.05f, 0.0f, 1.0f),
            FMath::Clamp(RarityTint.B * 0.6f + 0.05f, 0.0f, 1.0f), 1.0f).ToFColor(false);
        const FColor RingBright = FLinearColor(
            FMath::Clamp(RarityTint.R * 1.2f + 0.10f, 0.0f, 1.0f),
            FMath::Clamp(RarityTint.G * 1.2f + 0.10f, 0.0f, 1.0f),
            FMath::Clamp(RarityTint.B * 1.2f + 0.10f, 0.0f, 1.0f), 1.0f).ToFColor(false);

        const float CapsuleRadius = GetCapsuleComponent() ? GetCapsuleComponent()->GetUnscaledCapsuleRadius() : 34.0f;
        const float Outer = CapsuleRadius * 1.18f;
        const float Inner = Outer - 9.0f;

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colors;

        constexpr int32 Segments = 26;
        constexpr float RingZ = 7.0f;
        for (int32 Slice = 0; Slice <= Segments; ++Slice)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Segments);
            const float CosT = FMath::Cos(Theta);
            const float SinT = FMath::Sin(Theta);
            Vertices.Add(FVector(CosT * Outer, SinT * Outer, RingZ));
            Normals.Add(FVector(0, 0, 1));
            UVs.Add(FVector2D(1.0f, 0.0f));
            Colors.Add(RingBright);
            Vertices.Add(FVector(CosT * Inner, SinT * Inner, RingZ));
            Normals.Add(FVector(0, 0, 1));
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(RingColor);
        }
        for (int32 Slice = 0; Slice < Segments; ++Slice)
        {
            const int32 A = Slice * 2;
            const int32 B = (Slice + 1) * 2;
            const int32 C = (Slice + 1) * 2 + 1;
            const int32 D = Slice * 2 + 1;
            Triangles.Append({ A, B, C, A, C, D });
        }

        BodyMesh->CreateMeshSection(1, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
        UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
        if (Material)
        {
            BodyMesh->SetMaterial(1, Material);
        }
    }

    // Element glow preparation: tint + height by size class (intensity set in
    // UpdateElementGlow — captured party members and nearby wild elementals only).
    if (ElementGlowLight && EchoDefinition)
    {
        ElementGlowLight->SetLightColor(FAstrawildVfxPalette::GetElementTint(EchoDefinition->Element));
        const float SizeScale = EchoDefinition ? BodyScaleForSize(EchoDefinition->SizeClass) : 1.0f;
        ElementGlowLight->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f * SizeScale));
    }
}

void AAstrawildEchoCharacter::UpdateElementGlow()
{
    // Runs everywhere (local cosmetic): captured Echoes always glow; wild
    // elementals glow only near a player pawn — the active light count stays
    // bounded no matter how many of the 200+ species roam the world.
    if (!ElementGlowLight || !EchoDefinition || EchoDefinition->Element == EAstrawildElementType::None || IsDefeated())
    {
        if (ElementGlowLight)
        {
            ElementGlowLight->SetIntensity(0.0f);
        }
        return;
    }

    if (bCaptured)
    {
        ElementGlowLight->SetIntensity(2.4f);
        return;
    }

    // Wild elemental: glow only when a player is close enough for it to matter.
    constexpr float GlowProximity = 3200.0f;
    bool bPlayerNear = false;
    if (UWorld* World = GetWorld())
    {
        for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
        {
            if (const APlayerController* PC = Cast<APlayerController>(*It))
            {
                if (const APawn* Pawn = PC->GetPawn())
                {
                    if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) < GlowProximity * GlowProximity)
                    {
                        bPlayerNear = true;
                        break;
                    }
                }
            }
        }
    }
    ElementGlowLight->SetIntensity(bPlayerNear ? 1.9f : 0.0f);
}

AAstrawildEchoCharacter::AAstrawildEchoCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    InstanceId = FGuid::NewGuid();

    bReplicates = true;
    SetReplicatingMovement(true);

    // Server-driven C++ AI that works without Behavior Tree assets (directive §6);
    // future BT/StateTree assets can possess through the same controller.
    AIControllerClass = AAstrawildEchoAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.8f));
    }

    // Audit C-3: runtime navmesh generation anchor — without an authored navmesh in
    // the zero-asset world, this invoker makes tiles generate around the creature so
    // all MoveTo* pathfinding works (project setting: navigation generation around
    // invokers only).
    NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
    NavInvoker->SetGenerationRadii(5000.0f, 7000.0f);

    // Production V2 Batch 2: element identity light — dark until the glow update
    // enables it for party members / nearby wild elementals (light budget stays tiny).
    ElementGlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ElementGlowLight"));

    // SCP Phase 9: sanity/illness simulation (server ticks, replicated state).
    SanityComponent = CreateDefaultSubobject<UAstrawildCreatureSanityComponent>(TEXT("Sanity"));

    // SCP Phase 5: riding contract (rider attach + input-driven movement).
    MountComponent = CreateDefaultSubobject<UAstrawildMountComponent>(TEXT("Mount"));
    ElementGlowLight->SetupAttachment(GetCapsuleComponent());
    ElementGlowLight->SetCastShadows(false);
    ElementGlowLight->SetIntensity(0.0f);
    ElementGlowLight->SetAttenuationRadius(520.0f);
    ElementGlowLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
}

void AAstrawildEchoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildEchoCharacter, Personality);
    DOREPLIFETIME(AAstrawildEchoCharacter, Needs);
    DOREPLIFETIME(AAstrawildEchoCharacter, Experience);
    DOREPLIFETIME(AAstrawildEchoCharacter, CurrentAIState);
    DOREPLIFETIME(AAstrawildEchoCharacter, ActiveCommand);
    DOREPLIFETIME(AAstrawildEchoCharacter, OwnerPlayerId);
    DOREPLIFETIME(AAstrawildEchoCharacter, StatusEffects);
    DOREPLIFETIME(AAstrawildEchoCharacter, AbilityCooldowns);
}

void AAstrawildEchoCharacter::BeginPlay()
{
    Super::BeginPlay();

    RegisterWithEcosystem();

    if (EchoDefinition)
    {
        InitializeFromDefinition(EchoDefinition, InstanceId);
    }
    else
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Echo %s has no EchoDefinition assigned."), *GetName());
    }
}

void AAstrawildEchoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // FCR-1-c fix (H-c3): an echo destroyed while ridden (QuickLoad during a ride,
    // roster despawn, save-load teardown) used to leave the rider soft-locked —
    // movement disabled, capsule collision off, no E-dismount target. Force the
    // mount cleanup BEFORE teardown; the component tolerates an invalid rider.
    if (MountComponent && MountComponent->IsMounted())
    {
        MountComponent->DismountRider();
    }

    UnregisterFromEcosystem();
    Super::EndPlay(EndPlayReason);
}

void AAstrawildEchoCharacter::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Production V2 Batch 2: element glow proximity update (1s cadence, local
    // cosmetic — runs on every machine against replicated bCaptured).
    {
        ElementGlowAccumulator += DeltaTime;
        if (ElementGlowAccumulator >= 1.0f)
        {
            ElementGlowAccumulator = 0.0f;
            UpdateElementGlow();
        }
    }

    // Needs + bond simulate server-side only (directive §28), throttled by LOD tier.
    if (GetLocalRole() == ROLE_Authority && !IsDefeated())
    {
        HandleNeedsDecay(DeltaTime);

        // Production V2 (Master Plan §6): party passive auras — captured Echoes
        // radiate their passive while healthy (throttled to once per second).
        if (bCaptured && IsValid(EchoDefinition) && EchoDefinition->Passive != EAstrawildEchoPassive::None)
        {
            PassiveAuraAccumulator += DeltaTime;
            if (PassiveAuraAccumulator >= 1.0f)
            {
                const float AuraSeconds = PassiveAuraAccumulator;
                PassiveAuraAccumulator = 0.0f;
                ApplyPartyPassive(AuraSeconds);
            }
        }

        // Batch 3 — Item A: status ticks (DoT + expiry + speed multiplier).
        // FCR-1-a fix: capture the PREVIOUS COMBINED multiplier (status x locomotion)
        // BEFORE the ticks — the old status-only capture compared against a product
        // that already included the CURRENT locomotion value on both sides, making
        // zone-crossing speed changes (GDP-2 water species) mathematically invisible.
        const float PreviousCombinedMultiplier = GetStatusSpeedMultiplier() * GetLocomotionSpeedMultiplier();
        ApplyStatusTicks(DeltaTime);

        // GDP-1: ability cooldown countdown (0.25s cadence, replicated for HUD readiness).
        if (!AbilityCooldowns.IsEmpty())
        {
            AbilityCooldownAccumulator += DeltaTime;
            if (AbilityCooldownAccumulator >= 0.25f)
            {
                const float Step = AbilityCooldownAccumulator;
                AbilityCooldownAccumulator = 0.0f;
                for (TPair<FName, float>& Pair : AbilityCooldowns)
                {
                    Pair.Value = FMath::Max(0.0f, Pair.Value - Step);
                }
                // Prune finished entries so the map (and its replication) stays tiny.
                for (auto It = AbilityCooldowns.CreateIterator(); It; ++It)
                {
                    if (It->Value <= 0.0f)
                    {
                        It.RemoveCurrent();
                    }
                }
            }
        }

        // Batch 3 — Item B: stagger countdown → restore speed + AI state on expiry.
        if (StaggerRemainingSeconds > 0.0f)
        {
            StaggerRemainingSeconds = FMath::Max(0.0f, StaggerRemainingSeconds - DeltaTime);
            if (StaggerRemainingSeconds <= 0.0f)
            {
                if (CurrentAIState == EAstrawildEchoAIState::Staggered)
                {
                    SetAIState(EAstrawildEchoAIState::Idle);
                }
                // REVIEW-3 (M-1): explicitly restore walk speed here — the status
                // multiplier below only recomputes when IT changes, and stagger does
                // not affect it. Without this, a staggered creature stayed at 0 speed.
                if (UCharacterMovementComponent* Movement = GetCharacterMovement())
                {
                    Movement->MaxWalkSpeed = FMath::Max(0.0f, CachedStats.MoveSpeed * GetStatusSpeedMultiplier() * GetLocomotionSpeedMultiplier());
                }
            }
        }

        // Recompute walk speed when the combined status/locomotion multiplier changed
        // (Chill/Shock, GDP-2 water species crossing zone borders, and Surge buffs).
        // FCR-1-c fix: never stomp the MOUNT speed while ridden — the mount component
        // owns MaxWalk/MaxFly speed for the whole ride (1.25x) and re-applies it on
        // dismount; the unguarded recompute used to silently revert the ride to base
        // species speed mid-ride whenever any status/zone multiplier changed.
        const bool bMounted = MountComponent && MountComponent->IsMounted();
        const float NewSpeedMultiplier = GetStatusSpeedMultiplier() * GetLocomotionSpeedMultiplier();
        if (!bMounted && !FMath::IsNearlyEqual(PreviousCombinedMultiplier, NewSpeedMultiplier))
        {
            if (UCharacterMovementComponent* Movement = GetCharacterMovement())
            {
                Movement->MaxWalkSpeed = FMath::Max(0.0f,
                    CachedStats.MoveSpeed * (IsStaggered() ? 0.0f : NewSpeedMultiplier));
            }
        }
    }
}

UAstrawildEcosystemSubsystem* AAstrawildEchoCharacter::GetEcosystem() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildEcosystemSubsystem>() : nullptr;
}

void AAstrawildEchoCharacter::RegisterWithEcosystem()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
        {
            Ecosystem->RegisterEcho(this);
        }
    }
}

void AAstrawildEchoCharacter::UnregisterFromEcosystem()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
        {
            Ecosystem->UnregisterEcho(this);
        }
    }
}

bool AAstrawildEchoCharacter::InitializeFromDefinition(UAstrawildEchoDefinition* InDefinition, const FGuid& OptionalInstanceId)
{
    if (!IsValid(InDefinition) || InDefinition->DefinitionId.IsNone())
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Echo initialization rejected: invalid definition."));
        return false;
    }

    EchoDefinition = InDefinition;
    CachedStats = InDefinition->BaseStats;
    CurrentHealth = FMath::Max(1.0f, CachedStats.MaxHealth);
    Trust = FMath::Max(0.0f, Trust);
    InstanceId = OptionalInstanceId.IsValid() ? OptionalInstanceId : FGuid::NewGuid();

    if (Personality == EAstrawildPersonality::Curious && !bCaptured)
    {
        RollPersonalityFromDefinition();
    }

    // FCR-1-a fix (H-a3): every spawn path creates the pawn BEFORE calling this
    // method, while AutoPossessAI completes inside SpawnActor — the AIController's
    // OnPossess therefore saw a NULL EchoDefinition (locomotion defaulted Land)
    // and MOVE_Flying was never applied: all flying species walked. Applying the
    // movement mode HERE — after the definition exists — is race-free; the
    // controller-side application remains as a redundant safety net.
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        // Include the locomotion multiplier from the start (FCR-1-a H-a2 companion:
        // a Water species spawning on dry land starts at x0.85, not x1.0).
        Movement->MaxWalkSpeed = FMath::Max(0.0f, CachedStats.MoveSpeed * GetLocomotionSpeedMultiplier());
        if (GetLocomotionClass() == EAstrawildLocomotionClass::Flying)
        {
            Movement->SetMovementMode(MOVE_Flying);
            // Species speed, not the engine default 600 (audit follow-up).
            Movement->MaxFlySpeed = FMath::Max(0.0f, CachedStats.MoveSpeed);
        }
    }

    // Audit C-9 (final run): re-register now that the species is set — BeginPlay
    // registered this Echo before the definition existed, so the population count
    // missed it (the subsystem counts idempotently by actor key).
    RegisterWithEcosystem();

    // Batch 8: assemble the species silhouette (no-op for definitions without
    // appearance data — the legacy placeholder sphere stays visible).
    // Art pack (Batch 4, CP-02): the skinned species mesh replaces the PMC
    // silhouette when its soft ref resolves; otherwise the procedural body stays.
    bSkeletalBodyActive = TryActivateSkeletalBody();
    if (!bSkeletalBodyActive)
    {
        BuildProceduralBody();
    }
    return true;
}

bool AAstrawildEchoCharacter::TryActivateSkeletalBody()
{
    if (!IsValid(EchoDefinition) || !EchoDefinition->SkeletalMesh.IsValid())
    {
        return false;
    }
    USkeletalMesh* SkelMesh = EchoDefinition->SkeletalMesh.LoadSynchronous();
    if (!SkelMesh)
    {
        return false;
    }

    EchoBodyMesh = NewObject<USkeletalMeshComponent>(this, TEXT("EchoBodyMesh"));
    if (!EchoBodyMesh)
    {
        return false;
    }
    EchoBodyMesh->SetupAttachment(GetCapsuleComponent());
    EchoBodyMesh->SetSkeletalMesh(SkelMesh);
    EchoBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EchoBodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    // Size-class scale mirrors BodyScaleForSize so Huge/Large/Small species read.
    const float S = BodyScaleForSize(EchoDefinition->SizeClass);
    const float HalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : (60.0f * S);
    EchoBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -HalfHeight));
    EchoBodyMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    EchoBodyMesh->SetRelativeScale3D(FVector(S));
    EchoBodyMesh->RegisterComponent();

    // Warm the locomotion clips + start the idle loop.
    EchoDefinition->IdleAnimation.LoadSynchronous();
    EchoDefinition->MoveAnimation.LoadSynchronous();
    UpdateSkeletalAnimation();

    // Cadence: idle/move selection follows AI speed without per-tick cost.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(EchoAnimTimerHandle, this,
            &AAstrawildEchoCharacter::UpdateSkeletalAnimation, 0.15f, true);
    }
    return true;
}

void AAstrawildEchoCharacter::UpdateSkeletalAnimation()
{
    if (!EchoBodyMesh || !IsValid(EchoDefinition))
    {
        return;
    }
    UAnimSequenceBase* Target = GetVelocity().Size() < 60.0f
        ? EchoDefinition->IdleAnimation.Get()
        : EchoDefinition->MoveAnimation.Get();
    if (Target && Target != CurrentLoopAnimation)
    {
        EchoBodyMesh->PlayAnimation(Target, true);
        CurrentLoopAnimation = Target;
    }
}

bool AAstrawildEchoCharacter::InitializeFromDefinitionWithPersonality(UAstrawildEchoDefinition* InDefinition, const EAstrawildPersonality InPersonality, const FGuid& OptionalInstanceId)
{
    Personality = InPersonality;
    return InitializeFromDefinition(InDefinition, OptionalInstanceId);
}

void AAstrawildEchoCharacter::RollPersonalityFromDefinition()
{
    if (!IsValid(EchoDefinition))
    {
        return;
    }

    // 70% species-dominant personality, 30% random archetype — creatures feel varied (directive §5).
    if (FMath::FRand() < 0.7f)
    {
        Personality = EchoDefinition->DominantPersonality;
    }
    else
    {
        const int32 Roll = FMath::RandRange(0, 9);
        Personality = static_cast<EAstrawildPersonality>(Roll);
    }
}

void AAstrawildEchoCharacter::SetAIState(const EAstrawildEchoAIState NewState)
{
    // Public entry (audit H-8): broadcast on every transition so UI/audio observers
    // can react to AI state changes.
    if (CurrentAIState != NewState)
    {
        CurrentAIState = NewState;
        OnAIStateChanged.Broadcast(this, NewState);
    }
}

// --- Batch 3 — Item A: status effects ---

void AAstrawildEchoCharacter::AddStatusEffect(const FAstrawildStatusEffect& Effect)
{
    if (GetLocalRole() != ROLE_Authority || Effect.StatusId.IsNone() || Effect.RemainingSeconds <= 0.0f)
    {
        return;
    }

    // Refresh if already applied, otherwise append (mirrors SurvivalComponent).
    if (FAstrawildStatusEffect* Existing = StatusEffects.FindByPredicate(
        [&Effect](const FAstrawildStatusEffect& Item) { return Item.StatusId == Effect.StatusId; }))
    {
        *Existing = Effect;
    }
    else
    {
        StatusEffects.Add(Effect);
    }
}

bool AAstrawildEchoCharacter::HasStatusEffect(const FName StatusId) const
{
    return StatusEffects.ContainsByPredicate(
        [&StatusId](const FAstrawildStatusEffect& Item) { return Item.StatusId == StatusId; });
}

float AAstrawildEchoCharacter::GetStatusSpeedMultiplier() const
{
    // Combined multiplicative status effect on speed. Slow effects (Chill 0.5,
    // Shock 0.3, SprainedAnkle 0.75, hitstop 0.15) multiply below 1.0; GDP-1
    // Mobility/Surge buffs multiply ABOVE 1.0 (clamped 1.0-2.5 at grant).
    // FCR-1-a fix: the old filter (only < 1.0) silently no-oped every speed BUFF —
    // now all non-unit multipliers apply and the product is clamped to a sane
    // band so a Surge can never stack into an exploit.
    float Multiplier = 1.0f;
    for (const FAstrawildStatusEffect& Effect : StatusEffects)
    {
        if (Effect.SpeedMultiplier > 0.0f && !FMath::IsNearlyEqual(Effect.SpeedMultiplier, 1.0f))
        {
            Multiplier *= Effect.SpeedMultiplier;
        }
    }
    // SCP Phase 9: illness slows stack with combat statuses (SprainedAnkle 0.75).
    if (SanityComponent)
    {
        Multiplier *= SanityComponent->GetSpeedMultiplier();
    }
    return FMath::Clamp(Multiplier, 0.05f, 2.5f);
}

void AAstrawildEchoCharacter::ApplyStatusTicks(const float DeltaTime)
{
    if (StatusEffects.IsEmpty())
    {
        return;
    }

    bool bAnyExpired = false;
    bool bHealthDecreasedThisTick = false;
    for (int32 i = StatusEffects.Num() - 1; i >= 0; --i)
    {
        FAstrawildStatusEffect& Effect = StatusEffects[i];
        Effect.RemainingSeconds -= DeltaTime;
        if (Effect.DamagePerSecond > 0.0f)
        {
            CurrentHealth = FMath::Max(0.0f, CurrentHealth - Effect.DamagePerSecond * DeltaTime);
            bHealthDecreasedThisTick = true;
        }
        // GDP-1: negative DPS = heal over time (Blessing-style restore statuses),
        // clamped to max health so ward healing can never inflate a creature.
        else if (Effect.DamagePerSecond < 0.0f && !IsDefeated())
        {
            CurrentHealth = FMath::Min(GetMaxHealth(), CurrentHealth - Effect.DamagePerSecond * DeltaTime);
        }
        if (Effect.RemainingSeconds <= 0.0f)
        {
            StatusEffects.RemoveAt(i);
            bAnyExpired = true;
        }
    }

    if (bAnyExpired)
    {
        // FCR-1-a fix (L-a13): a self-healing creature used to BROADCAST OnDamaged
        // merely because a status expired — the AI's HandleDamaged read that as an
        // attack and wild self-healers aggroed the nearest player for healing.
        // Broadcast only when this tick actually REDUCED health (a DoT tick).
        if (bHealthDecreasedThisTick)
        {
            OnDamaged.Broadcast(this, CurrentHealth);
        }
        if (IsDefeated())
        {
            // DoT can finish a creature — route through the standard defeat pipeline
            // (loot, events, quest credit) exactly like a direct hit.
            OnDefeated.Broadcast(this);
            if (IsValid(EchoDefinition))
            {
                if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
                {
                    Ecosystem->OnEchoDefeated(this);
                }
                if (UWorld* World = GetWorld())
                {
                    if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
                    {
                        const bool bWasHostile = EchoDefinition->bHostileToPlayers;
                        // FCR-1-d fix (M-d8): a CAPTURED echo dying is player pressure
                        // (distinct event) — not a skill signal. Wild passives keep the
                        // EchoDefeated tag; DDA leans IN to help after party losses.
                        const FNativeGameplayTag DefeatTag = bWasHostile ? TAG_Astrawild_Event_HostileDefeated
                            : (bCaptured ? TAG_Astrawild_Event_PartyEchoDefeated : TAG_Astrawild_Event_EchoDefeated);
                        EventBus->PublishEvent(
                            DefeatTag,
                            GetInstigator(),
                            EchoDefinition->DefinitionId,
                            1,
                            GetActorLocation());
                    }
                }
            }
        }
    }
}

// --- Batch 3 — Item B: stagger ---

void AAstrawildEchoCharacter::ApplyStagger(const float Seconds)
{
    if (GetLocalRole() != ROLE_Authority || Seconds <= 0.0f || IsDefeated())
    {
        return;
    }

    // Clamp so stacked sources can never perma-lock a creature.
    StaggerRemainingSeconds = FMath::Max(StaggerRemainingSeconds, FMath::Min(Seconds, 2.0f));
    SetAIState(EAstrawildEchoAIState::Staggered);

    // Zero movement immediately (restored by Tick when the countdown expires).
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = 0.0f;
    }
}

bool AAstrawildEchoCharacter::ApplyDamage(const float DamageAmount)
{
    return ApplyElementalDamage(DamageAmount, EAstrawildElementType::None) > 0.0f;
}

float AAstrawildEchoCharacter::ApplyElementalDamage(const float DamageAmount, const EAstrawildElementType InElement)
{
    if (GetLocalRole() != ROLE_Authority || DamageAmount <= 0.0f || IsDefeated())
    {
        return 0.0f;
    }

    float Damage = DamageAmount;

    // GDP-1: defensive abilities (Photon Veil / Stone Skin / Glacial Wall / Shell
    // statuses) halve incoming damage while active — a real, readable shield.
    if (HasStatusEffect(TEXT("Shell")))
    {
        Damage *= 0.5f;
    }

    // DP-3: party element resonance (Aurora Veil / Steam Veil rows) — a captured
    // party echo standing in a two-element party shrugs a fraction of every hit.
    if (bCaptured)
    {
        const FAstrawildPartyResonance Resonance = GetOwnerPartyResonance();
        if (Resonance.IsValid())
        {
            Damage *= (1.0f - FMath::Clamp(Resonance.DamageMitigation, 0.0f, 0.5f));
        }
    }

    // Elemental interactions (directive §9): weakness x1.5, matching element resisted.
    if (IsValid(EchoDefinition))
    {
        if (InElement != EAstrawildElementType::None && InElement == EchoDefinition->WeaknessElement)
        {
            Damage *= 1.5f;
        }
        else if (InElement != EAstrawildElementType::None && InElement == EchoDefinition->Element)
        {
            Damage *= (1.0f - EchoDefinition->ElementalResistance);
        }
    }

    const float MitigatedDamage = FMath::Max(0.0f, Damage - GetDefense());
    if (MitigatedDamage <= 0.0f)
    {
        return 0.0f;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - MitigatedDamage);
    OnDamaged.Broadcast(this, CurrentHealth);

    // Batch 3 — Item A: apply the element's status effect (Burn/Chill/Poison/Shock)
    // through the shared factory. One vocabulary for player weapons and Echo attacks.
    if (InElement != EAstrawildElementType::None)
    {
        const FAstrawildStatusEffect StatusEffect = UAstrawildCombatComponent::MakeElementalStatusEffect(InElement, MitigatedDamage);
        if (!StatusEffect.StatusId.IsNone())
        {
            AddStatusEffect(StatusEffect);
        }
    }

    // Batch 3 — Item B: heavy hits stagger — a single hit at or above 20% of max
    // health interrupts AI and zeroes movement briefly (hit reaction, zero-asset).
    if (MitigatedDamage >= GetMaxHealth() * 0.2f)
    {
        ApplyStagger(0.8f);
    }

    // Aggressive/Brave personalities fight back harder; the AI controller listens to OnDamaged.
    if (IsDefeated())
    {
        OnDefeated.Broadcast(this);

        // Loot + events (server-side).
        if (IsValid(EchoDefinition))
        {
            // Final-audit (AUD-3 loot note): species DefeatLoot was authored across
            // the whole roster (ContentLibrary + bestiary + production) but had NO
            // runtime consumer — killing creatures yielded nothing. The nearest
            // living player (the killer, single-player-first) now collects it; the
            // grant goes through AddItem so genuine ItemCollected quests advance.
            // FCR-1-a fix (H-a7): the same killer lookup feeds PARTY XP — every
            // healthy party echo of the killer in a generous combat radius shares
            // kill XP scaled to the defeated creature's max health (growth loop:
            // fight -> XP -> level -> abilities unlock).
            if (!bCaptured)
            {
                AAstrawildPlayerCharacter* Killer = nullptr;
                if (UWorld* World = GetWorld())
                {
                    float BestDistSq = FMath::Square(2500.0f);
                    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
                    {
                        AAstrawildPlayerCharacter* Player = *It;
                        if (!Player || !Player->IsAlive())
                        {
                            continue;
                        }
                        const float DistSq = FVector::DistSquared(GetActorLocation(), Player->GetActorLocation());
                        if (DistSq < BestDistSq)
                        {
                            BestDistSq = DistSq;
                            Killer = Player;
                        }
                    }

                    if (Killer)
                    {
                        const FName KillerId = Killer->GetFName();
                        const float KillXP = FMath::Max(5.0f, GetMaxHealth() * 0.15f);
                        for (TActorIterator<AAstrawildEchoCharacter> XpIt(World); XpIt; ++XpIt)
                        {
                            AAstrawildEchoCharacter* PartyEcho = *XpIt;
                            if (PartyEcho && PartyEcho != this && PartyEcho->bCaptured &&
                                !PartyEcho->IsDefeated() && PartyEcho->OwnerPlayerId == KillerId &&
                                FVector::DistSquared(GetActorLocation(), PartyEcho->GetActorLocation()) < FMath::Square(4000.0f))
                            {
                                PartyEcho->AddExperience(KillXP);
                            }
                        }
                    }
                }

                if (Killer && Killer->InventoryComponent && EchoDefinition->DefeatLoot.Num() > 0)
                {
                    for (const FAstrawildItemStack& Drop : EchoDefinition->DefeatLoot)
                    {
                        if (Drop.IsValid())
                        {
                            Killer->InventoryComponent->AddItem(Drop.ItemId, Drop.Quantity);
                        }
                    }
                }
            }

            if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
            {
                Ecosystem->OnEchoDefeated(this);
            }

            if (UWorld* World = GetWorld())
            {
                if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
                {
                    const bool bWasHostile = IsValid(EchoDefinition) && EchoDefinition->bHostileToPlayers;
                    // FCR-1-d fix (M-d8): same party-pressure routing as the DoT path.
                    const FNativeGameplayTag DefeatTag = bWasHostile ? TAG_Astrawild_Event_HostileDefeated
                        : (bCaptured ? TAG_Astrawild_Event_PartyEchoDefeated : TAG_Astrawild_Event_EchoDefeated);
                    EventBus->PublishEvent(
                        DefeatTag,
                        GetInstigator() ? GetInstigator() : nullptr,
                        EchoDefinition->DefinitionId,
                        1,
                        GetActorLocation());
                }
            }
        }
    }

    return MitigatedDamage;
}

float AAstrawildEchoCharacter::GetHealthFraction() const
{
    const float MaxHealth = FMath::Max(1.0f, GetMaxHealth());
    return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

float AAstrawildEchoCharacter::GetMaxHealth() const
{
    // +10% per level above 1 (growth, directive §4); Sturdy traits stack on top.
    // FCR-1-d (H-d5): health IV (+1% per point, 0-31) — rolled at breeding.
    const float TraitHealth = UAstrawildGeneticsLibrary::ComputeTraitHealthMultiplier(InstanceTraits);
    const float IvHealth = UAstrawildGeneticsLibrary::ComputeIVStatMultiplier(InstanceIVs.X);
    return FMath::Max(1.0f, CachedStats.MaxHealth * (1.0f + 0.1f * (Level - 1)) * TraitHealth * IvHealth);
}

float AAstrawildEchoCharacter::GetAttackPower() const
{
    const float TraitAttack = UAstrawildGeneticsLibrary::ComputeTraitAttackMultiplier(InstanceTraits);
    const float IvAttack = UAstrawildGeneticsLibrary::ComputeIVStatMultiplier(InstanceIVs.Y);
    return CachedStats.AttackPower * (1.0f + 0.08f * (Level - 1)) * TraitAttack * IvAttack;
}

float AAstrawildEchoCharacter::GetDefense() const
{
    // FCR-1-d (H-d5): defense IV — consumed at the elemental mitigation site
    // (the old pipeline read CachedStats.Defense raw, so defense IVs did nothing).
    const float IvDefense = UAstrawildGeneticsLibrary::ComputeIVStatMultiplier(InstanceIVs.Z);
    return CachedStats.Defense * IvDefense;
}

float AAstrawildEchoCharacter::ComputeCaptureChance() const
{
    if (IsDefeated() || !IsValid(EchoDefinition))
    {
        return 0.0f;
    }

    const float HealthFraction = GetHealthFraction();
    const float Resilience = FMath::Clamp(CachedStats.CaptureResilience, 0.0f, 1.0f);
    const float Difficulty = FMath::Clamp(EchoDefinition->CaptureDifficulty, 0.0f, 1.0f);

    // Base: species difficulty reduces the floor; weaken bonus scales with missing health.
    const float Base = 0.05f * (1.0f - 0.5f * Difficulty);
    const float WeakenBonus = (1.0f - HealthFraction) * (1.0f - Resilience) * (1.0f - 0.5f * Difficulty);
    const float TrustBonus = FMath::Clamp(Trust / 100.0f, 0.0f, 1.0f) * 0.5f;

    // Situational bonuses (directive §8): preferred weather + activity window.
    float SituationalBonus = 0.0f;
    if (UWorld* World = GetWorld())
    {
        if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
        {
            if (EchoDefinition->PreferredWeather.Contains(GameState->WeatherState))
            {
                SituationalBonus += 0.10f;
            }
        }
    }
    if (IsCurrentlyActiveTime())
    {
        SituationalBonus += 0.05f;
    }

    // FCR-1-d fix (H-d6): Trait_Lucky is live — its authored +10% capture bonus
    // was never folded into the roll (GetTraitCaptureBonus had zero combat
    // callers), making the trait pure flavor.
    float LuckyBonus = 0.0f;
    for (const FName& TraitId : InstanceTraits)
    {
        LuckyBonus += UAstrawildGeneticsLibrary::GetTraitCaptureBonus(TraitId);
    }

    return FMath::Clamp(Base + WeakenBonus + TrustBonus + SituationalBonus + LuckyBonus, 0.02f, 0.95f);
}

bool AAstrawildEchoCharacter::IsCurrentlyActiveTime() const
{
    if (!IsValid(EchoDefinition))
    {
        return true;
    }

    const UWorld* World = GetWorld();
    const AAstrawildGameState* GameState = World ? World->GetGameState<AAstrawildGameState>() : nullptr;
    if (!GameState)
    {
        return true;
    }

    const float Hour = GameState->GetTimeOfDayHours();
    switch (EchoDefinition->ActivityPattern)
    {
    case EAstrawildActivityPattern::Diurnal:
        return Hour >= 5.5f && Hour < 19.5f;
    case EAstrawildActivityPattern::Nocturnal:
        return Hour < 5.5f || Hour >= 19.5f;
    case EAstrawildActivityPattern::Crepuscular:
        return (Hour >= 5.0f && Hour < 8.0f) || (Hour >= 17.0f && Hour < 20.5f);
    default:
        return true;
    }
}

float AAstrawildEchoCharacter::GetFleeHealthThresholdMultiplier() const
{
    switch (Personality)
    {
    case EAstrawildPersonality::Timid:    return 1.8f;
    case EAstrawildPersonality::Brave:    return 0.4f;
    case EAstrawildPersonality::Aggressive: return 0.5f;
    case EAstrawildPersonality::Protective: return 0.6f;
    default: return 1.0f;
    }
}

float AAstrawildEchoCharacter::GetAggroRadiusMultiplier() const
{
    switch (Personality)
    {
    case EAstrawildPersonality::Aggressive: return 1.5f;
    case EAstrawildPersonality::Brave:      return 1.2f;
    case EAstrawildPersonality::Timid:      return 0.5f;
    default: return 1.0f;
    }
}

float AAstrawildEchoCharacter::GetWorkSpeedMultiplier() const
{
    float PersonalityMultiplier = 1.0f;
    switch (Personality)
    {
    case EAstrawildPersonality::Lazy:      PersonalityMultiplier = 0.6f; break;
    case EAstrawildPersonality::Energetic: PersonalityMultiplier = 1.4f; break;
    case EAstrawildPersonality::Loyal:     PersonalityMultiplier = 1.15f; break;
    default: break;
    }

    // SCP Phase 10: Artisan traits multiply work output on top of personality.
    return PersonalityMultiplier * UAstrawildGeneticsLibrary::ComputeTraitWorkMultiplier(InstanceTraits);
}

void AAstrawildEchoCharacter::SetInstanceTraits(const TArray<FName>& InTraits)
{
    // Only the four slots are meaningful; duplicates inherit by design.
    InstanceTraits = InTraits;
    if (InstanceTraits.Num() > 4)
    {
        InstanceTraits.SetNum(4);
    }

    // Swift traits speed the base movement budget (speed recompute reads
    // CachedStats on the next multiplier change or restore — force it now).
    // FCR-1-d (H-d5): the speed IV rides along (+1% per point).
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = FMath::Max(0.0f, CachedStats.MoveSpeed *
            UAstrawildGeneticsLibrary::ComputeTraitSpeedMultiplier(InstanceTraits) *
            UAstrawildGeneticsLibrary::ComputeIVStatMultiplier(InstanceIVs.W) *
            GetStatusSpeedMultiplier() * GetLocomotionSpeedMultiplier());
    }
}

float AAstrawildEchoCharacter::GetCommandObedience() const
{
    float Obedience = 0.8f;
    switch (Personality)
    {
    case EAstrawildPersonality::Loyal:        Obedience = 1.0f; break;
    case EAstrawildPersonality::Independent:  Obedience = 0.5f; break;
    case EAstrawildPersonality::Lazy:         Obedience = 0.6f; break;
    case EAstrawildPersonality::Protective:   Obedience = 0.95f; break;
    default: break;
    }
    // Trust and bond raise obedience (directive §5/§10).
    Obedience += FMath::Clamp(Trust / 200.0f, 0.0f, 0.25f) + FMath::Clamp(Bond / 400.0f, 0.0f, 0.25f);
    return FMath::Clamp(Obedience, 0.1f, 1.0f);
}

bool AAstrawildEchoCharacter::Capture(const float InitialTrust)
{
    if (GetLocalRole() != ROLE_Authority || bCaptured || IsDefeated() || !IsValid(EchoDefinition))
    {
        return false;
    }

    bCaptured = true;
    Trust = FMath::Max(0.0f, InitialTrust) + EchoDefinition->TrustGainOnCapture;
    ActiveCommand = EAstrawildEchoCommand::Follow;
    SetAIState(EAstrawildEchoAIState::Follow);

    if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
    {
        // Audit C-9 (final run): actor-keyed idempotent bookkeeping (replaces the
        // definition-level NotifyCaptured call so the counted slot releases once).
        Ecosystem->OnEchoCaptured(this);
    }

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_EchoCaptured, GetInstigator(), EchoDefinition->DefinitionId, 1, GetActorLocation());
        }
    }

    OnCaptured.Broadcast(this);
    return true;
}

void AAstrawildEchoCharacter::AddTrust(const float Amount)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        Trust = FMath::Max(0.0f, Trust + Amount);
    }
}

float AAstrawildEchoCharacter::Feed(const FName FoodItemId, const float FeedValue)
{
    if (GetLocalRole() != ROLE_Authority || !IsValid(EchoDefinition) || IsDefeated())
    {
        return 0.0f;
    }

    const bool bPreferred = EchoDefinition->PreferredFoodIds.Contains(FoodItemId);
    const float Multiplier = bPreferred ? 2.0f : 1.0f;
    const float TrustGain = FMath::Max(0.0f, FeedValue) * Multiplier;

    Trust += TrustGain;
    Bond = FMath::Clamp(Bond + TrustGain * 0.25f, 0.0f, 100.0f);
    Needs.Hunger = FMath::Clamp(Needs.Hunger + 30.0f * Multiplier, 0.0f, 100.0f);
    Needs.Mood = FMath::Clamp(Needs.Mood + 10.0f * Multiplier, 0.0f, 100.0f);

    // FCR-1-a fix (H-a7): feeding now also grows the echo — AddExperience previously
    // had ZERO callers, so every echo was frozen at level 1 and 38 of the 44 ability
    // templates (UnlockLevel >= 2) were permanently unreachable. Care feeds growth.
    AddExperience(10.0f * Multiplier);

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_EchoFed, GetInstigator(), FoodItemId, 1, GetActorLocation());
        }
    }

    return TrustGain;
}

void AAstrawildEchoCharacter::AddExperience(const float Amount)
{
    if (GetLocalRole() != ROLE_Authority || Amount <= 0.0f || !IsValid(EchoDefinition))
    {
        return;
    }

    Experience += FMath::Max(0.0f, Amount);

    // Level curve: BaseExperienceToLevel * level (directive §4 Growth).
    const float Required = EchoDefinition->BaseExperienceToLevel * Level;
    if (Experience >= Required)
    {
        Experience -= Required;
        ++Level;
        CurrentHealth = GetMaxHealth(); // Full heal on level up.
        OnLevelUp.Broadcast(this, Level);
        UE_LOG(LogAstrawildAI, Log, TEXT("Echo %s reached level %d."), *GetName(), Level);
    }
}

bool AAstrawildEchoCharacter::IssueCommand(const EAstrawildEchoCommand Command)
{
    if (GetLocalRole() != ROLE_Authority || !bCaptured || IsDefeated())
    {
        return false;
    }

    // Obedience roll — disloyal Echoes sometimes ignore commands (directive §5/§10).
    if (FMath::FRand() > GetCommandObedience())
    {
        UE_LOG(LogAstrawildAI, Verbose, TEXT("Echo %s ignored command %d (obedience %.2f)."), *GetName(), static_cast<int32>(Command), GetCommandObedience());
        return false;
    }

    ActiveCommand = Command;
    OnCommandReceived.Broadcast(this, Command);
    return true;
}

void AAstrawildEchoCharacter::HandleNeedsDecay(const float DeltaSeconds)
{
    if (!IsValid(EchoDefinition))
    {
        return;
    }

    // Throttle by simulation tier (directive §34): far creatures update less often.
    float UpdateInterval = 0.0f;
    if (const UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
    {
        UpdateInterval = UAstrawildEcosystemSubsystem::GetRecommendedUpdateInterval(Ecosystem->GetTierForEcho(this));
    }

    float EffectiveDeltaSeconds = DeltaSeconds;
    if (UpdateInterval > 0.0f)
    {
        NeedsDecayAccumulator += DeltaSeconds;
        if (NeedsDecayAccumulator < UpdateInterval)
        {
            return;
        }
        EffectiveDeltaSeconds = NeedsDecayAccumulator;
        NeedsDecayAccumulator = 0.0f;
    }

    // Convert decay-per-in-world-hour into per-second using the time subsystem rate.
    const UWorld* World = GetWorld();
    const UAstrawildTimeSubsystem* TimeSubsystem = World ? World->GetSubsystem<UAstrawildTimeSubsystem>() : nullptr;
    const float WorldMinutesPerSecond = TimeSubsystem ? FMath::Max(0.001f, TimeSubsystem->MinutesPerRealSecond) : 1.0f;
    const float InWorldHoursThisTick = (EffectiveDeltaSeconds * WorldMinutesPerSecond) / 60.0f;

    const float HungerDecay = EchoDefinition->HungerDecayPerHour * InWorldHoursThisTick;
    const float EnergyDecay = EchoDefinition->EnergyDecayPerHour * InWorldHoursThisTick;

    // Captured Echoes in the player party burn needs slower (they are cared for).
    const float CareMultiplier = bCaptured ? 0.6f : 1.0f;

    Needs.Hunger = FMath::Clamp(Needs.Hunger - HungerDecay * CareMultiplier, 0.0f, 100.0f);
    Needs.Energy = FMath::Clamp(Needs.Energy - EnergyDecay * CareMultiplier, 0.0f, 100.0f);
    Needs.Mood = FMath::Clamp(Needs.Mood - (Needs.Hunger < 30.0f ? 2.0f : -0.5f) * InWorldHoursThisTick, 0.0f, 100.0f);

    // Bond grows slowly while traveling with the player (directive §4 Relationship).
    if (bCaptured)
    {
        Bond = FMath::Clamp(Bond + 0.2f * InWorldHoursThisTick, 0.0f, 100.0f);
    }

    // Critical needs injure the creature (soft pressure, directive §11 philosophy).
    if (Needs.IsCritical())
    {
        CurrentHealth = FMath::Max(1.0f, CurrentHealth - 1.0f * DeltaSeconds);
    }
}

FAstrawildEchoInstanceSaveData AAstrawildEchoCharacter::ToSaveData() const
{
    FAstrawildEchoInstanceSaveData Data;
    Data.InstanceId = InstanceId;
    Data.DefinitionId = EchoDefinition ? EchoDefinition->DefinitionId : NAME_None;
    Data.Level = FMath::Max(1, Level);
    Data.Trust = Trust;
    Data.Experience = 0;
    Data.bInRoster = bCaptured;
    Data.LastKnownTransform = GetActorTransform();
    return Data;
}

FAstrawildEchoInstanceV2 AAstrawildEchoCharacter::ToSaveDataV2() const
{
    FAstrawildEchoInstanceV2 Data;
    Data.InstanceId = InstanceId;
    Data.DefinitionId = EchoDefinition ? EchoDefinition->DefinitionId : NAME_None;
    Data.Personality = Personality;
    Data.Level = FMath::Max(1, Level);
    Data.Experience = Experience;
    Data.Trust = Trust;
    Data.Bond = Bond;
    Data.Needs = Needs;
    Data.LastKnownTransform = GetActorTransform();
    Data.bInParty = bCaptured; // Roster membership == captured in v2 schema.
    // Final-audit M-2: health at save time — a load must not free-heal the party
    // (defeated echoes used to revive on reload).
    Data.CurrentHealth = FMath::Max(0.0f, CurrentHealth);
    // SCP Phase 9: sanity + illness persist with the party.
    if (SanityComponent)
    {
        SanityComponent->ExportForSave(Data.Sanity, Data.IllnessId);
    }
    // SCP Phase 10: breeding traits persist with the instance.
    Data.Traits = InstanceTraits;
    // FCR-1-d (H-d5): IVs ride the instance payload (additive; pre-SCP-10 saves
    // default to zero = neutral multipliers).
    Data.IVs = InstanceIVs;
    return Data;
}

bool AAstrawildEchoCharacter::FromSaveDataV2(const FAstrawildEchoInstanceV2& Data)
{
    if (!Data.InstanceId.IsValid() || Data.DefinitionId.IsNone())
    {
        return false;
    }

    InstanceId = Data.InstanceId;
    Personality = Data.Personality;
    Level = FMath::Max(1, Data.Level);
    Experience = FMath::Max(0.0f, Data.Experience);
    Trust = FMath::Max(0.0f, Data.Trust);
    Bond = FMath::Clamp(Data.Bond, 0.0f, 100.0f);

    // Final-audit M-4: NaN-safe needs (FMath::Clamp passes NaN through verbatim —
    // a crafted save must not poison the needs-decay tick) + transform guard (the
    // player path has guarded this exact crash class since FR-2; the echo path
    // applied a crafted transform unchecked).
    const auto SafeNeed = [](const float Value)
    {
        return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, 100.0f) : 100.0f;
    };
    Needs.Hunger = SafeNeed(Data.Needs.Hunger);
    Needs.Energy = SafeNeed(Data.Needs.Energy);
    Needs.Mood = SafeNeed(Data.Needs.Mood);

    bCaptured = Data.bInParty;
    if (Data.LastKnownTransform.ContainsNaN() || Data.LastKnownTransform.Equals(FTransform::Identity))
    {
        // Keep the spawn-ring placement — a crafted/garbage transform is refused.
        UE_LOG(LogAstrawildAI, Warning, TEXT("Echo restore: rejected non-finite/identity transform (kept ring spawn)."));
    }
    else
    {
        SetActorTransform(Data.LastKnownTransform);
    }

    if (IsValid(EchoDefinition))
    {
        CachedStats = EchoDefinition->BaseStats;
        // Final-audit M-2: restore saved health when the field carries one
        // (legacy 0 = the pre-audit full-heal behavior, kept for old saves);
        // clamp to [1, MaxHealth] — no free revive, no overheal.
        if (FMath::IsFinite(Data.CurrentHealth) && Data.CurrentHealth > 0.0f)
        {
            CurrentHealth = FMath::Clamp(Data.CurrentHealth, 1.0f, GetMaxHealth());
        }
        else
        {
            CurrentHealth = FMath::Min(FMath::Max(1.0f, CurrentHealth > 0.0f ? CurrentHealth : GetMaxHealth()), GetMaxHealth());
        }
        GetCharacterMovement()->MaxWalkSpeed = CachedStats.MoveSpeed;
    }
    // SCP Phase 9: sanity + illness restore (sanitized import; legacy saves
    // without the fields keep the healthy 100 / no-illness defaults).
    // FCR-1-c fix: the legacy sentinel is 0.0f (finite!) — the old IsFinite gate
    // mapped every pre-SCP save to Sanity 0 (Depressed + immediate illness risk).
    // A positive value restores; anything else (0 / NaN / negative) is legacy → 100.
    if (SanityComponent)
    {
        SanityComponent->ImportFromSave(Data.Sanity > 0.0f ? Data.Sanity : 100.0f, Data.IllnessId);
    }
    // SCP Phase 10: restore breeding traits + reapply their stat effects
    // (only known trait ids survive — edited saves cannot inject effects).
    // FCR-1-d (H-d5): IVs round-trip too, sanitized component-wise to [0, 31]
    // (NaN/negative components read as 0 — a corrupt save cannot mint stats).
    {
        const FVector4 SafeIVs(
            FMath::Clamp(FMath::IsFinite(Data.IVs.X) ? Data.IVs.X : 0.0f, 0.0f, 31.0f),
            FMath::Clamp(FMath::IsFinite(Data.IVs.Y) ? Data.IVs.Y : 0.0f, 0.0f, 31.0f),
            FMath::Clamp(FMath::IsFinite(Data.IVs.Z) ? Data.IVs.Z : 0.0f, 0.0f, 31.0f),
            FMath::Clamp(FMath::IsFinite(Data.IVs.W) ? Data.IVs.W : 0.0f, 0.0f, 31.0f));
        InstanceIVs = SafeIVs;
        TArray<FName> SafeTraits;
        const TArray<FName>& Pool = UAstrawildGeneticsLibrary::GetTraitPool();
        for (const FName& Candidate : Data.Traits)
        {
            if (Pool.Contains(Candidate))
            {
                SafeTraits.Add(Candidate);
            }
        }
        SetInstanceTraits(SafeTraits);
    }
    return true;
}

void AAstrawildEchoCharacter::ApplyPartyPassive(const float AuraSeconds)
{
    // Production V2 (Master Plan §6): deterministic aura effects — healing
    // (Mending Aura), stamina regen (Rhythm Aura) apply to the owner player and
    // nearby party Echoes while this Echo is captured, healthy and close enough.
    UWorld* World = GetWorld();
    if (!World || !bCaptured || IsDefeated() || !IsValid(EchoDefinition) || AuraSeconds <= 0.0f)
    {
        return;
    }

    const EAstrawildEchoPassive Passive = EchoDefinition->Passive;
    if (Passive == EAstrawildEchoPassive::None)
    {
        return;
    }

    constexpr float AuraRadius = 1500.0f;
    constexpr float HealPerSecond = 1.0f;
    constexpr float StaminaPerSecond = 2.0f;

    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        AAstrawildPlayerCharacter* Player = *It;
        if (!Player || !Player->IsAlive() || Player->GetFName() != OwnerPlayerId)
        {
            continue;
        }
        if (FVector::Dist(GetActorLocation(), Player->GetActorLocation()) > AuraRadius)
        {
            continue;
        }

        if (Passive == EAstrawildEchoPassive::PartyHeal)
        {
            if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
            {
                Survival->RestoreHealth(HealPerSecond * AuraSeconds);
            }
        }
        else if (Passive == EAstrawildEchoPassive::PlayerStamina)
        {
            if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
            {
                Survival->RestoreStamina(StaminaPerSecond * AuraSeconds);
            }
        }

        // Mending Aura also tends wounded party Echoes near the player.
        if (Passive == EAstrawildEchoPassive::PartyHeal)
        {
            for (TActorIterator<AAstrawildEchoCharacter> EchoIt(World); EchoIt; ++EchoIt)
            {
                AAstrawildEchoCharacter* Ally = *EchoIt;
                if (!Ally || Ally == this || !Ally->bCaptured || Ally->IsDefeated())
                {
                    continue;
                }
                if (Ally->OwnerPlayerId != OwnerPlayerId)
                {
                    continue;
                }
                if (FVector::Dist(GetActorLocation(), Ally->GetActorLocation()) > AuraRadius)
                {
                    continue;
                }
                Ally->CurrentHealth = FMath::Min(Ally->CurrentHealth + HealPerSecond * AuraSeconds, Ally->CachedStats.MaxHealth);
            }
        }
        break; // One owner player per aura.
    }
}

bool AAstrawildEchoCharacter::HasPlayerPartyPassive(const UWorld* World, const AActor* Player,
    const EAstrawildEchoPassive Passive, const float Radius)
{
    // Static aura query: captured Echoes of THIS player radiate while healthy and
    // nearby. Used by the AI perception layer (ThreatDampener) and the inventory
    // (CarryBoost) — one shared truth for party passive presence.
    if (!World || !Player || Passive == EAstrawildEchoPassive::None)
    {
        return false;
    }
    const FName PlayerId = Player->GetFName();
    if (PlayerId.IsNone())
    {
        return false;
    }
    for (TActorIterator<AAstrawildEchoCharacter> It(const_cast<UWorld*>(World)); It; ++It)
    {
        const AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || !Echo->bCaptured || Echo->IsDefeated() || !IsValid(Echo->EchoDefinition))
        {
            continue;
        }
        if (Echo->OwnerPlayerId != PlayerId || Echo->EchoDefinition->Passive != Passive)
        {
            continue;
        }
        if (FVector::Dist(Echo->GetActorLocation(), Player->GetActorLocation()) <= Radius)
        {
            return true;
        }
    }
    return false;
}

// ===========================================================================
// DP-3 — party element resonance (15-pair static table)
// ===========================================================================

namespace
{
    /** One authored resonance row; A < B in canon element order (table order IS the dominance order). */
    struct FResonanceRow
    {
        EAstrawildElementType A;
        EAstrawildElementType B;
        const TCHAR* Id;
        const TCHAR* Name;
        float DamageMitigation;
        float AbilityPowerBonus;
        float StatusPotencyBonus;
    };

    // DP-3 — the element-pair resonance canon: 6 elements -> 15 unordered pairs,
    // one modest named bonus each (5 mitigation / 5 ability power / 5 status
    // potency — the same 8-12% band as the four species passives). The table is
    // ordered by canon element order (Light, Ash, Flora, Frost, Pulse, Ember);
    // with three distinct elements standing together the FIRST pair present
    // wins, so a party always resolves exactly one deterministic resonance.
    const FResonanceRow GPartyResonanceTable[] = {
        { EAstrawildElementType::Light, EAstrawildElementType::Ash,   TEXT("Resonance_Prismfall"),       TEXT("Prismfall"),       0.00f, 0.10f, 0.00f },
        { EAstrawildElementType::Light, EAstrawildElementType::Flora, TEXT("Resonance_Sunbloom"),        TEXT("Sunbloom"),        0.00f, 0.00f, 0.10f },
        { EAstrawildElementType::Light, EAstrawildElementType::Frost, TEXT("Resonance_AuroraVeil"),      TEXT("Aurora Veil"),     0.08f, 0.00f, 0.00f },
        { EAstrawildElementType::Light, EAstrawildElementType::Pulse, TEXT("Resonance_Overcharge"),      TEXT("Overcharge"),      0.00f, 0.10f, 0.00f },
        { EAstrawildElementType::Light, EAstrawildElementType::Ember, TEXT("Resonance_Dawnfire"),        TEXT("Dawnfire"),        0.00f, 0.00f, 0.10f },
        { EAstrawildElementType::Ash,   EAstrawildElementType::Flora, TEXT("Resonance_VerdantLoam"),     TEXT("Verdant Loam"),    0.08f, 0.00f, 0.00f },
        { EAstrawildElementType::Ash,   EAstrawildElementType::Frost, TEXT("Resonance_Rimefield"),       TEXT("Rimefield"),       0.00f, 0.00f, 0.10f },
        { EAstrawildElementType::Ash,   EAstrawildElementType::Pulse, TEXT("Resonance_MagnetiteShell"),  TEXT("Magnetite Shell"), 0.08f, 0.00f, 0.00f },
        { EAstrawildElementType::Ash,   EAstrawildElementType::Ember, TEXT("Resonance_BankedCoals"),     TEXT("Banked Coals"),    0.00f, 0.00f, 0.10f },
        { EAstrawildElementType::Flora, EAstrawildElementType::Frost, TEXT("Resonance_HoarfrostBloom"),  TEXT("Hoarfrost Bloom"), 0.08f, 0.00f, 0.00f },
        { EAstrawildElementType::Flora, EAstrawildElementType::Pulse, TEXT("Resonance_GalvanicBloom"),   TEXT("Galvanic Bloom"),  0.00f, 0.10f, 0.00f },
        { EAstrawildElementType::Flora, EAstrawildElementType::Ember, TEXT("Resonance_Wildfire"),        TEXT("Wildfire"),        0.00f, 0.00f, 0.10f },
        { EAstrawildElementType::Frost, EAstrawildElementType::Pulse, TEXT("Resonance_Superconductor"),  TEXT("Superconductor"),  0.00f, 0.10f, 0.00f },
        { EAstrawildElementType::Frost, EAstrawildElementType::Ember, TEXT("Resonance_SteamVeil"),       TEXT("Steam Veil"),      0.08f, 0.00f, 0.00f },
        { EAstrawildElementType::Pulse, EAstrawildElementType::Ember, TEXT("Resonance_PlasmaArc"),       TEXT("Plasma Arc"),      0.00f, 0.10f, 0.00f },
    };

    FAstrawildPartyResonance MakeResonance(const FResonanceRow& Row)
    {
        FAstrawildPartyResonance Resonance;
        Resonance.ResonanceId = Row.Id;
        Resonance.DisplayName = FText::FromString(Row.Name);
        Resonance.DamageMitigation = Row.DamageMitigation;
        Resonance.AbilityPowerBonus = Row.AbilityPowerBonus;
        Resonance.StatusPotencyBonus = Row.StatusPotencyBonus;
        return Resonance;
    }
}

FAstrawildPartyResonance AAstrawildEchoCharacter::ResolvePartyResonance(const EAstrawildElementType ElementA,
    const EAstrawildElementType ElementB)
{
    // Symmetric pure lookup — the pair order never matters, None never resonates.
    if (ElementA == EAstrawildElementType::None || ElementB == EAstrawildElementType::None ||
        ElementA == ElementB)
    {
        return FAstrawildPartyResonance();
    }
    for (const FResonanceRow& Row : GPartyResonanceTable)
    {
        if ((Row.A == ElementA && Row.B == ElementB) || (Row.A == ElementB && Row.B == ElementA))
        {
            return MakeResonance(Row);
        }
    }
    return FAstrawildPartyResonance();
}

FAstrawildPartyResonance AAstrawildEchoCharacter::ResolvePartyResonanceForElements(
    const TArray<EAstrawildElementType>& PartyElements)
{
    // Distinct elements only (duplicates never form a pair); the FIRST row in
    // canon table order with both elements present is the dominant resonance —
    // the outcome is deterministic regardless of input order.
    TArray<EAstrawildElementType> Distinct;
    for (const EAstrawildElementType Element : PartyElements)
    {
        if (Element != EAstrawildElementType::None && !Distinct.Contains(Element))
        {
            Distinct.Add(Element);
        }
    }
    if (Distinct.Num() < 2)
    {
        return FAstrawildPartyResonance();
    }
    for (const FResonanceRow& Row : GPartyResonanceTable)
    {
        if (Distinct.Contains(Row.A) && Distinct.Contains(Row.B))
        {
            return MakeResonance(Row);
        }
    }
    return FAstrawildPartyResonance();
}

FAstrawildPartyResonance AAstrawildEchoCharacter::GetActivePartyResonance(const UWorld* World,
    const AActor* Player, const float Radius)
{
    // Static party query, mirroring HasPlayerPartyPassive exactly: captured,
    // healthy Echoes of this player within Radius contribute their element.
    // Null-world fail-closed (invalid row) so tests and menu contexts are safe.
    if (!World || !Player)
    {
        return FAstrawildPartyResonance();
    }
    const FName PlayerId = Player->GetFName();
    if (PlayerId.IsNone())
    {
        return FAstrawildPartyResonance();
    }
    TArray<EAstrawildElementType> PartyElements;
    for (TActorIterator<AAstrawildEchoCharacter> It(const_cast<UWorld*>(World)); It; ++It)
    {
        const AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || !Echo->bCaptured || Echo->IsDefeated() || !IsValid(Echo->EchoDefinition))
        {
            continue;
        }
        if (Echo->OwnerPlayerId != PlayerId)
        {
            continue;
        }
        if (FVector::Dist(Echo->GetActorLocation(), Player->GetActorLocation()) > Radius)
        {
            continue;
        }
        if (Echo->EchoDefinition->Element != EAstrawildElementType::None &&
            !PartyElements.Contains(Echo->EchoDefinition->Element))
        {
            PartyElements.Add(Echo->EchoDefinition->Element);
        }
    }
    return ResolvePartyResonanceForElements(PartyElements);
}

FAstrawildPartyResonance AAstrawildEchoCharacter::GetOwnerPartyResonance() const
{
    // Per-echo convenience for the damage/ability hooks: an unowned or wild
    // Echo never carries a resonance (fail-closed, no world scan needed then).
    if (!bCaptured || OwnerPlayerId == NAME_None)
    {
        return FAstrawildPartyResonance();
    }
    UWorld* World = GetWorld();
    if (!World)
    {
        return FAstrawildPartyResonance();
    }
    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        const AAstrawildPlayerCharacter* Player = *It;
        if (Player && Player->GetFName() == OwnerPlayerId)
        {
            return GetActivePartyResonance(World, Player);
        }
    }
    return FAstrawildPartyResonance();
}

// ===========================================================================
// GDP-1 — Echo ability engine
// ===========================================================================

TArray<FName> AAstrawildEchoCharacter::GetAllAbilityIds() const
{
    return UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(EchoDefinition);
}

TArray<FName> AAstrawildEchoCharacter::GetKnownAbilityIds() const
{
    TArray<FName> Known;
    if (!IsValid(EchoDefinition))
    {
        return Known;
    }
    for (const FName& Id : GetAllAbilityIds())
    {
        const FAstrawildAbilityData* Data = UAstrawildAbilityLibrary::FindAbility(Id);
        if (Data && Data->UnlockLevel <= Level)
        {
            Known.Add(Id);
        }
    }
    return Known;
}

bool AAstrawildEchoCharacter::IsAbilityReady(FName AbilityId) const
{
    return GetAbilityCooldownRemaining(AbilityId) <= 0.0f && GetKnownAbilityIds().Contains(AbilityId);
}

float AAstrawildEchoCharacter::GetAbilityCooldownRemaining(FName AbilityId) const
{
    const float* Remaining = AbilityCooldowns.Find(AbilityId);
    return Remaining ? FMath::Max(0.0f, *Remaining) : 0.0f;
}

FName AAstrawildEchoCharacter::PickCombatAbility(const float DistanceToTarget, const bool bWantsHeal,
    const bool bWantsShield) const
{
    if (!IsValid(EchoDefinition))
    {
        return NAME_None;
    }
    return UAstrawildAbilityLibrary::ChooseAbilityForCombat(
        GetKnownAbilityIds(), AbilityCooldowns, Level, DistanceToTarget, bWantsHeal, bWantsShield);
}

bool AAstrawildEchoCharacter::ExecuteAbility(const FName AbilityId, AActor* TargetActor)
{
    // Fail-closed deny path — every denial is logged Verbose, never crashes,
    // never mutes the caller (AI just resumes melee next think).
    if (GetLocalRole() != ROLE_Authority)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("ExecuteAbility denied (no authority): %s"), *AbilityId.ToString());
        return false;
    }

    const FAstrawildAbilityData* Data = UAstrawildAbilityLibrary::FindAbility(AbilityId);
    if (!Data)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("ExecuteAbility denied (unknown id): %s"), *AbilityId.ToString());
        return false;
    }
    if (Data->UnlockLevel > Level)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("ExecuteAbility denied (level %d < %d): %s"),
            Level, Data->UnlockLevel, *AbilityId.ToString());
        return false;
    }
    if (GetAbilityCooldownRemaining(AbilityId) > 0.0f)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("ExecuteAbility denied (cooling down): %s"), *AbilityId.ToString());
        return false;
    }

    // DP-3: the owner's party resonance rides every cast of a captured party
    // echo (ability power on offensive/restore, potency on debuffs); wild
    // echoes and solo companions resolve no row and cast unmodified.
    const FAstrawildPartyResonance Resonance = bCaptured ? GetOwnerPartyResonance() : FAstrawildPartyResonance();

    UWorld* World = GetWorld();
    if (!World || IsDefeated())
    {
        return false;
    }

    bool bResolved = false;

    switch (Data->Category)
    {
    case EAstrawildAbilityCategory::Offensive:
    {
        // Level-scaled bolt down the projectile pipeline (homing when we have a target).
        const float ScaledPower = Data->Power * (1.0f + 0.05f * FMath::Max(0, Level - 1))
            * (1.0f + FMath::Clamp(Resonance.AbilityPowerBonus, 0.0f, 0.5f));
        FVector Direction = GetActorForwardVector();
        if (TargetActor)
        {
            Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
        }
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.Instigator = this;
        AAstrawildProjectileActor* Bolt = World->SpawnActor<AAstrawildProjectileActor>(
            AAstrawildProjectileActor::StaticClass(),
            GetActorLocation() + Direction * 90.0f + FVector(0, 0, 40.0f),
            Direction.Rotation(), Params);
        if (Bolt)
        {
            // FCR-1-a fixes (M-a9 + L-a15): the AUTHORED status payload rides the
            // bolt, and the bolt's LETIME derives from the ability's range (the
            // old fixed 3.0s x 3200uu flew 9600uu — far past any authored range).
            const float FlightSpeed = 3200.0f;
            const float Lifetime = FMath::Clamp(Data->Range / FlightSpeed, 0.35f, 3.0f);
            Bolt->SetStatusPayload(Data->StatusId, Data->StatusSeconds, Data->StatusSpeedMultiplier,
                FMath::Max(0.0f, Data->Power * 0.25f));
            Bolt->LaunchFromWeapon(Direction, ScaledPower, Data->Element, this, FlightSpeed,
                0.45f, Lifetime, TargetActor, 2400.0f);
            bResolved = true;
        }
        break;
    }

    case EAstrawildAbilityCategory::Debuff:
    {
        // FCR-1-a fix (H-a5): the debuff now also lands on PLAYER targets. The old
        // echo-only cast made every AI debuff pick silently fail against the player
        // (the wild AI's PRIMARY target), with a 4Hz retry loop and no cooldown.
        AAstrawildEchoCharacter* TargetEcho = Cast<AAstrawildEchoCharacter>(TargetActor);
        AAstrawildPlayerCharacter* TargetPlayer = Cast<AAstrawildPlayerCharacter>(TargetActor);
        const bool bInRange = IsValid(TargetActor) &&
            FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation()) <= Data->Range;
        if (bInRange)
        {
            FAstrawildStatusEffect Effect;
            Effect.StatusId = Data->StatusId != NAME_None ? Data->StatusId : TEXT("Chill");
            Effect.RemainingSeconds = FMath::Max(1.0f, Data->StatusSeconds);
            Effect.DamagePerSecond = FMath::Max(0.0f, Data->Power);
            Effect.SpeedMultiplier = FMath::Clamp(Data->StatusSpeedMultiplier, 0.2f, 1.0f);
            if (Resonance.IsValid() && Resonance.StatusPotencyBonus > 0.0f)
            {
                Effect.DamagePerSecond *= (1.0f + FMath::Clamp(Resonance.StatusPotencyBonus, 0.0f, 0.5f));
                Effect.RemainingSeconds *= (1.0f + FMath::Clamp(Resonance.StatusPotencyBonus, 0.0f, 0.5f));
            }
            if (TargetEcho && !TargetEcho->IsDefeated())
            {
                TargetEcho->AddStatusEffect(Effect);
                bResolved = true;
            }
            else if (TargetPlayer && TargetPlayer->IsAlive())
            {
                if (UAstrawildSurvivalComponent* Survival = TargetPlayer->FindComponentByClass<UAstrawildSurvivalComponent>())
                {
                    Survival->AddStatusEffect(Effect);
                    bResolved = true;
                }
            }
        }
        break;
    }

    case EAstrawildAbilityCategory::Defensive:
    {
        FAstrawildStatusEffect Effect;
        Effect.StatusId = Data->StatusId != NAME_None ? Data->StatusId : TEXT("Shell");
        Effect.RemainingSeconds = FMath::Max(1.0f, Data->StatusSeconds);
        Effect.DamagePerSecond = 0.0f;
        Effect.SpeedMultiplier = 1.0f;
        AddStatusEffect(Effect);
        bResolved = true;
        break;
    }

    case EAstrawildAbilityCategory::Restore:
    {
        // Heal-over-time ward when the ability carries a status payload, direct
        // burst otherwise. Either way: self + nearby party members.
        FAstrawildStatusEffect Ward;
        if (Data->StatusId != NAME_None && Data->StatusSeconds > 0.0f)
        {
            Ward.StatusId = Data->StatusId;
            Ward.RemainingSeconds = Data->StatusSeconds;
            Ward.DamagePerSecond = -(Data->Power / Data->StatusSeconds);
            Ward.SpeedMultiplier = 1.0f;
        }
        else
        {
            Ward.StatusId = TEXT("Mend");
            Ward.RemainingSeconds = 1.0f;
            Ward.DamagePerSecond = -Data->Power;
            Ward.SpeedMultiplier = 1.0f;
        }

        if (Resonance.IsValid() && Resonance.AbilityPowerBonus > 0.0f)
        {
            // Negative DoT = heal; a resonant caster's ward heals deeper (party-wide
            // copies carry the same amplified magnitude).
            Ward.DamagePerSecond *= (1.0f + FMath::Clamp(Resonance.AbilityPowerBonus, 0.0f, 0.5f));
        }

        AddStatusEffect(Ward);
        bResolved = true;

        // Party-wide: every healthy captured echo of the same owner in range.
        if (bCaptured && OwnerPlayerId != NAME_None)
        {
            for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
            {
                AAstrawildEchoCharacter* Other = *It;
                if (Other && Other != this && Other->bCaptured && !Other->IsDefeated() &&
                    Other->OwnerPlayerId == OwnerPlayerId &&
                    FVector::Dist(GetActorLocation(), Other->GetActorLocation()) <= Data->Range)
                {
                    Other->AddStatusEffect(Ward);
                }
            }
        }
        break;
    }

    case EAstrawildAbilityCategory::Mobility:
    {
        FAstrawildStatusEffect Effect;
        Effect.StatusId = Data->StatusId != NAME_None ? Data->StatusId : TEXT("Surge");
        Effect.RemainingSeconds = FMath::Max(1.0f, Data->StatusSeconds);
        Effect.DamagePerSecond = 0.0f;
        Effect.SpeedMultiplier = FMath::Clamp(Data->StatusSpeedMultiplier, 1.0f, 2.5f);
        AddStatusEffect(Effect);
        bResolved = true;
        break;
    }

    default:
        break;
    }

    if (bResolved)
    {
        AbilityCooldowns.Add(AbilityId, Data->CooldownSeconds);
        OnAbilityExecuted.Broadcast(this, AbilityId, true);
        // FCR-1-a fix (M-a10): the delegate had zero subscribers — feed the
        // player-facing toast directly for OWNED echoes (mirrors the capture
        // toast pattern; the delegate stays for future subsystem consumers).
        if (bCaptured && GetWorld())
        {
            if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetWorld()->GetFirstPlayerController()))
            {
                if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
                {
                    if (Player->GetFName() == OwnerPlayerId)
                    {
                        PC->Notify(FText::FromString(FString::Printf(TEXT("%s: %s"),
                            *EchoDefinition->DisplayName.ToString(), *Data->DisplayName.ToString())));
                    }
                }
            }
        }
        UE_LOG(LogAstrawild, Log, TEXT("%s (Lv %d) cast %s."), *GetName(), Level, *AbilityId.ToString());
    }
    else
    {
        OnAbilityExecuted.Broadcast(this, AbilityId, false);
    }
    return bResolved;
}

// ===========================================================================
// GDP-2 — locomotion classes
// ===========================================================================

EAstrawildLocomotionClass AAstrawildEchoCharacter::DeriveLocomotionClass(const EAstrawildEchoFamily Family,
    const EAstrawildBodyPlan BodyPlan, const EAstrawildZone HomeZone)
{
    // 1) Explicit winged bodies and the Avian family fly.
    if (BodyPlan == EAstrawildBodyPlan::Avian || Family == EAstrawildEchoFamily::Avian)
    {
        return EAstrawildLocomotionClass::Flying;
    }
    // 2) The Aquatic family and the three sea zones make water movers.
    if (Family == EAstrawildEchoFamily::Aquatic)
    {
        return EAstrawildLocomotionClass::Water;
    }
    if (HomeZone == EAstrawildZone::AzureShallows || HomeZone == EAstrawildZone::TidebreakerIsles ||
        HomeZone == EAstrawildZone::PearlseaReef)
    {
        // Sea-zone species that are not winged: amphibious water movers.
        return EAstrawildLocomotionClass::Water;
    }
    // 3) Floating spirit/elemental bodies hover — treated as flying movers.
    if (BodyPlan == EAstrawildBodyPlan::Floating)
    {
        return EAstrawildLocomotionClass::Flying;
    }
    return EAstrawildLocomotionClass::Land;
}

EAstrawildLocomotionClass AAstrawildEchoCharacter::GetLocomotionClass() const
{
    if (!IsValid(EchoDefinition))
    {
        return EAstrawildLocomotionClass::Land;
    }
    if (EchoDefinition->Locomotion != EAstrawildLocomotionClass::Auto)
    {
        return EchoDefinition->Locomotion;
    }
    return DeriveLocomotionClass(EchoDefinition->Family, EchoDefinition->BodyPlan, EchoDefinition->HomeZone);
}

float AAstrawildEchoCharacter::GetLocomotionSpeedMultiplier() const
{
    const EAstrawildLocomotionClass Loco = GetLocomotionClass();
    if (Loco != EAstrawildLocomotionClass::Water)
    {
        // Land and Flying movers are unaffected (flying runs MOVE_Flying speed).
        return 1.0f;
    }

    // Water species: +40% in the three sea zones, -15% drag on dry land.
    const EAstrawildZone CurrentZone = UAstrawildZoneSubsystem::GetZoneAt(GetActorLocation());
    const bool bInSeaZone = CurrentZone == EAstrawildZone::AzureShallows ||
        CurrentZone == EAstrawildZone::TidebreakerIsles ||
        CurrentZone == EAstrawildZone::PearlseaReef;
    return bInSeaZone ? 1.4f : 0.85f;
}
