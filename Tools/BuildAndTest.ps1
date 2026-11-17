# 에디터 타깃을 빌드하고 자동화 테스트를 돌린다 (에디터를 끈 상태에서).
# powershell -File Tools/BuildAndTest.ps1 [-NoTest]
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [switch]$NoTest
)

$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$BuildLog = Join-Path $ProjectDir "Saved\Logs\Build.log"
New-Item -ItemType Directory -Force (Split-Path $BuildLog) | Out-Null

& (Join-Path $EngineDir "Build\BatchFiles\Build.bat") YJJDefenseEditor Win64 Development "-project=$Project" -WaitMutex *> $BuildLog
$BuildExit = $LASTEXITCODE
Select-String -Path $BuildLog -Encoding UTF8 -Pattern "error |error:|warning C|Result:|Total execution" | Select-Object -First 40 | ForEach-Object { $_.Line }
if ($BuildExit -ne 0) { Write-Host "Build failed. Log: $BuildLog"; exit $BuildExit }

if ($NoTest) { exit 0 }
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "RunTests.ps1") -EngineDir $EngineDir
exit $LASTEXITCODE
