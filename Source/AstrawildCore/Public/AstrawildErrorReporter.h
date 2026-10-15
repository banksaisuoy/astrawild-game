#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildErrorReporter.generated.h"

/** Severity ladder for runtime error records. */
UENUM(BlueprintType)
enum class EAstrawildErrorSeverity : uint8
{
    Info UMETA(DisplayName="Info"),
    Warning UMETA(DisplayName="Warning"),
    Error UMETA(DisplayName="Error")
};

/** One structured diagnostic record (directive [3] Phase 2.3). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildErrorRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Diagnostics")
    EAstrawildErrorSeverity Severity = EAstrawildErrorSeverity::Info;

    /** Originating system, e.g. "DataValidator", "AssetFallback", "Save". */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Diagnostics")
    FName Category = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Diagnostics")
    FString Message;

    /** Seconds since process start (FPlatformTime based, stable for one session). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Diagnostics")
    double TimestampSeconds = 0.0;

    FAstrawildErrorRecord() = default;

    FAstrawildErrorRecord(EAstrawildErrorSeverity InSeverity, FName InCategory, const FString& InMessage, double InTimestamp)
        : Severity(InSeverity), Category(InCategory), Message(InMessage), TimestampSeconds(InTimestamp)
    {
    }
};

/**
 * SCP Phase 2 — persistent error/warning trail for Standalone builds
 * (directive [3] Phase 2.3).
 *
 * A module-level ring buffer collects structured records from any system
 * (validator, asset fallback, save migration, gameplay fault handlers).
 * The game-instance subsystem flushes the buffer to
 * Saved/Logs/AstrawildErrorReport.log on shutdown so a packaged game
 * without an editor still leaves a readable diagnostic trail.
 *
 * Thread-safe: report calls may arrive from worker threads.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildErrorReporterLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Queue an error record (oldest records are dropped beyond the cap). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Diagnostics")
    static void ReportError(FName Category, const FString& Message);

    /** Queue a warning record. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Diagnostics")
    static void ReportWarning(FName Category, const FString& Message);

    /** Queue an informational record. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Diagnostics")
    static void ReportInfo(FName Category, const FString& Message);

    /** Snapshot of the current records (oldest first). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Diagnostics")
    static TArray<FAstrawildErrorRecord> GetRecords();

    /** Number of records currently held. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Diagnostics")
    static int32 GetRecordCount();

    /** Errors + warnings currently held (HUD "diagnostics pending" hint). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Diagnostics")
    static int32 GetNonInfoCount();

    /** Clear all records (test isolation). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Diagnostics")
    static void Clear();

    /** Render the records as a plain-text report block (one line per record). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Diagnostics")
    static FString FormatReport(const FString& Header);

    /**
     * Write the formatted report to a file (creates parent folders).
     * Returns false when the write fails — never crashes on IO problems.
     */
    static bool WriteReportToFile(const FString& FilePath, const FString& Header);

    /** Default on-disk location: <ProjectSaved>/Logs/AstrawildErrorReport.log. */
    static FString GetDefaultReportPath();

    /** Ring buffer capacity (bounded memory in long sessions). */
    static constexpr int32 MaxRecords = 512;
};

/**
 * Game-instance owner: flushes the error trail to disk when the game shuts
 * down and exposes it to Blueprint (pause menu diagnostics screen hook).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildErrorReporterSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // UGameInstanceSubsystem interface.
    virtual void Deinitialize() override;

    /** True when at least one error/warning was reported this session. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Diagnostics")
    bool HasPendingDiagnostics() const;

    /** Last flush result (empty string when no write has happened yet). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Diagnostics")
    FString GetLastFlushPath() const { return LastFlushPath; }

private:
    FString LastFlushPath;
};
