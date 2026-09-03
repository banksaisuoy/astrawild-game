#include "AstrawildAbilityLibrary.h"

#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "Misc/ScopeLock.h"

// ---------------------------------------------------------------------------
// Static table: built once per process, read-only afterwards. A critical
// section guards the lazy build so early world loads and automation tests
// can race safely.
// ---------------------------------------------------------------------------
namespace
{
    FCriticalSection GAbilityTableLock;
    TMap<FName, FAstrawildAbilityData>& GetAbilityTable()
    {
        static TMap<FName, FAstrawildAbilityData> Table;
        return Table;
    }

    bool GAbilityTableBuilt = false;

    FAstrawildAbilityData MakeAbility(FName Id, const FString& Name, const FString& Description,
        EAstrawildAbilityCategory Category, EAstrawildElementType Element, float Power,
        float Cooldown, float Range, int32 UnlockLevel, FName StatusId = NAME_None,
        float StatusSeconds = 0.0f, float StatusSpeed = 1.0f)
    {
        FAstrawildAbilityData Data;
        Data.AbilityId = Id;
        Data.DisplayName = FText::FromString(Name);
        Data.Description = FText::FromString(Description);
        Data.Category = Category;
        Data.Element = Element;
        Data.Power = Power;
        Data.CooldownSeconds = Cooldown;
        Data.Range = Range;
        Data.UnlockLevel = UnlockLevel;
        Data.StatusId = StatusId;
        Data.StatusSeconds = StatusSeconds;
        Data.StatusSpeedMultiplier = StatusSpeed;
        return Data;
    }
}

void UAstrawildAbilityLibrary::BuildDefaults()
{
    FScopeLock Lock(&GAbilityTableLock);
    TMap<FName, FAstrawildAbilityData>& Table = GetAbilityTable();
    if (GAbilityTableBuilt)
    {
        return;
    }

    // ------------------------------------------------------------------
    // Element signature sets (4 per element = 24). Each element reads as
    // itself in combat: Light dazzles, Ash grinds, Flora roots, Ember burns,
    // Frost chills, Pulse shocks.
    // ------------------------------------------------------------------

    // Light — precision and restoration.
    Table.Add(TEXT("Ability_Dawnflash"),
        MakeAbility(TEXT("Ability_Dawnflash"), TEXT("Dawnflash"), TEXT("A searing pulse of first light that blinds the target's next strike."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Light, 42.0f, 8.0f, 1200.0f, 5));
    Table.Add(TEXT("Ability_LumenBurst"),
        MakeAbility(TEXT("Ability_LumenBurst"), TEXT("Lumen Burst"), TEXT("Detonates a sphere of daylight for heavy radiant damage."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Light, 60.0f, 12.0f, 700.0f, 12));
    Table.Add(TEXT("Ability_PhotonVeil"),
        MakeAbility(TEXT("Ability_PhotonVeil"), TEXT("Photon Veil"), TEXT("Refracts light into a hard shell that halves incoming damage briefly."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::Light, 0.0f, 16.0f, 0.0f, 8, TEXT("Shell"), 6.0f));
    Table.Add(TEXT("Ability_RestoringGleam"),
        MakeAbility(TEXT("Ability_RestoringGleam"), TEXT("Restoring Gleam"), TEXT("Knits the wounds of every nearby ally with warm light."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Light, 35.0f, 14.0f, 800.0f, 6));

    // Ash — attrition and endurance.
    Table.Add(TEXT("Ability_GravelSpit"),
        MakeAbility(TEXT("Ability_GravelSpit"), TEXT("Gravel Spit"), TEXT("Fires hardened grit that scrapes armor off the target."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Ash, 38.0f, 7.0f, 1000.0f, 4));
    Table.Add(TEXT("Ability_DustScreen"),
        MakeAbility(TEXT("Ability_DustScreen"), TEXT("Dust Screen"), TEXT("A churning wall of ash that slows anything caught inside."),
            EAstrawildAbilityCategory::Debuff, EAstrawildElementType::Ash, 6.0f, 11.0f, 600.0f, 7, TEXT("Chill"), 5.0f, 0.65f));
    Table.Add(TEXT("Ability_StoneSkin"),
        MakeAbility(TEXT("Ability_StoneSkin"), TEXT("Stone Skin"), TEXT("Compresses its shell into basalt — damage taken is halved for a short time."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::Ash, 0.0f, 18.0f, 0.0f, 6, TEXT("Shell"), 7.0f));
    Table.Add(TEXT("Ability_SiftHeal"),
        MakeAbility(TEXT("Ability_SiftHeal"), TEXT("Sifting Motes"), TEXT("Filters nourishing grit from the air to mend its pack."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Ash, 26.0f, 15.0f, 700.0f, 9));

    // Flora — control and growth.
    Table.Add(TEXT("Ability_ThornLash"),
        MakeAbility(TEXT("Ability_ThornLash"), TEXT("Thorn Lash"), TEXT("A whipping vine that leaves bleeding sap in the wound."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Flora, 34.0f, 6.0f, 900.0f, 3,
            TEXT("Poison"), 4.0f));
    Table.Add(TEXT("Ability_RootSnare"),
        MakeAbility(TEXT("Ability_RootSnare"), TEXT("Root Snare"), TEXT("Roots erupt under the target, tangling its stride."),
            EAstrawildAbilityCategory::Debuff, EAstrawildElementType::Flora, 4.0f, 10.0f, 800.0f, 5, TEXT("Chill"), 6.0f, 0.5f));
    Table.Add(TEXT("Ability_BloomGuard"),
        MakeAbility(TEXT("Ability_BloomGuard"), TEXT("Bloom Guard"), TEXT("Petals fold into a layered barrier against the next blows."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::Flora, 0.0f, 17.0f, 0.0f, 7, TEXT("Shell"), 5.0f));
    Table.Add(TEXT("Ability_SapSurge"),
        MakeAbility(TEXT("Ability_SapSurge"), TEXT("Sap Surge"), TEXT("Draws energizing sap — the whole pack moves faster for a moment."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::Flora, 0.0f, 13.0f, 0.0f, 8, TEXT("Surge"), 5.0f, 1.5f));

    // Ember — burst damage and burns.
    Table.Add(TEXT("Ability_CinderBolt"),
        MakeAbility(TEXT("Ability_CinderBolt"), TEXT("Cinder Bolt"), TEXT("Hurls a crackling coal that sets the target alight."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Ember, 40.0f, 7.0f, 1100.0f, 4,
            TEXT("Burn"), 5.0f));
    Table.Add(TEXT("Ability_FlareNova"),
        MakeAbility(TEXT("Ability_FlareNova"), TEXT("Flare Nova"), TEXT("A point-blank ring of fire that scorches everything nearby."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Ember, 58.0f, 14.0f, 500.0f, 13,
            TEXT("Burn"), 3.0f));
    Table.Add(TEXT("Ability_HeatHaze"),
        MakeAbility(TEXT("Ability_HeatHaze"), TEXT("Heat Haze"), TEXT("Shimmers into a mirage — pursuers can barely hold speed."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::Ember, 0.0f, 12.0f, 0.0f, 6, TEXT("Surge"), 4.0f, 1.45f));
    Table.Add(TEXT("Ability_WarmBlood"),
        MakeAbility(TEXT("Ability_WarmBlood"), TEXT("Warm-Blooded Rally"), TEXT("Shares ember-warm vitality with wounded packmates."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Ember, 30.0f, 16.0f, 750.0f, 10));

    // Frost — denial and rescue.
    Table.Add(TEXT("Ability_ShardShot"),
        MakeAbility(TEXT("Ability_ShardShot"), TEXT("Shard Shot"), TEXT("A razor ice splinter that chills the target's muscles."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Frost, 36.0f, 7.0f, 1100.0f, 4,
            TEXT("Chill"), 4.0f, 0.8f));
    Table.Add(TEXT("Ability_GlacialWall"),
        MakeAbility(TEXT("Ability_GlacialWall"), TEXT("Glacial Wall"), TEXT("Freezes its own hide into armor plating."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::Frost, 0.0f, 15.0f, 0.0f, 5, TEXT("Shell"), 6.5f));
    Table.Add(TEXT("Ability_DeepFreeze"),
        MakeAbility(TEXT("Ability_DeepFreeze"), TEXT("Deep Freeze"), TEXT("Flash-freezes the target solid — it can barely move."),
            EAstrawildAbilityCategory::Debuff, EAstrawildElementType::Frost, 8.0f, 18.0f, 700.0f, 11, TEXT("Chill"), 5.0f, 0.45f));
    Table.Add(TEXT("Ability_Snowmelt"),
        MakeAbility(TEXT("Ability_Snowmelt"), TEXT("Snowmelt"), TEXT("Melts its frost into clean, restorative water for the pack."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Frost, 28.0f, 14.0f, 700.0f, 8));

    // Pulse — chaos and tempo.
    Table.Add(TEXT("Ability_ArcBolt"),
        MakeAbility(TEXT("Ability_ArcBolt"), TEXT("Arc Bolt"), TEXT("A snapping charge that leaves the target's nerves scrambled."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Pulse, 38.0f, 6.0f, 1200.0f, 4,
            TEXT("Shock"), 4.0f, 0.85f));
    Table.Add(TEXT("Ability_StormLatch"),
        MakeAbility(TEXT("Ability_StormLatch"), TEXT("Storm Latch"), TEXT("Magnetizes itself to the fight — sudden burst of closing speed."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::Pulse, 0.0f, 11.0f, 0.0f, 7, TEXT("Surge"), 5.0f, 1.6f));
    Table.Add(TEXT("Ability_Overload"),
        MakeAbility(TEXT("Ability_Overload"), TEXT("Overload"), TEXT("Dumps its stored charge — heavy damage and lingering static."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Pulse, 56.0f, 15.0f, 800.0f, 14,
            TEXT("Shock"), 5.0f, 0.8f));
    Table.Add(TEXT("Ability_Galvanize"),
        MakeAbility(TEXT("Ability_Galvanize"), TEXT("Galvanize"), TEXT("Jolts wounded packmates back into the fight."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Pulse, 32.0f, 13.0f, 800.0f, 9));

    // ------------------------------------------------------------------
    // Role kits (one signature per role = 4): what the species DOES.
    // ------------------------------------------------------------------
    Table.Add(TEXT("Ability_HuntersPounce"),
        MakeAbility(TEXT("Ability_HuntersPounce"), TEXT("Hunter's Pounce"), TEXT("Closes the gap in one bound and claws on landing."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::None, 0.0f, 9.0f, 0.0f, 3, TEXT("Surge"), 4.0f, 1.7f));
    Table.Add(TEXT("Ability_WardDrum"),
        MakeAbility(TEXT("Ability_WardDrum"), TEXT("Ward Drum"), TEXT("A steadying rhythm — the pack braces behind it."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::None, 0.0f, 16.0f, 0.0f, 5, TEXT("Shell"), 6.0f));
    Table.Add(TEXT("Ability_FieldTriage"),
        MakeAbility(TEXT("Ability_FieldTriage"), TEXT("Field Triage"), TEXT("Practical field medicine — the strongest pack heal."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::None, 40.0f, 12.0f, 900.0f, 4));
    Table.Add(TEXT("Ability_ScoutBurst"),
        MakeAbility(TEXT("Ability_ScoutBurst"), TEXT("Scout Burst"), TEXT("The longest sustained speed burst of any species."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::None, 0.0f, 8.0f, 0.0f, 2, TEXT("Surge"), 6.0f, 1.75f));

    // ------------------------------------------------------------------
    // Family signatures (one per family = 8): how the species LOOKS doing it.
    // ------------------------------------------------------------------
    Table.Add(TEXT("Ability_BeastFrenzy"),
        MakeAbility(TEXT("Ability_BeastFrenzy"), TEXT("Beast Frenzy"), TEXT("A feral multi-strike flurry."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::None, 30.0f, 10.0f, 400.0f, 6,
            TEXT("Rally"), 5.0f, 1.1f));
    Table.Add(TEXT("Ability_AvianDivebomb"),
        MakeAbility(TEXT("Ability_AvianDivebomb"), TEXT("Avian Divebomb"), TEXT("Climbs, then drops on the target with full weight."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::None, 48.0f, 12.0f, 1500.0f, 8));
    Table.Add(TEXT("Ability_FloralRegrowth"),
        MakeAbility(TEXT("Ability_FloralRegrowth"), TEXT("Floral Regrowth"), TEXT("Sheds and regrows damaged tissue — steady self mend."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::None, 36.0f, 12.0f, 0.0f, 5));
    Table.Add(TEXT("Ability_SwarmBite"),
        MakeAbility(TEXT("Ability_SwarmBite"), TEXT("Swarm Bite"), TEXT("A thousand small mouths — damage that keeps bleeding."),
            EAstrawildAbilityCategory::Debuff, EAstrawildElementType::None, 10.0f, 9.0f, 600.0f, 4,
            TEXT("Poison"), 7.0f));
    Table.Add(TEXT("Ability_ClockworkReinforce"),
        MakeAbility(TEXT("Ability_ClockworkReinforce"), TEXT("Clockwork Reinforce"), TEXT("Locks its plating — the most durable shell known."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::None, 0.0f, 20.0f, 0.0f, 7, TEXT("Shell"), 8.0f));
    Table.Add(TEXT("Ability_SpiritWard"),
        MakeAbility(TEXT("Ability_SpiritWard"), TEXT("Spirit Ward"), TEXT("A quiet ward that mends allies while it lasts."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::None, 20.0f, 18.0f, 1000.0f, 10,
            TEXT("Blessing"), 6.0f));
    Table.Add(TEXT("Ability_ElementalSurge"),
        MakeAbility(TEXT("Ability_ElementalSurge"), TEXT("Elemental Surge"), TEXT("Bleeds raw element — area damage around the caster."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::None, 44.0f, 11.0f, 550.0f, 9));
    Table.Add(TEXT("Ability_DragonBreath"),
        MakeAbility(TEXT("Ability_DragonBreath"), TEXT("Dragon Breath"), TEXT("The signature cone of devastation."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::None, 70.0f, 17.0f, 900.0f, 15,
            TEXT("Burn"), 6.0f));

    // ------------------------------------------------------------------
    // Authored-species signatures (8): the starter roster fights like itself.
    // ------------------------------------------------------------------
    Table.Add(TEXT("Ability_LumewispDawn"),
        MakeAbility(TEXT("Ability_LumewispDawn"), TEXT("First Dawn"), TEXT("Lumewisp's signature: a restorative flash that also cleanses fear."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Light, 45.0f, 11.0f, 900.0f, 1));
    Table.Add(TEXT("Ability_StonehideBulwark"),
        MakeAbility(TEXT("Ability_StonehideBulwark"), TEXT("Bulwark Stance"), TEXT("Stonehide plants itself and becomes a wall."),
            EAstrawildAbilityCategory::Defensive, EAstrawildElementType::Ash, 0.0f, 14.0f, 0.0f, 1, TEXT("Shell"), 8.0f));
    Table.Add(TEXT("Ability_VoltlingStatic"),
        MakeAbility(TEXT("Ability_VoltlingStatic"), TEXT("Static Jump"), TEXT("Voltling blinks with a crackle and recharges its charge."),
            EAstrawildAbilityCategory::Mobility, EAstrawildElementType::Pulse, 0.0f, 9.0f, 0.0f, 1, TEXT("Surge"), 4.0f, 1.65f));
    Table.Add(TEXT("Ability_DuskmothPowder"),
        MakeAbility(TEXT("Ability_DuskmothPowder"), TEXT("Dusk Powder"), TEXT("Duskmoth's soporific cloud — targets drift and slow."),
            EAstrawildAbilityCategory::Debuff, EAstrawildElementType::Flora, 5.0f, 10.0f, 700.0f, 1,
            TEXT("Chill"), 6.0f, 0.55f));
    Table.Add(TEXT("Ability_GloomfangTerror"),
        MakeAbility(TEXT("Ability_GloomfangTerror"), TEXT("Night Terror"), TEXT("Gloomfang strikes from the dark — heavy and frightening."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Ash, 50.0f, 13.0f, 600.0f, 1,
            TEXT("Fear"), 4.0f, 0.75f));
    Table.Add(TEXT("Ability_SpriglingCheer"),
        MakeAbility(TEXT("Ability_SpriglingCheer"), TEXT("Meadow Cheer"), TEXT("Sprigling's presence coaxes growth — a gentle pack mend."),
            EAstrawildAbilityCategory::Restore, EAstrawildElementType::Flora, 25.0f, 10.0f, 800.0f, 1));
    Table.Add(TEXT("Ability_VanguardSmite"),
        MakeAbility(TEXT("Ability_VanguardSmite"), TEXT("Vanguard Smite"), TEXT("The trained strike of the Vanguard line."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Light, 46.0f, 12.0f, 1000.0f, 1));
    Table.Add(TEXT("Ability_SovereignTide"),
        MakeAbility(TEXT("Ability_SovereignTide"), TEXT("Sovereign Tide"), TEXT("The Drowned Sovereign's parting gift: crushing pressure."),
            EAstrawildAbilityCategory::Offensive, EAstrawildElementType::Frost, 90.0f, 20.0f, 1200.0f, 20,
            TEXT("Chill"), 6.0f, 0.6f));

    GAbilityTableBuilt = true;
    UE_LOG(LogAstrawild, Log, TEXT("Ability library built: %d ability templates."), Table.Num());
}

const FAstrawildAbilityData* UAstrawildAbilityLibrary::FindAbility(FName AbilityId)
{
    BuildDefaults();
    FScopeLock Lock(&GAbilityTableLock);
    const TMap<FName, FAstrawildAbilityData>& Table = GetAbilityTable();
    return Table.Find(AbilityId);
}

bool UAstrawildAbilityLibrary::IsKnownAbility(FName AbilityId)
{
    return FindAbility(AbilityId) != nullptr;
}

int32 UAstrawildAbilityLibrary::GetAbilityCount()
{
    BuildDefaults();
    FScopeLock Lock(&GAbilityTableLock);
    return GetAbilityTable().Num();
}

TArray<FName> UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(const UAstrawildEchoDefinition* Definition)
{
    TArray<FName> Result;
    if (!Definition)
    {
        return Result;
    }

    // Authored list first (deduped, order preserved).
    for (const FName& Id : Definition->AbilityIds)
    {
        if (Id != NAME_None && !Result.Contains(Id))
        {
            Result.Add(Id);
        }
    }

    // Then the derived loadout for anything the author did not cover.
    const TArray<FName> Derived = ComputeDerivedAbilityIds(
        Definition->Element, Definition->Role, Definition->Family);
    for (const FName& Id : Derived)
    {
        if (!Result.Contains(Id))
        {
            Result.Add(Id);
        }
    }
    return Result;
}

TArray<FName> UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(EAstrawildElementType Element,
    EAstrawildEchoRole Role, EAstrawildEchoFamily Family)
{
    BuildDefaults();

    TArray<FName> Result;

    // Two element-flavored picks (explicit switch — enum order is not load-bearing).
    switch (Element)
    {
    case EAstrawildElementType::Light:
        Result.Add(TEXT("Ability_Dawnflash"));
        Result.Add(TEXT("Ability_PhotonVeil"));
        break;
    case EAstrawildElementType::Ash:
        Result.Add(TEXT("Ability_GravelSpit"));
        Result.Add(TEXT("Ability_DustScreen"));
        break;
    case EAstrawildElementType::Flora:
        Result.Add(TEXT("Ability_ThornLash"));
        Result.Add(TEXT("Ability_RootSnare"));
        break;
    case EAstrawildElementType::Ember:
        Result.Add(TEXT("Ability_CinderBolt"));
        Result.Add(TEXT("Ability_HeatHaze"));
        break;
    case EAstrawildElementType::Frost:
        Result.Add(TEXT("Ability_ShardShot"));
        Result.Add(TEXT("Ability_DeepFreeze"));
        break;
    case EAstrawildElementType::Pulse:
        Result.Add(TEXT("Ability_ArcBolt"));
        Result.Add(TEXT("Ability_StormLatch"));
        break;
    case EAstrawildElementType::None:
    default:
        Result.Add(TEXT("Ability_HuntersPounce"));
        Result.Add(TEXT("Ability_ScoutBurst"));
        break;
    }

    // One role signature.
    switch (Role)
    {
    case EAstrawildEchoRole::Combat:  Result.Add(TEXT("Ability_HuntersPounce")); break;
    case EAstrawildEchoRole::Base:    Result.Add(TEXT("Ability_WardDrum")); break;
    case EAstrawildEchoRole::Support: Result.Add(TEXT("Ability_FieldTriage")); break;
    case EAstrawildEchoRole::Explorer:
    default:                          Result.Add(TEXT("Ability_ScoutBurst")); break;
    }

    // One family signature (explicit switch — enum order is not load-bearing).
    switch (Family)
    {
    case EAstrawildEchoFamily::Beast:      Result.Add(TEXT("Ability_BeastFrenzy")); break;
    case EAstrawildEchoFamily::Dragon:     Result.Add(TEXT("Ability_DragonBreath")); break;
    case EAstrawildEchoFamily::Construct:  Result.Add(TEXT("Ability_ClockworkReinforce")); break;
    case EAstrawildEchoFamily::Spirit:     Result.Add(TEXT("Ability_SpiritWard")); break;
    case EAstrawildEchoFamily::Elemental:  Result.Add(TEXT("Ability_ElementalSurge")); break;
    case EAstrawildEchoFamily::Aquatic:    Result.Add(TEXT("Ability_ElementalSurge")); break;
    case EAstrawildEchoFamily::Insectoid:  Result.Add(TEXT("Ability_SwarmBite")); break;
    case EAstrawildEchoFamily::Flora:      Result.Add(TEXT("Ability_FloralRegrowth")); break;
    case EAstrawildEchoFamily::Avian:      Result.Add(TEXT("Ability_AvianDivebomb")); break;
    case EAstrawildEchoFamily::Ancient:    Result.Add(TEXT("Ability_SpiritWard")); break;
    default: break;
    }

    return Result;
}

FName UAstrawildAbilityLibrary::ChooseAbilityForCombat(const TArray<FName>& KnownAbilityIds,
    const TMap<FName, float>& CooldownsRemaining, int32 EchoLevel, float DistanceToTarget,
    bool bWantsHeal, bool bWantsShield)
{
    BuildDefaults();
    FScopeLock Lock(&GAbilityTableLock);
    const TMap<FName, FAstrawildAbilityData>& Table = GetAbilityTable();

    const FAstrawildAbilityData* BestRestore = nullptr;
    const FAstrawildAbilityData* BestDefensive = nullptr;
    const FAstrawildAbilityData* BestOffensive = nullptr;
    const FAstrawildAbilityData* BestDebuff = nullptr;
    const FAstrawildAbilityData* BestMobility = nullptr;

    for (const FName& Id : KnownAbilityIds)
    {
        const FAstrawildAbilityData* Data = Table.Find(Id);
        if (!Data)
        {
            continue;
        }
        if (Data->UnlockLevel > EchoLevel)
        {
            continue;
        }
        if (const float* Remaining = CooldownsRemaining.Find(Id))
        {
            if (*Remaining > 0.0f)
            {
                continue;
            }
        }
        if (Data->Category == EAstrawildAbilityCategory::Offensive ||
            Data->Category == EAstrawildAbilityCategory::Debuff)
        {
            if (DistanceToTarget > Data->Range)
            {
                continue; // Out of range — not usable this beat.
            }
        }

        switch (Data->Category)
        {
        case EAstrawildAbilityCategory::Restore:
            if (!BestRestore || Data->Power > BestRestore->Power) BestRestore = Data;
            break;
        case EAstrawildAbilityCategory::Defensive:
            if (!BestDefensive || Data->StatusSeconds > BestDefensive->StatusSeconds) BestDefensive = Data;
            break;
        case EAstrawildAbilityCategory::Offensive:
            if (!BestOffensive || Data->Power > BestOffensive->Power) BestOffensive = Data;
            break;
        case EAstrawildAbilityCategory::Debuff:
            if (!BestDebuff || Data->Power > BestDebuff->Power) BestDebuff = Data;
            break;
        case EAstrawildAbilityCategory::Mobility:
            if (!BestMobility || Data->StatusSpeedMultiplier > BestMobility->StatusSpeedMultiplier) BestMobility = Data;
            break;
        default: break;
        }
    }

    // Tactical priority (deterministic).
    if (bWantsHeal && BestRestore)
    {
        return BestRestore->AbilityId;
    }
    if (bWantsShield && BestDefensive)
    {
        return BestDefensive->AbilityId;
    }
    if (BestOffensive)
    {
        return BestOffensive->AbilityId;
    }
    if (BestDebuff)
    {
        return BestDebuff->AbilityId;
    }
    if (BestMobility && DistanceToTarget > 600.0f)
    {
        return BestMobility->AbilityId;
    }
    return NAME_None;
}

void UAstrawildAbilityLibrary::ValidateTable(TArray<FString>& OutProblems)
{
    BuildDefaults();
    FScopeLock Lock(&GAbilityTableLock);
    const TMap<FName, FAstrawildAbilityData>& Table = GetAbilityTable();

    TSet<FName> SeenIds;
    for (const TPair<FName, FAstrawildAbilityData>& Pair : Table)
    {
        const FAstrawildAbilityData& Data = Pair.Value;

        if (Pair.Key != Data.AbilityId)
        {
            OutProblems.Add(FString::Printf(TEXT("Ability key/id mismatch: %s"), *Pair.Key.ToString()));
        }
        if (SeenIds.Contains(Data.AbilityId))
        {
            OutProblems.Add(FString::Printf(TEXT("Duplicate ability id: %s"), *Data.AbilityId.ToString()));
        }
        SeenIds.Add(Data.AbilityId);

        if (Data.DisplayName.IsEmpty())
        {
            OutProblems.Add(FString::Printf(TEXT("Ability missing display name: %s"), *Data.AbilityId.ToString()));
        }
        if (Data.CooldownSeconds < 1.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Ability cooldown below floor: %s"), *Data.AbilityId.ToString()));
        }
        if (Data.UnlockLevel < 1)
        {
            OutProblems.Add(FString::Printf(TEXT("Ability unlock level below 1: %s"), *Data.AbilityId.ToString()));
        }
        if (Data.Category == EAstrawildAbilityCategory::Offensive && Data.Power <= 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Offensive ability has no power: %s"), *Data.AbilityId.ToString()));
        }
        if (Data.Category == EAstrawildAbilityCategory::Restore && Data.Power <= 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Restore ability has no power: %s"), *Data.AbilityId.ToString()));
        }
        if ((Data.Category == EAstrawildAbilityCategory::Defensive || Data.Category == EAstrawildAbilityCategory::Mobility) &&
            (Data.StatusId == NAME_None || Data.StatusSeconds <= 0.0f))
        {
            OutProblems.Add(FString::Printf(TEXT("Self-buff ability missing status payload: %s"), *Data.AbilityId.ToString()));
        }
        if (Data.Range < 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Ability negative range: %s"), *Data.AbilityId.ToString()));
        }
    }

    if (Table.Num() != 44)
    {
        OutProblems.Add(FString::Printf(TEXT("Ability table expected 44 templates, found %d"), Table.Num()));
    }
}
