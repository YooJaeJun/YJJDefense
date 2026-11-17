# YJJDefense 도구

엔진: `D:\source\repos\UnrealEngine` (UE 5.8.1 소스 빌드, YJJJump와 같은 엔진). 모두 에디터를 끈 상태에서 실행한다.

| 스크립트 | 하는 일 |
|---|---|
| `Art/BuildArt.ps1` | `Art/generate_art.py`로 그림 PNG + 맵 데이터(`Source/.../Game/YJJMap01.inl`), `Audio/generate_audio.py`로 소리 WAV 생성 → `Art/import_assets.py`로 언리얼 에셋 임포트. `-SkipImport`면 생성만 |
| `BuildAndTest.ps1` | 에디터 타깃 빌드 + 자동화 테스트 (`-NoTest`면 빌드만) |
| `RunTests.ps1` | `YJJDefense.*` 자동화 테스트(스펠 조합, 원소 반응, 타워, 한 판 전체·밸런스, 수집) |
| `RunUITest.ps1` | 게임을 띄워 Slate에 마우스·키 이벤트를 넣으며 UI 19단계 확인 → `Saved/Screenshots/UITest_*.png` |
| `RunSmoke.ps1` | 봇이 한 판(`-Runs N`)을 플레이 → 로그 요약 + `Saved/Screenshots/AutoPlay_*.png`. `-Fresh`면 수집 진행 초기화 |

UI 테스트·스모크는 `-YJJSave=UITest/Smoke` 전용 저장 슬롯을 써서 실제 진행(`Saved/SaveGames/YJJDefense.sav`)은 건드리지 않는다.
그림은 Pillow로 그린다. `BuildArt.ps1`이 처음 한 번 `Tools/Art/.venv`를 만든다 (Python 3 필요).

## 고칠 곳

- 룬·반응 규칙: `Source/YJJDefense/Game/YJJSpellForge.cpp`, 반응 효과: `YJJDefenseSim.cpp`의 `TriggerReaction`
- 타워·몬스터·웨이브 수치: `Game/YJJGameData.cpp`
- 해금 순서·점수: `Meta/YJJSaveGame.cpp`의 `GUnlockTrack`
- 맵(길, 건설 자리, 마법사, 성문): `Tools/Art/map_data.py` → `BuildArt.ps1` 다시 실행
- 그림: `Tools/Art/generate_art.py`의 `make_*`

## 조작

- 빈 자리 클릭 → 병졸 고르기, 지은 타워 클릭 → 강화/팔기 (정령 제단은 원소 바꾸기)
- 룬 가방의 룬을 디스크 칸으로 끌어다 놓기 (또는 룬 클릭 후 칸 클릭), 칸 우클릭으로 빼기
- 디스크 클릭 또는 1·2·3 → 맵 클릭으로 스펠 시전
- 스페이스: 웨이브 호출 (일찍 부르면 골드 보너스), P: 일시정지, F: 2배속, ESC/우클릭: 취소

## 콘솔 변수

- `yjj.AutoPlay 1`, `yjj.AutoPlaySpeed N`, `yjj.AutoPlayExit N`(N판 후 종료), `yjj.AutoPlayShotInterval 초`
- `yjj.UITest 1`
