#pragma once
#include "Commandlets/Commandlet.h"
#include "CreateVehicleUICommandlet.generated.h"

UCLASS()
class UCreateVehicleUICommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UCreateVehicleUICommandlet();
    virtual int32 Main(const FString& Params) override;
};
