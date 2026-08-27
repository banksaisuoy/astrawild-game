#include "AstrawildCore.h"

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAstrawild);

void FAstrawildCoreModule::StartupModule()
{
    UE_LOG(LogAstrawild, Log, TEXT("ASTRAWILD Core module started."));
}

void FAstrawildCoreModule::ShutdownModule()
{
    UE_LOG(LogAstrawild, Log, TEXT("ASTRAWILD Core module stopped."));
}

IMPLEMENT_MODULE(FAstrawildCoreModule, AstrawildCore);
