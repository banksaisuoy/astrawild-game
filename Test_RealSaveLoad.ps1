Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Input {
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
    public const byte VK_H = 0x48;
    public const byte VK_J = 0x4A;
    public const byte VK_K = 0x4B;
    public const byte VK_F5 = 0x74;
    public const byte VK_F9 = 0x78;

    public static void PressKey(byte vk, int holdMs = 100) {
        keybd_event(vk, 0, 0, 0);
        System.Threading.Thread.Sleep(holdMs);
        keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    }
}
"@

$PackagedExe = "E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe"
$LogPath = "E:\AstrawildGame\Saved\Logs\RealSaveLoad_Verification.log"

if (Test-Path $LogPath) { Remove-Item $LogPath -Force }

Write-Host "=================================================="
Write-Host " REAL 3-CYCLE SAVE/LOAD PLAYTEST VERIFICATION"
Write-Host "=================================================="

$Args = @(
    "-game",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log",
    "-abslog=`"$LogPath`""
)

$proc = Start-Process -FilePath $PackagedExe -ArgumentList $Args -PassThru
Start-Sleep -Seconds 3

if (-not $proc.HasExited) {
    [Win32Input]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 800

    Write-Host "`n--- CYCLE 1: Locomotion, Interaction, Save/Load ---"
    Write-Host "  Step 1.1: Walk forward (W)"
    [Win32Input]::PressKey([Win32Input]::VK_W, 1000)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 1.2: Interact (E)"
    [Win32Input]::PressKey([Win32Input]::VK_E, 150)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 1.3: QuickSave (F5)"
    [Win32Input]::PressKey([Win32Input]::VK_F5, 200)
    Start-Sleep -Seconds 2

    Write-Host "  Step 1.4: QuickLoad (F9)"
    [Win32Input]::PressKey([Win32Input]::VK_F9, 200)
    Start-Sleep -Seconds 2

    Write-Host "`n--- CYCLE 2: Inventory, Research, Save/Load ---"
    Write-Host "  Step 2.1: Open/Close Inventory (Tab)"
    [Win32Input]::PressKey([Win32Input]::VK_TAB, 200)
    Start-Sleep -Milliseconds 800
    [Win32Input]::PressKey([Win32Input]::VK_TAB, 200)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 2.2: Open/Close Research (K)"
    [Win32Input]::PressKey([Win32Input]::VK_K, 200)
    Start-Sleep -Milliseconds 800
    [Win32Input]::PressKey([Win32Input]::VK_K, 200)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 2.3: QuickSave (F5)"
    [Win32Input]::PressKey([Win32Input]::VK_F5, 200)
    Start-Sleep -Seconds 2

    Write-Host "  Step 2.4: QuickLoad (F9)"
    [Win32Input]::PressKey([Win32Input]::VK_F9, 200)
    Start-Sleep -Seconds 2

    Write-Host "`n--- CYCLE 3: Build Mode, Scanner, Save/Load ---"
    Write-Host "  Step 3.1: Toggle Build Mode (B)"
    [Win32Input]::PressKey([Win32Input]::VK_B, 200)
    Start-Sleep -Milliseconds 800
    [Win32Input]::PressKey([Win32Input]::VK_B, 200)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 3.2: Hold Scanner (V)"
    [Win32Input]::PressKey([Win32Input]::VK_V, 1200)
    Start-Sleep -Milliseconds 500

    Write-Host "  Step 3.3: QuickSave (F5)"
    [Win32Input]::PressKey([Win32Input]::VK_F5, 200)
    Start-Sleep -Seconds 2

    Write-Host "  Step 3.4: QuickLoad (F9)"
    [Win32Input]::PressKey([Win32Input]::VK_F9, 200)
    Start-Sleep -Seconds 2
}

Write-Host "`nStopping session cleanly..."
if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}

Write-Host "`n=================================================="
Write-Host " LOG VERIFICATION REPORT"
Write-Host "=================================================="

if (Test-Path $LogPath) {
    $Log = Get-Content $LogPath
    Write-Host "Total Log Lines: $($Log.Count)"

    $Matches = $Log | Select-String -Pattern "Saved game to slot|Loaded game from slot|QuickSave|QuickLoad|SaveWorld|LoadWorld|SaveSubsystem|Power grid state"
    Write-Host "Found $($Matches.Count) Save/Load log events:"
    foreach ($m in $Matches) {
        Write-Host "  $m"
    }
}
