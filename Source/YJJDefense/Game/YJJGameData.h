#pragma once

#include "CoreMinimal.h"
#include "Data/YJJTuning.h"
#include "Data/YJJUILayout.h"
#include "Game/YJJDefenseTypes.h"
#include "HAL/IConsoleManager.h"

class UDataTable;
class UScriptStruct;

/**
 * 게임 데이터 전부. 처음 쓸 때 Content/YJJDefense/Data/*.csv 를 엔진 DataTable로 읽어 아래 표로 푼다.
 * 게임 중에는 콘솔 yjj.ReloadData 로 다시 읽는다.
 *
 *   Tuning.csv      규칙의 낱개 수치 (FYJJTuning)        UILayout.csv   화면 배치·연출 수치 (FYJJUILayout)
 *   Runes.csv       룬과 원소 효과                       Reactions.csv  원소 반응 15가지
 *   Towers.csv      병졸 + 강화 단계                     Enemies.csv    몬스터
 *   Maps.csv        지도 이름·미리보기                   MapGeometry.csv 지도 모양 (Tools/Art/map_data.py가 생성)
 *   Waves.csv       지도별 웨이브                        Unlocks.csv    수집 트랙
 *   StartDisks.csv  첫 판 디스크                         BotBuildOrder.csv, Sounds.csv
 * 글자는 Localization.csv (YJJLoc).
 */
class YJJDEFENSE_API FYJJGameDatabase
{
public:
	static FYJJGameDatabase& Get();

	void Reload();

	FYJJTuning Tuning;
	FYJJUILayout Layout;
	TArray<FYJJRuneDef> Runes;           // EYJJRune 순서
	TArray<FYJJReactionDef> Reactions;   // EYJJReaction 순서 (0번 = None)
	TArray<FYJJTowerDef> Towers;         // EYJJTower 순서
	TArray<FYJJEnemyDef> Enemies;        // EYJJEnemy 순서
	TArray<FYJJMapDef> Maps;             // Maps.csv Order 순서
	TArray<FYJJUnlock> UnlockTrack;      // 점수로 풀리는 것만, Order 순서
	uint32 StartRuneMask = 0;            // 처음부터 가진 룬 (비트 = EYJJRune)
	uint32 StartTowerMask = 0;
	EYJJRune StartDisks[YJJ::NumDisks][YJJ::NumSockets];
	TArray<EYJJTower> BotBuildPattern;
	TMap<FName, FYJJSoundDef> Sounds;
	EYJJReaction PairReactions[static_cast<int32>(EYJJElement::Count)][static_cast<int32>(EYJJElement::Count)];
	int32 ActiveMap = 0;

	/** 읽으면서 생긴 문제 (빠진 열·이름, 모르는 몬스터…). 테스트가 비어 있는지 확인한다 */
	TArray<FString> Problems;

private:
	FYJJGameDatabase();

	void Load();
	void Problem(const FString& Message);
	UDataTable* LoadTable(const TCHAR* File, UScriptStruct* RowStruct);
	/** Name,Value 표를 USTRUCT의 같은 이름 숫자 변수에 넣는다 */
	void FillNameValues(const TCHAR* File, const UScriptStruct* Struct, void* Data);

	void LoadRunes();
	void LoadReactions();
	void LoadTowers();
	void LoadEnemies();
	void LoadMaps();
	void LoadWaves();
	void LoadUnlocks();
	void LoadStartDisks();
	void LoadBot();
	void LoadSounds();
	void Validate();

	static FString DataPath(const TCHAR* File);
	/** "FF6B29" → 선형 색 (그대로 0~1로 나눈다) */
	static FLinearColor ParseColor(const FString& Hex);

	TArray<UDataTable*> Tables;   // AddToRoot로 붙잡아 둔다
	bool bLoaded = false;

	static FAutoConsoleCommand ReloadCommand;
};

// 자주 쓰는 표를 짧게 부르는 길 (모두 FYJJGameDatabase::Get()을 거친다)
namespace YJJData
{
	YJJDEFENSE_API const FYJJTuning& Tune();
	YJJDEFENSE_API const FYJJUILayout& Layout();

	YJJDEFENSE_API const FYJJRuneDef& Rune(EYJJRune Rune);
	YJJDEFENSE_API const FYJJTowerDef& Tower(EYJJTower Tower);
	YJJDEFENSE_API const FYJJEnemyDef& Enemy(EYJJEnemy Enemy);
	YJJDEFENSE_API const FYJJReactionDef& Reaction(EYJJReaction Reaction);
	/** 두 원소가 만나면 일어나는 반응 (같거나 없으면 None) */
	YJJDEFENSE_API EYJJReaction ReactionOf(EYJJElement A, EYJJElement B);

	// 지금 지도(YJJMap::SetActive)의 웨이브
	YJJDEFENSE_API int32 NumWaves();
	YJJDEFENSE_API TConstArrayView<FYJJWaveGroup> Wave(int32 Index);

	YJJDEFENSE_API TConstArrayView<FYJJUnlock> UnlockTrack();
	YJJDEFENSE_API uint32 StartRuneMask();
	YJJDEFENSE_API uint32 StartTowerMask();
	YJJDEFENSE_API EYJJRune StartDisk(int32 Disk, int32 Socket);
	YJJDEFENSE_API TConstArrayView<EYJJTower> BotBuildPattern();
	/** Sounds.csv 행. 없으면 nullptr */
	YJJDEFENSE_API const FYJJSoundDef* Sound(FName Key);

	/** 몬스터 특수 능력을 짧은 말로 ("갑옷, 쪼개짐"…). 없으면 빈 문자열 */
	YJJDEFENSE_API FString EnemyTraits(EYJJEnemy Enemy);
	YJJDEFENSE_API const FString& ElementName(EYJJElement Element);
	YJJDEFENSE_API FLinearColor ElementColor(EYJJElement Element);

	inline bool IsElementRune(EYJJRune Rune) { return static_cast<uint8>(Rune) <= static_cast<uint8>(EYJJRune::Wind); }
	inline EYJJElement RuneElement(EYJJRune Rune) { return IsElementRune(Rune) ? static_cast<EYJJElement>(static_cast<uint8>(Rune) + 1) : EYJJElement::None; }
	inline EYJJRune ElementRune(EYJJElement Element) { return Element == EYJJElement::None ? EYJJRune::None : static_cast<EYJJRune>(static_cast<uint8>(Element) - 1); }
}

// 지도. 한 번에 한 판만 돌기 때문에 "지금 지도"를 하나 둔다 (FYJJDefenseSim::Reset이 고른다)
namespace YJJMap
{
	YJJDEFENSE_API int32 NumMaps();
	YJJDEFENSE_API void SetActive(int32 Map);
	YJJDEFENSE_API int32 GetActive();
	YJJDEFENSE_API const FYJJMapDef& Def(int32 Map);
	YJJDEFENSE_API const FString& Name(int32 Map);
	YJJDEFENSE_API const FString& Desc(int32 Map);

	YJJDEFENSE_API TConstArrayView<FVector2f> Path();
	YJJDEFENSE_API float PathLength();
	/** 길 위 Dist 지점. Lateral은 진행 방향의 오른쪽으로 벗어난 거리 */
	YJJDEFENSE_API FVector2f PointAt(float Dist, float Lateral = 0.f, FVector2f* OutDirection = nullptr);
	YJJDEFENSE_API float NearestDist(const FVector2f& Pos);
	YJJDEFENSE_API TConstArrayView<FVector2f> Spots();
	YJJDEFENSE_API FVector2f WizardPos();
	YJJDEFENSE_API FVector2f GatePos();
}
