#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Game/YJJAutoPlayer.h"
#include "Game/YJJDefenseSim.h"
#include "HAL/IConsoleManager.h"
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
	bool IsFast() const { return Speed > 1.f; }
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
	static void TakeShot(const FString& Name);
	/** Sounds.csv 행 이름: 사건 이름, 투사체가 있는 사건은 사건_투사체 */
	static FName SoundKey(const FYJJSimEvent& Event);

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

	// 오토플레이 진행 시각 (테스트 도구라 데이터 표가 아니라 여기에 둔다)
	static constexpr float AutoPlayLoadoutShotTime = 1.5f;
	static constexpr float AutoPlayStartTime = 2.5f;
	static constexpr float AutoPlayResultShotTime = 1.5f;
	static constexpr float AutoPlayResultLeaveTime = 3.f;
	static constexpr float AutoPlayReportInterval = 20.f;
	static constexpr int32 AutoPlayMaxShots = 12;
	static constexpr float AutoPlayFirstShotShare = 0.5f;   // 첫 전투 스크린샷 = 간격 x 이것

	static TAutoConsoleVariable<int32> CVarAutoPlay;
	static TAutoConsoleVariable<float> CVarAutoPlaySpeed;
	static TAutoConsoleVariable<int32> CVarAutoPlayExit;
	static TAutoConsoleVariable<int32> CVarAutoPlayMap;
	static TAutoConsoleVariable<float> CVarAutoPlayShotInterval;
	static TAutoConsoleVariable<int32> CVarUITest;
};
