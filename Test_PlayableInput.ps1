Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32PlayInput {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo);

    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, int dx, int dy, uint dwData, int dwExtraInfo);

    public const int KEYEVENTF_EXTENDEDKEY = 0x0001;
    public const int KEYEVENTF_KEYUP = 0x0002;

    public const uint MOUSEEVENTF_LEFTDOWN = 0x0002;
    public const uint MOUSEEVENTF_LEFTUP = 0x0004;
    public const uint MOUSEEVENTF_MOVE = 0x0001;

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
    public const byte VK_F5 = 0x74;
    public const byte VK_F9 = 0x78;

    public static void HoldKey(byte vk, int holdMs) {
        keybd_event(vk, 0, 0, 0);
        System.Threading.Thread.Sleep(holdMs);
        keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    }

    public static void PressKey(byte vk, int holdMs = 120) {
        keybd_event(vk, 0, 0, 0);
        System.Threading.Thread.Sleep(holdMs);
        keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
    }

    public static void ClickLeftMouse() {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        System.Threading.Thread.Sleep(80);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }

    public static void MoveMouseDelta(int dx, int dy) {
        mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
    }
}
"@

$EditorPath = "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe"
$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"
$LogPath = "E:\AstrawildGame\Saved\Logs\PlayableInput_Test.log"

if (Test-Path $LogPath) { Remove-Item $LogPath -Force }

Write-Host "=================================================="
Write-Host " ASTRAWILD REAL PLAYABLE INPUT & CAMERA TEST"
Write-Host "=================================================="

$Args = @(
    "`"$ProjectPath`"",
    "-game",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log",
    "-abslog=`"$LogPath`""
)

Write-Host "Launching Game Session..."
$proc = Start-Process -FilePath $EditorPath -ArgumentList $Args -PassThru

Write-Host "Waiting for Engine to load modules and bootstrap world map..."
$bReady = $false
for ($i = 0; $i -lt 50; $i++) {
    Start-Sleep -Seconds 1
    if (Test-Path $LogPath) {
        $found = Select-String -Path $LogPath -Pattern "ASTRAWILD game mode online|HUD widget created" -SimpleMatch -ErrorAction SilentlyContinue
        if ($found) {
            Write-Host "  -> World & Game Mode Online detected at second $($i + 1)!"
            $bReady = $true
            break
        }
    }
    if ($i % 10 -eq 0) { Write-Host "  ... waiting for map load ($($i)s)" }
}

Start-Sleep -Seconds 2

if (-not $proc.HasExited) {
    [Win32PlayInput]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 600

    # Ensure viewport focus
    [Win32PlayInput]::ClickLeftMouse()
    Start-Sleep -Milliseconds 400

    Write-Host "`nExecuting Playable Input Sequence on Live Viewport:"

    # 1. Forward Walk [W] (hold 3.0s)
    Write-Host "  [1/10] Testing [W] Move Forward (hold 3.0s)..."
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_W, 3000)
    Start-Sleep -Milliseconds 400

    # 2. Sprint [Shift+W] (hold 2.5s)
    Write-Host "  [2/10] Testing [Shift+W] Sprint (hold 2.5s)..."
    [Win32PlayInput]::keybd_event([Win32PlayInput]::VK_SHIFT, 0, 0, 0)
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_W, 2500)
    [Win32PlayInput]::keybd_event([Win32PlayInput]::VK_SHIFT, 0, [Win32PlayInput]::KEYEVENTF_KEYUP, 0)
    Start-Sleep -Milliseconds 400

    # 3. Space Jump
    Write-Host "  [3/10] Testing [Space] Jump..."
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_SPACE, 150)
    Start-Sleep -Milliseconds 600

    # 4. Mouse Look (Yaw & Pitch)
    Write-Host "  [4/10] Testing Mouse Look (Camera Rotation)..."
    for ($i = 0; $i -lt 12; $i++) {
        [Win32PlayInput]::MoveMouseDelta(50, -15)
        Start-Sleep -Milliseconds 50
    }
    Start-Sleep -Milliseconds 400

    # 5. Strafe A / D / S
    Write-Host "  [5/10] Testing Strafe Left [A], Right [D], Backward [S]..."
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_A, 1000)
    Start-Sleep -Milliseconds 200
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_D, 1000)
    Start-Sleep -Milliseconds 200
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_S, 1000)
    Start-Sleep -Milliseconds 400

    # 6. Interact [E]
    Write-Host "  [6/10] Testing [E] Interact..."
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_E, 150)
    Start-Sleep -Milliseconds 500

    # 7. Inventory [Tab]
    Write-Host "  [7/10] Testing [Tab] Inventory Toggle..."
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_TAB, 150)
    Start-Sleep -Milliseconds 800
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_TAB, 150)
    Start-Sleep -Milliseconds 400

    # 8. Build Mode [B]
    Write-Host "  [8/10] Testing [B] Build Mode Toggle..."
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_B, 150)
    Start-Sleep -Milliseconds 800
    [Win32PlayInput]::PressKey([Win32PlayInput]::VK_B, 150)
    Start-Sleep -Milliseconds 400

    # 9. Scanner [V]
    Write-Host "  [9/10] Testing [V] Hold Scanner..."
    [Win32PlayInput]::HoldKey([Win32PlayInput]::VK_V, 1500)
    Start-Sleep -Milliseconds 400

    # 10. Attack / Fire (Left Mouse Click)
    Write-Host "  [10/10] Testing Left Mouse Click (Attack/Fire)..."
    [Win32PlayInput]::ClickLeftMouse()
    Start-Sleep -Milliseconds 500
}

Write-Host "`nAllowing session to finish recording (5 seconds)..."
Start-Sleep -Seconds 5

if (-not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}

Write-Host "`n=================================================="
Write-Host " PLAYABLE INPUT LOG VERIFICATION"
Write-Host "=================================================="

if (Test-Path $LogPath) {
    $Log = Get-Content $LogPath
    Write-Host "Total Log Lines: $($Log.Count)"
    $Highlights = $Log | Select-String -Pattern "Survivor character mesh active|HUD widget created|Runtime default input mapping|ApplyMappingContext|ASTRAWILD game mode online|Power grid state|Dungeon generated"
    Write-Host "Found $($Highlights.Count) Core Gameplay Events:"
    foreach ($h in $Highlights) {
        Write-Host "  $h"
    }
}
