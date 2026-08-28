#include "Characters/AstrawildMountedWeaponBase.h"

#include "Characters/AstrawildCharacter.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Components/AstrawildTechnologyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AstrawildDamageableInterface.h"
#include "Net/UnrealNetwork.h"

AAstrawildMountedWeaponBase::AAstrawildMountedWeaponBase()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    RootComponent = WeaponRoot;
}

void AAstrawildMountedWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority() && GearData)
    {
        CurrentAmmo = FMath::Max(0, GearData->MagazineSize);
        CurrentHeat = 0.0f;
        bOverheated = false;
    }
}

void AAstrawildMountedWeaponBase::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority())
    {
        return;
    }

    FireCooldownRemaining = FMath::Max(0.0f, FireCooldownRemaining - DeltaSeconds);
    OverheatLockoutRemaining = FMath::Max(0.0f, OverheatLockoutRemaining - DeltaSeconds);
    CoolHeat(DeltaSeconds);
    if (bOverheated && OverheatLockoutRemaining <= 0.0f && CurrentHeat <= GearData->MaxHeat * 0.25f)
    {
        bOverheated = false;
    }
}

void AAstrawildMountedWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, GearData);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, MountedOwner);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, CurrentAmmo);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, CurrentHeat);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, bOverheated);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, FireCooldownRemaining);
    DOREPLIFETIME(AAstrawildMountedWeaponBase, OverheatLockoutRemaining);
}

bool AAstrawildMountedWeaponBase::HasAuthorityForWeapon() const
{
    return HasAuthority() && IsValid(MountedOwner);
}

bool AAstrawildMountedWeaponBase::EquipToMount(AActor* MountActor)
{
    if (!HasAuthority())
    {
        Fail(FText::FromString(TEXT("Mounted weapon equip must be requested by the server.")));
        return false;
    }
    if (!IsValid(MountActor) || !GearData)
    {
        Fail(FText::FromString(TEXT("Mounted weapon requires a valid mount and gear data.")));
        return false;
    }
    FText FailureReason;
    if (!HasRequiredTechnology())
    {
        Fail(FText::FromString(TEXT("Required partner-gear technology is not unlocked.")));
        return false;
    }

    MountedOwner = MountActor;
    FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
    if (!AttachToComponent(MountActor->GetRootComponent(), AttachRules, GearData->MuzzleSocketName))
    {
        MountedOwner = nullptr;
        Fail(FText::FromString(TEXT("Mount does not expose the required weapon socket.")));
        return false;
    }
    CurrentAmmo = FMath::Clamp(CurrentAmmo, 0, GearData->MagazineSize);
    if (CurrentAmmo == 0)
    {
        CurrentAmmo = GearData->MagazineSize;
    }
    return true;
}

void AAstrawildMountedWeaponBase::UnequipFromMount()
{
    if (!HasAuthority())
    {
        return;
    }
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    MountedOwner = nullptr;
    FireCooldownRemaining = 0.0f;
    OverheatLockoutRemaining = 0.0f;
    CurrentHeat = 0.0f;
    bOverheated = false;
}

bool AAstrawildMountedWeaponBase::Fire()
{
    FText FailureReason;
    if (!CanFire(FailureReason))
    {
        Fail(FailureReason);
        return false;
    }

    const FVector MuzzleLocation = GetMuzzleLocation();
    const FRotator AimRotation = GetAimRotation();
    const FVector TraceEnd = MuzzleLocation + AimRotation.Vector() * GearData->MaxRange;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstrawildMountedWeapon), true, this);
    QueryParams.AddIgnoredActor(MountedOwner);
    QueryParams.AddIgnoredActor(GetOwner());

    AActor* HitActor = nullptr;
    if (GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLocation, TraceEnd, ECC_Visibility, QueryParams))
    {
        HitActor = Hit.GetActor();
        if (IsValid(HitActor) && HitActor->GetClass()->ImplementsInterface(UAstrawildDamageableInterface::StaticClass()))
        {
            FAstrawildDamageEvent DamageEvent;
            DamageEvent.BaseDamage = GearData->DamagePerShot;
            DamageEvent.DamageElement = GearData->DamageElement;
            DamageEvent.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Damage.PartnerGear")), false);
            DamageEvent.DamageCauser = this;
            DamageEvent.InstigatorActor = GetInstigator();
            DamageEvent.HitLocation = Hit.ImpactPoint;
            DamageEvent.HitDirection = AimRotation.Vector();
            IAstrawildDamageableInterface::Execute_TakeAstrawildDamage(HitActor, DamageEvent);
        }
    }

    CurrentAmmo = FMath::Max(0, CurrentAmmo - FMath::Max(1, GearData->AmmoPerShot));
    CurrentHeat = FMath::Clamp(CurrentHeat + FMath::Max(0.0f, GearData->HeatPerShot), 0.0f, GearData->MaxHeat);
    FireCooldownRemaining = FMath::Max(0.01f, GearData->FireIntervalSeconds);
    if (CurrentHeat >= GearData->MaxHeat)
    {
        bOverheated = true;
        OverheatLockoutRemaining = FMath::Max(0.0f, GearData->OverheatLockoutSeconds);
    }
    OnWeaponFired.Broadcast(HitActor);
    return true;
}

bool AAstrawildMountedWeaponBase::ActivatePartnerSkill()
{
    if (!HasAuthorityForWeapon() || !GearData)
    {
        Fail(FText::FromString(TEXT("Partner skill activation requires an equipped server-authoritative weapon.")));
        return false;
    }
    BP_OnPartnerSkillActivated();
    OnPartnerSkillActivated.Broadcast();
    return true;
}

float AAstrawildMountedWeaponBase::GetHeatNormalized() const
{
    return GearData && GearData->MaxHeat > 0.0f ? FMath::Clamp(CurrentHeat / GearData->MaxHeat, 0.0f, 1.0f) : 0.0f;
}

float AAstrawildMountedWeaponBase::GetAmmoNormalized() const
{
    return GearData && GearData->MagazineSize > 0 ? FMath::Clamp(static_cast<float>(CurrentAmmo) / GearData->MagazineSize, 0.0f, 1.0f) : 0.0f;
}

bool AAstrawildMountedWeaponBase::CanFire(FText& OutFailureReason) const
{
    if (!HasAuthorityForWeapon())
    {
        OutFailureReason = FText::FromString(TEXT("Mounted weapon fire is server-authoritative."));
        return false;
    }
    if (!GearData)
    {
        OutFailureReason = FText::FromString(TEXT("No partner gear data is assigned."));
        return false;
    }
    if (!HasRequiredTechnology())
    {
        OutFailureReason = FText::FromString(TEXT("Required partner-gear technology is not unlocked."));
        return false;
    }
    if (FireCooldownRemaining > 0.0f)
    {
        OutFailureReason = FText::FromString(TEXT("Mounted weapon is cooling between shots."));
        return false;
    }
    if (bOverheated || OverheatLockoutRemaining > 0.0f)
    {
        OutFailureReason = FText::FromString(TEXT("Mounted weapon is overheated."));
        return false;
    }
    if (CurrentAmmo < FMath::Max(1, GearData->AmmoPerShot))
    {
        OutFailureReason = FText::FromString(TEXT("Mounted weapon magazine is empty."));
        return false;
    }
    return true;
}

bool AAstrawildMountedWeaponBase::HasRequiredTechnology() const
{
    if (!GearData)
    {
        return false;
    }

    const TArray<AActor*> Candidates = {MountedOwner.Get(), GetOwner()};
    bool bTechnologySatisfied = !GearData->RequiredTechnologyTag.IsValid();
    if (!bTechnologySatisfied)
    {
        for (AActor* Candidate : Candidates)
        {
            if (const AAstrawildCharacter* Character = Cast<AAstrawildCharacter>(Candidate))
            {
                bTechnologySatisfied = Character->Technology && Character->Technology->IsTechnologyUnlocked(GearData->RequiredTechnologyTag);
                if (bTechnologySatisfied)
                {
                    break;
                }
            }
            if (Candidate && Candidate->GetOwner())
            {
                if (const AAstrawildCharacter* OwnerCharacter = Cast<AAstrawildCharacter>(Candidate->GetOwner()))
                {
                    bTechnologySatisfied = OwnerCharacter->Technology && OwnerCharacter->Technology->IsTechnologyUnlocked(GearData->RequiredTechnologyTag);
                    if (bTechnologySatisfied)
                    {
                        break;
                    }
                }
            }
        }
    }

    if (!bTechnologySatisfied)
    {
        return false;
    }
    if (!GearData->RequiredPartnerSkillTag.IsValid())
    {
        return true;
    }

    for (AActor* Candidate : Candidates)
    {
        if (const AAstrawildEchoBase* Echo = Cast<AAstrawildEchoBase>(Candidate))
        {
            if (Echo->GetPartnerSkillTag() == GearData->RequiredPartnerSkillTag)
            {
                return true;
            }
        }
        if (Candidate && Candidate->GetOwner())
        {
            if (const AAstrawildEchoBase* OwnerEcho = Cast<AAstrawildEchoBase>(Candidate->GetOwner()))
            {
                if (OwnerEcho->GetPartnerSkillTag() == GearData->RequiredPartnerSkillTag)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

FVector AAstrawildMountedWeaponBase::GetMuzzleLocation() const
{
    if (WeaponRoot && GearData && WeaponRoot->DoesSocketExist(GearData->MuzzleSocketName))
    {
        return WeaponRoot->GetSocketLocation(GearData->MuzzleSocketName);
    }
    return GetActorLocation();
}

FRotator AAstrawildMountedWeaponBase::GetAimRotation() const
{
    const FRotator BaseRotation = IsValid(MountedOwner) ? MountedOwner->GetActorRotation() : GetActorRotation();
    return FRotator(AimPitchDegrees + BaseRotation.Pitch, AimYawDegrees + BaseRotation.Yaw, BaseRotation.Roll);
}

void AAstrawildMountedWeaponBase::CoolHeat(const float DeltaSeconds)
{
    if (!GearData)
    {
        return;
    }
    CurrentHeat = FMath::Max(0.0f, CurrentHeat - FMath::Max(0.0f, GearData->CoolRatePerSecond) * FMath::Max(0.0f, DeltaSeconds));
}

void AAstrawildMountedWeaponBase::Fail(const FText& Reason)
{
    OnWeaponFailed.Broadcast(Reason);
}
