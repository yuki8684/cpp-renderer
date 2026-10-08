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
$swBuild = [Diagnostics.Stopwatch]::StartNew()
cmake --build build -j
# ⚠️ 必须用 $LASTEXITCODE 而不是 $?：
#    PowerShell 5.1 里，只要原生程序往 stderr 写了任何东西（哪怕是警告），
#    $? 就可能变成 False —— 会造成"编译其实成功却报失败"的误判。
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "BUILD FAILED -- stopped. Fix the errors above, then re-run." -ForegroundColor Red
    exit 1
}
$swBuild.Stop()

Write-Host ""
Write-Host "[2/3] Rendering..." -ForegroundColor Cyan
$swRender = [Diagnostics.Stopwatch]::StartNew()
.\build\raytracer.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "RENDER FAILED -- stopped." -ForegroundColor Red
    exit 1
}
$swRender.Stop()

Write-Host ""
Write-Host "[3/3] Converting to PNG..." -ForegroundColor Cyan
$swConv = [Diagnostics.Stopwatch]::StartNew()
.\tools\ppm2png.ps1 -InputPath output\image.ppm -OutputPath output\image.png
$swConv.Stop()

Write-Host ""
Write-Host "DONE. Open output\image.png to see the result." -ForegroundColor Green
Write-Host ""
# ⚠️ 把编译和渲染分开报！
#    Day 7 的教训：以前只报一个"总耗时"，里面 87% 是编译时间，
#    导致所有性能对比都是错的。讨论渲染性能请看 RENDER 那一行。
Write-Host "--- timing breakdown ---"
Write-Host ("  BUILD   {0,7:N2} s" -f $swBuild.Elapsed.TotalSeconds) -ForegroundColor DarkGray
Write-Host ("  RENDER  {0,7:N2} s   <- this is the renderer's real speed" -f $swRender.Elapsed.TotalSeconds) -ForegroundColor Yellow
Write-Host ("  CONVERT {0,7:N2} s" -f $swConv.Elapsed.TotalSeconds) -ForegroundColor DarkGray
Write-Host ("  TOTAL   {0,7:N2} s" -f ($swBuild.Elapsed.TotalSeconds + $swRender.Elapsed.TotalSeconds + $swConv.Elapsed.TotalSeconds))
Write-Host ""
Write-Host "--- output files ---"
Get-Item output\image.ppm, output\image.png | Select-Object Name, Length, LastWriteTime
