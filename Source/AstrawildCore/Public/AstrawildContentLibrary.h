#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildContentLibrary.generated.h"

class UAstrawildItemRegistrySubsystem;

/**
 * Code-defined default content (vertical-slice data backbone).
 *
 * Every definition created here is tagged CODE_DEFAULT and is replaceable: once real
 * .uasset definitions exist under Content/ASTRAWILD/Data they register into the same
 * registry with the same ids and take precedence (Docs/ASTRAWILD_ASSET_MANIFEST.md).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildContentLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Registers the full default content set into the registry (idempotent per world). */
    static void BuildDefaults(UAstrawildItemRegistrySubsystem* Registry);

private:
    static void BuildItems(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildRecipes(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildEchoes(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildBuildings(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildTechnologies(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildQuests(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildLootTables(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildNPCs(UAstrawildItemRegistrySubsystem* Registry);
};
