# YJJDefense 자동화 테스트(스펠 조합, 원소 반응, 한 판 진행, 수집)를 헤드리스로 실행하고 결과를 요약한다.
# 에디터를 끈 상태에서 실행: powershell -File Tools/RunTests.ps1
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [string]$Filter = "YJJDefense"
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
$Log = Join-Path $ProjectDir "Saved\Logs\RunTests.log"
New-Item -ItemType Directory -Force (Split-Path $Log) | Out-Null

& $Editor $Project -ExecCmds="Automation RunTests $Filter;Quit" -unattended -nosplash -nop4 -nullrhi -NoSound -stdout -FullStdOutLogOutput *> $Log

$Results = Select-String -Path $Log -Encoding UTF8 -Pattern "Test Completed\. Result=\{(\w+)\} Name=\{([^}]*)\}" | ForEach-Object {
    [pscustomobject]@{ Result = $_.Matches[0].Groups[1].Value; Name = $_.Matches[0].Groups[2].Value }
}
$Results | Format-Table -AutoSize

# 밸런스 확인용 수치 (FullRun 테스트가 남긴다)
Select-String -Path $Log -Encoding UTF8 -Pattern "LogAutomationController: .*(스펠 |단계: )|LogAutomationController: Error: Expected" | ForEach-Object { $_.Line } | Select-Object -First 40

$Failed = @($Results | Where-Object { $_.Result -ne "Success" }).Count
if ($Results.Count -eq 0) { Write-Host "No tests ran. See $Log"; exit 2 }
Write-Host "$($Results.Count) test(s), $Failed failed. Log: $Log"
exit [int]($Failed -gt 0)
