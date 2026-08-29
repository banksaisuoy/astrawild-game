#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

// Logging categories per directive §40 — meaningful subsystem-level logging.
// LogAstrawild (general) stays declared in AstrawildCore.h for backward compatibility.

DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildCombat, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildSave, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildNetwork, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildBuilding, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildWorld, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAstrawildEconomy, Log, All);
