#pragma once

#include "CoreMinimal.h"

// ENGINE-RUN-1 audit (P0): LogAstrawild used to be re-declared here AND in
// AstrawildLog.h — 41 translation units include both headers, and the double
// DECLARE_LOG_CATEGORY_EXTERN is a struct redefinition compile error. The
// single canonical declaration now lives in AstrawildLog.h (FCR-1-b intent);
// this include keeps every TU that pulled the category through the module
// header resolving with zero call-site changes.
#include "AstrawildLog.h"

class FAstrawildCoreModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
