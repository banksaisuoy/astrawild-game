#include "Components/AstrawildRangedCombatComponent.h"

#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildTechnologyComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Actor.h"

UAstrawildRangedCombatComponent::UAstrawildRangedCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildRangedCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    FireCooldownRemaining = FMath::Max(0.0f, FireCooldownRemaining - DeltaTime);
    if (bIsReloading)
    {
        ReloadRemaining = FMath::Max(0.0f, ReloadRemaining - DeltaTime);
        if (ReloadRemaining <= 0.0f)
        {
            if (const FAstrawildRangedWeaponRow* Row = FindWeaponRow(EquippedWeaponTag))
            {
                CurrentAmmo = FMath::Max(1, Row->MagazineSize);
            }
            bIsReloading = false;
            OnWeaponReloaded.Broadcast(EquippedWeaponTag);
        }
    }
}

bool UAstrawildRangedCombatComponent::EquipWeapon(const FGameplayTag& WeaponTag)
{
    const FAstrawildRangedWeaponRow* Row = FindWeaponRow(WeaponTag);
    if (!Row)
    {
        OnWeaponFailed.Broadcast(FText::FromString(TEXT("Weapon is not configured.")));
        return false;
    }
    if (!HasTechnology(Row->RequiredTechnologyTag))
    {
        OnWeaponFailed.Broadcast(FText::FromString(TEXT("Required technology is not unlocked.")));
        return false;
    }

    EquippedWeaponTag = WeaponTag;
    CurrentAmmo = FMath::Max(1, Row->MagazineSize);
    bIsReloading = false;
    FireCooldownRemaining = 0.0f;
    return true;
}

bool UAstrawildRangedCombatComponent::Fire()
{
    if (bIsReloading || FireCooldownRemaining > 0.0f)
    {
        return false;
    }

    const FAstrawildRangedWeaponRow* Row = FindWeaponRow(EquippedWeaponTag);
    ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
    if (!Row || !CharacterOwner || !GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }
    if (CurrentAmmo <= 0)
    {
        Reload();
        return false;
    }

    const FVector Start = CharacterOwner->GetPawnViewLocation();
    const FRotator ViewRotation = CharacterOwner->GetController() ? CharacterOwner->GetController()->GetControlRotation() : CharacterOwner->GetActorRotation();
    const FVector End = Start + ViewRotation.Vector() * FMath::Max(100.0f, Row->RangeCentimeters);
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstrawildRangedFire), true, GetOwner());
    const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
    if (bHit && Hit.GetActor())
    {
        if (UAstrawildCombatComponent* Combat = GetOwner()->FindComponentByClass<UAstrawildCombatComponent>())
        {
            Combat->ApplyDamageToTarget(Hit.GetActor(), Row->BaseDamage, Row->DamageElement, GetOwner());
        }
    }

    --CurrentAmmo;
    FireCooldownRemaining = FMath::Max(0.05f, Row->FireIntervalSeconds);
    OnWeaponFired.Broadcast(EquippedWeaponTag);
    if (CurrentAmmo <= 0)
    {
        Reload();
    }
    return true;
}

void UAstrawildRangedCombatComponent::Reload()
{
    if (bIsReloading)
    {
        return;
    }

    if (const FAstrawildRangedWeaponRow* Row = FindWeaponRow(EquippedWeaponTag))
    {
        bIsReloading = true;
        ReloadRemaining = FMath::Max(0.1f, Row->ReloadDurationSeconds);
    }
}

const FAstrawildRangedWeaponRow* UAstrawildRangedCombatComponent::FindWeaponRow(const FGameplayTag& WeaponTag) const
{
    if (!WeaponTable || !WeaponTag.IsValid())
    {
        return nullptr;
    }

    for (const TPair<FName, uint8*>& RowPair : WeaponTable->GetRowMap())
    {
        const FAstrawildRangedWeaponRow* Row = reinterpret_cast<const FAstrawildRangedWeaponRow*>(RowPair.Value);
        if (Row && Row->WeaponTag == WeaponTag)
        {
            return Row;
        }
    }
    return nullptr;
}

bool UAstrawildRangedCombatComponent::HasTechnology(const FGameplayTag& TechnologyTag) const
{
    if (!TechnologyTag.IsValid())
    {
        return true;
    }
    const UAstrawildTechnologyComponent* Technology = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildTechnologyComponent>() : nullptr;
    return Technology && Technology->IsTechnologyUnlocked(TechnologyTag);
}
