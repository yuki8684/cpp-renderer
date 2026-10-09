# ============================================================
# test.ps1 -- 编译并运行单元测试（不需要渲染，秒级反馈）
#
# 用法（在项目根目录）:
#     .\tools\test.ps1
#
# 为什么单独做一个脚本？
#   渲染一次要 13 秒。改一行代码就等 13 秒实在太慢。
#   单元测试只编译一个 .cpp，1 秒内出结果 —— 先用它把算法验对，
#   再去跑渲染。
#
# 注意: 本脚本刻意只用 ASCII 提示文字。
#       原因: PowerShell 5.1 读取"无 BOM 的 UTF-8"脚本时会按 GBK 解码。
# ============================================================

Set-Location (Join-Path $PSScriptRoot '..')

if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

$tests = Get-ChildItem "tests\*_test.cpp" -ErrorAction SilentlyContinue
if (-not $tests) {
    Write-Host "No test files found in tests/ ." -ForegroundColor Yellow
    exit 0
}

$totalFail = 0

foreach ($t in $tests) {
    $exe = "build\" + $t.BaseName + ".exe"
    Write-Host "[build] $($t.Name) ..." -ForegroundColor Cyan

    g++ -std=c++17 -O2 -Wall -Wextra -I include -o $exe $t.FullName 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Host "COMPILE FAILED -- $($t.Name)" -ForegroundColor Red
        $totalFail++
        continue
    }

    Write-Host "[run]   $exe" -ForegroundColor Cyan
    & ".\$exe"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "TEST FAILED -- $($t.Name)" -ForegroundColor Red
        $totalFail++
    }
}

Write-Host ""
if ($totalFail -eq 0) {
    Write-Host "ALL TESTS PASSED" -ForegroundColor Green
    exit 0
} else {
    Write-Host "$totalFail test file(s) FAILED" -ForegroundColor Red
    exit 1
}
