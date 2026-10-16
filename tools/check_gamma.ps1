# ============================================================
# check_gamma.ps1 -- 验证 gamma 校正的【数学恒等式】
#
#   线性光强 L   →  旧字节 old = 255.999 * L        （直接写，没校正）
#   开平方后     →  新字节 new = 255.999 * sqrt(L)
#
#   消掉 L：  new = 255.999 * sqrt( old / 255.999 )
#
# 只要渲染的【线性光强】没变，这个式子就必须对【每一个像素】成立。
# 这比"看起来变亮了"强得多 —— 它是一个可证伪的量化判据。
#
# 用法（在项目根目录）:
#     1. 先把校正前的图存成 output/_pre_gamma.ppm
#     2. 改代码、重新渲染 output/image.ppm
#     3. .\tools\check_gamma.ps1
#
# 注意：本脚本只用 ASCII 提示文字，避开 PowerShell 5.1 的编码坑。
# ============================================================

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$beforePath = 'output/_pre_gamma.ppm'
$afterPath  = 'output/image.ppm'

foreach ($p in @($beforePath, $afterPath)) {
    if (-not (Test-Path $p)) {
        Write-Host "ERROR: $p not found." -ForegroundColor Red
        exit 1
    }
}

$before = [System.IO.File]::ReadAllLines((Join-Path (Get-Location) $beforePath))
$after  = [System.IO.File]::ReadAllLines((Join-Path (Get-Location) $afterPath))

# 前 3 行是 PPM 头: "P3" / "宽 高" / "255"
if ($before.Count -ne $after.Count) {
    Write-Host "ERROR: line counts differ ($($before.Count) vs $($after.Count))." -ForegroundColor Red
    Write-Host "       Did the resolution or sample count change? They must stay identical." -ForegroundColor Yellow
    exit 1
}

$total   = $before.Count - 3
$stride  = 37                       # 抽样步长（与分辨率互质，采样均匀）
$checked = 0
$within  = 0
$maxDev  = 0.0
$worstOld = $null
$worstNew = $null
$worstExp = 0.0

for ($i = 3; $i -lt $before.Count; $i += $stride) {
    $b = $before[$i] -split '\s+'
    $a = $after[$i]  -split '\s+'
    if ($b.Count -lt 3 -or $a.Count -lt 3) { continue }

    for ($c = 0; $c -lt 3; $c++) {
        $old = [double]$b[$c]
        $new = [double]$a[$c]

        # ⚠️ old 是【截断】出来的整数，所以它代表的是一段区间：
        #      old = trunc(255.999 * L)   =>   255.999*L in [old, old+1)
        #                                       L      in [old/255.999, (old+1)/255.999)
        #    因此 new 的合法范围是一个【区间】，不是单点：
        $lo = 255.999 * [Math]::Sqrt($old / 255.999)
        $hi = 255.999 * [Math]::Sqrt([Math]::Min($old + 1.0, 255.999) / 255.999)

        # new 也是截断的，再给 1 的浮点余量
        $dev = 0.0
        if ($new -lt $lo) { $dev = $lo - $new }
        elseif ($new -gt $hi) { $dev = $new - $hi }

        $checked++
        if ($dev -le 1.0) { $within++ }
        if ($dev -gt $maxDev) {
            $maxDev   = $dev
            $worstOld = $old
            $worstNew = $new
            $worstExp = $lo
        }
    }
}

Write-Host ''
Write-Host '===== Gamma identity check =====' -ForegroundColor Cyan
Write-Host ("  formula  : new = 255.999 * sqrt( old / 255.999 )")
Write-Host ("  sampled  : {0} of {1} channels (stride {2})" -f $checked, ($total * 3), $stride)
Write-Host ''
Write-Host ("  within +-2 : {0} / {1}  ({2:N2}%)" -f $within, $checked, (100.0 * $within / $checked))
Write-Host ("  max dev    : {0:N2}" -f $maxDev)
Write-Host ("  worst case : old={0}  got={1}  expected={2:N1}" -f $worstOld, $worstNew, $worstExp)
Write-Host ''

if ($within -eq $checked) {
    Write-Host 'PASS -- every sampled channel satisfies the gamma identity.' -ForegroundColor Green
    Write-Host '        The linear radiance did not change; only the encoding did.' -ForegroundColor DarkGray
    exit 0
}

if (($within / $checked) -gt 0.99) {
    Write-Host 'MOSTLY PASS -- a few outliers (likely 0 or 255 saturation).' -ForegroundColor Yellow
    exit 0
}

Write-Host 'FAIL -- the linear radiance changed too. Did you modify something else?' -ForegroundColor Red
Write-Host '        Remember: change ONE variable at a time.' -ForegroundColor Yellow
exit 1
