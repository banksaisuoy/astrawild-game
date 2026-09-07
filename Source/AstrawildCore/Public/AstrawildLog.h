#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Logging categories per directive §40 — meaningful subsystem-level logging.
// FCR-1-b fix (L-b11): LogAstrawild (general) is declared HERE — 11 cpp files
// used it via unity-build include bleed-through from AstrawildCore.h, which
// breaks under non-unity/IWYU builds. AstrawildCore.h keeps a redundant include
// of this header so both paths resolve.
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawild, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildSave, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildNetwork, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildBuilding, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildWorld, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildEconomy, Log, All);
