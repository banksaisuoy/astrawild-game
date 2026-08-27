// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/AstrawildEchoDataAsset.h"
#include "AstrawildLogChannels.h"

UAstrawildEchoDataAsset::UAstrawildEchoDataAsset()
	: ElementalAffinity(EAstrawildElement::Neutral)
	, Role(EAstrawildEchoRole::Combat)
	, BaseMaxHealth(250.0f)
	, BaseAttackPower(30.0f)
	, BaseDefensePower(20.0f)
	, BaseWalkSpeed(250.0f)
	, BaseRunSpeed(550.0f)
	, WorkEfficiencyMultiplier(1.0f)
	, CaptureDifficultyModifier(1.0f)
	, PlaceholderTint(FColor(180, 180, 180))
{
}

FAstrawildEchoInstance UAstrawildEchoDataAsset::CreateInstance(int32 InLevel, const FText& CustomName) const
{
	FAstrawildEchoInstance Instance;
	Instance.UniqueEchoId = FGuid::NewGuid();
	Instance.SpeciesTag = SpeciesTag;
	Instance.CustomNickname = CustomName.IsEmpty() ? SpeciesName : CustomName;
	Instance.Level = FMath::Max(1, InLevel);
	Instance.ElementalAffinities = ElementalAffinities;
	if (Instance.ElementalAffinities.Num() == 0)
	{
		Instance.ElementalAffinities.Add(ElementalAffinity);
	}
	Instance.Element = Instance.ElementalAffinities[0];
	Instance.Role = Role;
	Instance.OwnershipState = EAstrawildEchoOwnership::Wild;
	Instance.TrustScore = 50.0f;
	Instance.PassiveTraitTags = PassiveTraitTags;
	Instance.WorkSuitabilityTags = WorkSuitabilityTags;
	Instance.PartnerSkillTag = PartnerSkillTag;
	Instance.MountProfileId = MountProfileId;
	Instance.BreedingGroupId = BreedingGroupId;

	// Stat scaling per level (+8% HP, +5% Atk, +4% Def per level)
	const float LevelMultiplier = 1.0f + ((Instance.Level - 1) * 0.08f);
	Instance.MaxHealth = FMath::RoundToFloat(BaseMaxHealth * LevelMultiplier);
	Instance.CurrentHealth = Instance.MaxHealth;
	Instance.AttackPower = FMath::RoundToFloat(BaseAttackPower * (1.0f + ((Instance.Level - 1) * 0.05f)));
	Instance.DefensePower = FMath::RoundToFloat(BaseDefensePower * (1.0f + ((Instance.Level - 1) * 0.04f)));

	// Copy ability tags
	for (const FAstrawildEchoAbility& Ability : InnateAbilities)
	{
		Instance.EquippedAbilities.Add(Ability.AbilityTag);
	}

	// Pick random personality trait if pool exists
	if (DefaultPersonalityPool.Num() > 0)
	{
		const int32 RandIndex = FMath::RandRange(0, DefaultPersonalityPool.Num() - 1);
		Instance.PersonalityTraits.AddTag(DefaultPersonalityPool[RandIndex]);
	}

	UE_LOG(LogAstrawildEcho, Log, TEXT("Created Echo Instance: %s (Level %d, HP: %.0f, Role: %s)"),
		*Instance.CustomNickname.ToString(), Instance.Level, Instance.MaxHealth, *UEnum::GetValueAsString(Role));

	return Instance;
}