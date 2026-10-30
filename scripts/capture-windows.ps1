param(
    [string]$GamePath = "build/golfgame.exe",
    [string]$WorkingDirectory = "build"
)
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$game = Join-Path $repoRoot $GamePath
$work = Join-Path $repoRoot $WorkingDirectory
$logRoot = Join-Path $repoRoot "logs"
$shotRoot = Join-Path $logRoot "screenshots"
if (-not (Test-Path $game)) { throw "Game executable not found: $game" }
if (-not (Test-Path (Join-Path $work "data/static_data.static_data"))) {
    throw "Runtime data is missing beside the executable. Expected $work\data"
}
Remove-Item $logRoot -Recurse -Force -ErrorAction SilentlyContinue
New-Item $shotRoot -ItemType Directory -Force | Out-Null
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class GolfgameWin32 {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string className, string windowName);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")]
    public static extern void mouse_event(uint flags, uint dx, uint dy, uint data, UIntPtr extraInfo);
    [DllImport("user32.dll")]
    public static extern void keybd_event(byte key, byte scan, uint flags, UIntPtr extraInfo);
}
"@
$stdout = Join-Path $logRoot "runtime-stdout.txt"
$stderr = Join-Path $logRoot "runtime-stderr.txt"
$transcript = Join-Path $logRoot "capture-report.txt"
"Started $(Get-Date -Format o)" | Set-Content $transcript
"Executable: $game" | Add-Content $transcript
"Working directory: $work" | Add-Content $transcript
$proc = Start-Process -FilePath $game -WorkingDirectory $work -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
function Get-GameWindow {
    $handle = [IntPtr]::Zero
    for ($i = 0; $i -lt 80 -and $handle -eq [IntPtr]::Zero; $i++) {
        $handle = [GolfgameWin32]::FindWindow($null, "Golfgame")
        if ($handle -eq [IntPtr]::Zero) { Start-Sleep -Milliseconds 250 }
    }
    return $handle
}
function Get-ClientOrigin([IntPtr]$handle) {
    $point = New-Object -TypeName 'GolfgameWin32+POINT'
    $rect = New-Object -TypeName 'GolfgameWin32+RECT'
    if (-not [GolfgameWin32]::GetClientRect($handle, [ref]$rect)) {
        throw "Could not read game client rectangle."
    }
    [void][GolfgameWin32]::ClientToScreen($handle, [ref]$point)
    return @{
        X = $point.X; Y = $point.Y
        Width = $rect.Right - $rect.Left
        Height = $rect.Bottom - $rect.Top
    }
}
function Capture-Game([IntPtr]$handle, [string]$name) {
    $client = Get-ClientOrigin $handle
    $requested = [System.Drawing.Rectangle]::new($client.X, $client.Y, $client.Width, $client.Height)
    $desktop = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bounds = [System.Drawing.Rectangle]::Intersect($requested, $desktop)
    if ($bounds.Width -lt 100 -or $bounds.Height -lt 100) {
        throw "Game window is not visible on the desktop; refusing to save an empty screenshot."
    }
    $bitmap = New-Object System.Drawing.Bitmap($bounds.Width, $bounds.Height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bounds.Size)
        $path = Join-Path $shotRoot $name
        $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
        $bytes = (Get-Item $path).Length
        "$name | $($bounds.Width)x$($bounds.Height) | $bytes bytes | $(Get-Date -Format o)" | Add-Content $transcript
        Write-Host "Captured $path ($bytes bytes)"
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}
function Click-Game([IntPtr]$handle, [int]$x, [int]$y) {
    $client = Get-ClientOrigin $handle
    [void][GolfgameWin32]::SetCursorPos($client.X + $x, $client.Y + $y)
    Start-Sleep -Milliseconds 150
    [GolfgameWin32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [GolfgameWin32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 250
}
function Tap-Grave([IntPtr]$handle) {
    [void][GolfgameWin32]::SetForegroundWindow($handle)
    [GolfgameWin32]::keybd_event(0xC0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 100
    [GolfgameWin32]::keybd_event(0xC0, 0, 0x0002, [UIntPtr]::Zero)
}
try {
    $window = Get-GameWindow
    if ($window -eq [IntPtr]::Zero) {
        throw "The game did not create a window. Check runtime-stdout.txt and runtime-stderr.txt."
    }
    [void][GolfgameWin32]::SetForegroundWindow($window)
    Start-Sleep -Seconds 1
    Capture-Game $window "00-startup.png"
    Start-Sleep -Seconds 7
    Capture-Game $window "01-main-menu.png"
    # Open the original Open-Golf course selector.
    Click-Game $window 640 540
    Start-Sleep -Seconds 1
    Capture-Game $window "02-course-select.png"
    # Select the first course tile in the default 1280x720 layout.
    Click-Game $window 424 246
    Start-Sleep -Seconds 6
    Capture-Game $window "03-course-gameplay.png"
    # The Multiplayer tab is available in the in-game debug console (grave key).
    Tap-Grave $window
    Start-Sleep -Seconds 1
    Capture-Game $window "04-debug-console.png"
    Click-Game $window 225 48
    Start-Sleep -Milliseconds 500
    Capture-Game $window "05-multiplayer-tab.png"
    "Screenshots captured successfully." | Add-Content $transcript
} finally {
    if ($proc -and -not $proc.HasExited) {
        $null = $proc.CloseMainWindow()
        if (-not $proc.WaitForExit(3000)) { $proc.Kill() }
    }
}
$captures = @(Get-ChildItem $shotRoot -Filter "*.png" -ErrorAction SilentlyContinue)
if ($captures.Count -lt 3) {
    throw "Expected at least three visual checkpoints, found $($captures.Count)."
}
"Completed $(Get-Date -Format o)" | Add-Content $transcript
