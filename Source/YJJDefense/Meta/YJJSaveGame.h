#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Game/YJJDefenseTypes.h"
#include "YJJSaveGame.generated.h"

// 판과 판 사이에 남는 것: 수집한 룬·병졸, 조합 도감, 수집 점수
UCLASS()
class YJJDEFENSE_API UYJJSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 비트 = EYJJRune / EYJJTower
	UPROPERTY()
	int32 UnlockedRunes = 0x4F;   // 처음부터: 불·얼음·물·번개·폭발

	UPROPERTY()
	int32 UnlockedTowers = 0x07;

	// 이번 판에 들고 나갈 것
	UPROPERTY()
	int32 SelectedRunes = 0;

	UPROPERTY()
	int32 SelectedTowers = 0;

	// 써 본 2룬 조합 (YJJSpellForge::MakeComboId)
	UPROPERTY()
	TArray<int32> DiscoveredCombos;

	UPROPERTY()
	int32 Points = 0;

	UPROPERTY()
	int32 UnlockIndex = 0;   // 수집 트랙에서 다음에 풀릴 항목

	UPROPERTY()
	int32 MaxTier = 1;       // 도전할 수 있는 최고 단계

	UPROPERTY()
	int32 SelectedTier = 1;

	// 열린 지도 수 (앞 지도를 깨면 다음 지도가 열린다)
	UPROPERTY()
	int32 UnlockedMaps = 1;

	UPROPERTY()
	int32 SelectedMap = 0;

	UPROPERTY()
	int32 RunsPlayed = 0;

	UPROPERTY()
	int32 Wins = 0;

	// 마지막으로 쓴 디스크 구성 (EYJJRune, 빈 칸은 255)
	UPROPERTY()
	TArray<uint8> LastDisks;
};

struct FYJJUnlock
{
	bool bTower;
	uint8 Index;         // EYJJRune 또는 EYJJTower
	int32 PointsNeeded;  // 누적 수집 점수
};

struct FYJJRunStats
{
	int32 Map = 0;
	bool bVictory = false;
	int32 Stars = 0;
	int32 WavesCleared = 0;
	int32 Tier = 1;
	TArray<int32> CombosCast;
};

// 한 판이 끝났을 때 저장 데이터에 반영된 결과 (결과 화면에 그대로 보여 준다)
struct FYJJRunReward
{
	int32 WavePoints = 0;
	int32 VictoryPoints = 0;
	int32 StarPoints = 0;
	int32 ComboPoints = 0;
	int32 NewCombos = 0;
	int32 TotalPoints = 0;
	TArray<FYJJUnlock> NewUnlocks;
	bool bNewTier = false;
	int32 NewMap = INDEX_NONE;   // 이번에 열린 지도
};

namespace YJJProgression
{
	inline constexpr int32 MaxBagRunes = 5;
	inline constexpr int32 MaxBagTowers = 4;
	inline constexpr int32 MaxTier = 9;

	YJJDEFENSE_API TConstArrayView<FYJJUnlock> UnlockTrack();
	YJJDEFENSE_API const TCHAR* UnlockName(const FYJJUnlock& Unlock);

	YJJDEFENSE_API UYJJSaveGame* LoadOrCreate();
	YJJDEFENSE_API void Save(UYJJSaveGame* SaveGame);

	// 선택이 비었거나 잠긴 것이 섞였으면 고친다
	YJJDEFENSE_API void SanitizeSelection(UYJJSaveGame& SaveGame);
	YJJDEFENSE_API bool ToggleRune(UYJJSaveGame& SaveGame, EYJJRune Rune);
	YJJDEFENSE_API bool ToggleTower(UYJJSaveGame& SaveGame, EYJJTower Tower);

	// 점수 계산 + 해금. SaveGame을 고치지만 디스크에 쓰지는 않는다
	YJJDEFENSE_API FYJJRunReward ApplyRun(UYJJSaveGame& SaveGame, const FYJJRunStats& Stats);
}
