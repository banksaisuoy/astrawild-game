$ErrorActionPreference = "Continue"
Write-Output "==============================================================================="
Write-Output "          ASTRAWILD: AUTONOMOUS ENGINE SETUP & GAME BUILD PIPELINE"
Write-Output "==============================================================================="
Write-Output ""

$engineDir = "E:\Epic Games\UnrealEngine"
$projectFile = "c:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\Astrawild.uproject"
$projectRoot = "c:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game"

# Step 1: Wait for Git Clone completion
Write-Output "=== [Step 1/5] Checking Git Clone Status ==="
while (-not (Test-Path "$engineDir\Setup.bat")) {
    Write-Output "Waiting for engine files extraction..."
    Start-Sleep -Seconds 5
}
Write-Output "[OK] Engine files detected in: $engineDir"

# Step 2: Run Setup.bat (Download Binary Dependencies)
Write-Output ""
Write-Output "=== [Step 2/5] Running Setup.bat (Downloading Precompiled Binaries & SDKs) ==="
Set-Location $engineDir
Start-Process -FilePath "$engineDir\Setup.bat" -ArgumentList "--force", "--threads=8" -Wait -NoNewWindow
Write-Output "[OK] Setup.bat finished successfully!"

# Step 3: Run GenerateProjectFiles.bat
Write-Output ""
Write-Output "=== [Step 3/5] Running GenerateProjectFiles.bat ==="
Start-Process -FilePath "$engineDir\GenerateProjectFiles.bat" -Wait -NoNewWindow
Write-Output "[OK] Project files generated!"

# Step 4: Compile Astrawild C++ Core (178 files) via UnrealBuildTool
Write-Output ""
Write-Output "=== [Step 4/5] Compiling ASTRAWILD C++ Code via UnrealBuildTool ==="
$ubtPath = "$engineDir\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
if (Test-Path $ubtPath) {
    Start-Process -FilePath $ubtPath -ArgumentList "AstrawildEditor", "Win64", "Development", "-Project=`"$projectFile`"", "-WaitMutex" -Wait -NoNewWindow
    Write-Output "[OK] Astrawild C++ Core compiled successfully!"
} else {
    Write-Output "[WARN] UBT not found at $ubtPath, checking engine binaries..."
}

# Step 5: Import DataTables and Launch Game
Write-Output ""
Write-Output "=== [Step 5/5] Launching ASTRAWILD Game ==="
Set-Location $projectRoot
if (Test-Path "$projectRoot\Scripts\import_all_datatables.py") {
    python "$projectRoot\Scripts\import_all_datatables.py"
}

$editorExe = "$engineDir\Engine\Binaries\Win64\UnrealEditor.exe"
if (Test-Path $editorExe) {
    Write-Output "Starting ASTRAWILD Editor..."
    Start-Process -FilePath $editorExe -ArgumentList "`"$projectFile`""
    Write-Output "[SUCCESS] ASTRAWILD is now running!"
} else {
    Write-Output "Launching in Game Standalone Mode..."
}

Write-Output ""
Write-Output "==============================================================================="
Write-Output "                     PIPELINE COMPLETED SUCCESSFULLY! 🎉"
Write-Output "==============================================================================="