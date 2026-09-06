#pragma once

#include "CoreMinimal.h"
#include "Math/Color.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoMutator.generated.h"

class AAstrawildEchoCharacter;
class UAstrawildEchoDefinition;
class UNiagaraComponent;

/**
 * Sci-Fantasy Monster directive — theme vocabulary (Phase 1).
 *
 * The 204 generated bestiary species regroup into 8 Sci-Fantasy visual
 * identities. THEME (not BodyPlan) is the visual grouping axis from this
 * pass: the body plan stays the procedural silhouette kit / rig family,
 * while the theme owns the material language, mutation bias, base-mesh
 * archetype and persistent element VFX of every Echo.
 */
UENUM(BlueprintType)
enum class EAstrawildSciFantasyTheme : uint8
{
    AncientConstruct UMETA(DisplayName = "Ancient Construct"),
    ElementalBeast UMETA(DisplayName = "Elemental Beast"),
    MutatedFauna UMETA(DisplayName = "Mutated Fauna"),
    ArmoredOrganic UMETA(DisplayName = "Armored Organic"),
    EtherealSpirit UMETA(DisplayName = "Ethereal Spirit"),
    MechanicalHybrid UMETA(DisplayName = "Mechanical Hybrid"),
    PlantMonster UMETA(DisplayName = "Plant Monster"),
    VoidAbomination UMETA(DisplayName = "Void Abomination")
};

/** Runtime mutation attachments (Phase 2 — mesh attachment vocabulary). */
UENUM(BlueprintType)
enum class EAstrawildEchoAttachment : uint8
{
    None UMETA(DisplayName = "None"),
    DorsalSpikes UMETA(DisplayName = "Dorsal Spikes"),
    Wings UMETA(DisplayName = "Extra Wings"),
    ThirdEye UMETA(DisplayName = "Third Eye"),
    ExtraArms UMETA(DisplayName = "Extra Arms"),
    HornCrown UMETA(DisplayName = "Horn Crown"),
    GlowNodes UMETA(DisplayName = "Glow Nodes"),
    TailFin UMETA(DisplayName = "Tail Fin")
};

/** Material language per theme (Phase 2 — material swapping vocabulary). */
UENUM(BlueprintType)
enum class EAstrawildMutationMaterialTheme : uint8
{
    Metallic UMETA(DisplayName = "Metallic"),
    Stony UMETA(DisplayName = "Stony"),
    Energy UMETA(DisplayName = "Energy"),
    Slime UMETA(DisplayName = "Slime"),
    Chitin UMETA(DisplayName = "Chitin"),
    Organic UMETA(DisplayName = "Organic"),
    Crystalline UMETA(DisplayName = "Crystalline"),
    Void UMETA(DisplayName = "Void")
};

/** Persistent element VFX bound to the body (Phase 2 — VFX binding vocabulary). */
UENUM(BlueprintType)
enum class EAstrawildEchoVfxType : uint8
{
    None UMETA(DisplayName = "None"),
    Fire UMETA(DisplayName = "Fire"),
    Frost UMETA(DisplayName = "Frost"),
    Electric UMETA(DisplayName = "Electric"),
    Void UMETA(DisplayName = "Void"),
    Poison UMETA(DisplayName = "Poison"),
    Spore UMETA(DisplayName = "Spore"),
    Radiant UMETA(DisplayName = "Radiant")
};

/**
 * Sci-Fantasy mutation spec (Phase 2) — one row per species.
 *
 * Mirror of the generated table in AstrawildEchoMutationData.cpp (204
 * bestiary rows; hero/authored species derive a deterministic spec through
 * FAstrawildEchoMutator::BuildDeterministicSpec instead). Pure data: no
 * engine object loads, so automation tests can verify the contract without
 * a world — the same contract style as AstrawildArtPack.
 */
struct FEchoMutationSpec
{
    /** Bestiary species id this row mutates (NAME_None on derived specs). */
    FName SpeciesId;

    /** Sci-Fantasy visual identity group. */
    EAstrawildSciFantasyTheme Theme = EAstrawildSciFantasyTheme::MutatedFauna;

    /** Base archetype mesh id (one of the 16 SK_Base_* bakes). */
    FName BaseMeshId;

    /** Per-part scale multipliers (head / torso / limbs / tail), 0.75..1.45. */
    float HeadScale = 1.0f;
    float TorsoScale = 1.0f;
    float LimbScale = 1.0f;
    float TailScale = 1.0f;

    /** Bit mask of EAstrawildEchoAttachment bits (see GetAttachmentBits). */
    int32 AttachmentMask = 0;

    /** Material language applied over the body palette. */
    EAstrawildMutationMaterialTheme MaterialTheme = EAstrawildMutationMaterialTheme::Organic;

    /** Independent pattern tint (สีลาย — separate from the body base color). */
    FLinearColor PatternTint = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    /** Persistent element VFX type (None = glow light only). */
    EAstrawildEchoVfxType VfxType = EAstrawildEchoVfxType::None;

    /** Theme sound set id (cue resolution via BuildSoundSetCuePath). */
    FName SoundSetId;
};

/**
 * The generated 204-row mutation table (AstrawildEchoMutationData.cpp —
 * emitted by Scripts/generate_echo_mutations.py from the ACTUAL bestiary
 * rows). Pure data; consumers iterate or look up via FindSpec.
 */
namespace AstrawildEchoMutation
{
    ASTRAWILDCORE_API const TArray<FEchoMutationSpec>& GetMutationSpecs();
}

/**
 * Sci-Fantasy mutation system (Phase 2) — runtime mutator.
 *
 * Applies a species' FEchoMutationSpec onto BOTH render paths of
 * AAstrawildEchoCharacter:
 *   - PMC procedural body: per-part scale multipliers + attachment geometry +
 *     theme-modulated palette (vertex-color material language), applied inside
 *     the character's BuildProceduralBody (it owns the local part helpers).
 *   - Skinned body: root scale jitter + theme material swap (dynamic instance
 *     of the M_SciFi_* master, parameterized per species) + persistent
 *     element VFX (+ optional sound-set cue) — per-bone skeletal scaling is
 *     explicitly NOT attempted here (documented engine-verification item,
 *     not a silent fake).
 *
 * Everything is deterministic (species id + instance salt → identical output),
 * pure helpers are world-free and automation-tested, and every engine-asset
 * binding is OPT-IN / fail-closed (derived convention paths — validator
 * check 8 stays clean, LoadObject failing keeps the existing fallback).
 */
struct ASTRAWILDCORE_API FAstrawildEchoMutator
{
    /** Table lookup (204 generated rows). nullptr for non-bestiary species. */
    static const FEchoMutationSpec* FindSpec(const FName& SpeciesId);

    /** Row count of the generated table (automation contract: 204). */
    static int32 GetMutationSpecCount();

    /**
     * Deterministic fallback spec for authored/hero species without a table
     * row: derives the theme from the definition (family / body plan / name
     * keywords / element) with the same rules the generator applies — the
     * same species always derives the same spec.
     */
    static FEchoMutationSpec BuildDeterministicSpec(const UAstrawildEchoDefinition* Definition);

    /**
     * Per-part scale multipliers for ONE instance: the spec's table scales
     * with a small deterministic instance jitter (±5%), clamped to
     * [0.6, 1.6]. Pure math — automation-tested.
     */
    static void ComputePartScales(const FEchoMutationSpec& Spec, int32 InstanceSalt,
        float& OutHeadScale, float& OutTorsoScale, float& OutLimbScale, float& OutTailScale);

    /** Root (whole-body) scale jitter for the skinned path, 0.94..1.06. */
    static float ComputeRootScaleJitter(const FName& SpeciesId, int32 InstanceSalt);

    /** Attachment bit for one attachment type (None = 0). */
    static int32 GetAttachmentBits(EAstrawildEchoAttachment Attachment);

    /** True when the spec's mask carries the attachment. */
    static bool HasAttachment(const FEchoMutationSpec& Spec, EAstrawildEchoAttachment Attachment);

    /**
     * Material-theme modulation + independent pattern tint (pure color math):
     * transforms the species' procedural body palette (primary/secondary)
     * into the Sci-Fantasy theme's material language. Automation-tested.
     */
    static void ApplyThemeToBodyColors(const FColor& Primary, const FColor& Secondary,
        const FEchoMutationSpec& Spec, FColor& OutPrimary, FColor& OutSecondary);

    /** Theme identity tint (the generator's THEME_TINTS mirror). */
    static FLinearColor ResolveThemeTint(EAstrawildSciFantasyTheme Theme);

    /** Persistent VFX particle tint per VFX type. */
    static FLinearColor ResolveVfxTint(EAstrawildEchoVfxType VfxType);

    /** Base-mesh id for a theme + body plan (the generator's bucket rule). */
    static FName ResolveBaseMeshId(EAstrawildSciFantasyTheme Theme, EAstrawildBodyPlan BodyPlan);

    /**
     * Derived engine paths (convention, never literals — validator check 8
     * clean). Resolve only after the Phase 4 import pass lands the packages;
     * until then LoadObject fails closed and the PMC body stays (opt-in).
     */
    static FString BuildSciFantasyBaseMeshPath(const FName& BaseMeshId);
    static FString BuildSciFantasyAnimPath(const FName& BaseMeshId, bool bMoveClip);
    static FString BuildElementVfxSystemPath(EAstrawildEchoVfxType VfxType);
    static FString BuildSoundSetCuePath(const FName& SoundSetId, int32 CueIndex);

    /**
     * Derived engine path of the theme MASTER material for one material
     * language (the 8 M_SciFi_* masters import_echo_bases.py authors —
     * MetallicRobot / EnergyBody / StonyGolem / Slime / Chitin / VoidFlesh /
     * OrganicHide / FocusCrystal). Convention path, never a literal; resolves
     * only after the Phase 4 import pass, fail-closed otherwise.
     */
    static FString BuildThemeMaterialPath(EAstrawildMutationMaterialTheme MaterialTheme);

    /**
     * Material/theme switching on the SKINNED path (Phase 2 amendment — the
     * runtime consumer of the M_SciFi_* masters): creates one dynamic
     * material instance of the theme master over every material slot of the
     * component, parameterized by the species identity — Tint = the
     * definition's primary tint pushed toward the theme tint, PatternTint =
     * the spec's independent pattern tint, GlowIntensity = the material
     * language's default. Opt-in / fail-closed: when the master has not
     * imported, the GLB's own imported materials stay untouched (returns
     * false; no silent fake). Returns true when the swap applied.
     */
    static bool ApplyThemeMaterial(class USkeletalMeshComponent* MeshComponent,
        const UAstrawildEchoDefinition* Definition, const FEchoMutationSpec& Spec);

    /**
     * Persistent element VFX (Phase 2 — VFX binding): attaches an
     * auto-activating Niagara component to the Echo root with the theme
     * tint, IF the NS_AW_Elem_* system imported (opt-in, fail-closed →
     * the existing element glow light stays the floor). Returns the
     * component or nullptr; no-op in worlds without rendering (dedicated
     * server) and for None-type specs.
     */
    static UNiagaraComponent* ApplyElementVfx(AAstrawildEchoCharacter* Echo, const FEchoMutationSpec& Spec);

    /** Player-facing display names (journal / roster / capture cards). */
    static FText GetThemeDisplayName(EAstrawildSciFantasyTheme Theme);
    static FText GetMaterialThemeDisplayName(EAstrawildMutationMaterialTheme MaterialTheme);
    static FText GetVfxTypeDisplayName(EAstrawildEchoVfxType VfxType);
};
