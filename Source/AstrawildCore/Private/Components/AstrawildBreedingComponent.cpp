#include "Components/AstrawildBreedingComponent.h"

#include "GameFramework/Actor.h"

UAstrawildBreedingComponent::UAstrawildBreedingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildBreedingComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        AdvanceIncubation(DeltaTime);
    }
}

bool UAstrawildBreedingComponent::CanBreed(const FAstrawildCapturedEchoData& ParentA, const FAstrawildCapturedEchoData& ParentB, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();

    if (!ParentA.UniqueEchoId.IsValid() || !ParentB.UniqueEchoId.IsValid() || ParentA.UniqueEchoId == ParentB.UniqueEchoId)
    {
        OutFailureReason = FText::FromString(TEXT("Two distinct Echo parents are required."));
        return false;
    }

    if (ParentA.BreedingGroupId.IsNone() || ParentB.BreedingGroupId.IsNone() || ParentA.BreedingGroupId != ParentB.BreedingGroupId)
    {
        OutFailureReason = FText::FromString(TEXT("These Echoes do not share a compatible breeding group."));
        return false;
    }

    if (ParentA.OwnershipState == EAstrawildEchoOwnership::Wild || ParentB.OwnershipState == EAstrawildEchoOwnership::Wild)
    {
        OutFailureReason = FText::FromString(TEXT("Wild Echoes must be captured before breeding."));
        return false;
    }

    if (ParentA.CurrentHealth <= 0.0f || ParentB.CurrentHealth <= 0.0f)
    {
        OutFailureReason = FText::FromString(TEXT("A defeated Echo cannot be used for breeding."));
        return false;
    }

    return true;
}

bool UAstrawildBreedingComponent::TryBreed(const FAstrawildCapturedEchoData& ParentA, const FAstrawildCapturedEchoData& ParentB, const FGameplayTag& OffspringSpeciesTag, FAstrawildEchoEggData& OutEgg)
{
    FText FailureReason;
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        FailureReason = FText::FromString(TEXT("Breeding must be performed by the server."));
        OnBreedingFailed.Broadcast(FailureReason);
        return false;
    }
    if (!OffspringSpeciesTag.IsValid() || !CanBreed(ParentA, ParentB, FailureReason))
    {
        if (FailureReason.IsEmpty())
        {
            FailureReason = FText::FromString(TEXT("A valid offspring species tag is required."));
        }
        OnBreedingFailed.Broadcast(FailureReason);
        return false;
    }

    OutEgg = FAstrawildEchoEggData();
    OutEgg.BreedingGroupId = ParentA.BreedingGroupId;
    OutEgg.OffspringSpeciesTag = OffspringSpeciesTag;
    OutEgg.ParentAId = ParentA.UniqueEchoId;
    OutEgg.ParentBId = ParentB.UniqueEchoId;
    OutEgg.Generation = FMath::Max(ParentA.Generation, ParentB.Generation) + 1;
    OutEgg.IncubationDurationSeconds = FMath::Max(1.0f, DefaultIncubationDurationSeconds);

    for (const EAstrawildElement Element : ParentA.ElementalAffinities)
    {
        AddUniqueAffinity(OutEgg.InheritedElementalAffinities, Element);
    }
    for (const EAstrawildElement Element : ParentB.ElementalAffinities)
    {
        AddUniqueAffinity(OutEgg.InheritedElementalAffinities, Element);
    }
    if (OutEgg.InheritedElementalAffinities.Num() == 0)
    {
        AddUniqueAffinity(OutEgg.InheritedElementalAffinities, ParentA.Element);
        AddUniqueAffinity(OutEgg.InheritedElementalAffinities, ParentB.Element);
    }

    AddUniqueTraits(OutEgg.InheritedPassiveTraits, ParentA.PassiveTraitTags);
    AddUniqueTraits(OutEgg.InheritedPassiveTraits, ParentB.PassiveTraitTags);
    NormalizeEgg(OutEgg);
    IncubatingEggs.Add(OutEgg);
    OnEggCreated.Broadcast(OutEgg);
    return true;
}

bool UAstrawildBreedingComponent::TryBreedFusion(const FAstrawildCapturedEchoData& ParentA, const FAstrawildCapturedEchoData& ParentB, FAstrawildEchoEggData& OutEgg)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        const FText FailureReason = FText::FromString(TEXT("Fusion breeding must be performed by the server."));
        OnBreedingFailed.Broadcast(FailureReason);
        return false;
    }
    if (!FusionTable)
    {
        const FText FailureReason = FText::FromString(TEXT("No fusion DataTable is assigned."));
        OnBreedingFailed.Broadcast(FailureReason);
        return false;
    }

    const FAstrawildBreedingFusionRow* Fusion = FindFusionRow(ParentA.SpeciesTag, ParentB.SpeciesTag);
    if (!Fusion)
    {
        const FText FailureReason = FText::FromString(TEXT("These parent species do not have a registered fusion result."));
        OnBreedingFailed.Broadcast(FailureReason);
        return false;
    }
    if (!TryBreed(ParentA, ParentB, Fusion->OffspringSpeciesTag, OutEgg))
    {
        return false;
    }

    OutEgg.InheritedElementalAffinities.Reset();
    for (const EAstrawildElement Element : Fusion->OffspringElementalAffinities)
    {
        AddUniqueAffinity(OutEgg.InheritedElementalAffinities, Element);
    }
    for (const FGameplayTag& Trait : Fusion->GuaranteedInheritedTraitTags)
    {
        if (Trait.IsValid())
        {
            OutEgg.InheritedPassiveTraits.AddTag(Trait);
        }
    }
    NormalizeEgg(OutEgg);
    if (FAstrawildEchoEggData* StoredEgg = IncubatingEggs.FindByPredicate([&OutEgg](const FAstrawildEchoEggData& Egg)
    {
        return Egg.EggId == OutEgg.EggId;
    }))
    {
        *StoredEgg = OutEgg;
    }
    return true;
}

void UAstrawildBreedingComponent::AdvanceIncubation(const float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    for (int32 Index = IncubatingEggs.Num() - 1; Index >= 0; --Index)
    {
        FAstrawildEchoEggData& Egg = IncubatingEggs[Index];
        const float Duration = FMath::Max(1.0f, Egg.IncubationDurationSeconds);
        Egg.IncubationProgress = FMath::Clamp(Egg.IncubationProgress + (DeltaSeconds / Duration), 0.0f, 1.0f);
        if (Egg.IncubationProgress >= 1.0f)
        {
            const FAstrawildEchoEggData HatchedEgg = Egg;
            IncubatingEggs.RemoveAt(Index);
            OnEggHatched.Broadcast(HatchedEgg);
        }
    }
}

void UAstrawildBreedingComponent::LoadIncubatingEggs(const TArray<FAstrawildEchoEggData>& InEggs)
{
    IncubatingEggs = InEggs;
    for (FAstrawildEchoEggData& Egg : IncubatingEggs)
    {
        NormalizeEgg(Egg);
    }
}

const FAstrawildBreedingFusionRow* UAstrawildBreedingComponent::FindFusionRow(const FGameplayTag& ParentSpeciesA, const FGameplayTag& ParentSpeciesB) const
{
    if (!FusionTable || !ParentSpeciesA.IsValid() || !ParentSpeciesB.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildBreedingFusionRow*> Rows;
    FusionTable->GetAllRows<FAstrawildBreedingFusionRow>(TEXT("AstrawildFusionLookup"), Rows);
    for (const FAstrawildBreedingFusionRow* Row : Rows)
    {
        if (!Row)
        {
            continue;
        }
        const bool bDirectMatch = Row->ParentSpeciesA == ParentSpeciesA && Row->ParentSpeciesB == ParentSpeciesB;
        const bool bReverseMatch = Row->ParentSpeciesA == ParentSpeciesB && Row->ParentSpeciesB == ParentSpeciesA;
        if (bDirectMatch || bReverseMatch)
        {
            return Row;
        }
    }
    return nullptr;
}

void UAstrawildBreedingComponent::AddUniqueAffinity(TArray<EAstrawildElement>& Affinities, const EAstrawildElement Element)
{
    if (Element != EAstrawildElement::Neutral && !Affinities.Contains(Element))
    {
        Affinities.Add(Element);
    }
}

void UAstrawildBreedingComponent::AddUniqueTraits(FGameplayTagContainer& Traits, const FGameplayTagContainer& Source)
{
    for (const FGameplayTag& Tag : Source.GetGameplayTagArray())
    {
        if (Tag.IsValid())
        {
            Traits.AddTag(Tag);
        }
    }
}

void UAstrawildBreedingComponent::NormalizeEgg(FAstrawildEchoEggData& Egg)
{
    Egg.IncubationDurationSeconds = FMath::Max(1.0f, Egg.IncubationDurationSeconds);
    Egg.IncubationProgress = FMath::Clamp(Egg.IncubationProgress, 0.0f, 1.0f);
    Egg.Generation = FMath::Max(1, Egg.Generation);
    if (!Egg.EggId.IsValid())
    {
        Egg.EggId = FGuid::NewGuid();
    }
}
