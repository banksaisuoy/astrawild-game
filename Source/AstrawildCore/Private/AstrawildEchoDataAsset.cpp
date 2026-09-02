// ===========================================================================
// AstrawildEchoDataAsset.cpp
//
// Implementation ของ Data Asset สำหรับ Echo แต่ละสายพันธุ์
// ใช้สำหรับให้ antigravity/k อ่านแล้วสร้างโมเดล 3D, Materials, Blueprints
// ===========================================================================

#include "AstrawildEchoDataAsset.h"
#include "AstrawildLog.h"

void UAstrawildEchoDataAsset::GenerateAssetPaths()
{
    // สร้าง path อัตโนมัติจาก EchoId
    // ตัวอย่าง: Echo_Mosspaw -> /Game/Characters/Echo/Mosspaw/...
    
    FString CleanName = EchoId;
    CleanName.RemoveFromStart(TEXT("Echo_"));
    
    const FString BasePath = TEXT("/Game/Characters/Echo/");
    const FString SpeciesFolder = CleanName;
    
    SkeletalMeshPath = FString::Printf(TEXT("%s%s/SK_%s"), 
        BasePath, *SpeciesFolder, *CleanName);
    
    PrimaryMaterialPath = FString::Printf(TEXT("%s%s/MI_%s_Primary"), 
        BasePath, *SpeciesFolder, *CleanName);
    
    SecondaryMaterialPath = FString::Printf(TEXT("%s%s/MI_%s_Secondary"), 
        BasePath, *SpeciesFolder, *CleanName);
    
    BlueprintPath = FString::Printf(TEXT("%s%s/BP_Echo_%s"), 
        BasePath, *SpeciesFolder, *CleanName);
    
    AnimBlueprintPath = FString::Printf(TEXT("%s%s/ABP_%s"), 
        BasePath, *SpeciesFolder, *CleanName);
}

bool UAstrawildEchoDataAsset::ValidateData(TArray<FString>& OutErrors) const
{
    bool bValid = true;
    
    // Validate EchoId
    if (EchoId.IsEmpty())
    {
        OutErrors.Add(TEXT("EchoId is empty"));
        bValid = false;
    }
    
    // Validate DisplayName
    if (DisplayName.IsEmpty())
    {
        OutErrors.Add(FString::Printf(TEXT("DisplayName is empty for %s"), *EchoId));
        bValid = false;
    }
    
    // Validate stats are positive
    if (BaseHP <= 0.0f)
    {
        OutErrors.Add(FString::Printf(TEXT("BaseHP must be > 0 for %s"), *EchoId));
        bValid = false;
    }
    
    if (BaseATK <= 0.0f)
    {
        OutErrors.Add(FString::Printf(TEXT("BaseATK must be > 0 for %s"), *EchoId));
        bValid = false;
    }
    
    if (BaseDEF < 0.0f)
    {
        OutErrors.Add(FString::Printf(TEXT("BaseDEF must be >= 0 for %s"), *EchoId));
        bValid = false;
    }
    
    if (MovementSpeed <= 0.0f)
    {
        OutErrors.Add(FString::Printf(TEXT("MovementSpeed must be > 0 for %s"), *EchoId));
        bValid = false;
    }
    
    // Validate CaptureDifficulty range
    if (CaptureDifficulty < 0.0f || CaptureDifficulty > 1.0f)
    {
        OutErrors.Add(FString::Printf(TEXT("CaptureDifficulty must be 0.0-1.0 for %s"), *EchoId));
        bValid = false;
    }
    
    // Validate Weakness != Element
    if (Weakness == Element)
    {
        OutErrors.Add(FString::Printf(TEXT("Weakness cannot equal Element for %s"), *EchoId));
        bValid = false;
    }
    
    // Validate loot quantities
    if (LootQuantityA < 0 || LootQuantityB < 0)
    {
        OutErrors.Add(FString::Printf(TEXT("Loot quantities must be >= 0 for %s"), *EchoId));
        bValid = false;
    }
    
    return bValid;
}
