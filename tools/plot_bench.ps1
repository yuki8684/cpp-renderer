# ============================================================
# plot_bench.ps1 -- 读 output/bench.csv，生成 gallery/day11-scaling-curve.svg
#
# 用法（在项目根目录）:
#     .\tools\plot_bench.ps1
#
# 前置：先跑 .\build\raytracer.exe --bench 生成 output/bench.csv
#
# ⚠️ 编码注意：Windows PowerShell 5.1 读【无 BOM 的 UTF-8】脚本时
#    会按 GBK 解码，中文字符串会变成乱码。本文件必须保存为
#    "UTF-8 with BOM"。若被编辑器改成无 BOM，中文标签会全乱。
# ============================================================

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$csvPath = 'output/bench.csv'
$outPath = 'gallery/day11-scaling-curve.svg'

if (-not (Test-Path $csvPath)) {
    Write-Host "ERROR: $csvPath not found. Run first: .\build\raytracer.exe --bench" -ForegroundColor Red
    exit 1
}

$rows = @(Import-Csv $csvPath)
if ($rows.Count -lt 2) {
    Write-Host 'ERROR: need at least 2 rows in bench.csv' -ForegroundColor Red
    exit 1
}

# 取出四列数字
$data = @()
foreach ($r in $rows) {
    $data += [pscustomobject]@{
        n       = [double]$r.n
        build   = [double]$r.build_ms
        naive   = [double]$r.naive_ms
        bvh     = [double]$r.bvh_ms
        speedup = [double]$r.naive_ms / [double]$r.bvh_ms
    }
}
$maxSpeedup = ($data | Measure-Object -Property speedup -Maximum).Maximum
$nMax       = $data[-1].n
$speedAxisMax = [Math]::Ceiling($maxSpeedup / 25.0) * 25.0   # 向上取整到 25 的倍数

# ------------------------------------------------------------
# 几何参数
# ------------------------------------------------------------
$W = 920; $H = 700

# ---- 面板 1：耗时 vs N（双对数）----
$p1L = 110; $p1R = 850; $p1T = 130; $p1B = 370
$logMin = -2.0       # 0.01 ms
$logMax = 4.0        # 10000 ms

# 双对数坐标下，折线斜率 = 复杂度指数（算出来给图例用，不靠手写）
function Get-Slope([string]$key) {
    $x1 = [Math]::Log10($data[0].n);            $y1 = [Math]::Log10($data[0].($key))
    $x2 = [Math]::Log10($data[-1].n);           $y2 = [Math]::Log10($data[-1].($key))
    if ($x2 -eq $x1) { return 0 }
    return ($y2 - $y1) / ($x2 - $x1)
}
$slopeNaive = Get-Slope 'naive'
$slopeBvh   = Get-Slope 'bvh'

# 规模增长倍数（用于脚注）
$nGrow     = $data[-1].n / $data[0].n
$naiveGrow = $data[-1].naive / $data[0].naive
$bvhGrow   = $data[-1].bvh / $data[0].bvh
$logGrow   = [Math]::Log($data[-1].n, 2) / [Math]::Log($data[0].n, 2)

function Get-LogY([double]$v) {
    $l = [Math]::Log10([Math]::Max($v, 1e-9))
    $t = ($l - $logMin) / ($logMax - $logMin)
    return $p1B - $t * ($p1B - $p1T)
}
function Get-X([int]$i) {
    return $p1L + $i * ($p1R - $p1L) / ($data.Count - 1)
}

# ---- 面板 2：加速比 ----
$p2L = 110; $p2R = 850; $p2T = 470; $p2B = 600

function Get-BarX([int]$i) {
    $slot = ($p2R - $p2L) / $data.Count
    return $p2L + $slot * $i + $slot * 0.20
}
$barW = (($p2R - $p2L) / $data.Count) * 0.60

# ------------------------------------------------------------
# 拼 SVG
# ------------------------------------------------------------
$sb = New-Object System.Text.StringBuilder
$null = $sb.AppendLine('<svg xmlns="http://www.w3.org/2000/svg" width="920" height="700" viewBox="0 0 920 700"')
$null = $sb.AppendLine('     font-family="-apple-system, BlinkMacSystemFont, ''Segoe UI'', ''PingFang SC'', ''Microsoft YaHei'', sans-serif">')
$null = $sb.AppendLine('  <rect width="920" height="700" fill="#0d1117"/>')

# ---------- 标题 ----------
$null = $sb.AppendLine('  <text x="460" y="42" fill="#e6edf3" font-size="21" font-weight="700" text-anchor="middle">Day 11 · 加速结构的复杂度验证</text>')
$null = $sb.AppendLine('  <text x="460" y="68" fill="#8b949e" font-size="13" text-anchor="middle">random_scene · 120×68 · 4 spp · 单线程 · g++ -O3 · 每档取 3 次最小值</text>')

# ---------- 面板 1 ----------
$null = $sb.AppendLine('  <text x="110" y="104" fill="#e6edf3" font-size="15" font-weight="700">① 渲染耗时 vs 场景规模（双对数坐标）</text>')
$null = $sb.AppendLine('  <text x="110" y="122" fill="#8b949e" font-size="11.5">双对数下，直线的斜率 = 复杂度指数：斜率 1 = 线性 O(N)，斜率 0 = 完全不变</text>')

# 图例（横排，放在图表上方右侧）—— 数字由数据算出
$lg = @(
    @('#f85149', ('朴素遍历 · 斜率 {0:N2}' -f $slopeNaive), ''),
    @('#3fb950', ('BVH 求交 · 斜率 {0:N2}' -f $slopeBvh), ''),
    @('#d29922', 'BVH 建树', ' stroke-dasharray="7 4"')
)
$lgx = 400
for ($k = 0; $k -lt $lg.Count; $k++) {
    $null = $sb.AppendLine("  <line x1=""$lgx"" y1=""142"" x2=""$($lgx + 20)"" y2=""142"" stroke=""$($lg[$k][0])"" stroke-width=""2.6""$($lg[$k][2])/>")
    $null = $sb.AppendLine("  <text x=""$($lgx + 26)"" y=""146"" fill=""#c9d1d9"" font-size=""11.5"">$($lg[$k][1])</text>")
    $lgx += 160
}

# 网格 + Y 轴刻度
foreach ($p in @(@(-2.0,'0.01 ms'), @(-1.0,'0.1 ms'), @(0.0,'1 ms'), @(1.0,'10 ms'), @(2.0,'100 ms'), @(3.0,'1000 ms'), @(4.0,'10000 ms'))) {
    $y = Get-LogY ([Math]::Pow(10, $p[0]))
    $y = [Math]::Round($y, 1)
    $dash = if ($p[0] -eq 4.0) { '' } else { ' stroke-dasharray="3 5"' }
    $col  = '#21262d'
    $null = $sb.AppendLine("  <line x1=""$p1L"" y1=""$y"" x2=""$p1R"" y2=""$y"" stroke=""$col"" stroke-width=""1""$dash/>")
    $label = $p[1]
    $null = $sb.AppendLine("  <text x=""$($p1L - 8)"" y=""$([Math]::Round($y + 4, 1))"" fill=""#484f58"" font-size=""11"" text-anchor=""end"">$label</text>")
}
# Y 轴 / X 轴
$null = $sb.AppendLine("  <line x1=""$p1L"" y1=""$p1T"" x2=""$p1L"" y2=""$p1B"" stroke=""#30363d"" stroke-width=""2""/>")
$null = $sb.AppendLine("  <line x1=""$p1L"" y1=""$p1B"" x2=""$p1R"" y2=""$p1B"" stroke=""#30363d"" stroke-width=""2""/>")

# 三条折线
$series = @(
    @{ key='naive'; color='#f85149'; name='朴素遍历' },
    @{ key='bvh';   color='#3fb950'; name='BVH 求交' },
    @{ key='build'; color='#d29922'; name='BVH 建树' }
)
foreach ($s in $series) {
    $pts = @()
    for ($i = 0; $i -lt $data.Count; $i++) {
        $v = $data[$i].($s.key)
        if ($v -le 0) { continue }
        if ([Math]::Log10($v) -lt $logMin) { continue }   # 低于纵轴下界就不画，否则会画出坐标区
        $pts += "$([Math]::Round((Get-X $i),1)),$([Math]::Round((Get-LogY $v),1))"
    }
    $style = if ($s.key -eq 'build') { ' stroke-dasharray="7 4"' } else { '' }
    $null = $sb.AppendLine("  <polyline points=""$($pts -join ' ')"" fill=""none"" stroke=""$($s.color)"" stroke-width=""2.6"" stroke-linejoin=""round""$style/>")
    for ($i = 0; $i -lt $data.Count; $i++) {
        $v = $data[$i].($s.key)
        if ($v -le 0) { continue }
        if ([Math]::Log10($v) -lt $logMin) { continue }
        $x = [Math]::Round((Get-X $i),1); $y = [Math]::Round((Get-LogY $v),1)
        $null = $sb.AppendLine("  <circle cx=""$x"" cy=""$y"" r=""4"" fill=""#0d1117"" stroke=""$($s.color)"" stroke-width=""2.5""/>")
    }
}

# x 轴标签（真实 N 值）
for ($i = 0; $i -lt $data.Count; $i++) {
    $x = [Math]::Round((Get-X $i),1)
    $null = $sb.AppendLine("  <text x=""$x"" y=""$($p1B + 20)"" fill=""#c9d1d9"" font-size=""12.5"" text-anchor=""middle"">$([int]$data[$i].n)</text>")
}
$null = $sb.AppendLine("  <text x=""$([Math]::Round(($p1L + $p1R) / 2, 1))"" y=""$($p1B + 42)"" fill=""#8b949e"" font-size=""12"" text-anchor=""middle"">物体个数 N（横轴按对数等距 = 每档约 ×4）</text>")

# ---------- 面板 2 ----------
$null = $sb.AppendLine('  <text x="110" y="452" fill="#e6edf3" font-size="15" font-weight="700">② 加速比 = 朴素耗时 / BVH 耗时</text>')

# 刻度线
for ($t = 0; $t -le $speedAxisMax; $t += 25) {
    $y = [Math]::Round($p2B - ($t / $speedAxisMax) * ($p2B - $p2T), 1)
    $null = $sb.AppendLine("  <line x1=""$p2L"" y1=""$y"" x2=""$p2R"" y2=""$y"" stroke=""#21262d"" stroke-width=""1"" stroke-dasharray=""3 5""/>")
    $null = $sb.AppendLine("  <text x=""$($p2L - 8)"" y=""$($y + 4)"" fill=""#484f58"" font-size=""11"" text-anchor=""end"">$t×</text>")
}
$null = $sb.AppendLine("  <line x1=""$p2L"" y1=""$p2T"" x2=""$p2L"" y2=""$p2B"" stroke=""#30363d"" stroke-width=""2""/>")
$null = $sb.AppendLine("  <line x1=""$p2L"" y1=""$p2B"" x2=""$p2R"" y2=""$p2B"" stroke=""#30363d"" stroke-width=""2""/>")

# 柱子
for ($i = 0; $i -lt $data.Count; $i++) {
    $v  = $data[$i].speedup
    $bx = [Math]::Round((Get-BarX $i), 1)
    $bh = [Math]::Round(($v / $speedAxisMax) * ($p2B - $p2T), 1)
    $by = [Math]::Round($p2B - $bh, 1)
    $null = $sb.AppendLine("  <rect x=""$bx"" y=""$by"" width=""$([Math]::Round($barW,1))"" height=""$bh"" rx=""5"" fill=""#3fb950"" fill-opacity=""0.85""/>")
    $cx = [Math]::Round($bx + $barW / 2, 1)
    $null = $sb.AppendLine("  <text x=""$cx"" y=""$($by - 10)"" fill=""#3fb950"" font-size=""17"" font-weight=""700"" text-anchor=""middle"">$([Math]::Round($v,1))×</text>")
    $null = $sb.AppendLine("  <text x=""$cx"" y=""$($p2B + 22)"" fill=""#c9d1d9"" font-size=""13"" text-anchor=""middle"">N = $([int]$data[$i].n)</text>")
    $null = $sb.AppendLine("  <text x=""$cx"" y=""$($p2B + 40)"" fill=""#484f58"" font-size=""11"" text-anchor=""middle"">$([Math]::Round($data[$i].naive,0)) / $([Math]::Round($data[$i].bvh,1)) ms</text>")
}

# ---------- 脚注（由数据算出）----------
$foot = ("N 涨 {0:N1} 倍：朴素涨 {1:N1} 倍（斜率 {2:N2} ≈ 线性），BVH 只涨 {3:N2} 倍" +
         "（斜率 {4:N2}，log2 理论值 {5:N2}）") -f $nGrow, $naiveGrow, $slopeNaive, $bvhGrow, $slopeBvh, $logGrow
$null = $sb.AppendLine('  <g transform="translate(110, 664)">')
$null = $sb.AppendLine('    <rect x="0" y="0" width="740" height="26" rx="6" fill="#161b22" stroke="#30363d"/>')
$null = $sb.AppendLine('    <text x="14" y="17.5" fill="#d29922" font-size="12">' + $foot + '</text>')
$null = $sb.AppendLine('  </g>')
$null = $sb.AppendLine('</svg>')

# ------------------------------------------------------------
# 写文件（UTF-8 无 BOM）
# ------------------------------------------------------------
[System.IO.File]::WriteAllText(
    (Join-Path (Get-Location) $outPath),
    $sb.ToString(),
    (New-Object System.Text.UTF8Encoding $false)
)

Write-Host "OK -> $outPath" -ForegroundColor Green
Write-Host ("     {0} rows, max speedup = {1:N1}x" -f $data.Count, $maxSpeedup) -ForegroundColor DarkGray
