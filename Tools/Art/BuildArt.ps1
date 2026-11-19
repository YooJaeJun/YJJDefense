# 그림(PNG)과 소리(WAV)를 생성하고 언리얼 에셋으로 임포트한다.
# 에디터를 끈 상태에서 실행: powershell -File Tools/Art/BuildArt.ps1
# -SkipImport 를 주면 PNG/WAV와 지도 모양(Content/YJJDefense/Data/MapGeometry.csv)만 다시 만든다.
param(
    [string]$EngineDir = "D:\source\repos\UnrealEngine\Engine",
    [switch]$SkipImport
)

$ErrorActionPreference = "Stop"
$ProjectDir = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$Project = Join-Path $ProjectDir "YJJDefense.uproject"
$Editor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
$Venv = Join-Path $PSScriptRoot ".venv"
$Python = Join-Path $Venv "Scripts\python.exe"

# 그림은 Pillow로 그린다. 처음 한 번 전용 가상환경을 만든다 (git 제외)
if (-not (Test-Path $Python)) {
    python -m venv $Venv
    if ($LASTEXITCODE -ne 0) { throw "python -m venv failed (Python 3 필요)" }
    & $Python -m pip install --quiet --disable-pip-version-check pillow
    if ($LASTEXITCODE -ne 0) { throw "pip install pillow failed" }
}

& $Python (Join-Path $PSScriptRoot "generate_art.py") (Join-Path $PSScriptRoot "Generated") (Join-Path $ProjectDir "Content\YJJDefense\Data\MapGeometry.csv")
if ($LASTEXITCODE -ne 0) { throw "generate_art.py failed" }
& $Python (Join-Path $PSScriptRoot "..\Audio\generate_audio.py") (Join-Path $PSScriptRoot "..\Audio\Generated")
if ($LASTEXITCODE -ne 0) { throw "generate_audio.py failed" }

if ($SkipImport) { exit 0 }

$Script = (Join-Path $PSScriptRoot "import_assets.py").Replace('\', '/')
& $Editor $Project -run=pythonscript -script="$Script" -unattended -nosplash -nop4 -nullrhi -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) { throw "import_assets.py failed ($LASTEXITCODE)" }
