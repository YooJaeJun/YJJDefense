#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/YJJAutoPlayer.h"
#include "Game/YJJDefenseSim.h"
#include "Meta/YJJSaveGame.h"
#include "Tests/YJJUITestDriver.h"
#include "YJJDefenseGameMode.generated.h"

class AYJJDefenseHUD;

enum class EYJJScreen : uint8
{
	Loadout,   // 준비: 수집품 확인, 들고 나갈 룬·병졸 고르기
	Battle,
	Result
};

// 한 판의 흐름을 쥔다: 준비 → 전투(시뮬레이션을 고정 스텝으로 진행) → 결과(수집 점수·해금 반영)
UCLASS()
class YJJDEFENSE_API AYJJDefenseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYJJDefenseGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	FYJJDefenseSim& GetSim() { return Sim; }
	UYJJSaveGame* GetSave() const { return SaveGame; }
	EYJJScreen GetScreen() const { return Screen; }
	bool IsBattlePaused() const { return bBattlePaused; }
	float GetSpeed() const { return Speed; }
	const FYJJRunReward& GetLastReward() const { return LastReward; }
	const FYJJRunStats& GetLastStats() const { return LastStats; }

	// 도감에 있거나 이번 판에 이미 써 본 조합인지
	bool IsComboKnown(int32 ComboId) const;

	void StartRun();
	void ReturnToLoadout();
	void TogglePause();
	void ToggleSpeed();
	void SaveProgress();

private:
	void FinishRun();
	void DispatchEvents();
	void TickAutoPlay(float DeltaSeconds);
	AYJJDefenseHUD* GetDefenseHUD() const;

	FYJJDefenseSim Sim;
	FYJJAutoPlayer AutoPlayer;

	UPROPERTY()
	TObjectPtr<UYJJSaveGame> SaveGame;

	EYJJScreen Screen = EYJJScreen::Loadout;
	bool bBattlePaused = false;
	float Speed = 1.f;
	float StepAccumulator = 0.f;
	float ResultDelay = 0.f;

	FYJJRunStats LastStats;
	FYJJRunReward LastReward;

	// UI 테스트 (yjj.UITest)
	FYJJUITestDriver UITest;
	bool bUITestDone = false;

	// 오토플레이 (스모크 테스트)
	float AutoPlayTime = 0.f;
	float AutoPlayScreenTime = 0.f;
	float AutoPlayNextReport = 0.f;
	int32 AutoPlayShotIndex = 0;
	int32 AutoPlayRuns = 0;
	bool bAutoPlayResultShot = false;
};
