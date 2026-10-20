// ===========================================================================
// AstrawildEchoMutator.cpp
//
// Sci-Fantasy Monster directive Phase 2 — runtime mutation system.
//
// Consumers:
//   - AAstrawildEchoCharacter::BuildProceduralBody  (PMC path: per-part scale
//     + theme palette + attachment geometry — geometry added by the character
//     itself because the part helpers are file-local there)
//   - AAstrawildEchoCharacter::TryActivateSkeletalBody (skinned path: root
//     scale jitter + persistent element VFX)
//   - AAstrawildEchoCharacter hit/vocalization hooks (opt-in sound-set cue)
//
// The 204-row generated table lives in AstrawildEchoMutationData.cpp.
// ===========================================================================

#include "AstrawildEchoMutator.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Crc.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// ---------------------------------------------------------------------------
// Generated-table access (AstrawildEchoMutationData.cpp — declaration in
// AstrawildEchoMutator.h)
// ---------------------------------------------------------------------------

const FEchoMutationSpec* FAstrawildEchoMutator::FindSpec(const FName& SpeciesId)
{
    if (SpeciesId.IsNone())
    {
        return nullptr;
    }
    const TArray<FEchoMutationSpec>& Rows = AstrawildEchoMutation::GetMutationSpecs();
    for (const FEchoMutationSpec& Row : Rows)
    {
        if (Row.SpeciesId == SpeciesId)
        {
            return &Row;
        }
    }
    return nullptr;
}

int32 FAstrawildEchoMutator::GetMutationSpecCount()
{
    return AstrawildEchoMutation::GetMutationSpecs().Num();
}

// ---------------------------------------------------------------------------
// Deterministic helpers (same math family as Scripts/generate_echo_mutations.py)
// ---------------------------------------------------------------------------
namespace
{
    int32 SpeciesSeed(const FName& SpeciesId)
    {
        return static_cast<int32>(FCrc::StrCrc32(*SpeciesId.ToString()) & 0x7FFFFFFF);
    }

    float JitterFromSeed(const int32 Seed, const int32 Salt, const float Spread, const float Lo)
    {
        const uint32 Bucket = static_cast<uint32>(Seed >> (Salt * 7)) & 1023u;
        const float Unit = static_cast<float>(Bucket) / 1023.0f;
        return Lo + Unit * Spread;
    }

    const TCHAR* VoidKeywords[] = { TEXT("void"), TEXT("abyss"), TEXT("shadow"), TEXT("gloom"), TEXT("dusk"), TEXT("hollow"), TEXT("murk"), TEXT("undertow"), TEXT("brine"), TEXT("deep"), TEXT("night"), TEXT("dark"), TEXT("shade") };
    const TCHAR* MechKeywords[] = { TEXT("volt"), TEXT("gear"), TEXT("piston"), TEXT("arc"), TEXT("circuit"), TEXT("spark"), TEXT("coil"), TEXT("mech"), TEXT("chrome"), TEXT("steel"), TEXT("bolt"), TEXT("engine"), TEXT("dyna"), TEXT("charge") };
    const TCHAR* ConstructKeywords[] = { TEXT("golem"), TEXT("monolith"), TEXT("coloss"), TEXT("relic"), TEXT("ancient"), TEXT("stone"), TEXT("ruin"), TEXT("granite"), TEXT("prime"), TEXT("elder"), TEXT("hallow"), TEXT("vesper"), TEXT("primarch") };
    const TCHAR* PlantKeywords[] = { TEXT("moss"), TEXT("bloom"), TEXT("fern"), TEXT("spore"), TEXT("thorn"), TEXT("bramble"), TEXT("sprig"), TEXT("flora"), TEXT("root"), TEXT("vine"), TEXT("seed"), TEXT("mush"), TEXT("petal"), TEXT("leaf"), TEXT("verdant"), TEXT("coral"), TEXT("reed"), TEXT("bough"), TEXT("canopy") };
    const TCHAR* SpiritKeywords[] = { TEXT("ghost"), TEXT("spirit"), TEXT("wisp"), TEXT("phantom"), TEXT("specter"), TEXT("soul"), TEXT("wail"), TEXT("lume"), TEXT("mist"), TEXT("glimmer"), TEXT("dream"), TEXT("aura"), TEXT("chime") };
    const TCHAR* ElementalKeywords[] = { TEXT("ember"), TEXT("pyre"), TEXT("flame"), TEXT("blaze"), TEXT("magma"), TEXT("frost"), TEXT("rime"), TEXT("glacier"), TEXT("gale"), TEXT("storm"), TEXT("thunder"), TEXT("tide"), TEXT("wave"), TEXT("current"), TEXT("sun"), TEXT("dawn"), TEXT("ray"), TEXT("melt"), TEXT("freeze") };
    const TCHAR* ArmorKeywords[] = { TEXT("shell"), TEXT("plate"), TEXT("carapace"), TEXT("scale"), TEXT("chitin"), TEXT("bastion"), TEXT("bulwark"), TEXT("shield"), TEXT("armor"), TEXT("crest"), TEXT("horn"), TEXT("beetle"), TEXT("crab") };

    const TCHAR* AttachBias_AncientConstruct[] = { TEXT("HornCrown"), TEXT("GlowNodes"), TEXT("DorsalSpikes") };
    const TCHAR* AttachBias_ElementalBeast[] = { TEXT("GlowNodes"), TEXT("Wings"), TEXT("DorsalSpikes") };
    const TCHAR* AttachBias_MutatedFauna[] = { TEXT("DorsalSpikes"), TEXT("ThirdEye"), TEXT("ExtraArms"), TEXT("Wings") };
    const TCHAR* AttachBias_ArmoredOrganic[] = { TEXT("DorsalSpikes"), TEXT("HornCrown"), TEXT("TailFin") };
    const TCHAR* AttachBias_EtherealSpirit[] = { TEXT("GlowNodes"), TEXT("Wings") };
    const TCHAR* AttachBias_MechanicalHybrid[] = { TEXT("ExtraArms"), TEXT("GlowNodes"), TEXT("ThirdEye") };
    const TCHAR* AttachBias_PlantMonster[] = { TEXT("DorsalSpikes"), TEXT("ThirdEye"), TEXT("ExtraArms") };
    const TCHAR* AttachBias_VoidAbomination[] = { TEXT("ExtraArms"), TEXT("ThirdEye"), TEXT("GlowNodes"), TEXT("Wings") };

    const TCHAR* ThemeSoundSets[] =
    {
        TEXT("SFXSet_AncientConstruct"), TEXT("SFXSet_ElementalBeast"), TEXT("SFXSet_MutatedFauna"),
        TEXT("SFXSet_ArmoredOrganic"), TEXT("SFXSet_EtherealSpirit"), TEXT("SFXSet_MechanicalHybrid"),
        TEXT("SFXSet_PlantMonster"), TEXT("SFXSet_VoidAbomination"),
    };

    EAstrawildSciFantasyTheme ThemeFromSignals(const FString& NameLower, const EAstrawildEchoFamily Family, const EAstrawildBodyPlan BodyPlan)
    {
        struct FSignal { const TCHAR* const* Keywords; int32 Count; EAstrawildSciFantasyTheme Theme; };
        const FSignal Signals[] =
        {
            { VoidKeywords, ARRAY_COUNT(VoidKeywords), EAstrawildSciFantasyTheme::VoidAbomination },
            { MechKeywords, ARRAY_COUNT(MechKeywords), EAstrawildSciFantasyTheme::MechanicalHybrid },
            { ConstructKeywords, ARRAY_COUNT(ConstructKeywords), EAstrawildSciFantasyTheme::AncientConstruct },
            { PlantKeywords, ARRAY_COUNT(PlantKeywords), EAstrawildSciFantasyTheme::PlantMonster },
            { SpiritKeywords, ARRAY_COUNT(SpiritKeywords), EAstrawildSciFantasyTheme::EtherealSpirit },
            { ElementalKeywords, ARRAY_COUNT(ElementalKeywords), EAstrawildSciFantasyTheme::ElementalBeast },
            { ArmorKeywords, ARRAY_COUNT(ArmorKeywords), EAstrawildSciFantasyTheme::ArmoredOrganic },
        };
        for (const FSignal& Signal : Signals)
        {
            for (int32 i = 0; i < Signal.Count; ++i)
            {
                if (NameLower.Contains(Signal.Keywords[i], ESearchCase::IgnoreCase))
                {
                    return Signal.Theme;
                }
            }
        }

        if (BodyPlan == EAstrawildBodyPlan::Amorphous && Family != EAstrawildEchoFamily::Flora && Family != EAstrawildEchoFamily::Spirit)
        {
            return EAstrawildSciFantasyTheme::VoidAbomination;
        }
        if (BodyPlan == EAstrawildBodyPlan::Crystalline)
        {
            return EAstrawildSciFantasyTheme::AncientConstruct;
        }

        switch (Family)
        {
        case EAstrawildEchoFamily::Construct:
        case EAstrawildEchoFamily::Ancient:
            return EAstrawildSciFantasyTheme::AncientConstruct;
        case EAstrawildEchoFamily::Spirit:
            return EAstrawildSciFantasyTheme::EtherealSpirit;
        case EAstrawildEchoFamily::Elemental:
            return EAstrawildSciFantasyTheme::ElementalBeast;
        case EAstrawildEchoFamily::Flora:
            return EAstrawildSciFantasyTheme::PlantMonster;
        case EAstrawildEchoFamily::Insectoid:
            return EAstrawildSciFantasyTheme::ArmoredOrganic;
        default:
            return EAstrawildSciFantasyTheme::MutatedFauna;
        }
    }

    EAstrawildMutationMaterialTheme MaterialFromTheme(const EAstrawildSciFantasyTheme Theme, const EAstrawildBodyPlan BodyPlan)
    {
        if (BodyPlan == EAstrawildBodyPlan::Crystalline)
        {
            return EAstrawildMutationMaterialTheme::Crystalline;
        }
        if (Theme == EAstrawildSciFantasyTheme::VoidAbomination && BodyPlan == EAstrawildBodyPlan::Amorphous)
        {
            return EAstrawildMutationMaterialTheme::Void;
        }
        switch (Theme)
        {
        case EAstrawildSciFantasyTheme::AncientConstruct: return EAstrawildMutationMaterialTheme::Stony;
        case EAstrawildSciFantasyTheme::ElementalBeast:   return EAstrawildMutationMaterialTheme::Energy;
        case EAstrawildSciFantasyTheme::EtherealSpirit:   return EAstrawildMutationMaterialTheme::Energy;
        case EAstrawildSciFantasyTheme::MechanicalHybrid: return EAstrawildMutationMaterialTheme::Metallic;
        case EAstrawildSciFantasyTheme::ArmoredOrganic:   return EAstrawildMutationMaterialTheme::Chitin;
        case EAstrawildSciFantasyTheme::PlantMonster:     return EAstrawildMutationMaterialTheme::Organic;
        case EAstrawildSciFantasyTheme::VoidAbomination:  return EAstrawildMutationMaterialTheme::Slime;
        default:                                          return EAstrawildMutationMaterialTheme::Organic;
        }
    }

    EAstrawildEchoVfxType VfxFromElement(const EAstrawildElementType Element, const EAstrawildSciFantasyTheme Theme)
    {
        switch (Element)
        {
        case EAstrawildElementType::Ember:  return EAstrawildEchoVfxType::Fire;
        case EAstrawildElementType::Frost:  return EAstrawildEchoVfxType::Frost;
        case EAstrawildElementType::Pulse:  return EAstrawildEchoVfxType::Electric;
        case EAstrawildElementType::Flora:  return EAstrawildEchoVfxType::Spore;
        case EAstrawildElementType::Light:  return EAstrawildEchoVfxType::Radiant;
        case EAstrawildElementType::Ash:    return EAstrawildEchoVfxType::Poison;
        default: break;
        }
        if (Theme == EAstrawildSciFantasyTheme::VoidAbomination)
        {
            return EAstrawildEchoVfxType::Void;
        }
        return EAstrawildEchoVfxType::None;
    }

    bool IsSecondaryPlanBucket(const EAstrawildBodyPlan BodyPlan)
    {
        switch (BodyPlan)
        {
        case EAstrawildBodyPlan::Avian:
        case EAstrawildBodyPlan::Floating:
        case EAstrawildBodyPlan::Serpent:
        case EAstrawildBodyPlan::Amorphous:
            return true;
        default:
            return false;
        }
    }
}

FName FAstrawildEchoMutator::ResolveBaseMeshId(const EAstrawildSciFantasyTheme Theme, const EAstrawildBodyPlan BodyPlan)
{
    const bool bSecondary = IsSecondaryPlanBucket(BodyPlan);
    switch (Theme)
    {
    case EAstrawildSciFantasyTheme::AncientConstruct: return bSecondary ? TEXT("SK_Base_MonolithColossus") : TEXT("SK_Base_GolemQuadruped");
    case EAstrawildSciFantasyTheme::ElementalBeast:   return bSecondary ? TEXT("SK_Base_ElemWisp") : TEXT("SK_Base_ElemDrake");
    case EAstrawildSciFantasyTheme::MutatedFauna:     return bSecondary ? TEXT("SK_Base_MutantAvian") : TEXT("SK_Base_MutantBeast");
    case EAstrawildSciFantasyTheme::ArmoredOrganic:   return bSecondary ? TEXT("SK_Base_ArmoredCrab") : TEXT("SK_Base_ArmoredBeetle");
    case EAstrawildSciFantasyTheme::EtherealSpirit:   return bSecondary ? TEXT("SK_Base_SpiritOrb") : TEXT("SK_Base_SpiritWisp");
    case EAstrawildSciFantasyTheme::MechanicalHybrid: return bSecondary ? TEXT("SK_Base_CyborgSerpent") : TEXT("SK_Base_CyborgBeast");
    case EAstrawildSciFantasyTheme::PlantMonster:     return bSecondary ? TEXT("SK_Base_Mushroomling") : TEXT("SK_Base_PlantMaw");
    case EAstrawildSciFantasyTheme::VoidAbomination:  return bSecondary ? TEXT("SK_Base_VoidTentacle") : TEXT("SK_Base_VoidBlob");
    default:                                          return TEXT("SK_Base_MutantBeast");
    }
}

FEchoMutationSpec FAstrawildEchoMutator::BuildDeterministicSpec(const UAstrawildEchoDefinition* Definition)
{
    FEchoMutationSpec Spec;
    if (!Definition || Definition->DefinitionId.IsNone())
    {
        Spec.Theme = EAstrawildSciFantasyTheme::MutatedFauna;
        Spec.BaseMeshId = ResolveBaseMeshId(Spec.Theme, EAstrawildBodyPlan::Quadruped);
        Spec.AttachmentMask = GetAttachmentBits(EAstrawildEchoAttachment::DorsalSpikes);
        Spec.SoundSetId = TEXT("SFXSet_MutatedFauna");
        return Spec;
    }

    const FString Name = Definition->DefinitionId.ToString();
    const int32 Seed = SpeciesSeed(Definition->DefinitionId);

    Spec.SpeciesId = Definition->DefinitionId;
    Spec.Theme = ThemeFromSignals(Name, Definition->Family, Definition->BodyPlan);
    Spec.BaseMeshId = ResolveBaseMeshId(Spec.Theme, Definition->BodyPlan);
    Spec.HeadScale = JitterFromSeed(Seed, 1, 0.70f, 0.75f);
    Spec.TorsoScale = JitterFromSeed(Seed, 2, 0.60f, 0.80f);
    Spec.LimbScale = JitterFromSeed(Seed, 3, 0.55f, 0.80f);
    Spec.TailScale = JitterFromSeed(Seed, 4, 0.65f, 0.70f);
    Spec.MaterialTheme = MaterialFromTheme(Spec.Theme, Definition->BodyPlan);
    Spec.VfxType = VfxFromElement(Definition->Element, Spec.Theme);
    const int32 ThemeIndex = static_cast<int32>(Spec.Theme);
    Spec.SoundSetId = ThemeSoundSets[(ThemeIndex >= 0 && ThemeIndex < ARRAY_COUNT(ThemeSoundSets)) ? ThemeIndex : 2];

    // Attachment picks from the theme bias (deterministic count 1..3).
    const TCHAR* const* Bias = nullptr;
    int32 BiasCount = 0;
    switch (Spec.Theme)
    {
    case EAstrawildSciFantasyTheme::AncientConstruct: Bias = AttachBias_AncientConstruct; BiasCount = ARRAY_COUNT(AttachBias_AncientConstruct); break;
    case EAstrawildSciFantasyTheme::ElementalBeast:   Bias = AttachBias_ElementalBeast;   BiasCount = ARRAY_COUNT(AttachBias_ElementalBeast); break;
    case EAstrawildSciFantasyTheme::MutatedFauna:     Bias = AttachBias_MutatedFauna;     BiasCount = ARRAY_COUNT(AttachBias_MutatedFauna); break;
    case EAstrawildSciFantasyTheme::ArmoredOrganic:   Bias = AttachBias_ArmoredOrganic;   BiasCount = ARRAY_COUNT(AttachBias_ArmoredOrganic); break;
    case EAstrawildSciFantasyTheme::EtherealSpirit:   Bias = AttachBias_EtherealSpirit;   BiasCount = ARRAY_COUNT(AttachBias_EtherealSpirit); break;
    case EAstrawildSciFantasyTheme::MechanicalHybrid: Bias = AttachBias_MechanicalHybrid; BiasCount = ARRAY_COUNT(AttachBias_MechanicalHybrid); break;
    case EAstrawildSciFantasyTheme::PlantMonster:     Bias = AttachBias_PlantMonster;     BiasCount = ARRAY_COUNT(AttachBias_PlantMonster); break;
    case EAstrawildSciFantasyTheme::VoidAbomination:  Bias = AttachBias_VoidAbomination;  BiasCount = ARRAY_COUNT(AttachBias_VoidAbomination); break;
    default: Bias = AttachBias_MutatedFauna; BiasCount = ARRAY_COUNT(AttachBias_MutatedFauna); break;
    }

    const int32 AttachCount = 1 + (Seed % 3);
    for (int32 i = 0; i < AttachCount && Bias; ++i)
    {
        const FString Pick = Bias[(Seed >> (4 + i * 3)) % BiasCount];
        if (Pick == TEXT("DorsalSpikes")) { Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::DorsalSpikes); }
        else if (Pick == TEXT("Wings"))   { Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::Wings); }
        else if (Pick == TEXT("ThirdEye")){ Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::ThirdEye); }
        else if (Pick == TEXT("ExtraArms")){ Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::ExtraArms); }
        else if (Pick == TEXT("HornCrown")){ Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::HornCrown); }
        else if (Pick == TEXT("GlowNodes")){ Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::GlowNodes); }
        else if (Pick == TEXT("TailFin"))  { Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::TailFin); }
    }
    if (IsSecondaryPlanBucket(Definition->BodyPlan) && (Seed % 4 == 0))
    {
        Spec.AttachmentMask |= GetAttachmentBits(EAstrawildEchoAttachment::Wings);
    }
    if (Spec.AttachmentMask == 0)
    {
        Spec.AttachmentMask = GetAttachmentBits(EAstrawildEchoAttachment::GlowNodes);
    }

    // Pattern tint: theme palette + deterministic jitter (independent of the
    // definition's body tints — the "สีลายสุ่มอิสระ" rule).
    const FLinearColor ThemeTint = ResolveThemeTint(Spec.Theme);
    Spec.PatternTint = FLinearColor(
        FMath::Clamp(ThemeTint.R + JitterFromSeed(Seed, 5, 0.24f, -0.12f), 0.0f, 1.0f),
        FMath::Clamp(ThemeTint.G + JitterFromSeed(Seed, 6, 0.24f, -0.12f), 0.0f, 1.0f),
        FMath::Clamp(ThemeTint.B + JitterFromSeed(Seed, 7, 0.24f, -0.12f), 0.0f, 1.0f),
        1.0f);
    return Spec;
}

// ---------------------------------------------------------------------------
// Pure mutation math (automation-tested)
// ---------------------------------------------------------------------------
void FAstrawildEchoMutator::ComputePartScales(const FEchoMutationSpec& Spec, const int32 InstanceSalt,
    float& OutHeadScale, float& OutTorsoScale, float& OutLimbScale, float& OutTailScale)
{
    const uint32 Mixed = static_cast<uint32>(SpeciesSeed(Spec.SpeciesId)) ^ (static_cast<uint32>(InstanceSalt) * 2654435761u);
    const int32 Seed = static_cast<int32>(Mixed & 0x7FFFFFFF);
    const float J = JitterFromSeed(Seed, 1, 0.10f, -0.05f); // ±5%
    auto Apply = [J](const float Base)
    {
        return FMath::Clamp(Base * (1.0f + J), 0.6f, 1.6f);
    };
    OutHeadScale = Apply(Spec.HeadScale);
    OutTorsoScale = Apply(Spec.TorsoScale);
    OutLimbScale = Apply(Spec.LimbScale);
    OutTailScale = Apply(Spec.TailScale);
}

float FAstrawildEchoMutator::ComputeRootScaleJitter(const FName& SpeciesId, const int32 InstanceSalt)
{
    const uint32 Mixed = static_cast<uint32>(SpeciesSeed(SpeciesId)) ^ (static_cast<uint32>(InstanceSalt) * 40503u);
    const int32 Seed = static_cast<int32>(Mixed & 0x7FFFFFFF);
    return JitterFromSeed(Seed, 1, 0.12f, 0.94f); // 0.94..1.06
}

int32 FAstrawildEchoMutator::GetAttachmentBits(const EAstrawildEchoAttachment Attachment)
{
    switch (Attachment)
    {
    case EAstrawildEchoAttachment::DorsalSpikes: return 1;
    case EAstrawildEchoAttachment::Wings:        return 2;
    case EAstrawildEchoAttachment::ThirdEye:     return 4;
    case EAstrawildEchoAttachment::ExtraArms:    return 8;
    case EAstrawildEchoAttachment::HornCrown:    return 16;
    case EAstrawildEchoAttachment::GlowNodes:    return 32;
    case EAstrawildEchoAttachment::TailFin:      return 64;
    default:                                     return 0;
    }
}

bool FAstrawildEchoMutator::HasAttachment(const FEchoMutationSpec& Spec, const EAstrawildEchoAttachment Attachment)
{
    return (Spec.AttachmentMask & GetAttachmentBits(Attachment)) != 0;
}

void FAstrawildEchoMutator::ApplyThemeToBodyColors(const FColor& Primary, const FColor& Secondary,
    const FEchoMutationSpec& Spec, FColor& OutPrimary, FColor& OutSecondary)
{
    const FLinearColor LinPrimary(Primary);
    const FLinearColor LinSecondary(Secondary);
    const FLinearColor Pattern = Spec.PatternTint;

    FLinearColor NewPrimary = LinPrimary;
    FLinearColor NewSecondary = LinSecondary;

    const float Lum = LinPrimary.R * 0.299f + LinPrimary.G * 0.587f + LinPrimary.B * 0.114f;
    const float LumSec = LinSecondary.R * 0.299f + LinSecondary.G * 0.587f + LinSecondary.B * 0.114f;

    switch (Spec.MaterialTheme)
    {
    case EAstrawildMutationMaterialTheme::Metallic:
        // Brushed metal: cool, desaturated, darker secondary.
        NewPrimary = FLinearColor(FMath::Lerp(LinPrimary.R, Lum * 0.8f, 0.35f) + 0.04f,
            FMath::Lerp(LinPrimary.G, Lum * 0.85f, 0.35f) + 0.05f,
            FMath::Lerp(LinPrimary.B, Lum * 1.0f, 0.35f) + 0.08f, 1.0f);
        NewSecondary = FLinearColor(LumSec * 0.35f, LumSec * 0.38f, LumSec * 0.44f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Stony:
        // Granite: heavy desaturation, low contrast.
        NewPrimary = FLinearColor(Lum * 0.85f, Lum * 0.82f, Lum * 0.78f, 1.0f);
        NewSecondary = FLinearColor(LumSec * 0.7f, LumSec * 0.68f, LumSec * 0.64f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Energy:
        // Living energy body: brighten toward the pattern tint.
        NewPrimary = FLinearColor(FMath::Lerp(LinPrimary.R, Pattern.R + 0.25f, 0.45f),
            FMath::Lerp(LinPrimary.G, Pattern.G + 0.25f, 0.45f),
            FMath::Lerp(LinPrimary.B, Pattern.B + 0.25f, 0.45f), 1.0f);
        NewSecondary = FLinearColor(Pattern.R * 1.1f, Pattern.G * 1.1f, Pattern.B * 1.1f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Slime:
        // Wet slime: saturate + darken primary, glossy lighter secondary.
        NewPrimary = FLinearColor(LinPrimary.R * 0.75f + 0.03f, LinPrimary.G * 0.9f + 0.06f, LinPrimary.B * 0.85f + 0.05f, 1.0f);
        NewSecondary = FLinearColor(LumSec * 0.6f + 0.12f, LumSec * 0.7f + 0.16f, LumSec * 0.62f + 0.12f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Chitin:
        // Insect shell: high contrast, darker plates with bright edges.
        NewPrimary = FLinearColor(Lum * 0.55f + 0.05f, Lum * 0.5f + 0.05f, Lum * 0.45f + 0.05f, 1.0f);
        NewSecondary = FLinearColor(FMath::Lerp(LinSecondary.R, Pattern.R, 0.55f) + 0.1f,
            FMath::Lerp(LinSecondary.G, Pattern.G, 0.55f) + 0.1f,
            FMath::Lerp(LinSecondary.B, Pattern.B, 0.55f) + 0.1f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Organic:
        // Mutated flesh/flora: warm the primary, pattern-marked secondary.
        NewPrimary = FLinearColor(LinPrimary.R * 1.08f + 0.03f, LinPrimary.G * 0.98f, LinPrimary.B * 0.92f, 1.0f);
        NewSecondary = FLinearColor(FMath::Lerp(LinSecondary.R, Pattern.R, 0.5f),
            FMath::Lerp(LinSecondary.G, Pattern.G, 0.5f),
            FMath::Lerp(LinSecondary.B, Pattern.B, 0.5f), 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Crystalline:
        // Faceted crystal: bright primary, luminous secondary.
        NewPrimary = FLinearColor(FMath::Lerp(LinPrimary.R, Pattern.R, 0.4f) + 0.15f,
            FMath::Lerp(LinPrimary.G, Pattern.G, 0.4f) + 0.15f,
            FMath::Lerp(LinPrimary.B, Pattern.B, 0.4f) + 0.18f, 1.0f);
        NewSecondary = FLinearColor(Pattern.R + 0.3f, Pattern.G + 0.3f, Pattern.B + 0.3f, 1.0f);
        break;
    case EAstrawildMutationMaterialTheme::Void:
    default:
        // Void flesh: dark, bruised violet cast.
        NewPrimary = FLinearColor(LinPrimary.R * 0.45f, LinPrimary.G * 0.35f, LinPrimary.B * 0.55f + 0.06f, 1.0f);
        NewSecondary = FLinearColor(Pattern.R * 0.5f, Pattern.G * 0.4f, Pattern.B * 0.8f + 0.1f, 1.0f);
        break;
    }

    auto ClampColor = [](const FLinearColor& C)
    {
        return FLinearColor(FMath::Clamp(C.R, 0.0f, 1.0f), FMath::Clamp(C.G, 0.0f, 1.0f),
            FMath::Clamp(C.B, 0.0f, 1.0f), 1.0f).ToFColor(true);
    };
    OutPrimary = ClampColor(NewPrimary);
    OutSecondary = ClampColor(NewSecondary);
}

FLinearColor FAstrawildEchoMutator::ResolveThemeTint(const EAstrawildSciFantasyTheme Theme)
{
    switch (Theme)
    {
    case EAstrawildSciFantasyTheme::AncientConstruct: return FLinearColor(0.62f, 0.58f, 0.50f, 1.0f);
    case EAstrawildSciFantasyTheme::ElementalBeast:   return FLinearColor(0.95f, 0.62f, 0.30f, 1.0f);
    case EAstrawildSciFantasyTheme::MutatedFauna:     return FLinearColor(0.55f, 0.48f, 0.38f, 1.0f);
    case EAstrawildSciFantasyTheme::ArmoredOrganic:   return FLinearColor(0.45f, 0.52f, 0.42f, 1.0f);
    case EAstrawildSciFantasyTheme::EtherealSpirit:   return FLinearColor(0.72f, 0.82f, 0.95f, 1.0f);
    case EAstrawildSciFantasyTheme::MechanicalHybrid: return FLinearColor(0.58f, 0.63f, 0.70f, 1.0f);
    case EAstrawildSciFantasyTheme::PlantMonster:     return FLinearColor(0.42f, 0.71f, 0.33f, 1.0f);
    case EAstrawildSciFantasyTheme::VoidAbomination:  return FLinearColor(0.38f, 0.30f, 0.44f, 1.0f);
    default:                                          return FLinearColor(0.55f, 0.55f, 0.55f, 1.0f);
    }
}

FLinearColor FAstrawildEchoMutator::ResolveVfxTint(const EAstrawildEchoVfxType VfxType)
{
    switch (VfxType)
    {
    case EAstrawildEchoVfxType::Fire:    return FLinearColor(1.00f, 0.42f, 0.12f, 1.0f);
    case EAstrawildEchoVfxType::Frost:   return FLinearColor(0.55f, 0.88f, 0.98f, 1.0f);
    case EAstrawildEchoVfxType::Electric:return FLinearColor(0.36f, 0.92f, 0.86f, 1.0f);
    case EAstrawildEchoVfxType::Void:    return FLinearColor(0.42f, 0.20f, 0.55f, 1.0f);
    case EAstrawildEchoVfxType::Poison:  return FLinearColor(0.62f, 0.92f, 0.28f, 1.0f);
    case EAstrawildEchoVfxType::Spore:   return FLinearColor(0.72f, 0.95f, 0.40f, 1.0f);
    case EAstrawildEchoVfxType::Radiant: return FLinearColor(0.98f, 0.95f, 0.72f, 1.0f);
    default:                             return FLinearColor::White;
    }
}

// ---------------------------------------------------------------------------
// Derived engine paths (convention — validator check 8 clean)
// ---------------------------------------------------------------------------
FString FAstrawildEchoMutator::BuildSciFantasyBaseMeshPath(const FName& BaseMeshId)
{
    const FString Id = BaseMeshId.IsNone() ? FString(TEXT("SK_Base_MutantBeast")) : BaseMeshId.ToString();
    return FString::Printf(TEXT("/Game/Characters/Echoes/BaseMeshes/%s.%s"), *Id, *Id);
}

FString FAstrawildEchoMutator::BuildSciFantasyAnimPath(const FName& BaseMeshId, const bool bMoveClip)
{
    const FString Id = BaseMeshId.IsNone() ? FString(TEXT("SK_Base_MutantBeast")) : BaseMeshId.ToString();
    const TCHAR* Clip = bMoveClip ? TEXT("Move") : TEXT("Idle");
    return FString::Printf(TEXT("/Game/Characters/Echoes/BaseMeshes/AM_%s_%s.AM_%s_%s"), *Id, Clip, *Id, Clip);
}

FString FAstrawildEchoMutator::BuildElementVfxSystemPath(const EAstrawildEchoVfxType VfxType)
{
    const TCHAR* Suffix = TEXT("Fire");
    switch (VfxType)
    {
    case EAstrawildEchoVfxType::Frost:    Suffix = TEXT("Frost"); break;
    case EAstrawildEchoVfxType::Electric: Suffix = TEXT("Electric"); break;
    case EAstrawildEchoVfxType::Void:     Suffix = TEXT("Void"); break;
    case EAstrawildEchoVfxType::Poison:   Suffix = TEXT("Poison"); break;
    case EAstrawildEchoVfxType::Spore:    Suffix = TEXT("Spore"); break;
    case EAstrawildEchoVfxType::Radiant:  Suffix = TEXT("Radiant"); break;
    default:                              break;
    }
    return FString::Printf(TEXT("/Game/VFX/NS_AW_Elem_%s.NS_AW_Elem_%s"), Suffix, Suffix);
}

FString FAstrawildEchoMutator::BuildSoundSetCuePath(const FName& SoundSetId, const int32 CueIndex)
{
    const FString Id = SoundSetId.IsNone() ? FString(TEXT("SFXSet_MutatedFauna")) : SoundSetId.ToString();
    return FString::Printf(TEXT("/Game/Audio/Echoes/%s_%d.%s_%d"), *Id, CueIndex, *Id, CueIndex);
}

FString FAstrawildEchoMutator::BuildThemeMaterialPath(const EAstrawildMutationMaterialTheme MaterialTheme)
{
    const TCHAR* Name = TEXT("M_SciFi_OrganicHide");
    switch (MaterialTheme)
    {
    case EAstrawildMutationMaterialTheme::Metallic:    Name = TEXT("M_SciFi_MetallicRobot"); break;
    case EAstrawildMutationMaterialTheme::Stony:       Name = TEXT("M_SciFi_StonyGolem"); break;
    case EAstrawildMutationMaterialTheme::Energy:      Name = TEXT("M_SciFi_EnergyBody"); break;
    case EAstrawildMutationMaterialTheme::Slime:       Name = TEXT("M_SciFi_Slime"); break;
    case EAstrawildMutationMaterialTheme::Chitin:      Name = TEXT("M_SciFi_Chitin"); break;
    case EAstrawildMutationMaterialTheme::Organic:     Name = TEXT("M_SciFi_OrganicHide"); break;
    case EAstrawildMutationMaterialTheme::Crystalline: Name = TEXT("M_SciFi_FocusCrystal"); break;
    case EAstrawildMutationMaterialTheme::Void:        Name = TEXT("M_SciFi_VoidFlesh"); break;
    default: break;
    }
    return FString::Printf(TEXT("/Game/Materials/%s.%s"), Name, Name);
}

bool FAstrawildEchoMutator::ApplyThemeMaterial(USkeletalMeshComponent* MeshComponent,
    const UAstrawildEchoDefinition* Definition, const FEchoMutationSpec& Spec)
{
    if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset() || !MeshComponent->GetWorld())
    {
        return false;
    }
    if (MeshComponent->GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        return false; // cosmetic only — never touch materials on the headless server
    }

    const FString MaterialPath = BuildThemeMaterialPath(Spec.MaterialTheme);
    UMaterial* Master = LoadObject<UMaterial>(nullptr, *MaterialPath);
    if (!Master)
    {
        // Opt-in contract: the M_SciFi_* master has not imported yet — the
        // GLB's own imported materials stay (no silent fake).
        UE_LOG(LogAstrawildAI, Verbose, TEXT("SciFantasy theme master not imported (%s) — skinned materials stay as imported."), *MaterialPath);
        return false;
    }

    // Species identity parameters (mirrors the PMC path's vertex-color
    // language): the definition's primary tint pushed toward the theme
    // tint, the spec's independent pattern tint, and the material
    // language's glow default (energy bodies glow, stone barely does).
    FLinearColor Tint = ResolveThemeTint(Spec.Theme);
    if (Definition)
    {
        Tint = FMath::Lerp(Definition->PrimaryTint, ResolveThemeTint(Spec.Theme), 0.35f);
    }
    float GlowIntensity = 0.20f;
    switch (Spec.MaterialTheme)
    {
    case EAstrawildMutationMaterialTheme::Energy:      GlowIntensity = 2.20f; break;
    case EAstrawildMutationMaterialTheme::Crystalline: GlowIntensity = 0.90f; break;
    case EAstrawildMutationMaterialTheme::Void:        GlowIntensity = 0.80f; break;
    case EAstrawildMutationMaterialTheme::Slime:       GlowIntensity = 0.45f; break;
    case EAstrawildMutationMaterialTheme::Metallic:    GlowIntensity = 0.35f; break;
    case EAstrawildMutationMaterialTheme::Chitin:      GlowIntensity = 0.30f; break;
    case EAstrawildMutationMaterialTheme::Stony:       GlowIntensity = 0.12f; break;
    case EAstrawildMutationMaterialTheme::Organic:
    default:                                          GlowIntensity = 0.20f; break;
    }

    const int32 NumSlots = MeshComponent->GetNumMaterials();
    int32 Applied = 0;
    for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
    {
        UMaterialInstanceDynamic* Instance = MeshComponent->CreateDynamicMaterialInstance(SlotIndex, Master);
        if (!Instance)
        {
            continue;
        }
        Instance->SetVectorParameterValue(TEXT("Tint"), Tint);
        Instance->SetVectorParameterValue(TEXT("PatternTint"), Spec.PatternTint);
        Instance->SetScalarParameterValue(TEXT("GlowIntensity"), GlowIntensity);
        ++Applied;
    }
    UE_LOG(LogAstrawildAI, Log, TEXT("SciFantasy theme material bound (%s, %d/%d slots, tint %s)."),
        *MaterialPath, Applied, NumSlots, *Tint.ToString());
    return Applied > 0;
}

// ---------------------------------------------------------------------------
// Runtime VFX binding (opt-in, fail-closed)
// ---------------------------------------------------------------------------
UNiagaraComponent* FAstrawildEchoMutator::ApplyElementVfx(AAstrawildEchoCharacter* Echo, const FEchoMutationSpec& Spec)
{
    if (!Echo || Spec.VfxType == EAstrawildEchoVfxType::None)
    {
        return nullptr;
    }

    UWorld* World = Echo->GetWorld();
    if (!World || World->IsNetMode(NM_DedicatedServer))
    {
        return nullptr; // cosmetic only — never spawn on the headless server
    }

    UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *BuildElementVfxSystemPath(Spec.VfxType));
    if (!System)
    {
        // Opt-in contract: the NS_AW_Elem_* template is not imported yet —
        // the element glow light stays the visual floor (no silent fake).
        UE_LOG(LogAstrawildAI, Verbose, TEXT("SciFantasy element VFX not imported (%s) — glow light floor stays."),
            *BuildElementVfxSystemPath(Spec.VfxType));
        return nullptr;
    }

    UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAttached(
        System, Echo->GetRootComponent(), NAME_None, FVector(0.0f, 0.0f, 55.0f), FRotator::ZeroRotator,
        EAttachLocation::KeepRelativeOffset, /*bAutoDestroy*/ false, ENCPoolMethod::None, /*bAutoActivate*/ true);
    if (Component)
    {
        UE_LOG(LogAstrawildAI, Log, TEXT("SciFantasy element VFX bound to %s (type %d)."),
            *Echo->GetName(), static_cast<int32>(Spec.VfxType));
    }
    return Component;
}

// ---------------------------------------------------------------------------
// Display names (player-facing vocabulary — no implementation jargon)
// ---------------------------------------------------------------------------
FText FAstrawildEchoMutator::GetThemeDisplayName(const EAstrawildSciFantasyTheme Theme)
{
    switch (Theme)
    {
    case EAstrawildSciFantasyTheme::AncientConstruct: return NSLOCTEXT("Astrawild", "Theme_AncientConstruct", "Ancient Construct");
    case EAstrawildSciFantasyTheme::ElementalBeast:   return NSLOCTEXT("Astrawild", "Theme_ElementalBeast", "Elemental Beast");
    case EAstrawildSciFantasyTheme::MutatedFauna:     return NSLOCTEXT("Astrawild", "Theme_MutatedFauna", "Mutated Fauna");
    case EAstrawildSciFantasyTheme::ArmoredOrganic:   return NSLOCTEXT("Astrawild", "Theme_ArmoredOrganic", "Armored Organic");
    case EAstrawildSciFantasyTheme::EtherealSpirit:   return NSLOCTEXT("Astrawild", "Theme_EtherealSpirit", "Ethereal Spirit");
    case EAstrawildSciFantasyTheme::MechanicalHybrid: return NSLOCTEXT("Astrawild", "Theme_MechanicalHybrid", "Mechanical Hybrid");
    case EAstrawildSciFantasyTheme::PlantMonster:     return NSLOCTEXT("Astrawild", "Theme_PlantMonster", "Plant Monster");
    case EAstrawildSciFantasyTheme::VoidAbomination:  return NSLOCTEXT("Astrawild", "Theme_VoidAbomination", "Void Abomination");
    default:                                          return NSLOCTEXT("Astrawild", "Theme_Unknown", "Unknown");
    }
}

FText FAstrawildEchoMutator::GetMaterialThemeDisplayName(const EAstrawildMutationMaterialTheme MaterialTheme)
{
    switch (MaterialTheme)
    {
    case EAstrawildMutationMaterialTheme::Metallic:   return NSLOCTEXT("Astrawild", "Mat_Metallic", "Plated Metal");
    case EAstrawildMutationMaterialTheme::Stony:      return NSLOCTEXT("Astrawild", "Mat_Stony", "Carved Stone");
    case EAstrawildMutationMaterialTheme::Energy:     return NSLOCTEXT("Astrawild", "Mat_Energy", "Living Energy");
    case EAstrawildMutationMaterialTheme::Slime:      return NSLOCTEXT("Astrawild", "Mat_Slime", "Oozing Slime");
    case EAstrawildMutationMaterialTheme::Chitin:     return NSLOCTEXT("Astrawild", "Mat_Chitin", "Hardened Shell");
    case EAstrawildMutationMaterialTheme::Organic:    return NSLOCTEXT("Astrawild", "Mat_Organic", "Wild Flesh");
    case EAstrawildMutationMaterialTheme::Crystalline:return NSLOCTEXT("Astrawild", "Mat_Crystalline", "Focus Crystal");
    case EAstrawildMutationMaterialTheme::Void:       return NSLOCTEXT("Astrawild", "Mat_Void", "Void Matter");
    default:                                         return NSLOCTEXT("Astrawild", "Mat_Unknown", "Strange Matter");
    }
}

FText FAstrawildEchoMutator::GetVfxTypeDisplayName(const EAstrawildEchoVfxType VfxType)
{
    switch (VfxType)
    {
    case EAstrawildEchoVfxType::Fire:     return NSLOCTEXT("Astrawild", "Vfx_Fire", "Burning");
    case EAstrawildEchoVfxType::Frost:    return NSLOCTEXT("Astrawild", "Vfx_Frost", "Frosted");
    case EAstrawildEchoVfxType::Electric: return NSLOCTEXT("Astrawild", "Vfx_Electric", "Crackling");
    case EAstrawildEchoVfxType::Void:     return NSLOCTEXT("Astrawild", "Vfx_Void", "Unraveling");
    case EAstrawildEchoVfxType::Poison:   return NSLOCTEXT("Astrawild", "Vfx_Poison", "Venomous");
    case EAstrawildEchoVfxType::Spore:    return NSLOCTEXT("Astrawild", "Vfx_Spore", "Blooming");
    case EAstrawildEchoVfxType::Radiant:  return NSLOCTEXT("Astrawild", "Vfx_Radiant", "Radiant");
    default:                              return NSLOCTEXT("Astrawild", "Vfx_None", "Quiet");
    }
}
