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
$ahead  = (git rev-list --count 'origin/main..HEAD' 2>$null)
if (-not $ahead) { $ahead = 0 }

if ([string]::IsNullOrWhiteSpace($status)) {
    Write-Host "[2/4] Working tree is clean." -ForegroundColor DarkGray
    if ($ahead -eq 0) {
        Write-Host "Nothing to commit and nothing to push. Done." -ForegroundColor Yellow
        exit 0
    }
    # 重要：上次推送失败时，改动已经 commit 了。
    #       这时候应该【跳过 commit，直接 push】，而不是直接退出。
    Write-Host "  but $ahead local commit(s) are not pushed yet. Skipping commit, going to push." -ForegroundColor Yellow
    $skipCommit = $true
} else {
    Write-Host ""
    Write-Host "[2/4] Changes:" -ForegroundColor Cyan
    git status --short
    $skipCommit = $false
}

# ---------- 3. 提交 ----------
if (-not $skipCommit) {
    Write-Host ""
    Write-Host "[3/4] Committing..." -ForegroundColor Cyan
    git add -A
    # ⚠️ 用 cmd /c 包一层：
    #    PowerShell 5.1 会把原生程序写到 stderr 的每一行都包装成红字 ErrorRecord，
    #    哪怕命令完全成功（git 的进度信息就是走 stderr 的）。
    #    cmd /c "... 2>&1" 在 cmd 层就把 stderr 并进 stdout，PowerShell 就安静了。
    $commitOut = cmd /c "git commit -m `"$Message`" 2>&1"
    $code = $LASTEXITCODE
    Write-Host $commitOut
    if ($code -ne 0) {
        Write-Host "COMMIT FAILED -- stopped." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host ""
    Write-Host "[3/4] Commit skipped (already committed)." -ForegroundColor DarkGray
}
if ($code -ne 0) {
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
$pushOut = cmd /c "git push 2>&1"
$code = $LASTEXITCODE
Write-Host $pushOut

# 代理可能抽风，给最多 3 次重试
$attempt = 1
while ($code -ne 0 -and $attempt -lt 3) {
    $attempt++
    Write-Host "Push failed, retry $attempt/3 ..." -ForegroundColor Yellow
    $pushOut = cmd /c "git push 2>&1"
    $code = $LASTEXITCODE
    Write-Host $pushOut
}

if ($code -ne 0) {
    Write-Host ""
    Write-Host "PUSH FAILED -- commit saved locally, nothing lost." -ForegroundColor Red
    Write-Host "  1. Make sure your proxy (v2rayN, port 10808) is running." -ForegroundColor Yellow
    Write-Host "  2. Then just run this script again (any -Message); it will skip the commit and push." -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "DONE. Pushed to GitHub." -ForegroundColor Green
git log --oneline -n 3
