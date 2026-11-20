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

# ---- 先把像素数据算出来，然后写两遗 ----
#   ★ 同一份像素、两种编码 —— 这就是 P3 / P6 的【唯一】区别
$cellW = $W / $CELLS
$cellH = $H / $CELLS
$npix  = $W * $H * 3
$px = New-Object 'int[]' $npix
$k = 0
for ($y = 0; $y -lt $H; $y++) {
    $cy = [Math]::Floor($y / $cellH)
    for ($x = 0; $x -lt $W; $x++) {
        $cx = [Math]::Floor($x / $cellW)
        if ((($cx + $cy) % 2) -eq 0) { $c = $light } else { $c = $dark }

        # corner markers (image space: y = 0 is the TOP row)
        if     ($y -lt $MARK -and $x -lt $MARK)             { $c = $red }
        elseif ($y -lt $MARK -and $x -ge ($W - $MARK))      { $c = $green }
        elseif ($y -ge ($H - $MARK) -and $x -lt $MARK)      { $c = $blue }
        elseif ($y -ge ($H - $MARK) -and $x -ge ($W - $MARK)) { $c = $yellow }

        $px[$k++] = $c[0]
        $px[$k++] = $c[1]
        $px[$k++] = $c[2]
    }
}

# ---------------- P3 (ASCII) ----------------
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine("P3")
[void]$sb.AppendLine("$W $H")
[void]$sb.AppendLine("255")
for ($y = 0; $y -lt $H; $y++) {
    $row = New-Object System.Text.StringBuilder
    for ($x = 0; $x -lt $W; $x++) {
        $o = ($y * $W + $x) * 3
        [void]$row.Append("$($px[$o]) $($px[$o+1]) $($px[$o+2]) ")
    }
    [void]$sb.AppendLine($row.ToString().TrimEnd())
}
$out = Join-Path $PWD 'models\uvgrid.ppm'
[System.IO.File]::WriteAllText($out, $sb.ToString(), (New-Object System.Text.UTF8Encoding($false)))

# ---------------- P6 (binary) ----------------
#   header is pure ASCII; then exactly ONE whitespace byte; then raw data
function Write-P6([string]$path, [int]$pw, [int]$ph, [int]$maxval, [int[]]$data) {
    $bps = 1
    if ($maxval -gt 255) { $bps = 2 }        # >255 -> 2 bytes per sample
    $hdr  = "P6`n$pw $ph`n$maxval`n"
    $hb   = [System.Text.Encoding]::ASCII.GetBytes($hdr)
    $body = New-Object 'byte[]' ($pw * $ph * 3 * $bps)
    $j = 0
    for ($i = 0; $i -lt $data.Length; $i++) {
        if ($bps -eq 1) {
            $body[$j++] = [byte]$data[$i]
        } else {
            # BIG-ENDIAN: most significant byte first
            $body[$j++] = [byte](($data[$i] -shr 8) -band 0xFF)
            $body[$j++] = [byte]($data[$i] -band 0xFF)
        }
    }
    $all = New-Object 'byte[]' ($hb.Length + $body.Length)
    [Array]::Copy($hb,   0, $all, 0,           $hb.Length)
    [Array]::Copy($body, 0, $all, $hb.Length,  $body.Length)
    [System.IO.File]::WriteAllBytes($path, $all)
    return $all.Length
}

$null = Write-P6 (Join-Path $PWD 'models\uvgrid_p6.ppm') $W $H 255 $px

# ---- tiny P6 fixtures for the loader test ----
#   maxval = 15  -> 1 byte per sample
$p15 = @(  0,  1,  2,   3,  4,  5,   6,  7,  8,   9, 10, 11,
          12, 13, 14,  15,  0,  1,   2,  3,  4,   5,  6,  7)
$null = Write-P6 (Join-Path $PWD 'models\p6_max15.ppm') 4 2 15 $p15

#   maxval = 65535 -> 2 bytes per sample, big-endian.
#   Values 256 and 1 are deliberate: read with the wrong endianness they swap.
$p16 = @(65535,    0,  256,    1,  257,    2,  512,    3,  513,    4, 1024,    5,
         65535,  255,    1,   16,   17,65535,   32,   33,   34,   64,  128,  256)
$null = Write-P6 (Join-Path $PWD 'models\p6_max65535.ppm') 4 2 65535 $p16

#   truncated: header promises 8x8 (=192 data bytes) but only 100 are present
$truncHdr = [System.Text.Encoding]::ASCII.GetBytes("P6`n8 8`n255`n")
$truncAll = New-Object 'byte[]' ($truncHdr.Length + 100)
[Array]::Copy($truncHdr, 0, $truncAll, 0, $truncHdr.Length)
for ($i = 0; $i -lt 100; $i++) { $truncAll[$truncHdr.Length + $i] = [byte](($i * 7) -band 0xFF) }
[System.IO.File]::WriteAllBytes((Join-Path $PWD 'models\p6_truncated.ppm'), $truncAll)

$fi = Get-Item $out
Write-Host ("OK -> models/uvgrid.ppm  {0}x{1}  {2:N0} bytes" -f $W, $H, $fi.Length) -ForegroundColor Green
Get-ChildItem models\*.ppm | ForEach-Object {
    "  {0,-22} {1,9:N0} B" -f $_.Name, $_.Length
}
