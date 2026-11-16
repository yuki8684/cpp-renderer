# ============================================================
# gen_texture.ps1 -- generate test texture models/uvgrid.ppm
#
#   .\tools\gen_texture.ps1
#
# Why a corner-marked grid instead of a plain checker:
#   Four DIFFERENT corner colours tell you immediately whether the
#   UV is flipped or mirrored. A symmetric checker cannot.
#
#   image space (row 0 = TOP of the picture)
#     +---------------------------+
#     |  RED              GREEN   |   <- v = 1  (after the 1-v flip)
#     |                           |
#     |  BLUE            YELLOW   |   <- v = 0
#     +---------------------------+
#      u=0                     u=1
#
# ASCII only: PowerShell 5.1 decodes BOM-less UTF-8 as GBK.
# ============================================================

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$W = 256
$H = 256
$CELLS = 8                      # checker cells across
$MARK = 40                      # corner marker size in pixels

$light  = @(205, 205, 200)
$dark   = @( 75,  80,  90)
$red    = @(230,  40,  40)
$green  = @( 40, 200,  70)
$blue   = @( 40,  90, 230)
$yellow = @(240, 200,  40)

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("P3")
[void]$sb.AppendLine("$W $H")
[void]$sb.AppendLine("255")

$cellW = $W / $CELLS
$cellH = $H / $CELLS

for ($y = 0; $y -lt $H; $y++) {
    $row = New-Object System.Text.StringBuilder
    $cy = [Math]::Floor($y / $cellH)
    for ($x = 0; $x -lt $W; $x++) {
        $cx = [Math]::Floor($x / $cellW)
        if ((($cx + $cy) % 2) -eq 0) { $c = $light } else { $c = $dark }

        # corner markers (image space: y = 0 is the TOP row)
        if     ($y -lt $MARK -and $x -lt $MARK)             { $c = $red }
        elseif ($y -lt $MARK -and $x -ge ($W - $MARK))      { $c = $green }
        elseif ($y -ge ($H - $MARK) -and $x -lt $MARK)      { $c = $blue }
        elseif ($y -ge ($H - $MARK) -and $x -ge ($W - $MARK)) { $c = $yellow }

        [void]$row.Append("$($c[0]) $($c[1]) $($c[2]) ")
    }
    [void]$sb.AppendLine($row.ToString().TrimEnd())
}

$out = Join-Path $PWD 'models\uvgrid.ppm'
[System.IO.File]::WriteAllText($out, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))

$fi = Get-Item $out
Write-Host ("OK -> models/uvgrid.ppm  {0}x{1}  {2:N0} bytes" -f $W, $H, $fi.Length) -ForegroundColor Green
