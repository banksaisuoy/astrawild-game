#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildPlayerCharacter.generated.h"

class AAstrawildDamageTarget;
class AAstrawildEchoCharacter;
class UAstrawildCaptureComponent;
class UAstrawildCraftingComponent;
class UAstrawildInventoryComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAstrawildPlayerCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Visual")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildCraftingComponent> CraftingComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildCaptureComponent> CaptureComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> InteractAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> AttackAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="50.0"))
    float AttackDistance = 280.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="0.0"))
    float AttackCooldownSeconds = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Prototype")
    bool bGivePrototypeStarterItems = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Prototype")
    TArray<FAstrawildItemStack> StarterItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Movement", meta=(ClampMin="0.0"))
    float WalkSpeed = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Movement", meta=(ClampMin="0.0"))
    float SprintSpeed = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Interaction", meta=(ClampMin="50.0"))
    float InteractionDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Interaction")
    bool bDrawInteractionDebug = false;

protected:
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    void Interact(const FInputActionValue& Value);
    void Attack(const FInputActionValue& Value);

private:
    void SetMovementSpeed(float NewSpeed);
    AActor* FindInteractableActor() const;
    double LastAttackTimeSeconds = -BIG_NUMBER;
};
