# ============================================================
# publish.ps1 -- 一键发布: 效果图归档 -> git commit -> git push
#
# 用法（在项目根目录）:
#     .\tools\publish.ps1 -Message "Day 8: AABB bounding box" -Image day08-aabb.png
#
# 参数:
#     -Message  必填，提交信息
#     -Image    可选，把 output/image.png 复制成 gallery/<这个名字>
#     -NoPush   只提交，不推送
#
# 注意: 本脚本刻意只用 ASCII 提示文字。
#       原因: Windows PowerShell 5.1 读取"无 BOM 的 UTF-8"脚本时
#             会按 GBK 解码，中文字符串会变成乱码。
# ============================================================

param(
    [Parameter(Mandatory = $true)][string]$Message,
    [string]$Image = "",
    [switch]$NoPush
)

# 无论从哪里调用，都切到项目根目录
Set-Location (Join-Path $PSScriptRoot '..')

# ---------- 1. 效果图归档 ----------
if ($Image -ne "") {
    if (-not (Test-Path "output/image.png")) {
        Write-Host "ERROR: output/image.png not found. Render first (see tools/run.ps1)." -ForegroundColor Red
        exit 1
    }
    if (-not (Test-Path "gallery")) {
        New-Item -ItemType Directory -Path "gallery" | Out-Null
    }
    Copy-Item "output/image.png" (Join-Path "gallery" $Image) -Force
    Write-Host "[1/4] Image archived -> gallery\$Image" -ForegroundColor Cyan
} else {
    Write-Host "[1/4] No image specified, skipping." -ForegroundColor DarkGray
}

# ---------- 2. 检查有没有改动 ----------
$status = git status --short
if ([string]::IsNullOrWhiteSpace($status)) {
    Write-Host "Nothing to commit. Working tree is clean." -ForegroundColor Yellow
    exit 0
}

Write-Host ""
Write-Host "[2/4] Changes:" -ForegroundColor Cyan
git status --short

# ---------- 3. 提交 ----------
Write-Host ""
Write-Host "[3/4] Committing..." -ForegroundColor Cyan
git add -A
git commit -m $Message
if ($LASTEXITCODE -ne 0) {
    Write-Host "COMMIT FAILED -- stopped." -ForegroundColor Red
    exit 1
}

# ---------- 4. 推送 ----------
if ($NoPush) {
    Write-Host ""
    Write-Host "DONE (committed only, not pushed)." -ForegroundColor Green
    exit 0
}

Write-Host ""
Write-Host "[4/4] Pushing..." -ForegroundColor Cyan
# ⚠️ git 会把进度写到 stderr，PowerShell 会把它当成错误输出刷屏。
#    用 Out-String 接住，再一次性打印，看起来就干净了。
$pushOut = (& git push 2>&1 | Out-String)
$code = $LASTEXITCODE
Write-Host $pushOut.Trim()

# 代理可能抽风，给最多 3 次重试
$attempt = 1
while ($code -ne 0 -and $attempt -lt 3) {
    $attempt++
    Write-Host "Push failed, retry $attempt/3 ..." -ForegroundColor Yellow
    $pushOut = (& git push 2>&1 | Out-String)
    $code = $LASTEXITCODE
    Write-Host $pushOut.Trim()
}

if ($code -ne 0) {
    Write-Host ""
    Write-Host "PUSH FAILED -- commit saved locally. Try: .\\tools\\publish.ps1 -Message <same msg> -NoPush  (or just 'git push' later)" -ForegroundColor Red
    Write-Host "Hint: check that your proxy (v2rayN, port 10808) is running." -ForegroundColor DarkYellow
    exit 1
}

Write-Host ""
Write-Host "DONE. Pushed to GitHub." -ForegroundColor Green
git log --oneline -n 3
