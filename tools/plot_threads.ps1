# ============================================================
# plot_threads.ps1 -- measure thread scaling and emit an SVG chart
#
#   .\tools\plot_threads.ps1                  # default: 64 spp, threads 1,2,4,8,16,24
#   .\tools\plot_threads.ps1 -Spp 128
#   .\tools\plot_threads.ps1 -Threads 1,2,4,8
#
# Output: gallery/day18-thread-scaling.svg
#
# NOTE: this file is deliberately ASCII-only.
#       PowerShell 5.1 decodes BOM-less UTF-8 scripts as GBK, which garbles
#       non-ASCII string literals. Keeping it ASCII removes the whole class
#       of problem. (Same reason compare_crop.ps1 uses English labels.)
# ============================================================
param(
    [int]   $Spp     = 64,
    [int[]] $Threads = @(1, 2, 4, 8, 16, 24)
)

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$exe = '.\build\raytracer.exe'
if (-not (Test-Path $exe)) {
    Write-Host "ERROR: $exe not found. Build first." -ForegroundColor Red
    exit 1
}

$cpuName = (Get-CimInstance Win32_Processor).Name
$cores   = (Get-CimInstance Win32_Processor).NumberOfCores
$logical = (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors

# ---------- 1. measure ----------
$rows = @()
foreach ($n in $Threads) {
    # The renderer writes its progress to stderr. With ErrorActionPreference='Stop'
    # PowerShell turns native stderr into a terminating error, so relax it here.
    $ErrorActionPreference = 'Continue'
    $t = Measure-Command { & $exe --cornell --spp $Spp --threads $n 2>&1 | Out-Null }
    $ErrorActionPreference = 'Stop'
    $rows += [pscustomobject]@{
        Threads = $n
        Sec     = [double]$t.TotalSeconds
    }
    Write-Host ("  threads={0,-4} {1,8:N2} s" -f $n, $t.TotalSeconds)
}

$base = ($rows | Where-Object { $_.Threads -eq 1 }).Sec
foreach ($r in $rows) {
    $r | Add-Member -NotePropertyName Speedup -NotePropertyValue ($base / $r.Sec)
}

$maxT   = ($rows | Measure-Object -Property Threads -Maximum).Maximum
$maxS   = ($rows | Measure-Object -Property Speedup -Maximum).Maximum
$maxSec = ($rows | Measure-Object -Property Sec     -Maximum).Maximum

# ---------- 2. geometry ----------
$W = 960; $H = 480
$ax0 = 80;  $ay0 = 380; $aw = 360; $ah = 240     # panel A plot box
$bx0 = 570; $by0 = 380; $bw = 330; $bh = 240     # panel B plot box

function AX($t) { return $ax0 + (($t - 1) / [double][math]::Max(1, ($maxT - 1))) * $aw }
function AY($s) { return $ay0 - ($s / [double][math]::Max(1, $maxT)) * $ah }
function BX($i) { return $bx0 + $bw / 2.0 / $rows.Count + $i * ($bw / $rows.Count) }
function BY($v) { return $by0 - ($v / [double][math]::Max(1e-9, $maxSec)) * $bh }

$ideal = @()
$real  = @()
for ($i = 0; $i -lt $rows.Count; $i++) {
    $r = $rows[$i]
    $ideal += ('{0:N1},{1:N1}' -f (AX $r.Threads), (AY $r.Threads))
    $real  += ('{0:N1},{1:N1}' -f (AX $r.Threads), (AY $r.Speedup))
}

$bars = ''
for ($i = 0; $i -lt $rows.Count; $i++) {
    $r = $rows[$i]
    $x = BX $i
    $y = BY $r.Sec
    # NOTE: do NOT name this $h -- PowerShell variables are case-INSENSITIVE,
    #       so $h would silently overwrite $H (the canvas height).
    $barHeight = $by0 - $y
    $bars += ('<rect x="{0:N1}" y="{1:N1}" width="34" height="{2:N1}" rx="3" fill="#f0883e" opacity="0.85"/>' -f ($x - 17), $y, $barHeight)
    $bars += ('<text x="{0:N1}" y="{1:N1}" font-size="11" fill="#c9d1d9" text-anchor="middle">{2:N2}s</text>' -f $x, ($y - 6), $r.Sec)
}

$dots = ''
for ($i = 0; $i -lt $rows.Count; $i++) {
    $r = $rows[$i]
    $dots += ('<circle cx="{0:N1}" cy="{1:N1}" r="4.5" fill="#f0883e" stroke="#0d1117" stroke-width="1.5"/>' -f (AX $r.Threads), (AY $r.Speedup))
    $dots += ('<text x="{0:N1}" y="{1:N1}" font-size="11" fill="#f0883e" text-anchor="middle">{2:N2}x</text>' -f (AX $r.Threads), ((AY $r.Speedup) - 11), $r.Speedup)
}

$xLabels = ''
foreach ($r in $rows) {
    $xLabels += ('<text x="{0:N1}" y="{1:N1}" font-size="12" fill="#8b949e" text-anchor="middle">{2}</text>' -f (AX $r.Threads), ($ay0 + 20), $r.Threads)
}

$bxLabels = ''
for ($i = 0; $i -lt $rows.Count; $i++) {
    $bxLabels += ('<text x="{0:N1}" y="{1:N1}" font-size="12" fill="#8b949e" text-anchor="middle">{2}</text>' -f (BX $i), ($by0 + 20), $rows[$i].Threads)
}

# ---------- 3. emit SVG ----------
$svg = @"
<svg xmlns="http://www.w3.org/2000/svg" width="$W" height="$H" viewBox="0 0 $W $H">
  <rect width="$W" height="$H" fill="#0d1117"/>

  <text x="40" y="42" font-size="21" font-weight="700" fill="#e6edf3" font-family="Segoe UI, Arial">Multithread scaling &#8212; Cornell Box $($Spp) spp, 400x225</text>
  <text x="40" y="66" font-size="12.5" fill="#8b949e" font-family="Segoe UI, Arial">$cpuName &#183; $cores physical / $logical logical cores &#183; image is byte-identical for every thread count</text>

  <!-- ============ panel A : speedup ============ -->
  <text x="$ax0" y="116" font-size="14.5" font-weight="600" fill="#e6edf3" font-family="Segoe UI, Arial">Speedup vs threads</text>

  <line x1="$ax0" y1="$ay0" x2="$(($ax0 + $aw))" y2="$ay0" stroke="#30363d" stroke-width="1"/>
  <line x1="$ax0" y1="$ay0" x2="$ax0" y2="$(($ay0 - $ah))" stroke="#30363d" stroke-width="1"/>

  <polyline points="$($ideal -join ' ')" fill="none" stroke="#484f58" stroke-width="2" stroke-dasharray="6,5"/>
  <polyline points="$($real  -join ' ')" fill="none" stroke="#f0883e" stroke-width="2.5"/>
  $dots
  $xLabels

  <text x="$(($ax0 + 10))" y="$(($ay0 - $ah + 20))" font-size="11.5" fill="#8b949e" text-anchor="start" font-family="Segoe UI, Arial">ideal (speedup = threads)</text>
  <text x="$ax0" y="$(($ay0 + 46))" font-size="12" fill="#8b949e" text-anchor="middle" font-family="Segoe UI, Arial">threads</text>
  <text x="46" y="$(($ay0 - $ah / 2))" font-size="12" fill="#8b949e" text-anchor="middle" font-family="Segoe UI, Arial" transform="rotate(-90 46 $(($ay0 - $ah / 2)))">speedup</text>

  <!-- ============ panel B : wall time ============ -->
  <text x="$bx0" y="116" font-size="14.5" font-weight="600" fill="#e6edf3" font-family="Segoe UI, Arial">Wall time vs threads</text>

  <line x1="$bx0" y1="$by0" x2="$(($bx0 + $bw))" y2="$by0" stroke="#30363d" stroke-width="1"/>
  <line x1="$bx0" y1="$by0" x2="$bx0" y2="$(($by0 - $bh))" stroke="#30363d" stroke-width="1"/>
  $bars
  $bxLabels

  <text x="$(($bx0 + $bw / 2))" y="$(($by0 + 46))" font-size="12" fill="#8b949e" text-anchor="middle" font-family="Segoe UI, Arial">threads</text>

  <!-- ============ footnote ============ -->
  <text x="40" y="$(($H - 18))" font-size="12" fill="#8b949e" font-family="Segoe UI, Arial">Not linear because: single-core boost clock (~5.2 GHz) drops under all-core load, P/E hybrid cores, SMT shares execution units, memory bandwidth is shared.</text>
</svg>
"@

$out = 'gallery/day18-thread-scaling.svg'
Set-Content -Path $out -Value $svg -Encoding UTF8
Write-Host ""
Write-Host "OK -> $out" -ForegroundColor Green
