# ============================================================
#  Day 30 : phase-1 "graduation" assets
#
#    1) hero still : 1920x1080 @ 1024 spp  -> output/hero.ppm
#    2) video      : 960x540 @ 12 spp, 20 fps, 4 shots x 15 s = 1200 frames
#                                         -> output/frames/frame_XXXX.ppm
#
#  Why is the video lower resolution than the still?
#    1200 frames x per-frame cost. 1920x1080 costs 4x per spp compared
#    to 960x540, i.e. about 4 hours for the same sample count. Once the
#    image is moving, resolution matters far less than the still, so the
#    budget goes into "enough frames + acceptable noise".
#
#  Measured (see docs/devlog.md Day 30):
#    Cornell 1920x1080 : 0.936 s/spp
#    Cornell  960x540  : 0.241 s/spp
#
#  NOTE: keep this file ASCII-only. PowerShell 5.1 decodes a BOM-less
#        UTF-8 script as GBK, and a Chinese character can swallow the
#        closing quote -> "TerminatorExpectedAtEndOfString".
# ============================================================
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)

$exe   = '.\build\raytracer.exe'
$total = [Diagnostics.Stopwatch]::StartNew()
$log   = 'output\day30_render.log'
if (-not (Test-Path 'output')) { New-Item -ItemType Directory -Force 'output' | Out-Null }

function Stamp { param($msg)
    Write-Host ("[{0,6:N1} min] {1}" -f $total.Elapsed.TotalMinutes, $msg)
}

# ---------------- 1) hero still ----------------
if (-not (Test-Path 'output\hero.ppm')) {
    Stamp 'rendering hero still 1920x1080 @ 1024 spp (~16 min)'
    # NOTE: must go through cmd.exe. PowerShell 5.1 turns a native program's
    #       stderr into an ErrorRecord, and with ErrorActionPreference=Stop
    #       that becomes a terminating error -> the script dies mid-render.
    cmd /c "`"$exe`" --cornell-hero --spp 1024 --width 1920 --height 1080 --out output\hero.ppm 2>> $log"
    if ($LASTEXITCODE -ne 0) { throw 'hero still failed' }
    Stamp ("hero still done ({0:N1} MB)" -f ((Get-Item output\hero.ppm).Length / 1MB))
} else {
    Stamp 'hero still already exists, skip'
}

# ---------------- 2) video frames ----------------
$W = 960; $H = 540; $SPP = 12; $N = 300; $FPS = 20

$shots = @(
    @{ name = 'cornell-dolly';      args = @('--cornell-hero',    '--anim', 'dolly') },
    @{ name = 'torus-normal-orbit'; args = @('--obj-torus-normal','--anim', 'orbit') },
    @{ name = 'cube-img-orbit';     args = @('--obj-cube-img',    '--anim', 'orbit') },
    @{ name = 'ico-orbit';          args = @('--obj-ico',         '--anim', 'orbit') }
)

if (-not (Test-Path 'output\frames\frame_1199.ppm')) {
    if (Test-Path 'output\frames') { Remove-Item 'output\frames' -Recurse -Force }

    $start = 0
    foreach ($s in $shots) {
        Stamp ("shot '{0}' : {1} frames" -f $s.name, $N)
        $argLine = (@($s.args) + @('--frames', $N, '--frame-start', $start,
                                   '--spp', $SPP, '--width', $W, '--height', $H,
                                   '--outdir', 'output\frames')) -join ' '
        cmd /c "`"$exe`" $argLine 2>> $log"
        if ($LASTEXITCODE -ne 0) { throw "shot $($s.name) failed" }
        $start += $N
    }
    Stamp 'all frames done'
} else {
    Stamp 'video frames already exist, skip'
}

$have = @(Get-ChildItem 'output\frames' -Filter 'frame_*.ppm').Count
Stamp ("frames = {0} (expect {1}), duration {2:N1} s" -f $have, (4 * $N), ($have / [double]$FPS))
Stamp 'next: .\tools\make_video.ps1 -Frames output\frames -Out gallery\day30-demo.mp4 -Fps 20 -Denoise'
