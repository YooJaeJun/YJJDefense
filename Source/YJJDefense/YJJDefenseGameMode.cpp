#include "YJJDefenseGameMode.h"
#include "Audio/YJJAudio.h"
#include "Game/YJJGameData.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Player/YJJDefensePlayerController.h"
#include "UI/YJJDefenseHUD.h"
#include "UnrealClient.h"
#include "YJJDefense.h"

TAutoConsoleVariable<int32> AYJJDefenseGameMode::CVarAutoPlay(
	TEXT("yjj.AutoPlay"), 0,
	TEXT("1이면 봇이 병졸을 짓고 스펠을 조합해 쏘며 한 판을 플레이한다 (스모크 테스트용)"));

TAutoConsoleVariable<float> AYJJDefenseGameMode::CVarAutoPlaySpeed(
	TEXT("yjj.AutoPlaySpeed"), 3.f,
	TEXT("오토플레이 중 게임 배속"));

TAutoConsoleVariable<int32> AYJJDefenseGameMode::CVarAutoPlayExit(
	TEXT("yjj.AutoPlayExit"), 0,
	TEXT("1 이상이면 오토플레이로 그 판 수만큼 끝낸 뒤 스크린샷을 남기고 게임을 종료한다"));

TAutoConsoleVariable<int32> AYJJDefenseGameMode::CVarAutoPlayMap(
	TEXT("yjj.AutoPlayMap"), INDEX_NONE,
	TEXT("0 이상이면 오토플레이가 이 지도로 출전한다 (잠겨 있어도 연다)"));

TAutoConsoleVariable<float> AYJJDefenseGameMode::CVarAutoPlayShotInterval(
	TEXT("yjj.AutoPlayShotInterval"), 45.f,
	TEXT("오토플레이 중 스크린샷 간격 (게임 시간 초). 0이면 찍지 않는다"));

TAutoConsoleVariable<int32> AYJJDefenseGameMode::CVarUITest(
	TEXT("yjj.UITest"), 0,
	TEXT("1이면 마우스 조작을 흉내 내는 UI 테스트를 돌리고 게임을 종료한다 (Tools/RunUITest.ps1)"));

AYJJDefenseGameMode::AYJJDefenseGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AYJJDefensePlayerController::StaticClass();
	HUDClass = AYJJDefenseHUD::StaticClass();
}

void AYJJDefenseGameMode::TakeShot(const FString& Name)
{
	const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Name + TEXT(".png"));
	FScreenshotRequest::RequestScreenshot(Path, true, false);
	UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] screenshot %s"), *Path);
}

void AYJJDefenseGameMode::StartPlay()
{
	SaveGame = FYJJProgression::LoadOrCreate();
	Sim.Reset(FYJJRunConfig());

	Super::StartPlay();

	FYJJAudio::StartMusic(this);
}

AYJJDefenseHUD* AYJJDefenseGameMode::GetDefenseHUD() const
{
	const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return Controller ? Cast<AYJJDefenseHUD>(Controller->GetHUD()) : nullptr;
}

bool AYJJDefenseGameMode::IsComboKnown(int32 ComboId) const
{
	return ComboId == INDEX_NONE || (SaveGame && SaveGame->DiscoveredCombos.Contains(ComboId)) || Sim.CombosCast.Contains(ComboId);
}

void AYJJDefenseGameMode::StartRun()
{
	if (!SaveGame)
	{
		return;
	}
	FYJJProgression::SanitizeSelection(*SaveGame);

	FYJJRunConfig Config;
	Config.Map = SaveGame->SelectedMap;
	Config.Tier = SaveGame->SelectedTier;
	Config.Seed = YJJData::Tune().RunSeedBase + SaveGame->RunsPlayed;
	Config.RuneMask = static_cast<uint32>(SaveGame->SelectedRunes);
	Config.TowerMask = static_cast<uint32>(SaveGame->SelectedTowers);

	// 지난 판에 쓰던 디스크 구성을 이어 쓴다 (들고 나오지 않은 룬은 Reset이 비운다)
	if (SaveGame->LastDisks.Num() == YJJ::NumDisks * YJJ::NumSockets)
	{
		for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
		{
			for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
			{
				Config.Disks[Disk][Socket] = static_cast<EYJJRune>(SaveGame->LastDisks[Disk * YJJ::NumSockets + Socket]);
			}
		}
	}

	Sim.Reset(Config);
	AutoPlayer.Reset();
	Screen = EYJJScreen::Battle;
	bBattlePaused = false;
	Speed = 1.f;
	StepAccumulator = 0.f;
	ResultDelay = 0.f;

	if (AYJJDefenseHUD* HUD = GetDefenseHUD())
	{
		HUD->OnRunStarted();
	}
	UE_LOG(LogYJJDefense, Display, TEXT("[Run] start map=%d tier=%d runes=0x%X towers=0x%X"), Config.Map, Config.Tier, Config.RuneMask, Config.TowerMask);
	FYJJAudio::Play(this, FYJJAudio::ClickKey);
	SaveProgress();
}

void AYJJDefenseGameMode::ReturnToLoadout()
{
	Screen = EYJJScreen::Loadout;
	FYJJAudio::Play(this, FYJJAudio::ClickKey);
}

void AYJJDefenseGameMode::TogglePause()
{
	if (Screen == EYJJScreen::Battle)
	{
		bBattlePaused = !bBattlePaused;
	}
}

void AYJJDefenseGameMode::ToggleSpeed()
{
	Speed = IsFast() ? 1.f : YJJData::Tune().FastSpeed;
}

void AYJJDefenseGameMode::SaveProgress()
{
	FYJJProgression::Save(SaveGame);
}

void AYJJDefenseGameMode::FinishRun()
{
	LastStats = FYJJRunStats();
	LastStats.bVictory = Sim.Result == EYJJRunResult::Victory;
	LastStats.Stars = Sim.Stars();
	LastStats.Tier = Sim.Config.Tier;
	LastStats.Map = Sim.Config.Map;
	LastStats.CombosCast = Sim.CombosCast;
	// 끝까지 버틴 웨이브 수 (지면 진행 중이던 웨이브는 빼고)
	LastStats.WavesCleared = LastStats.bVictory ? YJJData::NumWaves() : FMath::Max(Sim.WaveIndex, 0);

	if (SaveGame)
	{
		SaveGame->LastDisks.Reset();
		for (const FYJJDiskState& Disk : Sim.Disks)
		{
			for (const EYJJRune Rune : Disk.Sockets)
			{
				SaveGame->LastDisks.Add(static_cast<uint8>(Rune));
			}
		}
		LastReward = FYJJProgression::ApplyRun(*SaveGame, LastStats);
		SaveProgress();
	}

	Screen = EYJJScreen::Result;
	UE_LOG(LogYJJDefense, Display, TEXT("[Run] %s map=%d tier=%d lives=%d stars=%d waves=%d kills=%d casts=%d reactions=%d combos=%d(+%d) points=+%d unlocks=%d newmap=%d"),
		LastStats.bVictory ? TEXT("VICTORY") : TEXT("DEFEAT"), LastStats.Map, LastStats.Tier, Sim.Lives, LastStats.Stars, LastStats.WavesCleared, Sim.Kills, Sim.SpellsCast,
		Sim.Reactions, Sim.CombosCast.Num(), LastReward.NewCombos, LastReward.TotalPoints, LastReward.NewUnlocks.Num(), LastReward.NewMap);
}

void AYJJDefenseGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bAutoPlay = CVarAutoPlay.GetValueOnGameThread() != 0;
	if (bAutoPlay)
	{
		TickAutoPlay(DeltaSeconds);
	}
	else if (CVarUITest.GetValueOnGameThread() != 0 && !bUITestDone)
	{
		AYJJDefenseHUD* HUD = GetDefenseHUD();
		if (HUD && UITest.Tick(*this, *HUD, DeltaSeconds))
		{
			bUITestDone = true;
			FPlatformMisc::RequestExit(false, TEXT("yjj.UITest"));
		}
	}

	if (Screen != EYJJScreen::Battle)
	{
		return;
	}

	if (Sim.Result != EYJJRunResult::Playing)
	{
		// 승패 연출을 잠깐 보여 준 뒤 결과 화면으로
		ResultDelay += DeltaSeconds;
		if (ResultDelay >= YJJData::Tune().ResultDelay)
		{
			FinishRun();
		}
		return;
	}

	if (bBattlePaused)
	{
		return;
	}

	// 고정 스텝. 프레임이 튀어도 한 번에 너무 많이 돌리지 않는다
	StepAccumulator = FMath::Min(StepAccumulator + DeltaSeconds * Speed, YJJData::Tune().MaxStepBacklog);
	while (StepAccumulator >= FYJJDefenseSim::FixedStep)
	{
		StepAccumulator -= FYJJDefenseSim::FixedStep;
		if (bAutoPlay)
		{
			AutoPlayer.Tick(Sim);
		}
		Sim.Step();
	}
	DispatchEvents();
}

FName AYJJDefenseGameMode::SoundKey(const FYJJSimEvent& Event)
{
	const FString EventName = StaticEnum<EYJJSimEvent>()->GetNameStringByValue(static_cast<int64>(Event.Type));
	if (Event.Type == EYJJSimEvent::TowerFired || Event.Type == EYJJSimEvent::ProjectileHit)
	{
		const FString Projectile = StaticEnum<EYJJProjectile>()->GetNameStringByValue(Event.Int);
		return FName(*FString::Printf(TEXT("%s_%s"), *EventName, *Projectile));
	}
	return FName(*EventName);
}

void AYJJDefenseGameMode::DispatchEvents()
{
	if (Sim.Events.IsEmpty())
	{
		return;
	}

	if (AYJJDefenseHUD* HUD = GetDefenseHUD())
	{
		HUD->OnSimEvents(Sim.Events);
	}

	for (const FYJJSimEvent& Event : Sim.Events)
	{
		FYJJAudio::Play(this, SoundKey(Event));
		if (Event.Type == EYJJSimEvent::TowerBuilt)
		{
			UE_LOG(LogYJJDefense, Display, TEXT("[Battle] tower built at (%.0f, %.0f) gold=%d"), Event.Pos.X, Event.Pos.Y, Sim.Gold);
		}
		else if (Event.Type == EYJJSimEvent::WaveStarted)
		{
			UE_LOG(LogYJJDefense, Display, TEXT("[Battle] wave %d started lives=%d gold=%d"), Event.Int + 1, Sim.Lives, Sim.Gold);
		}
	}
	Sim.Events.Reset();
}

void AYJJDefenseGameMode::TickAutoPlay(float DeltaSeconds)
{
	AutoPlayTime += DeltaSeconds;
	AutoPlayScreenTime += DeltaSeconds;
	const int32 ExitAfterRuns = CVarAutoPlayExit.GetValueOnGameThread();

	switch (Screen)
	{
	case EYJJScreen::Loadout:
		// 준비 화면을 한 장 찍고 출전
		if (AutoPlayScreenTime > AutoPlayLoadoutShotTime && AutoPlayShotIndex == 0)
		{
			TakeShot(TEXT("AutoPlay_Loadout"));
			AutoPlayShotIndex = 1;
		}
		if (AutoPlayScreenTime > AutoPlayStartTime)
		{
			const int32 ForcedMap = CVarAutoPlayMap.GetValueOnGameThread();
			if (SaveGame && ForcedMap >= 0)
			{
				SaveGame->UnlockedMaps = FMath::Max(SaveGame->UnlockedMaps, FMath::Min(ForcedMap + 1, YJJMap::NumMaps()));
				SaveGame->SelectedMap = FMath::Min(ForcedMap, YJJMap::NumMaps() - 1);
			}
			StartRun();
			Speed = CVarAutoPlaySpeed.GetValueOnGameThread();
			AutoPlayScreenTime = 0.f;
			AutoPlayNextReport = 0.f;
			bAutoPlayResultShot = false;
		}
		break;

	case EYJJScreen::Battle:
	{
		Speed = CVarAutoPlaySpeed.GetValueOnGameThread();
		if (Sim.Time >= AutoPlayNextReport)
		{
			AutoPlayNextReport += AutoPlayReportInterval;
			UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] t=%.0f wave=%d/%d lives=%d gold=%d enemies=%d kills=%d casts=%d reactions=%d combos=%d"),
				Sim.Time, Sim.WaveIndex + 1, YJJData::NumWaves(), Sim.Lives, Sim.Gold, Sim.Enemies.Num(), Sim.Kills, Sim.SpellsCast, Sim.Reactions, Sim.CombosCast.Num());
		}
		const float Interval = CVarAutoPlayShotInterval.GetValueOnGameThread();
		if (Interval > 0.f && AutoPlayRuns == 0 && AutoPlayShotIndex < AutoPlayMaxShots
			&& Sim.Time >= Interval * AutoPlayShotIndex - Interval * AutoPlayFirstShotShare)
		{
			TakeShot(FString::Printf(TEXT("AutoPlay_Battle_%d"), AutoPlayShotIndex));
			AutoPlayShotIndex++;
		}
		AutoPlayScreenTime = 0.f;
		break;
	}

	case EYJJScreen::Result:
		if (AutoPlayScreenTime > AutoPlayResultShotTime && !bAutoPlayResultShot)
		{
			bAutoPlayResultShot = true;
			TakeShot(FString::Printf(TEXT("AutoPlay_Result_%d"), AutoPlayRuns));
		}
		if (AutoPlayScreenTime > AutoPlayResultLeaveTime)
		{
			AutoPlayRuns++;
			if (ExitAfterRuns > 0 && AutoPlayRuns >= ExitAfterRuns)
			{
				UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] finished runs=%d points=%d"), AutoPlayRuns, SaveGame ? SaveGame->Points : 0);
				FPlatformMisc::RequestExit(false, TEXT("yjj.AutoPlayExit"));
				return;
			}
			ReturnToLoadout();
			// 두 번째 판부터는 준비 화면을 다시 찍지 않는다
			AutoPlayScreenTime = AutoPlayLoadoutShotTime;
		}
		break;
	}
}
