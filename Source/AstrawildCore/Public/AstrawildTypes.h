#pragma once

#include "CoreMinimal.h"
#include "AstrawildTypes.generated.h"

UENUM(BlueprintType)
enum class EAstrawildElementType : uint8
{
    None UMETA(DisplayName="None"),
    Light UMETA(DisplayName="Light"),
    Ash UMETA(DisplayName="Ash"),
    Flora UMETA(DisplayName="Flora"),
    Frost UMETA(DisplayName="Frost"),
    Pulse UMETA(DisplayName="Pulse")
};

UENUM(BlueprintType)
enum class EAstrawildEchoRole : uint8
{
    Explorer UMETA(DisplayName="Explorer"),
    Combat UMETA(DisplayName="Combat"),
    Base UMETA(DisplayName="Base"),
    Support UMETA(DisplayName="Support")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildStableId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Identity")
    FName Value = NAME_None;

    FAstrawildStableId() = default;
    explicit FAstrawildStableId(const FName InValue) : Value(InValue) {}

    bool IsValid() const { return !Value.IsNone(); }
    friend bool operator==(const FAstrawildStableId& A, const FAstrawildStableId& B)
    {
        return A.Value == B.Value;
    }
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Inventory")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Inventory", meta=(ClampMin="0"))
    int32 Quantity = 0;

    bool IsValid() const
    {
        return !ItemId.IsNone() && Quantity > 0;
    }
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float AttackPower = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Defense = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float MoveSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Stamina = 100.0f;

    /** 0 = very easy to capture, 1 = almost impossible. Scales the weaken bonus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CaptureResilience = 0.35f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoInstanceSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid InstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0.0"))
    float Trust = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 Experience = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bInRoster = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform LastKnownTransform;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRestPointSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid WorldObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bActive = false;
};
