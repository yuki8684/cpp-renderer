# ============================================================
# run.ps1 -- one-click: build -> render -> convert to PNG
#
# 用法（在项目根目录）:
#     .\tools\run.ps1
#
# 特点: 任何一步失败都会【立刻停下】并退出，
#       不会再让你看到"假的成功"（比如编译失败了却还在跑旧 exe）。
#
# 注意: 本脚本刻意只用 ASCII 提示文字。
#       原因: Windows PowerShell 5.1 读取"无 BOM 的 UTF-8"脚本时
#             会按 GBK 解码，中文字符串会变成乱码。
# ============================================================

# 无论从哪里调用，都切到项目根目录（exe 用的是相对路径 output/）
Set-Location (Join-Path $PSScriptRoot '..')

Write-Host "[1/3] Building..." -ForegroundColor Cyan
cmake --build build -j
if (-not $?) {
    Write-Host ""
    Write-Host "BUILD FAILED -- stopped. Fix the errors above, then re-run." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[2/3] Rendering..." -ForegroundColor Cyan
.\build\raytracer.exe
if (-not $?) {
    Write-Host ""
    Write-Host "RENDER FAILED -- stopped." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[3/3] Converting to PNG..." -ForegroundColor Cyan
.\tools\ppm2png.ps1 -InputPath output\image.ppm -OutputPath output\image.png

Write-Host ""
Write-Host "DONE. Open output\image.png to see the result." -ForegroundColor Green
Write-Host ""
Write-Host "--- timestamps (both should be 'just now') ---"
Get-Item output\image.ppm, output\image.png | Select-Object Name, Length, LastWriteTime
