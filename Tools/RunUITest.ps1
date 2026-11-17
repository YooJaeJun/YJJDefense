# 게임을 띄워 마우스 조작을 흉내 내는 UI 테스트를 돌린다 (건설 메뉴, 룬 끌어다 꽂기, 스펠 조준·시전 등).
# 결과: [UITest] PASS/FAIL 로그 + Saved/Screenshots/UITest_*.png. 전용 저장 슬롯을 쓰므로 실제 진행 상황은 그대로다.
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [int]$ResX = 1280,
    [int]$ResY = 720
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunUITest.log"

Remove-Item (Join-Path $ProjectDir "Saved\Screenshots\UITest_*.png") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $ProjectDir "Saved\SaveGames\UITest.sav") -ErrorAction SilentlyContinue

$GameArgs = @(
    "`"$Project`"", "-game", "-windowed", "-ResX=$ResX", "-ResY=$ResY", "-nosplash", "-unattended", "-NoSound", "-YJJSave=UITest",
    "-ExecCmds=`"yjj.UITest 1`"", "-log", "-abslog=`"$Log`""
)
$Process = Start-Process -FilePath $Editor -ArgumentList $GameArgs -PassThru
if (-not $Process.WaitForExit(300 * 1000)) {
    $Process.Kill()
    Write-Host "Timed out"
}

$Lines = Select-String -Path $Log -Encoding UTF8 -Pattern "\[UITest\]|Error:|Ensure condition failed|Fatal" | ForEach-Object { $_.Line -replace "^\[[^\]]*\]\[[^\]]*\]", "" }
$Lines
Get-ChildItem (Join-Path $ProjectDir "Saved\Screenshots\UITest_*.png") -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName }
$Finished = @($Lines | Where-Object { $_ -match "finished passed=\d+ failed=0" }).Count
exit [int]($Finished -eq 0)
