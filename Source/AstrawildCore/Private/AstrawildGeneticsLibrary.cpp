#include "AstrawildGeneticsLibrary.h"

const TArray<FName>& UAstrawildGeneticsLibrary::GetTraitPool()
{
    static const TArray<FName> Pool =
    {
        TEXT("Trait_Swift"),       // +30% move speed
        TEXT("Trait_Artisan"),     // +50% work rate
        TEXT("Trait_Ferocious"),   // +20% attack power
        TEXT("Trait_Sturdy"),      // +20% max health
        TEXT("Trait_Lucky"),       // +10% capture chance
        TEXT("Trait_Calm"),        // flavor: saner decay resistance (informational)
        TEXT("Trait_SharpEye"),    // flavor: observation bonus (informational)
        TEXT("Trait_Hardy")        // flavor: hunger resistance (informational)
    };
    return Pool;
}

float UAstrawildGeneticsLibrary::GetTraitSpeedMultiplier(FName TraitId)
{
    return TraitId == TEXT("Trait_Swift") ? 1.3f : 1.0f;
}

float UAstrawildGeneticsLibrary::GetTraitWorkMultiplier(FName TraitId)
{
    return TraitId == TEXT("Trait_Artisan") ? 1.5f : 1.0f;
}

float UAstrawildGeneticsLibrary::GetTraitAttackMultiplier(FName TraitId)
{
    return TraitId == TEXT("Trait_Ferocious") ? 1.2f : 1.0f;
}

float UAstrawildGeneticsLibrary::GetTraitHealthMultiplier(FName TraitId)
{
    return TraitId == TEXT("Trait_Sturdy") ? 1.2f : 1.0f;
}

float UAstrawildGeneticsLibrary::GetTraitCaptureBonus(FName TraitId)
{
    return TraitId == TEXT("Trait_Lucky") ? 0.1f : 0.0f;
}

float UAstrawildGeneticsLibrary::ComputeTraitSpeedMultiplier(const TArray<FName>& Traits)
{
    float Multiplier = 1.0f;
    for (const FName& Trait : Traits)
    {
        Multiplier *= GetTraitSpeedMultiplier(Trait);
    }
    return Multiplier;
}

float UAstrawildGeneticsLibrary::ComputeTraitWorkMultiplier(const TArray<FName>& Traits)
{
    float Multiplier = 1.0f;
    for (const FName& Trait : Traits)
    {
        Multiplier *= GetTraitWorkMultiplier(Trait);
    }
    return Multiplier;
}

float UAstrawildGeneticsLibrary::ComputeTraitAttackMultiplier(const TArray<FName>& Traits)
{
    float Multiplier = 1.0f;
    for (const FName& Trait : Traits)
    {
        Multiplier *= GetTraitAttackMultiplier(Trait);
    }
    return Multiplier;
}

float UAstrawildGeneticsLibrary::ComputeTraitHealthMultiplier(const TArray<FName>& Traits)
{
    float Multiplier = 1.0f;
    for (const FName& Trait : Traits)
    {
        Multiplier *= GetTraitHealthMultiplier(Trait);
    }
    return Multiplier;
}

FAstrawildGeneticsProfile UAstrawildGeneticsLibrary::RollOffspring(const TArray<FName>& ParentATraits,
    const TArray<FName>& ParentBTraits, int32 Seed)
{
    FAstrawildGeneticsProfile Profile;

    FRandomStream Stream(static_cast<int32>(Seed));
    const TArray<FName>& WildPool = GetTraitPool();

    // Four slots: 70% inherit from a random parent, 30% mutate into the wild
    // pool (directive: random draw of 4 slots including inheritance).
    for (int32 Slot = 0; Slot < 4; ++Slot)
    {
        const float Roll = Stream.FRand();
        if (Roll < 0.70f)
        {
            const TArray<FName>& Source = (Stream.FRand() < 0.5f && ParentATraits.Num() > 0)
                ? ParentATraits
                : ParentBTraits;
            if (Source.Num() > 0)
            {
                Profile.Traits.Add(Source[Stream.RandRange(0, Source.Num() - 1)]);
            }
            else
            {
                Profile.Traits.Add(WildPool[Stream.RandRange(0, WildPool.Num() - 1)]);
            }
        }
        else
        {
            Profile.Traits.Add(WildPool[Stream.RandRange(0, WildPool.Num() - 1)]);
        }
    }

    // IVs: midpoint of the "parents" (derived from the seed so tests stay
    // deterministic) plus a 0-8 mutation swing, clamped to the classic 0-31.
    const float BaseA = static_cast<float>(Stream.RandRange(0, 31));
    const float BaseB = static_cast<float>(Stream.RandRange(0, 31));
    const float Mutation = static_cast<float>(Stream.RandRange(0, 8));
    const float IVBase = FMath::Clamp(((BaseA + BaseB) * 0.5f) + Mutation - 4.0f, 0.0f, 31.0f);

    Profile.IVs = FVector4(
        FMath::Clamp(IVBase + static_cast<float>(Stream.RandRange(-2, 2)), 0.0f, 31.0f),
        FMath::Clamp(IVBase + static_cast<float>(Stream.RandRange(-2, 2)), 0.0f, 31.0f),
        FMath::Clamp(IVBase + static_cast<float>(Stream.RandRange(-2, 2)), 0.0f, 31.0f),
        FMath::Clamp(IVBase + static_cast<float>(Stream.RandRange(-2, 2)), 0.0f, 31.0f));

    return Profile;
}

float UAstrawildGeneticsLibrary::ComputeIVStatMultiplier(float IV)
{
    // +1% per IV point: 0 -> 1.0x, 31 -> 1.31x (hidden growth bias).
    return 1.0f + FMath::Clamp(IV, 0.0f, 31.0f) / 100.0f;
}
