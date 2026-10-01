param([string]$Out = (Join-Path $PSScriptRoot '..\dev-data\image'))

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Runtime.InteropServices;

public static class Dump {
    [DllImport("kernel32.dll", SetLastError = true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError = true)] static extern bool ReadProcessMemory(IntPtr p, IntPtr addr, byte[] buf, UIntPtr size, out UIntPtr read);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);

    static bool Read(IntPtr proc, long addr, byte[] buf, int len) {
        UIntPtr got;
        return ReadProcessMemory(proc, (IntPtr)addr, buf, (UIntPtr)len, out got) && (int)got == len;
    }

    public static string Run(int pid, long baseAddr, string path) {
        IntPtr proc = OpenProcess(0x0010 | 0x0400, false, pid);
        if (proc == IntPtr.Zero) return "OpenProcess failed " + Marshal.GetLastWin32Error();

        byte[] head = new byte[0x1000];
        if (!Read(proc, baseAddr, head, head.Length)) return "cannot read headers";
        int pe = BitConverter.ToInt32(head, 0x3C);
        int sections = BitConverter.ToUInt16(head, pe + 6);
        int optSize = BitConverter.ToUInt16(head, pe + 20);
        int opt = pe + 24;
        uint imageSize = BitConverter.ToUInt32(head, opt + 56);
        int secTable = opt + optSize;

        // headers: raw offset == RVA, so every section keeps its in-memory position
        BitConverter.GetBytes(0x1000u).CopyTo(head, opt + 36);
        BitConverter.GetBytes(0x1000u).CopyTo(head, opt + 32);
        long unreadable = 0;
        using (var fs = new FileStream(path, FileMode.Create, FileAccess.Write)) {
            fs.Write(head, 0, head.Length);
            byte[] chunk = new byte[1 << 20];
            for (int s = 0; s < sections; s++) {
                int e = secTable + s * 40;
                uint vsize = BitConverter.ToUInt32(head, e + 8);
                uint rva = BitConverter.ToUInt32(head, e + 12);
                uint raw = (vsize + 0xFFFu) & ~0xFFFu;
                BitConverter.GetBytes(raw).CopyTo(head, e + 16);
                BitConverter.GetBytes(rva).CopyTo(head, e + 20);
                fs.Position = rva;
                for (long off = 0; off < raw; off += chunk.Length) {
                    int n = (int)Math.Min(chunk.Length, raw - off);
                    byte[] buf = n == chunk.Length ? chunk : new byte[n];
                    if (!Read(proc, baseAddr + rva + off, buf, n)) {
                        Array.Clear(buf, 0, n);
                        byte[] page = new byte[0x1000];
                        for (int p = 0; p < n; p += 0x1000) {
                            int len = Math.Min(0x1000, n - p);
                            byte[] pg = len == 0x1000 ? page : new byte[len];
                            if (Read(proc, baseAddr + rva + off + p, pg, len)) Array.Copy(pg, 0, buf, p, len);
                            else unreadable += len;
                        }
                    }
                    fs.Write(buf, 0, n);
                }
            }
            fs.Position = 0;
            fs.Write(head, 0, head.Length);
            fs.SetLength(imageSize);
        }
        CloseHandle(proc);
        return "ok image=" + imageSize + " unreadable=" + unreadable;
    }
}
'@

$mc = @(Get-Process Minecraft.Windows -ErrorAction Stop)[0]
$module = $mc.MainModule
$ver = (Get-Process -Id $mc.Id).MainModule.FileVersionInfo.FileVersion
if (-not $ver) { $ver = 'unknown' }
New-Item -ItemType Directory -Force $Out | Out-Null
$file = Join-Path (Resolve-Path $Out).Path ("Minecraft.Windows-{0}.exe" -f ($ver -replace '[^\d\.]', ''))
$r = [Dump]::Run($mc.Id, [long]$module.BaseAddress, $file)
Write-Host "base 0x$('{0:x}' -f [long]$module.BaseAddress) -> $file : $r"
