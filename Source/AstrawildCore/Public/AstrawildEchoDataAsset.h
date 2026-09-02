#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoDataAsset.generated.h"

/**
 * Primary Data Asset สำหรับ Echo แต่ละสายพันธุ์
 * ใช้สำหรับให้ antigravity/k อ่านแล้วสร้าง:
 * - Skeletal Mesh / Geometry Collection
 * - Material Instances (Primary/Secondary colors)
 * - Blueprint Class
 * - Animation Blueprint
 */
UCLASS(BlueprintType)
class UAstrawildEchoDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // =========================================================================
    // ข้อมูลพื้นฐาน (Identity)
    // =========================================================================
    
    /** Echo ID แบบ unique (เช่น "Echo_Mosspaw") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FString EchoId;

    /** ชื่อแสดง (เช่น "Mosspaw") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText DisplayName;

    /** คำอธิบายสั้นๆ สำหรับ UI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Description;

    // =========================================================================
    // ลักษณะทางกายภาพ (Appearance & Silhouette)
    // =========================================================================

    /** ตระกูล Echo (Beast, Dragon, Construct, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    EAstrawildEchoFamily Family;

    /** โครงร่างหลัก (Quadruped, Biped, Avian, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    EAstrawildBodyPlan BodyPlan;

    /** ขนาด (Tiny, Small, Medium, Large, Huge) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    EAstrawildSizeClass SizeClass;

    /** สีหลัก RGB (0-1) สำหรับ Material Instance */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance|Colors")
    FLinearColor PrimaryTint;

    /** สีรอง RGB (0-1) สำหรับ Material Instance */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance|Colors")
    FLinearColor SecondaryTint;

    /** คำอธิบาย silhouette สำหรับทีม art (เช่น "four-legged body, low head, tail") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
    FString SilhouetteDescription;

    // =========================================================================
    // ธาตุและบทบาท (Element & Role)
    // =========================================================================

    /** ธาตุของ Echo */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Element")
    EAstrawildElementType Element;

    /** ธาตุที่อ่อนแอต่อ (จุดอ่อน) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Element")
    EAstrawildElementType Weakness;

    /** บทบาท (Base, Combat, Support, Explorer) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay")
    EAstrawildEchoRole Role;

    // =========================================================================
    // สถิติการต่อสู้ (Combat Stats)
    // =========================================================================

    /** HP พื้นฐาน */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Stats")
    float BaseHP;

    /** ATK พื้นฐาน */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Stats")
    float BaseATK;

    /** DEF พื้นฐาน */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Stats")
    float BaseDEF;

    /** ความเร็วในการเคลื่อนที่ */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Stats")
    float MovementSpeed;

    /** รัศมีการมองเห็น */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Stats")
    float SightRadius;

    // =========================================================================
    // ระบบจับและอุปนิสัย (Capture & Personality)
    // =========================================================================

    /** ค่าความยากในการจับ (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Capture")
    float CaptureDifficulty;

    /** เป็นศัตรูโดยธรรมชาติหรือไม่ */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Capture")
    uint32 bHostileByDefault : 1;

    /** บุคลิก (Curious, Loyal, Aggressive, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Personality")
    EAstrawildPersonality Personality;

    /** ช่วงเวลาออกหากิน (Diurnal, Nocturnal, Crepuscular) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Personality")
    EAstrawildActivityPattern ActivityPattern;

    // =========================================================================
    // อาหารและ Loot (Diet & Loot)
    // =========================================================================

    /** ไอเทมอาหารที่ชอบ 1 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Diet")
    TSoftObjectPtr<UDataAsset> PreferredFoodA;

    /** ไอเทมอาหารที่ชอบ 2 (ถ้ามี) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Diet")
    TSoftObjectPtr<UDataAsset> PreferredFoodB;

    /** ไอเทมที่ได้เมื่อจับสำเร็จ 1 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Loot")
    TSoftObjectPtr<UDataAsset> LootItemA;

    /** จำนวนไอเทม A */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Loot")
    int32 LootQuantityA;

    /** ไอเทมที่ได้เมื่อจับสำเร็จ 2 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Loot")
    TSoftObjectPtr<UDataAsset> LootItemB;

    /** จำนวนไอเทม B */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Loot")
    int32 LootQuantityB;

    // =========================================================================
    // งานในฐานทัพ (Work Affinities)
    // =========================================================================

    /** งานที่ถนัด 1 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Work")
    EAstrawildWorkType PrimaryWorkType;

    /** งานที่ถนัด 2 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Work")
    EAstrawildWorkType SecondaryWorkType;

    /** ค่าประสิทธิภาพงาน A (multiplier) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay|Work")
    float WorkAffinityA;

    // =========================================================================
    // ถิ่นที่อยู่ (Habitat)
    // =========================================================================

    /** โซนบ้านเกิด */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
    EAstrawildZone HomeZone;

    /** Zone ID (เช่น "Zone_DawnFields") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World")
    FName HomeZoneId;

    // =========================================================================
    // Assets ที่ต้องสร้าง (Art Pipeline)
    // =========================================================================

    /** 
     * Path สำหรับ Skeletal Mesh ที่ต้องสร้าง 
     * เช่น "/Game/Characters/Echo/Mosspaw/SK_Mosspaw"
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets|ToCreate")
    FString SkeletalMeshPath;

    /** 
     * Path สำหรับ Material Instance ที่ต้องสร้าง
     * เช่น "/Game/Characters/Echo/Mosspaw/MI_Mosspaw_Primary"
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets|ToCreate")
    FString PrimaryMaterialPath;

    /** Path สำหรับ Material Instance สีรอง */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets|ToCreate")
    FString SecondaryMaterialPath;

    /** 
     * Path สำหรับ Blueprint Class
     * เช่น "/Game/Characters/Echo/Mosspaw/BP_Echo_Mosspaw"
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets|ToCreate")
    FString BlueprintPath;

    /** 
     * Path สำหรับ Animation Blueprint
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Assets|ToCreate")
    FString AnimBlueprintPath;

    // =========================================================================
    // Functions
    // =========================================================================

    /** สร้าง Asset Paths อัตโนมัติจาก EchoId */
    void GenerateAssetPaths();

    /** Validate ข้อมูลทั้งหมด */
    bool ValidateData(TArray<FString>& OutErrors) const;

    /** Get Display Name เป็น FString */
    FString GetDisplayNameString() const { return DisplayName.ToString(); }

    /** Get Description เป็น FString */
    FString GetDescriptionString() const { return Description.ToString(); }
};
