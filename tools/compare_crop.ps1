# ============================================================
# compare_crop.ps1 -- 从两张 PNG 裁同一块区域，放大后左右并排
#
# 用途：验证"某处细节变了没有"。整图对比容易看不清局部差异，
#       放大后一眼就能看出来。
#
# 用法（在项目根目录）:
#   .\tools\compare_crop.ps1 -Before a.png -After b.png -Output c.png `
#                            -X 130 -Y 5 -W 110 -H 110 -Scale 3 `
#                            -LabelBefore "无 Schlick" -LabelAfter "有 Schlick"
#
# ⚠️ 只用 ASCII 提示文字（避开 PowerShell 5.1 的编码坑）。
#    标签文字是参数传进来的，不受本文件编码影响。
# ============================================================

param(
    [Parameter(Mandatory = $true)][string]$Before,
    [Parameter(Mandatory = $true)][string]$After,
    [Parameter(Mandatory = $true)][string]$Output,
    [Parameter(Mandatory = $true)][int]$X,
    [Parameter(Mandatory = $true)][int]$Y,
    [Parameter(Mandatory = $true)][int]$W,
    [Parameter(Mandatory = $true)][int]$H,
    [int]$Scale = 3,
    [string]$LabelBefore = "BEFORE",
    [string]$LabelAfter  = "AFTER"
)

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

Add-Type -AssemblyName System.Drawing

$imgB = [System.Drawing.Image]::FromFile((Join-Path (Get-Location) $Before))
$imgA = [System.Drawing.Image]::FromFile((Join-Path (Get-Location) $After))

if ($imgB.Width -ne $imgA.Width -or $imgB.Height -ne $imgA.Height) {
    Write-Host "WARNING: image sizes differ." -ForegroundColor Yellow
}

# 裁切区域不能越界
if ($X + $W -gt $imgB.Width)  { $W = $imgB.Width  - $X }
if ($Y + $H -gt $imgB.Height) { $H = $imgB.Height - $Y }
if ($W -le 0 -or $H -le 0) {
    Write-Host "ERROR: crop rectangle is outside the image." -ForegroundColor Red
    exit 1
}

$barH  = 30
$outW  = $W * $Scale * 2
$outH  = $H * $Scale + $barH

$bmp = New-Object System.Drawing.Bitmap($outW, $outH)
$g   = [System.Drawing.Graphics]::FromImage($bmp)

# 高质量放大（否则放大后是硬像素块）
$g.InterpolationMode  = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode    = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$g.SmoothingMode      = [System.Drawing.Drawing2D.SmoothingMode]::None

# 背景 + 分隔线
$g.Clear([System.Drawing.Color]::FromArgb(13, 17, 23))
$pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(48, 54, 61), 2)
$g.DrawLine($pen, ($W * $Scale), 0, ($W * $Scale), $outH)

# 画两块裁切区域（源矩形 -> 目标矩形）
$srcRect  = New-Object System.Drawing.Rectangle($X, $Y, $W, $H)
$dstRectB = New-Object System.Drawing.Rectangle(0,          $barH, ($W * $Scale), ($H * $Scale))
$dstRectA = New-Object System.Drawing.Rectangle(($W * $Scale), $barH, ($W * $Scale), ($H * $Scale))
$g.DrawImage($imgB, $dstRectB, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
$g.DrawImage($imgA, $dstRectA, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)

# 顶部标签
$font  = New-Object System.Drawing.Font("Consolas", 13, [System.Drawing.FontStyle]::Bold)
$brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(230, 237, 243))
$sf    = New-Object System.Drawing.StringFormat
$sf.Alignment     = [System.Drawing.StringAlignment]::Center
$sf.LineAlignment = [System.Drawing.StringAlignment]::Center   # 垂直居中，否则文字会被下边框裁掉
$g.DrawString($LabelBefore, $font, $brush, ($W * $Scale / 2), ($barH / 2), $sf)
$g.DrawString($LabelAfter,  $font, $brush, ($W * $Scale * 1.5), ($barH / 2), $sf)

$g.Dispose()

$outPath = Join-Path (Get-Location) $Output
$bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$imgB.Dispose()
$imgA.Dispose()

Write-Host "OK -> $Output  ($outW x $outH, crop ${W}x${H} at ($X,$Y), scale x$Scale)" -ForegroundColor Green
