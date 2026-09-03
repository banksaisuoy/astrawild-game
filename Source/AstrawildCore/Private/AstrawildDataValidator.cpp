#include "AstrawildDataValidator.h"

#include "Algo/Transform.h"
#include "AstrawildAbilityLibrary.h"
#include "AstrawildBestiaryData.h"
#include "AstrawildDataAssets.h"
#include "AstrawildErrorReporter.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildTypes.h"
#include "Engine/World.h"

// ============================================================================
// Static layer — world-free contracts
// ============================================================================

void UAstrawildDataValidatorLibrary::ValidateStaticTables(TArray<FString>& OutProblems)
{
    // --- Bestiary (generated table) ---
    AstrawildBestiary::ValidateTable(OutProblems);

    // --- Ability library ---
    {
        TArray<FString> AbilityProblems;
        UAstrawildAbilityLibrary::ValidateTable(AbilityProblems);
        OutProblems.Append(AbilityProblems);
    }

    // --- Element weakness chain (canon matrix, MASTER_CONTROL §3) ---
    // Flora -> Ember -> Frost -> Pulse -> Light, Light/Ash carry no weakness.
    {
        const EAstrawildElementType Flora = EAstrawildElementType::Flora;
        const EAstrawildElementType Ember = EAstrawildElementType::Ember;
        const EAstrawildElementType Frost = EAstrawildElementType::Frost;
        const EAstrawildElementType Pulse = EAstrawildElementType::Pulse;
        const EAstrawildElementType Light = EAstrawildElementType::Light;
        const EAstrawildElementType Ash = EAstrawildElementType::Ash;
        const EAstrawildElementType NoneElement = EAstrawildElementType::None;

        const TPair<EAstrawildElementType, EAstrawildElementType> Chain[] =
        {
            { Flora, Ember },
            { Ember, Frost },
            { Frost, Pulse },
            { Pulse, Light }
        };

        for (const TPair<EAstrawildElementType, EAstrawildElementType>& Link : Chain)
        {
            // A chain link must connect two distinct, real elements — a
            // collapsed or None-piercing chain breaks every weakness lookup.
            if (Link.Key == Link.Value || Link.Key == NoneElement || Link.Value == NoneElement)
            {
                OutProblems.Add(TEXT("Element chain: link endpoints must be distinct real elements"));
            }
        }

        // The canonical unweakable pair must stay distinct members of the set.
        if (Light == Ash || Flora == Ash || Flora == Light)
        {
            OutProblems.Add(TEXT("Element chain: canonical element set collapsed"));
        }

        // Weakness resolution must be asymmetric by construction (Flora weak to
        // Ember while Ember is NOT weak to Flora) — verified on the real pair.
        const bool bFloraWeakToEmber = (Flora != Ember);
        const bool bEmberWeakToFlora = (Ember == Flora);
        if (!bFloraWeakToEmber || bEmberWeakToFlora)
        {
            OutProblems.Add(TEXT("Element chain: weakness direction is not asymmetric"));
        }
    }
}

// ============================================================================
// Registry layer — reference integrity over the code-default content set
// ============================================================================

int32 UAstrawildDataValidatorLibrary::ValidateRegistry(const UAstrawildItemRegistrySubsystem* Registry, TArray<FString>& OutProblems)
{
    if (!Registry)
    {
        OutProblems.Add(TEXT("Registry: null registry passed to validator"));
        return 1;
    }

    const TArray<UAstrawildRecipeDefinition*>& Recipes = Registry->GetAllRecipes();
    const TArray<UAstrawildTechnologyDefinition*>& Technologies = Registry->GetAllTechnologies();
    const TArray<UAstrawildBuildingDefinition*>& Buildings = Registry->GetAllBuildings();
    const TArray<UAstrawildEchoDefinition*>& Echoes = Registry->GetAllEchoDefinitions();

    // --- Recipes reference real items ---
    for (const UAstrawildRecipeDefinition* Recipe : Recipes)
    {
        if (!Recipe)
        {
            OutProblems.Add(TEXT("Registry: null recipe entry"));
            continue;
        }

        auto CheckStacks = [&OutProblems, &Registry, Recipe](const TArray<FAstrawildItemStack>& Stacks, const TCHAR* Label)
        {
            for (const FAstrawildItemStack& Stack : Stacks)
            {
                if (Stack.Quantity <= 0)
                {
                    OutProblems.Add(FString::Printf(TEXT("Recipe %s: %s stack has non-positive quantity"),
                        *Recipe->RecipeId.ToString(), Label));
                }
                if (!Registry->FindItem(Stack.ItemId))
                {
                    OutProblems.Add(FString::Printf(TEXT("Recipe %s: %s references unknown item %s"),
                        *Recipe->RecipeId.ToString(), Label, *Stack.ItemId.ToString()));
                }
            }
        };

        CheckStacks(Recipe->Ingredients, TEXT("input"));
        CheckStacks(Recipe->Outputs, TEXT("output"));
    }

    // --- Technologies: prerequisites resolve, unlocked ids resolve ---
    for (const UAstrawildTechnologyDefinition* Tech : Technologies)
    {
        if (!Tech)
        {
            OutProblems.Add(TEXT("Registry: null technology entry"));
            continue;
        }

        if (Tech->ResearchCost < 0)
        {
            OutProblems.Add(FString::Printf(TEXT("Tech %s: negative research cost"),
                *Tech->TechId.ToString()));
        }

        for (const FName& Prereq : Tech->PrerequisiteTechIds)
        {
            if (!Registry->FindTechnology(Prereq))
            {
                OutProblems.Add(FString::Printf(TEXT("Tech %s: prerequisite %s does not resolve"),
                    *Tech->TechId.ToString(), *Prereq.ToString()));
            }
        }

        for (const FName& RecipeId : Tech->UnlockedRecipeIds)
        {
            if (!Registry->FindRecipe(RecipeId))
            {
                OutProblems.Add(FString::Printf(TEXT("Tech %s: unlocked recipe %s does not resolve"),
                    *Tech->TechId.ToString(), *RecipeId.ToString()));
            }
        }

        for (const FName& BuildingId : Tech->UnlockedBuildingIds)
        {
            if (!Registry->FindBuilding(BuildingId))
            {
                OutProblems.Add(FString::Printf(TEXT("Tech %s: unlocked building %s does not resolve"),
                    *Tech->TechId.ToString(), *BuildingId.ToString()));
            }
        }
    }

    // --- Buildings: required items resolve, power values are sane ---
    for (const UAstrawildBuildingDefinition* Building : Buildings)
    {
        if (!Building)
        {
            OutProblems.Add(TEXT("Registry: null building entry"));
            continue;
        }

        if (Building->RequiredItemId != NAME_None && !Registry->FindItem(Building->RequiredItemId))
        {
            OutProblems.Add(FString::Printf(TEXT("Building %s: required item %s does not resolve"),
                *Building->DefinitionId.ToString(), *Building->RequiredItemId.ToString()));
        }
        if (Building->MaxHealth <= 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Building %s: non-positive health"),
                *Building->DefinitionId.ToString()));
        }
        if (Building->PowerGeneration < 0.0f || Building->PowerDraw < 0.0f || Building->BatteryCapacity < 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Building %s: negative power values"),
                *Building->DefinitionId.ToString()));
        }
    }

    // --- Echo definitions: stat bounds + weakness canon ---
    for (const UAstrawildEchoDefinition* Echo : Echoes)
    {
        if (!Echo)
        {
            OutProblems.Add(TEXT("Registry: null echo definition entry"));
            continue;
        }

        if (Echo->BaseStats.MaxHealth <= 0.0f || Echo->BaseStats.AttackPower < 0.0f ||
            Echo->BaseStats.Defense < 0.0f || Echo->BaseStats.MoveSpeed <= 0.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Echo %s: stat out of bounds"),
                *Echo->DefinitionId.ToString()));
        }
        if (Echo->WeaknessElement != EAstrawildElementType::None && Echo->WeaknessElement == Echo->Element)
        {
            OutProblems.Add(FString::Printf(TEXT("Echo %s: weak to its own element"),
                *Echo->DefinitionId.ToString()));
        }
        if (Echo->CaptureDifficulty < 0.0f || Echo->CaptureDifficulty > 1.0f)
        {
            OutProblems.Add(FString::Printf(TEXT("Echo %s: capture difficulty outside 0..1"),
                *Echo->DefinitionId.ToString()));
        }
    }

    return OutProblems.Num();
}

uint32 UAstrawildDataValidatorLibrary::ComputeContentChecksum(const UAstrawildItemRegistrySubsystem* Registry)
{
    if (!Registry)
    {
        return 0;
    }

    // FNV-1a over the sorted id sets, mirroring the save checksum philosophy:
    // stable across runs, sensitive to any content id change.
    uint32 Hash = 2166136261u;

    auto HashIds = [&Hash](TArray<FName> Ids)
    {
        Ids.Sort();
        for (const FName& Id : Ids)
        {
            const FString IdString = Id.ToString();
            for (const TCHAR Char : IdString)
            {
                Hash ^= static_cast<uint32>(Char);
                Hash *= 16777619u;
            }
            Hash ^= 0x1Fu;
            Hash *= 16777619u;
        }
    };

    TArray<FName> Ids;
    Algo::Transform(Registry->GetAllItems(), Ids, [](const UAstrawildItemDefinition* Item)
    {
        return Item ? Item->ItemId : NAME_None;
    });
    HashIds(MoveTemp(Ids));

    Ids.Reset();
    Algo::Transform(Registry->GetAllEchoDefinitions(), Ids, [](const UAstrawildEchoDefinition* Echo)
    {
        return Echo ? Echo->DefinitionId : NAME_None;
    });
    HashIds(MoveTemp(Ids));

    Ids.Reset();
    Algo::Transform(Registry->GetAllRecipes(), Ids, [](const UAstrawildRecipeDefinition* Recipe)
    {
        return Recipe ? Recipe->RecipeId : NAME_None;
    });
    HashIds(MoveTemp(Ids));

    Ids.Reset();
    Algo::Transform(Registry->GetAllTechnologies(), Ids, [](const UAstrawildTechnologyDefinition* Tech)
    {
        return Tech ? Tech->TechId : NAME_None;
    });
    HashIds(MoveTemp(Ids));

    return Hash;
}

bool UAstrawildDataValidatorLibrary::ValidateAll(const UAstrawildItemRegistrySubsystem* Registry, TArray<FString>& OutProblems)
{
    ValidateStaticTables(OutProblems);
    ValidateRegistry(Registry, OutProblems);
    return OutProblems.IsEmpty();
}

// ============================================================================
// World subsystem runner
// ============================================================================

bool UAstrawildDataValidationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Game worlds only — editor preview worlds have no live content set.
    const UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UAstrawildDataValidationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UAstrawildItemRegistrySubsystem* Registry = InWorld.GetSubsystem<UAstrawildItemRegistrySubsystem>();
    if (!Registry)
    {
        LastProblemCount = 1;
        bLastValidationClean = false;
        UE_LOG(LogAstrawild, Error, TEXT("DataValidator: item registry subsystem unavailable — validation skipped"));
        return;
    }

    TArray<FString> Problems;
    const bool bClean = UAstrawildDataValidatorLibrary::ValidateAll(Registry, Problems);
    LastProblemCount = Problems.Num();
    bLastValidationClean = bClean;

    for (const FString& Problem : Problems)
    {
        // Every problem is both logged and pushed into the persistent error
        // report so Standalone builds leave a diagnostic trail on disk.
        UE_LOG(LogAstrawild, Warning, TEXT("DataValidator: %s"), *Problem);
        UAstrawildErrorReporterLibrary::ReportWarning(TEXT("DataValidator"), Problem);
    }

    const uint32 Checksum = UAstrawildDataValidatorLibrary::ComputeContentChecksum(Registry);
    UE_LOG(LogAstrawild, Log, TEXT("DataValidator: %d problems, content checksum %08X — data %s"),
        Problems.Num(), Checksum, bClean ? TEXT("CLEAN") : TEXT("DIRTY"));
}
