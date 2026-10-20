# ============================================================
# measure_noise.ps1 -- 量化图像的【高频噪声】
#
# 为什么不能用"标准差"？
#   画面本身有大范围渐变（灯附近亮、远处暗），标准差里混进了【信号】。
#   我们要的只是【噪声】—— 也就是相邻像素之间毫无规律的跳变。
#
# 做法：对每个像素算"二阶差分"
#
#     h = | I(x,y) - (I(x-1,y) + I(x+1,y)) / 2 |
#     v = | I(x,y) - (I(x,y-1) + I(x,y+1)) / 2 |
#     noise = mean(h + v) / 2
#
#   平滑渐变上 h 和 v 都接近 0（线性区域二阶差分恰好为 0）。
#   噪声则被完整保留。
#
# 用法:
#   .\tools\measure_noise.ps1 -Path output\image.png
#   .\tools\measure_noise.ps1 -Path a.png,b.png
#
# 注意: 本脚本只用 ASCII 提示文字（PowerShell 5.1 读无 BOM 的 UTF-8 会按 GBK 解码）
# ============================================================
param(
    [Parameter(Mandatory = $true)][string[]]$Path,
    [int]$X = 0,
    [int]$Y = 0,
    [int]$W = 0,
    [int]$H = 0
)

Add-Type -AssemblyName System.Drawing

foreach ($p in $Path) {
    $full = Join-Path (Get-Location) $p
    if (-not (Test-Path $full)) {
        Write-Host "ERROR: not found -> $p" -ForegroundColor Red
        continue
    }

    $img = [System.Drawing.Bitmap]::FromFile($full)
    $w = if ($W -gt 0) { [Math]::Min($W, $img.Width  - $X) } else { $img.Width  - $X }
    $h = if ($H -gt 0) { [Math]::Min($H, $img.Height - $Y) } else { $img.Height - $Y }

    # 锁定位图数据，直接读字节（比 GetPixel 快几百倍）
    $rect = New-Object System.Drawing.Rectangle(0, 0, $img.Width, $img.Height)
    $data = $img.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
                          [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $stride = $data.Stride
    $bytes  = New-Object byte[] ($stride * $img.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $img.UnlockBits($data)

    $sum = 0.0
    $cnt = 0
    for ($j = $Y + 1; $j -lt $Y + $h - 1; $j++) {
        for ($i = $X + 1; $i -lt $X + $w - 1; $i++) {
            $base = $j * $stride + $i * 3
            $left = $j * $stride + ($i - 1) * 3
            $right = $j * $stride + ($i + 1) * 3
            $up   = ($j - 1) * $stride + $i * 3
            $down = ($j + 1) * $stride + $i * 3

            for ($c = 0; $c -lt 3; $c++) {
                $hh = [Math]::Abs($bytes[$base + $c] - ($bytes[$left + $c] + $bytes[$right + $c]) / 2.0)
                $vv = [Math]::Abs($bytes[$base + $c] - ($bytes[$up   + $c] + $bytes[$down  + $c]) / 2.0)
                $sum += ($hh + $vv) / 2.0
                $cnt++
            }
        }
    }
    $img.Dispose()

    $noise = if ($cnt -gt 0) { $sum / $cnt } else { 0 }
    Write-Host ("{0,-46} noise = {1,7:N3}   ({2}x{3} px)" -f (Split-Path $p -Leaf), $noise, $w, $h)
}
