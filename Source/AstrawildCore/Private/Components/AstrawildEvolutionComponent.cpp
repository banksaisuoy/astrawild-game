#include "Components/AstrawildEvolutionComponent.h"

#include "Components/AstrawildInventoryComponent.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAstrawildEvolutionComponent::UAstrawildEvolutionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAstrawildEvolutionComponent::CanEvolve(AActor* Trainer, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();
    const AAstrawildEchoBase* Echo = Cast<AAstrawildEchoBase>(GetOwner());
    const FAstrawildEvolutionRow* Row = FindEvolutionRow(Echo);
    if (!Echo || !Row)
    {
        OutFailureReason = FText::FromString(TEXT("No valid evolution path is configured for this Echo."));
        return false;
    }
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        OutFailureReason = FText::FromString(TEXT("Evolution must be requested by the server."));
        return false;
    }
    if (Echo->InstanceData.Level < Row->RequiredLevel)
    {
        OutFailureReason = FText::FromString(FString::Printf(TEXT("This Echo needs level %d."), Row->RequiredLevel));
        return false;
    }
    if (Row->RequiredItemTag.IsValid())
    {
        const UAstrawildInventoryComponent* Inventory = Trainer ? Trainer->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
        if (!Inventory || !Inventory->HasItem(Row->RequiredItemTag, Row->RequiredItemQuantity))
        {
            OutFailureReason = FText::FromString(TEXT("The required evolution catalyst is missing."));
            return false;
        }
    }
    if (Row->TargetSpeciesTag.IsValid() && Row->TargetSpeciesData.IsNull())
    {
        OutFailureReason = FText::FromString(TEXT("The target species DataAsset is not assigned."));
        return false;
    }
    return true;
}

bool UAstrawildEvolutionComponent::Evolve(AActor* Trainer)
{
    AAstrawildEchoBase* Echo = Cast<AAstrawildEchoBase>(GetOwner());
    FText FailureReason;
    if (!Echo || !CanEvolve(Trainer, FailureReason))
    {
        OnEvolutionFailed.Broadcast(Echo, FailureReason.IsEmpty() ? FText::FromString(TEXT("Evolution failed.")) : FailureReason);
        return false;
    }

    const FAstrawildEvolutionRow* Row = FindEvolutionRow(Echo);
    if (!Row)
    {
        return false;
    }
    if (Row->RequiredItemTag.IsValid() && Row->RequiredItemQuantity > 0)
    {
        UAstrawildInventoryComponent* Inventory = Trainer ? Trainer->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
        if (!Inventory || !Inventory->RemoveItem(Row->RequiredItemTag, Row->RequiredItemQuantity))
        {
            OnEvolutionFailed.Broadcast(Echo, FText::FromString(TEXT("The evolution catalyst could not be consumed.")));
            return false;
        }
    }

    const FAstrawildEchoInstance Before = Echo->ExportCapturedData();
    UAstrawildEchoDataAsset* TargetData = Row->TargetSpeciesData.LoadSynchronous();
    if (!TargetData)
    {
        OnEvolutionFailed.Broadcast(Echo, FText::FromString(TEXT("The target species DataAsset could not be loaded.")));
        if (Row->RequiredItemTag.IsValid() && Row->RequiredItemQuantity > 0)
        {
            if (UAstrawildInventoryComponent* Inventory = Trainer ? Trainer->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
            {
                Inventory->AddItem(Row->RequiredItemTag, Row->RequiredItemQuantity);
            }
        }
        return false;
    }

    Echo->InitializeFromSpeciesData(TargetData, FMath::Max(1, Before.Level));
    FAstrawildEchoInstance After = Echo->ExportCapturedData();
    After.UniqueEchoId = Before.UniqueEchoId;
    After.CustomNickname = Before.CustomNickname;
    After.Level = Before.Level;
    After.CurrentEXP = Before.CurrentEXP;
    After.CurrentHealth = FMath::Clamp(Before.CurrentHealth, 1.0f, After.MaxHealth);
    After.TrustScore = Before.TrustScore;
    After.OwnershipState = Before.OwnershipState;
    After.ParentAId = Before.ParentAId;
    After.ParentBId = Before.ParentBId;
    After.Generation = FMath::Max(1, Before.Generation + 1);
    After.MutationCount = Before.MutationCount;
    After.SpeciesTag = Row->TargetSpeciesTag.IsValid() ? Row->TargetSpeciesTag : TargetData->SpeciesTag;
    Echo->ImportCapturedData(After);

    OnEchoEvolved.Broadcast(Echo, Before.SpeciesTag, After.SpeciesTag);
    return true;
}

const FAstrawildEvolutionRow* UAstrawildEvolutionComponent::FindEvolutionRow(const AAstrawildEchoBase* Echo) const
{
    if (!EvolutionTable || !Echo)
    {
        return nullptr;
    }
    const FGameplayTag SourceTag = Echo->InstanceData.SpeciesTag;
    for (const TPair<FName, uint8*>& Pair : EvolutionTable->GetRowMap())
    {
        const FAstrawildEvolutionRow* Row = reinterpret_cast<const FAstrawildEvolutionRow*>(Pair.Value);
        if (Row && Row->SourceSpeciesTag == SourceTag)
        {
            return Row;
        }
    }
    return nullptr;
}
