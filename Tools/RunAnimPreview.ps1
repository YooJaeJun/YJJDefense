# 자세히 움직이는 동작(Content/YJJDefense/Data/Anims.csv)을 게임 화면에서 서 있기·걷기·공격으로 나란히 돌려 본다.
# 그냥 실행하면 창을 띄워 두고(닫으면 끝), -Capture 를 주면 오토플레이 위에서 짧은 간격으로 스크린샷을 찍고 스스로 끝낸다.
#   결과: Saved/Screenshots/AutoPlay_Battle_*.png
# 게임 중 콘솔에서 yjj.AnimPreview 1 / 0 으로 켜고 끌 수도 있다.
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$ResX = 1920,
    [int]$ResY = 1080,
    [switch]$Capture,
    [double]$ShotInterval = 0.05,
    [int]$Shots = 60,
    [int]$CaptureSeconds = 45
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunAnimPreview.log"

$Commands = "yjj.AnimPreview 1"
if ($Capture) {
    Remove-Item (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_*.png") -ErrorAction SilentlyContinue
    Remove-Item (Join-Path $ProjectDir "Saved\SaveGames\AnimPreview.sav") -ErrorAction SilentlyContinue
    $Commands += ", yjj.AutoPlay 1, yjj.AutoPlaySpeed 1, yjj.AutoPlayShotInterval $ShotInterval, yjj.AutoPlayShotStart 2, yjj.AutoPlayShotCount $Shots"
}
$GameArgs = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-NoSound", "-YJJSave=AnimPreview",
    "-ExecCmds=`"$Commands`"", "-log", "-abslog=`"$Log`""
)
$Process = Start-Process -FilePath $Editor -ArgumentList $GameArgs -PassThru
if (-not $Capture) {
    exit 0
}
if (-not $Process.WaitForExit($CaptureSeconds * 1000)) {
    Stop-Process -Id $Process.Id -Force
}
Get-ChildItem (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_Battle_*.png") -ErrorAction SilentlyContinue | Measure-Object | ForEach-Object { "screenshots: $($_.Count)" }
