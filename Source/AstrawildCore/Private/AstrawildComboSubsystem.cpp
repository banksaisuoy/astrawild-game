#include "AstrawildComboSubsystem.h"

#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildTypes.h"
#include "Engine/World.h"

// ============================================================================
// Library — the authored reaction table
// ============================================================================

FAstrawildComboReaction UAstrawildComboLibrary::ResolveCombo(bool bPlayerEmpowered, EAstrawildElementType EchoElement)
{
    FAstrawildComboReaction Reaction;

    // Kinetic tier: the everyday combos — every element answers.
    if (!bPlayerEmpowered)
    {
        switch (EchoElement)
        {
        case EAstrawildElementType::Ember:
            Reaction.DisplayName = TEXT("Cinder Slam");
            Reaction.DamageMultiplier = 2.0f;
            Reaction.StatusId = TEXT("Status.Burning");
            break;
        case EAstrawildElementType::Frost:
            Reaction.DisplayName = TEXT("Shatterpoint");
            Reaction.DamageMultiplier = 2.2f;
            Reaction.StatusId = TEXT("Status.Chilled");
            break;
        case EAstrawildElementType::Pulse:
            Reaction.DisplayName = TEXT("Arc Cascade");
            Reaction.DamageMultiplier = 2.0f;
            Reaction.StatusId = TEXT("Status.Shocked");
            break;
        case EAstrawildElementType::Flora:
            Reaction.DisplayName = TEXT("Spore Burst");
            Reaction.DamageMultiplier = 1.8f;
            Reaction.StatusId = TEXT("Status.Poisoned");
            break;
        case EAstrawildElementType::Light:
            Reaction.DisplayName = TEXT("Radiant Lance");
            Reaction.DamageMultiplier = 2.4f;
            break;
        case EAstrawildElementType::Ash:
            Reaction.DisplayName = TEXT("Grave Echo");
            Reaction.DamageMultiplier = 2.0f;
            Reaction.StatusId = TEXT("Status.Chilled");
            break;
        default:
            break;
        }
        return Reaction;
    }

    // Empowered tier (Power Strike mark): the showcase reactions.
    switch (EchoElement)
    {
    case EAstrawildElementType::Ember:
        // The directive's signature: Fire slash + water jet -> Steam Explosion
        // x2.5 + hitstop. Frost-jet reads as the steam carrier here.
        Reaction.DisplayName = TEXT("Steam Explosion");
        Reaction.DamageMultiplier = SteamExplosionMultiplier;
        Reaction.StatusId = TEXT("Status.Hitstop");
        break;
    case EAstrawildElementType::Frost:
        Reaction.DisplayName = TEXT("Absolute Zero");
        Reaction.DamageMultiplier = SteamExplosionMultiplier;
        Reaction.StatusId = TEXT("Status.Shocked");
        break;
    case EAstrawildElementType::Pulse:
        Reaction.DisplayName = TEXT("Overload Storm");
        Reaction.DamageMultiplier = SteamExplosionMultiplier;
        Reaction.StatusId = TEXT("Status.Shocked");
        break;
    case EAstrawildElementType::Flora:
        Reaction.DisplayName = TEXT("Bloom Nova");
        Reaction.DamageMultiplier = 2.3f;
        Reaction.StatusId = TEXT("Status.Poisoned");
        break;
    case EAstrawildElementType::Light:
        Reaction.DisplayName = TEXT("Dawnbreaker");
        Reaction.DamageMultiplier = 3.0f;
        break;
    case EAstrawildElementType::Ash:
        Reaction.DisplayName = TEXT("Eclipse Verdict");
        Reaction.DamageMultiplier = 2.5f;
        Reaction.StatusId = TEXT("Status.Hitstop");
        break;
    default:
        break;
    }

    return Reaction;
}

int32 UAstrawildComboLibrary::GetReactionCount()
{
    // Both player tiers x the six reactive elements, every one authored.
    int32 Count = 0;
    const EAstrawildElementType Elements[6] =
    {
        EAstrawildElementType::Ember, EAstrawildElementType::Frost, EAstrawildElementType::Pulse,
        EAstrawildElementType::Flora, EAstrawildElementType::Light, EAstrawildElementType::Ash
    };
    for (const bool bEmpowered : { false, true })
    {
        for (const EAstrawildElementType Element : Elements)
        {
            if (UAstrawildComboLibrary::ResolveCombo(bEmpowered, Element).IsValid())
            {
                ++Count;
            }
        }
    }
    return Count;
}

// ============================================================================
// World subsystem — mark tracking
// ============================================================================

bool UAstrawildComboSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UAstrawildComboSubsystem::ExpireStaleMarks()
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double NowSeconds = World->GetTimeSeconds();
    Marks.RemoveAll([NowSeconds](const FComboMark& Mark)
    {
        return !Mark.Target.IsValid() ||
            (NowSeconds - Mark.MarkTimeSeconds) > UAstrawildComboLibrary::ComboWindowSeconds;
    });
}

void UAstrawildComboSubsystem::NotifyPlayerMeleeHit(AActor* Target, bool bEmpowered)
{
    if (!IsValid(Target))
    {
        return;
    }

    ExpireStaleMarks();

    // Refresh/insert the mark for this target (empowered marks overwrite —
    // the strongest tier the player landed is the one that counts).
    bool bFound = false;
    for (FComboMark& Mark : Marks)
    {
        if (Mark.Target.Get() == Target)
        {
            Mark.MarkTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            Mark.bEmpowered = Mark.bEmpowered || bEmpowered;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        FComboMark Mark;
        Mark.Target = Target;
        Mark.MarkTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        Mark.bEmpowered = bEmpowered;
        Marks.Add(Mark);
    }
}

FAstrawildComboReaction UAstrawildComboSubsystem::TryResolveEchoAbilityCombo(AActor* Target, const AAstrawildEchoCharacter* StrikingEcho)
{
    FAstrawildComboReaction Empty;
    if (!IsValid(Target) || !IsValid(StrikingEcho) || !StrikingEcho->bCaptured)
    {
        return Empty;
    }

    ExpireStaleMarks();

    for (int32 i = 0; i < Marks.Num(); ++i)
    {
        if (Marks[i].Target.Get() != Target)
        {
            continue;
        }

        const FAstrawildComboReaction Reaction = UAstrawildComboLibrary::ResolveCombo(
            Marks[i].bEmpowered, StrikingEcho->EchoDefinition ? StrikingEcho->EchoDefinition->Element : EAstrawildElementType::None);

        // Consume the mark — a reaction never double-dips on one window.
        Marks.RemoveAt(i);

        if (Reaction.IsValid())
        {
            LastComboName = Reaction.DisplayName;
            UE_LOG(LogAstrawildCombat, Log, TEXT("Combo: %s resolved (%.1fx)%s"),
                *Reaction.DisplayName, Reaction.DamageMultiplier,
                Reaction.StatusId.IsNone() ? TEXT("") : TEXT(" + status"));
        }
        return Reaction;
    }

    return Empty;
}
