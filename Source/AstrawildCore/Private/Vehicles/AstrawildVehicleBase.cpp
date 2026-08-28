#include "Vehicles/AstrawildVehicleBase.h"

AAstrawildVehicleBase::AAstrawildVehicleBase()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);
    VehicleComponent = CreateDefaultSubobject<UAstrawildVehicleComponent>(TEXT("VehicleComponent"));
}

UAstrawildVehicleComponent* AAstrawildVehicleBase::GetVehicleComponent() const
{
    return VehicleComponent;
}
