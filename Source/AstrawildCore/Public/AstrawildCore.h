#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAstrawild, Log, All);

class FAstrawildCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

// Include new components
#include "Components/AstrawildProceduralEchoMesh.h"
