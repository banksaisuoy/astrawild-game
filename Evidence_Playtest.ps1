Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo);

    public const int KEYEVENTF_EXTENDEDKEY = 0x0001;
    public const int KEYEVENTF_KEYUP = 0x0002;
    public const byte VK_W = 0x57;
    public const byte VK_A = 0x41;
    public const byte VK_S = 0x53;
    public const byte VK_D = 0x44;
    public const byte VK_E = 0x45;
    public const byte VK_SPACE = 0x20;
    public const byte VK_SHIFT = 0x10;
    public const byte VK_TAB = 0x09;
    public const byte VK_B = 0x42;
    public const byte VK_C = 0x43;
    public const byte VK_V = 0x56;
    public const byte VK_J = 0x4A;
    public const byte VK_K = 0x4B;
    public const byte VK_Z = 0x5A;
}
"@

$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"
$PackagedExe = "E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe"
$LogPath = "E:\AstrawildGame\Saved\Logs\Evidence_Playtest.log"
$ScreenshotDir = "E:\Astrawild_Packaged\Windows\ASTRAWILD\Saved\Screenshots\Windows"
$SaveDir = "E:\Astrawild_Packaged\Windows\ASTRAWILD\Saved\SaveGames"

if (Test-Path $LogPath) { Remove-Item $LogPath -Force }
if (-not (Test-Path $ScreenshotDir)) { New-Item -ItemType Directory -Path $ScreenshotDir -Force | Out-Null }
if (-not (Test-Path $SaveDir)) { New-Item -ItemType Directory -Path $SaveDir -Force | Out-Null }

Write-Host "=================================================="
Write-Host " ASTRAWILD EVIDENCE-GRADE RUNTIME PLAYTEST SESSION"
Write-Host "=================================================="

# Sequence of commands to execute during the interactive game session
$ExecCommands = @(
    "stat fps",
    "stat unit",
    "stat gpu",
    "stat game",
    "stat anim",
    "Shot",
    "AW.GiveItem Item_Wood 100",
    "AW.GiveItem Item_Stone 100",
    "AW.GiveItem Item_Resonator 10",
    "AW.GiveItem Item_Berry 20",
    "AW.SpawnEcho Echo_Lumewisp",
    "Shot",
    "AW.CaptureAll",
    "AW.SaveNow",
    "Shot",
    "AW.ResearchPoints 50",
    "AW.UnlockTech Tech_AdvancedFabrication",
    "AW.GiveItem Item_Weapon_Scrapshot 1",
    "AW.GiveItem Item_Ammo_RailSlug 50",
    "AW.EquipItem Item_Weapon_Scrapshot",
    "Shot",
    "AW.SaveNow",
    "Shot",
    "AW.SetWeather rain",
    "Shot",
    "AW.SetWeather clear",
    "AW.SetTime 12 0",
    "Shot",
    "AW.TeleportForward 1500",
    "Shot",
    "AW.SaveNow",
    "Shot"
) -join "; "

$Args = @(
    "-game",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log",
    "-abslog=`"$LogPath`"",
    "-ExecCmds=`"$ExecCommands`""
)

Write-Host "Launching Packaged Client for Evidence Playtest..."
$proc = Start-Process -FilePath $PackagedExe -ArgumentList $Args -PassThru

Start-Sleep -Seconds 3

# Send interactive keyboard inputs to the active game window
if (-not $proc.HasExited) {
    [Win32]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 500

    Write-Host "Sending Interactive Inputs:"
    
    # Locomotion (Walk W, Sprint Shift+W, Jump Space)
    Write-Host "  -> [W] Move Forward"
    [Win32]::keybd_event([Win32]::VK_W, 0, 0, 0)
    Start-Sleep -Milliseconds 800
    Write-Host "  -> [Shift+W] Sprint"
    [Win32]::keybd_event([Win32]::VK_SHIFT, 0, 0, 0)
    Start-Sleep -Milliseconds 800
    Write-Host "  -> [Space] Jump"
    [Win32]::keybd_event([Win32]::VK_SPACE, 0, 0, 0)
    Start-Sleep -Milliseconds 150
    [Win32]::keybd_event([Win32]::VK_SPACE, 0, [Win32]::KEYEVENTF_KEYUP, 0)
    Start-Sleep -Milliseconds 500
    [Win32]::keybd_event([Win32]::VK_SHIFT, 0, [Win32]::KEYEVENTF_KEYUP, 0)
    [Win32]::keybd_event([Win32]::VK_W, 0, [Win32]::KEYEVENTF_KEYUP, 0)

    # Interaction [E]
    Start-Sleep -Milliseconds 500
    Write-Host "  -> [E] Interact"
    [Win32]::keybd_event([Win32]::VK_E, 0, 0, 0)
    Start-Sleep -Milliseconds 150
    [Win32]::keybd_event([Win32]::VK_E, 0, [Win32]::KEYEVENTF_KEYUP, 0)

    # Inventory [Tab]
    Start-Sleep -Milliseconds 500
    Write-Host "  -> [Tab] Open Inventory"
    [Win32]::keybd_event([Win32]::VK_TAB, 0, 0, 0)
    Start-Sleep -Milliseconds 200
    [Win32]::keybd_event([Win32]::VK_TAB, 0, [Win32]::KEYEVENTF_KEYUP, 0)
    Start-Sleep -Milliseconds 500
    [Win32]::keybd_event([Win32]::VK_TAB, 0, 0, 0)
    Start-Sleep -Milliseconds 200
    [Win32]::keybd_event([Win32]::VK_TAB, 0, [Win32]::KEYEVENTF_KEYUP, 0)

    # Party Command [C]
    Start-Sleep -Milliseconds 500
    Write-Host "  -> [C] Cycle Party Command"
    [Win32]::keybd_event([Win32]::VK_C, 0, 0, 0)
    Start-Sleep -Milliseconds 150
    [Win32]::keybd_event([Win32]::VK_C, 0, [Win32]::KEYEVENTF_KEYUP, 0)

    # Hold Scanner [V]
    Start-Sleep -Milliseconds 500
    Write-Host "  -> [V] Hold Scanner"
    [Win32]::keybd_event([Win32]::VK_V, 0, 0, 0)
    Start-Sleep -Milliseconds 1000
    [Win32]::keybd_event([Win32]::VK_V, 0, [Win32]::KEYEVENTF_KEYUP, 0)

    # Base Build Mode [B]
    Start-Sleep -Milliseconds 500
    Write-Host "  -> [B] Toggle Build Mode"
    [Win32]::keybd_event([Win32]::VK_B, 0, 0, 0)
    Start-Sleep -Milliseconds 150
    [Win32]::keybd_event([Win32]::VK_B, 0, [Win32]::KEYEVENTF_KEYUP, 0)
    Start-Sleep -Milliseconds 500
    [Win32]::keybd_event([Win32]::VK_B, 0, 0, 0)
    Start-Sleep -Milliseconds 150
    [Win32]::keybd_event([Win32]::VK_B, 0, [Win32]::KEYEVENTF_KEYUP, 0)
}

Write-Host "Allowing session to complete telemetry recording (15 seconds)..."
Start-Sleep -Seconds 15

if (-not $proc.HasExited) {
    Write-Host "Stopping playtest session cleanly..."
    Stop-Process -Id $proc.Id -Force
    Start-Sleep -Seconds 2
}

Write-Host "`n=================================================="
Write-Host " PLAYTEST EVIDENCE LOG ANALYSIS"
Write-Host "=================================================="

if (Test-Path $LogPath) {
    $Log = Get-Content $LogPath
    Write-Host "Log Lines Recorded: $($Log.Count)"
    
    $Subsystems = @(
        "LogAstrawild",
        "LogAstrawildAI",
        "LogAstrawildCombat",
        "LogAstrawildEconomy",
        "LogAstrawildWorld",
        "LogAstrawildBuilding",
        "LogSaveSubsystem",
        "LogScreenShot",
        "LogRHI",
        "Power grid state",
        "Dungeon generated",
        "Boss initialized",
        "Saved game to slot",
        "Loaded game from slot"
    )

    foreach ($sub in $Subsystems) {
        $found = $Log | Select-String -Pattern $sub
        Write-Host "  -> [$sub]: $($found.Count) events"
        if ($found.Count -gt 0 -and $found.Count -le 5) {
            foreach ($entry in $found) {
                Write-Host "     * $entry"
            }
        }
    }
}

Write-Host "`nChecking Saved Games on Disk:"
$SavedFiles = Get-ChildItem -Path $SaveDir -ErrorAction SilentlyContinue
if ($SavedFiles) {
    foreach ($f in $SavedFiles) {
        Write-Host "  Found Save File: $($f.Name) ($($f.Length) bytes, Modified: $($f.LastWriteTime))"
    }
} else {
    Write-Host "  No save files in $SaveDir"
}

Write-Host "`nChecking Screenshots Captured:"
$ScreenFiles = Get-ChildItem -Path $ScreenshotDir -ErrorAction SilentlyContinue
if ($ScreenFiles) {
    foreach ($sf in $ScreenFiles) {
        Write-Host "  Found Screenshot: $($sf.Name) ($($sf.Length) bytes)"
    }
} else {
    Write-Host "  No screenshots in $ScreenshotDir"
}
