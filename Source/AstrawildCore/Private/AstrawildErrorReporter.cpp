#include "AstrawildErrorReporter.h"

#include "AstrawildLog.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"

namespace
{
    /** Module-level ring buffer + guard — shared by every library call site. */
    FCriticalSection ReportCriticalSection;
    TArray<FAstrawildErrorRecord> ReportRecords;

    double NowSeconds()
    {
        return FPlatformTime::Seconds();
    }

    void PushRecord(EAstrawildErrorSeverity Severity, FName Category, const FString& Message)
    {
        FScopeLock Lock(&ReportCriticalSection);
        if (ReportRecords.Num() >= UAstrawildErrorReporterLibrary::MaxRecords)
        {
            // Ring behaviour: drop the oldest record so memory stays bounded.
            ReportRecords.RemoveAt(0, 1, EAllowShrinking::No);
        }
        ReportRecords.Add(FAstrawildErrorRecord(Severity, Category, Message, NowSeconds()));
    }
}

void UAstrawildErrorReporterLibrary::ReportError(FName Category, const FString& Message)
{
    PushRecord(EAstrawildErrorSeverity::Error, Category, Message);
    UE_LOG(LogAstrawild, Error, TEXT("[Diagnostics:%s] %s"), *Category.ToString(), *Message);
}

void UAstrawildErrorReporterLibrary::ReportWarning(FName Category, const FString& Message)
{
    PushRecord(EAstrawildErrorSeverity::Warning, Category, Message);
    UE_LOG(LogAstrawild, Warning, TEXT("[Diagnostics:%s] %s"), *Category.ToString(), *Message);
}

void UAstrawildErrorReporterLibrary::ReportInfo(FName Category, const FString& Message)
{
    PushRecord(EAstrawildErrorSeverity::Info, Category, Message);
    UE_LOG(LogAstrawild, Log, TEXT("[Diagnostics:%s] %s"), *Category.ToString(), *Message);
}

TArray<FAstrawildErrorRecord> UAstrawildErrorReporterLibrary::GetRecords()
{
    FScopeLock Lock(&ReportCriticalSection);
    return ReportRecords;
}

int32 UAstrawildErrorReporterLibrary::GetRecordCount()
{
    FScopeLock Lock(&ReportCriticalSection);
    return ReportRecords.Num();
}

int32 UAstrawildErrorReporterLibrary::GetNonInfoCount()
{
    FScopeLock Lock(&ReportCriticalSection);
    int32 Count = 0;
    for (const FAstrawildErrorRecord& Record : ReportRecords)
    {
        if (Record.Severity != EAstrawildErrorSeverity::Info)
        {
            ++Count;
        }
    }
    return Count;
}

void UAstrawildErrorReporterLibrary::Clear()
{
    FScopeLock Lock(&ReportCriticalSection);
    ReportRecords.Reset();
}

FString UAstrawildErrorReporterLibrary::FormatReport(const FString& Header)
{
    FScopeLock Lock(&ReportCriticalSection);

    TArray<FString> Lines;
    Lines.Reserve(ReportRecords.Num() + 2);
    Lines.Add(Header);
    Lines.Add(FString::Printf(TEXT("Recorded: %d / capacity %d"), ReportRecords.Num(), MaxRecords));

    for (const FAstrawildErrorRecord& Record : ReportRecords)
    {
        const TCHAR* SeverityName = TEXT("INFO");
        if (Record.Severity == EAstrawildErrorSeverity::Warning)
        {
            SeverityName = TEXT("WARN");
        }
        else if (Record.Severity == EAstrawildErrorSeverity::Error)
        {
            SeverityName = TEXT("ERROR");
        }
        Lines.Add(FString::Printf(TEXT("[%10.1f] %-5s %-18s %s"),
            Record.TimestampSeconds, SeverityName, *Record.Category.ToString(), *Record.Message));
    }

    return FString::Join(Lines, LINE_TERMINATOR);
}

bool UAstrawildErrorReporterLibrary::WriteReportToFile(const FString& FilePath, const FString& Header)
{
    if (FilePath.IsEmpty())
    {
        return false;
    }

    // Ensure the parent directory exists — a missing Logs folder must not
    // take the game down (fault-tolerant by design, directive [2].1).
    const FString ParentFolder = FPaths::GetPath(FilePath);
    if (!ParentFolder.IsEmpty())
    {
        IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
        if (!FileManager.DirectoryExists(*ParentFolder))
        {
            FileManager.CreateDirectoryTree(*ParentFolder);
        }
    }

    const FString Contents = FormatReport(Header);
    const bool bWritten = FFileHelper::SaveStringToFile(Contents, *FilePath);
    if (!bWritten)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("ErrorReporter: could not write report to %s"), *FilePath);
    }
    return bWritten;
}

FString UAstrawildErrorReporterLibrary::GetDefaultReportPath()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs"), TEXT("AstrawildErrorReport.log"));
}

// ============================================================================
// Game instance subsystem — disk flush on shutdown
// ============================================================================

void UAstrawildErrorReporterSubsystem::Deinitialize()
{
    // Only stamp the file when something actually happened — a clean session
    // leaves no empty artifacts behind.
    if (GetNonInfoCount() > 0)
    {
        const FString Path = UAstrawildErrorReporterLibrary::GetDefaultReportPath();
        const FString Header = FString::Printf(TEXT("ASTRAWILD error report — session end %s"),
            *FDateTime::UtcNow().ToString());
        if (UAstrawildErrorReporterLibrary::WriteReportToFile(Path, Header))
        {
            LastFlushPath = Path;
        }
    }

    Super::Deinitialize();
}

bool UAstrawildErrorReporterSubsystem::HasPendingDiagnostics() const
{
    return UAstrawildErrorReporterLibrary::GetNonInfoCount() > 0;
}
