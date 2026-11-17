# 게임을 창 모드로 띄워 오토플레이 봇이 한 판(또는 여러 판)을 플레이하게 하고, 로그 요약과 스크린샷을 남긴다.
# 스크린샷: Saved/Screenshots/AutoPlay_*.png
# -Fresh 를 주면 저장 데이터(수집 진행)를 지우고 처음부터 시작한다.
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$Runs = 1,
    [double]$Speed = 4,
    [double]$ShotInterval = 45,
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [switch]$Fresh,
    [switch]$Sound
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunSmoke.log"

Remove-Item (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_*.png") -ErrorAction SilentlyContinue
# 실제 진행 상황(YJJDefense.sav)은 건드리지 않고 전용 슬롯을 쓴다
if ($Fresh) { Remove-Item (Join-Path $ProjectDir "Saved\SaveGames\Smoke.sav") -ErrorAction SilentlyContinue }

$GameArgs = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-unattended", "-YJJSave=Smoke",
    "-ExecCmds=`"yjj.AutoPlay 1, yjj.AutoPlaySpeed $Speed, yjj.AutoPlayExit $Runs, yjj.AutoPlayShotInterval $ShotInterval`"", "-log", "-abslog=`"$Log`""
)
if (-not $Sound) { $GameArgs += "-NoSound" }

$Process = Start-Process -FilePath $Editor -ArgumentList $GameArgs -PassThru
if (-not $Process.WaitForExit((600 * $Runs + 300) * 1000)) {
    $Process.Kill()
    Write-Host "Timed out"
}

Select-String -Path $Log -Encoding UTF8 -Pattern "\[AutoPlay\]|\[Run\]|Error:|Ensure condition failed|Fatal" | ForEach-Object { $_.Line }
Get-ChildItem (Join-Path $ProjectDir "Saved\Screenshots\AutoPlay_*.png") -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
