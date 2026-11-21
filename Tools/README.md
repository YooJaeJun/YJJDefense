# YJJDefense 도구

엔진: `D:\source\repos\UnrealEngine` (UE 5.8.1 소스 빌드, YJJJump와 같은 엔진). 모두 에디터를 끈 상태에서 실행한다.

| 스크립트 | 하는 일 |
|---|---|
| `Art/BuildArt.ps1` | `Art/generate_art.py`로 그림 PNG + 맵 데이터(`Content/YJJDefense/Data/MapGeometry.csv`), `Audio/generate_audio.py`로 소리 WAV 생성 → `Art/import_assets.py`로 언리얼 에셋 임포트. `-SkipImport`면 생성만 |
| `BuildAndTest.ps1` | 에디터 타깃 빌드 + 자동화 테스트 (`-NoTest`면 빌드만) |
| `RunTests.ps1` | `YJJDefense.*` 자동화 테스트(스펠 조합, 원소 반응, 타워, 한 판 전체·밸런스, 수집, 데이터 표 검사) |
| `RunUITest.ps1` | 게임을 띄워 Slate에 마우스·키 이벤트를 넣으며 UI 19단계 확인 → `Saved/Screenshots/UITest_*.png`. `-Lang en`으로 영어 화면 |
| `RunSmoke.ps1` | 봇이 한 판(`-Runs N`)을 플레이 → 로그 요약 + `Saved/Screenshots/AutoPlay_*.png`. `-Map N`으로 지도 지정(0 들판, 1 해변, 2 설산), `-Fresh`면 수집 진행 초기화, `-Lang en` 영어 |

UI 테스트·스모크는 `-YJJSave=UITest/Smoke` 전용 저장 슬롯을 써서 실제 진행(`Saved/SaveGames/YJJDefense.sav`)은 건드리지 않는다.
그림은 Pillow로 그린다. `BuildArt.ps1`이 처음 한 번 `Tools/Art/.venv`를 만든다 (Python 3 필요).

## 고칠 곳

숫자와 글자는 코드에 두지 않는다. 표는 `Content/YJJDefense/Data/*.csv` (UTF-8 BOM, 엑셀로 열어도 된다).
게임 중에는 콘솔 `yjj.ReloadData`로 다시 읽고, 자동화 테스트 `YJJDefense.Data.Tables`가 빠진 값·안 쓰는 값·없는 글자를 잡는다.

| 표 | 내용 |
|---|---|
| `Tuning.csv` | 규칙의 낱개 수치 (시작 골드, 스펠 기본값, 조합 배율, 병졸·장판·봇 값…) → `FYJJTuning` |
| `UILayout.csv` | 화면 배치·크기·연출 수치 (1920x1080 가상 좌표) → `FYJJUILayout` |
| `Localization.csv` | 화면에 나오는 모든 글자. 열 `Key, ko, en, Note` (언어는 열을 붙이면 늘어난다), `{0}` 서식, `
` 줄바꿈 |
| `Runes.csv` / `Reactions.csv` | 룬별 효과·스펠 보정 / 원소 쌍별 반응 효과 (즉시·번개 N명·장판) |
| `Towers.csv` / `Enemies.csv` | 병졸 단계별 수치 / 몬스터 (쪼개짐 `SplitInto`, 얼음 면역 `Immune`) |
| `Maps.csv` / `Waves.csv` / `MapGeometry.csv` | 지도 이름·그림·미리보기 / 웨이브 묶음 / 길·자리 (`map_data.py`가 생성) |
| `Unlocks.csv` / `StartDisks.csv` | 해금 순서(0점 = 처음부터) / 처음 디스크 구성 |
| `Sounds.csv` / `BotBuildOrder.csv` | 사건별 효과음 / 봇이 짓는 병졸 순서 |

- 언어: 명령줄 `-yjjlang=en` > 콘솔 `yjj.Lang en` > OS 표시 언어 > ko. 없는 글자는 `[키]`로 보인다
- 글자가 칸보다 길면 `DrawLabel`의 최대 너비로 줄여 맞춘다 (`UILayout.csv`의 `TextFitPad` 등)
- 룬 조합 규칙(코드): `Game/YJJSpellForge.cpp`, 반응·원소 효과 적용: `YJJDefenseSim.cpp`의 `TriggerReaction`·`ApplyIntrinsic`
- 색: `UI/YJJUIStyle.h` (아트 팔레트와 같다)
- 지도 모양: `Tools/Art/map_data.py`의 `MAPS` → `BuildArt.ps1` 다시 실행 (`MapGeometry.csv` 생성). 지도를 늘리면 `Maps.csv`에도 추가
- 그림: `Tools/Art/generate_art.py`가 모은다. 명암 도구 `paint.py`, 병졸·막사·궁수 `units_hd.py`, 나머지 타워 `towers_hd.py`, 몬스터 `monsters_hd.py`, 지도·성문 `maps_hd.py`
- 코드 규칙: `.cpp`에는 namespace·변수·함수·구조체·템플릿을 선언하지 않는다 (헤더에만). 상수는 표나 이름 붙은 변수로

## 조작

- 빈 자리 클릭 → 병졸 고르기, 지은 타워 클릭 → 강화/팔기 (정령 제단은 원소 바꾸기)
- 룬 가방의 룬을 디스크 칸으로 끌어다 놓기 (또는 룬 클릭 후 칸 클릭), 칸 우클릭으로 빼기
- 디스크 클릭 또는 1·2·3 → 맵 클릭으로 스펠 시전
- 스페이스: 웨이브 호출 (일찍 부르면 골드 보너스), P: 일시정지, F: 2배속, ESC/우클릭: 취소

## 콘솔 변수

- `yjj.AutoPlay 1`, `yjj.AutoPlaySpeed N`, `yjj.AutoPlayExit N`(N판 후 종료), `yjj.AutoPlayShotInterval 초`
- `yjj.UITest 1`, `yjj.Lang ko|en`, `yjj.ReloadData`
