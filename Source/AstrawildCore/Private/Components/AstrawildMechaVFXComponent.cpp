#include "Components/AstrawildMechaVFXComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UAstrawildMechaVFXComponent::UAstrawildMechaVFXComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAstrawildMechaVFXComponent::RequestEffect(const FGameplayTag EffectTag, const FVector Start, const FVector End, const float Intensity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !EffectTag.IsValid())
    {
        return false;
    }
    const FAstrawildMechaVFXBindingRow* Binding = FindBinding(EffectTag);
    if (!Binding || !Binding->EffectTag.IsValid() || !Binding->NiagaraSystemPath.IsValid())
    {
        return false;
    }
    OnVFXRequested.Broadcast(EffectTag, Start, End, FMath::Max(0.0f, Intensity), Binding->NiagaraSystemPath);
    return true;
}

bool UAstrawildMechaVFXComponent::GetEffectBinding(const FGameplayTag EffectTag, FAstrawildMechaVFXBindingRow& OutBinding) const
{
    const FAstrawildMechaVFXBindingRow* Binding = FindBinding(EffectTag);
    if (!Binding)
    {
        OutBinding = FAstrawildMechaVFXBindingRow();
        return false;
    }
    OutBinding = *Binding;
    return true;
}

const FAstrawildMechaVFXBindingRow* UAstrawildMechaVFXComponent::FindBinding(const FGameplayTag EffectTag) const
{
    if (!VFXTable || !EffectTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildMechaVFXBindingRow*> Rows;
    VFXTable->GetAllRows<FAstrawildMechaVFXBindingRow>(TEXT("AstrawildMechaVFXLookup"), Rows);
    FAstrawildMechaVFXBindingRow** Found = Rows.FindByPredicate([EffectTag](const FAstrawildMechaVFXBindingRow* Row)
    {
        return Row && Row->EffectTag == EffectTag;
    });
    return Found ? *Found : nullptr;
}
