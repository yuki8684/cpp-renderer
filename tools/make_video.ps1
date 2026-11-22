# ============================================================
#  Day 30 : frame sequence -> MP4
#
#  usage:
#    .\tools\make_video.ps1 -Frames output\frames -Out gallery\day30-demo.mp4
#    .\tools\make_video.ps1 -Frames output\frames -Out x.mp4 -Fps 20 -Denoise
#
#  key points:
#    1) input = "-framerate <fps> -i frame_%04d.ppm"
#       without -framerate ffmpeg reads at 25 fps and the clip runs fast/slow
#    2) -pix_fmt yuv420p is mandatory
#       H.264 with high bit depth / 4:4:4 will not play in many players
#    3) scale with lanczos: sharper than the default bilinear when upscaling
#    4) -crf 18 is close to visually lossless; smaller = better quality/bigger file
#    5) hqdn3d is a DENOISER. Two knobs, and they behave very differently:
#         luma_spatial : chroma_spatial : luma_tmp : chroma_tmp
#       The TEMPORAL part averages across neighbouring frames -- that only works
#       when the image is static. With a moving camera hqdn3d's motion detector
#       treats everything as motion and switches temporal filtering off, so a
#       temporal-only setting barely helps. What actually cleans up a camera-
#       animated path-traced clip is the SPATIAL part.
#       Measured on this project (60 s, 1200 frames):
#         hqdn3d=2:1:40:25   -> 32.6 MB and still grainy
#         hqdn3d=40:32:60:40 ->  6.0 MB and clean (slightly soft)
#       Not a cheat: real pipelines use denoisers (OIDN / NRD) the same way.
#
#  NOTE: keep this file ASCII-only (see day30_render.ps1 for the reason).
# ============================================================
param(
    [Parameter(Mandatory = $true)][string]$Frames,
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$Fps    = 20,
    [int]$Width  = 1280,
    [int]$Height = 720,
    [switch]$Denoise
)

$ErrorActionPreference = 'Stop'

# ---------- locate ffmpeg ----------
function Find-Ffmpeg {
    $cmd = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # winget installs are not necessarily on the current shell's PATH yet
    $root = Join-Path $env:LOCALAPPDATA 'Microsoft\WinGet\Packages'
    if (Test-Path $root) {
        $exe = Get-ChildItem $root -Recurse -Filter ffmpeg.exe -ErrorAction SilentlyContinue |
               Select-Object -First 1
        if ($exe) { return $exe.FullName }
    }
    throw 'ffmpeg not found. install: winget install --id Gyan.FFmpeg -e'
}

$ffmpeg = Find-Ffmpeg
Write-Host "[1/3] ffmpeg = $ffmpeg"

# ---------- sanity check ----------
if (-not (Test-Path $Frames)) { throw "frame dir not found: $Frames" }
$count = @(Get-ChildItem $Frames -Filter 'frame_*.ppm').Count
if ($count -eq 0) { throw "no frame_*.ppm in: $Frames" }

$seconds = $count / [double]$Fps
Write-Host ("[2/3] {0} frames @ {1} fps -> {2:N1} s" -f $count, $Fps, $seconds)

$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }

# ---------- encode ----------
$vf = "scale=${Width}:${Height}:flags=lanczos"
if ($Denoise) { $vf = "hqdn3d=40:32:60:40," + $vf }

# NOTE: go through cmd.exe. PowerShell 5.1 turns a native program's stderr into
#       an ErrorRecord, and with ErrorActionPreference=Stop that terminates the
#       script -- ffmpeg writes its banner to stderr, so this fires immediately.
$argLine = (@('-y', '-framerate', "$Fps",
              '-i', ('"' + (Join-Path $Frames 'frame_%04d.ppm') + '"'),
              '-vf', ('"' + $vf + '"'),
              '-c:v', 'libx264', '-preset', 'slow', '-crf', '23',
              '-pix_fmt', 'yuv420p', '-movflags', '+faststart',
              ('"' + $Out + '"')) -join ' ')

Write-Host '[3/3] encoding...'
cmd /c "`"$ffmpeg`" $argLine 2>&1" | Select-Object -Last 6 | Out-Host

if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed, exit $LASTEXITCODE" }

$size = (Get-Item $Out).Length / 1MB
Write-Host ("done -> {0}  ({1:N1} MB, {2:N1} s)" -f $Out, $size, $seconds)
