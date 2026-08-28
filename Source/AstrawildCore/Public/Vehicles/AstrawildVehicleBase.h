#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/AstrawildVehicleComponent.h"
#include "AstrawildVehicleBase.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildVehicleBase : public APawn
{
    GENERATED_BODY()

public:
    AAstrawildVehicleBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Vehicle")
    TObjectPtr<UAstrawildVehicleComponent> VehicleComponent;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle")
    UAstrawildVehicleComponent* GetVehicleComponent() const;
};
