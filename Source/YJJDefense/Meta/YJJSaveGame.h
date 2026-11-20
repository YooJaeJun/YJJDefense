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
	// 비트 = EYJJRune / EYJJTower. 0이면 처음 시작 (Unlocks.csv 의 처음부터 가진 것으로 채운다)
	UPROPERTY()
	int32 UnlockedRunes = 0;

	UPROPERTY()
	int32 UnlockedTowers = 0;

	// 이번 판에 들고 나갈 것
	UPROPERTY()
	int32 SelectedRunes = 0;

	UPROPERTY()
	int32 SelectedTowers = 0;

	// 써 본 2룬 조합 (FYJJSpellForge::MakeComboId)
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

	// 마지막으로 쓴 디스크 구성 (EYJJRune 값)
	UPROPERTY()
	TArray<uint8> LastDisks;
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

/** 수집·해금 규칙. 점수·가방 크기는 Tuning.csv, 해금 순서는 Unlocks.csv */
class YJJDEFENSE_API FYJJProgression
{
public:
	/** 해금 항목 이름 (지금 언어) */
	static const FString& UnlockName(const FYJJUnlock& Unlock);

	static UYJJSaveGame* LoadOrCreate();
	static void Save(UYJJSaveGame* SaveGame);

	/** 선택이 비었거나 잠긴 것이 섞였으면 고친다 */
	static void SanitizeSelection(UYJJSaveGame& SaveGame);
	static bool ToggleRune(UYJJSaveGame& SaveGame, EYJJRune Rune);
	static bool ToggleTower(UYJJSaveGame& SaveGame, EYJJTower Tower);

	/** 점수 계산 + 해금. SaveGame을 고치지만 디스크에 쓰지는 않는다 */
	static FYJJRunReward ApplyRun(UYJJSaveGame& SaveGame, const FYJJRunStats& Stats);

private:
	/** 테스트·스모크 실행은 -YJJSave=이름 으로 다른 슬롯을 써서 실제 진행 상황을 건드리지 않는다 */
	static FString SaveSlotName();
	/** 아래쪽 비트부터 MaxCount개만 남긴다 */
	static int32 KeepLowestBits(int32 Mask, int32 MaxCount);
	static bool ToggleBit(int32& Selected, int32 Unlocked, int32 Bit, int32 MaxCount);

	static constexpr const TCHAR* DefaultSlot = TEXT("YJJDefense");
	static constexpr const TCHAR* SlotCommandLine = TEXT("YJJSave=");
	static constexpr int32 MaskBits = 32;
};
