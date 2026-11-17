# ============================================================
# gen_normal_map.ps1 -- generate models/bumps_normal.ppm
#
#   .\tools\gen_normal_map.ps1
#
# A 4x4 grid of round "bumps", stored in TANGENT SPACE:
#   R -> +T   (tangent,  along  +u)
#   G -> +B   (bitangent, along +v)
#   B -> +N   (geometric normal, "out of the surface")
#
# Encoding: 0..1  where 0.5 == 0   (so flat surface = (128,128,128))
#
# Inside a bump:  n = (a, b, sqrt(1-a^2-b^2))  with (a,b) the local offset
# Outside a bump: n = (0, 0, 1)  -> flat
#
# !! This file is DATA, not a colour. It must be loaded with srgb = false,
# !! otherwise the decode skews every normal and you get weird hard edges.
#
# ASCII only: PowerShell 5.1 decodes BOM-less UTF-8 as GBK.
# ============================================================

$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$W = 256
$H = 256
$CELLS = 4                      # 4x4 bumps

$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("P3")
[void]$sb.AppendLine("$W $H")
[void]$sb.AppendLine("255")

$cellW = $W / $CELLS
$cellH = $H / $CELLS

for ($y = 0; $y -lt $H; $y++) {
    $row = New-Object System.Text.StringBuilder
    for ($x = 0; $x -lt $W; $x++) {
        # local coordinates inside this cell, mapped to [-1, 1]
        $fx = (($x % $cellW) / $cellW) * 2.0 - 1.0
        $fy = (($y % $cellH) / $cellH) * 2.0 - 1.0

        # distance from cell centre; scale so the bump nearly fills the cell
        $r2 = $fx * $fx + $fy * $fy
        if ($r2 -lt 0.81) {
            # inside the round bump: hemisphere normal
            # note: image y grows DOWNWARD, and we want +B along +v,
            # so the green channel uses -fy (see docs/devlog.md Day 25)
            $nx = $fx
            $ny = -$fy
            $nz = [Math]::Sqrt([Math]::Max(0.0, 1.0 - $r2))
        } else {
            # outside: flat normal
            $nx = 0.0
            $ny = 0.0
            $nz = 1.0
        }

        $r = [int][Math]::Round(($nx * 0.5 + 0.5) * 255)
        $g = [int][Math]::Round(($ny * 0.5 + 0.5) * 255)
        $b = [int][Math]::Round(($nz * 0.5 + 0.5) * 255)

        [void]$row.Append("$r $g $b ")
    }
    [void]$sb.AppendLine($row.ToString().TrimEnd())
}

$out = Join-Path $PWD 'models\bumps_normal.ppm'
[System.IO.File]::WriteAllText($out, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))

$fi = Get-Item $out
Write-Host ("OK -> models/bumps_normal.ppm  {0}x{1}  {2:N0} bytes" -f $W, $H, $fi.Length) -ForegroundColor Green
