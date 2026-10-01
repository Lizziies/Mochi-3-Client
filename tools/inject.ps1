param(
    [string]$Dll = (Join-Path $PSScriptRoot '..\build\Release\Mochi.dll'),
    [int]$WaitSeconds = 120,
    [switch]$Launch
)

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class Inj {
    [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr VirtualAllocEx(IntPtr p, IntPtr addr, UIntPtr size, uint type, uint prot);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool WriteProcessMemory(IntPtr p, IntPtr addr, byte[] buf, UIntPtr size, out UIntPtr written);
    [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr CreateRemoteThread(IntPtr p, IntPtr attr, UIntPtr stack, IntPtr start, IntPtr arg, uint flags, out uint id);
    [DllImport("kernel32.dll")] static extern uint WaitForSingleObject(IntPtr h, uint ms);
    [DllImport("kernel32.dll")] static extern bool GetExitCodeThread(IntPtr h, out uint code);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)] static extern IntPtr GetProcAddress(IntPtr m, string name);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] static extern IntPtr GetModuleHandle(string name);

    public static string Load(int pid, string dll) {
        const uint access = 0x0002 | 0x0400 | 0x0008 | 0x0020 | 0x0010;
        IntPtr proc = OpenProcess(access, false, pid);
        if (proc == IntPtr.Zero) return "OpenProcess failed " + Marshal.GetLastWin32Error();
        byte[] bytes = Encoding.Unicode.GetBytes(dll + "\0");
        IntPtr remote = VirtualAllocEx(proc, IntPtr.Zero, (UIntPtr)bytes.Length, 0x3000, 0x04);
        UIntPtr written;
        if (remote == IntPtr.Zero || !WriteProcessMemory(proc, remote, bytes, (UIntPtr)bytes.Length, out written)) {
            CloseHandle(proc);
            return "write failed " + Marshal.GetLastWin32Error();
        }
        IntPtr loader = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryW");
        uint tid;
        IntPtr th = CreateRemoteThread(proc, IntPtr.Zero, UIntPtr.Zero, loader, remote, 0, out tid);
        if (th == IntPtr.Zero) { CloseHandle(proc); return "CreateRemoteThread failed " + Marshal.GetLastWin32Error(); }
        WaitForSingleObject(th, 20000);
        uint code;
        GetExitCodeThread(th, out code);
        CloseHandle(th);
        CloseHandle(proc);
        return code == 0 ? "LoadLibrary returned 0" : "ok";
    }
}
'@

$dll = (Resolve-Path $Dll).Path
$bin = Join-Path $env:LOCALAPPDATA 'Mochi\bin'
New-Item -ItemType Directory -Force $bin | Out-Null
$target = Join-Path $bin 'Mochi.dll'
Copy-Item $dll $target -Force
icacls $target /grant '*S-1-15-2-1:(RX)' | Out-Null

if ($Launch -and -not (Get-Process Minecraft.Windows -ErrorAction SilentlyContinue)) {
    Start-Process 'explorer.exe' 'shell:AppsFolder\Microsoft.MinecraftUWP_8wekyb3d8bbwe!Game'
}

$deadline = (Get-Date).AddSeconds($WaitSeconds)
$mc = $null
while ((Get-Date) -lt $deadline) {
    $mc = @(Get-Process Minecraft.Windows -ErrorAction SilentlyContinue)[0]
    if ($mc) {
        $mods = $mc.Modules | ForEach-Object { $_.ModuleName }
        if ($mc.MainWindowHandle -ne 0 -and ($mods -contains 'd3d12.dll' -or $mods -contains 'd3d11.dll')) { break }
    }
    Start-Sleep -Milliseconds 500
}
if (-not $mc) { Write-Host 'Minecraft did not start'; exit 1 }
if (($mc.Modules | ForEach-Object { $_.ModuleName }) -contains 'Mochi.dll') { Write-Host 'already injected'; exit 0 }

Start-Sleep -Seconds 2
$r = [Inj]::Load($mc.Id, $target)
Write-Host "inject pid $($mc.Id): $r"
if ($r -ne 'ok') { exit 1 }
