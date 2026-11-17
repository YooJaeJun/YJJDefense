#include "YJJDefenseGameMode.h"
#include "Audio/YJJAudio.h"
#include "Game/YJJGameData.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Player/YJJDefensePlayerController.h"
#include "UI/YJJDefenseHUD.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogYJJDefense, Log, All);

static TAutoConsoleVariable<int32> CVarAutoPlay(
	TEXT("yjj.AutoPlay"), 0,
	TEXT("1이면 봇이 병졸을 짓고 스펠을 조합해 쏘며 한 판을 플레이한다 (스모크 테스트용)"));

static TAutoConsoleVariable<float> CVarAutoPlaySpeed(
	TEXT("yjj.AutoPlaySpeed"), 3.f,
	TEXT("오토플레이 중 게임 배속"));

static TAutoConsoleVariable<int32> CVarAutoPlayExit(
	TEXT("yjj.AutoPlayExit"), 0,
	TEXT("1 이상이면 오토플레이로 그 판 수만큼 끝낸 뒤 스크린샷을 남기고 게임을 종료한다"));

static TAutoConsoleVariable<float> CVarAutoPlayShotInterval(
	TEXT("yjj.AutoPlayShotInterval"), 45.f,
	TEXT("오토플레이 중 스크린샷 간격 (게임 시간 초). 0이면 찍지 않는다"));

static TAutoConsoleVariable<int32> CVarUITest(
	TEXT("yjj.UITest"), 0,
	TEXT("1이면 마우스 조작을 흉내 내는 UI 테스트를 돌리고 게임을 종료한다 (Tools/RunUITest.ps1)"));

namespace
{
	void TakeShot(const FString& Name)
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Screenshots") / (Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] screenshot %s"), *Path);
	}
}

AYJJDefenseGameMode::AYJJDefenseGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AYJJDefensePlayerController::StaticClass();
	HUDClass = AYJJDefenseHUD::StaticClass();
}

void AYJJDefenseGameMode::StartPlay()
{
	SaveGame = YJJProgression::LoadOrCreate();
	Sim.Reset(FYJJRunConfig());

	Super::StartPlay();

	YJJAudio::StartBgm(this);
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
	YJJProgression::SanitizeSelection(*SaveGame);

	FYJJRunConfig Config;
	Config.Tier = SaveGame->SelectedTier;
	Config.Seed = 1000 + SaveGame->RunsPlayed;
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
	UE_LOG(LogYJJDefense, Display, TEXT("[Run] start tier=%d runes=0x%X towers=0x%X"), Config.Tier, Config.RuneMask, Config.TowerMask);
	YJJAudio::Play(this, TEXT("SFX_Click"));
	SaveProgress();
}

void AYJJDefenseGameMode::ReturnToLoadout()
{
	Screen = EYJJScreen::Loadout;
	YJJAudio::Play(this, TEXT("SFX_Click"));
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
	Speed = Speed > 1.5f ? 1.f : 2.f;
}

void AYJJDefenseGameMode::SaveProgress()
{
	YJJProgression::Save(SaveGame);
}

void AYJJDefenseGameMode::FinishRun()
{
	LastStats = FYJJRunStats();
	LastStats.bVictory = Sim.Result == EYJJRunResult::Victory;
	LastStats.Stars = Sim.Stars();
	LastStats.Tier = Sim.Config.Tier;
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
		LastReward = YJJProgression::ApplyRun(*SaveGame, LastStats);
		SaveProgress();
	}

	Screen = EYJJScreen::Result;
	UE_LOG(LogYJJDefense, Display, TEXT("[Run] %s tier=%d lives=%d stars=%d waves=%d kills=%d casts=%d reactions=%d combos=%d(+%d) points=+%d unlocks=%d"),
		LastStats.bVictory ? TEXT("VICTORY") : TEXT("DEFEAT"), LastStats.Tier, Sim.Lives, LastStats.Stars, LastStats.WavesCleared, Sim.Kills, Sim.SpellsCast,
		Sim.Reactions, Sim.CombosCast.Num(), LastReward.NewCombos, LastReward.TotalPoints, LastReward.NewUnlocks.Num());
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
		if (ResultDelay >= 2.2f)
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
	StepAccumulator = FMath::Min(StepAccumulator + DeltaSeconds * Speed, 0.25f);
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
		switch (Event.Type)
		{
		case EYJJSimEvent::EnemyKilled: YJJAudio::Play(this, TEXT("SFX_Pop"), 0.7f); break;
		case EYJJSimEvent::EnemyLeaked: YJJAudio::Play(this, TEXT("SFX_Leak")); break;
		case EYJJSimEvent::TowerBuilt:
			UE_LOG(LogYJJDefense, Display, TEXT("[Battle] tower built at (%.0f, %.0f) gold=%d"), Event.Pos.X, Event.Pos.Y, Sim.Gold);
			YJJAudio::Play(this, TEXT("SFX_Build"));
			break;
		case EYJJSimEvent::TowerUpgraded:
		case EYJJSimEvent::WizardUpgraded: YJJAudio::Play(this, TEXT("SFX_Build")); break;
		case EYJJSimEvent::TowerSold: YJJAudio::Play(this, TEXT("SFX_Coin")); break;
		case EYJJSimEvent::TowerFired:
			YJJAudio::Play(this, Event.Int == static_cast<int32>(EYJJProjectile::Arrow) ? TEXT("SFX_Arrow")
				: (Event.Int == static_cast<int32>(EYJJProjectile::Bomb) ? TEXT("SFX_Cannon") : TEXT("SFX_Magic")), 0.5f);
			break;
		case EYJJSimEvent::ProjectileHit:
			if (Event.Int == static_cast<int32>(EYJJProjectile::Bomb))
			{
				YJJAudio::Play(this, TEXT("SFX_Boom"), 0.7f);
			}
			break;
		case EYJJSimEvent::SpellCast: YJJAudio::Play(this, TEXT("SFX_Cast")); break;
		case EYJJSimEvent::SpellImpact: YJJAudio::Play(this, TEXT("SFX_Boom"), 0.9f); break;
		case EYJJSimEvent::Reaction: YJJAudio::Play(this, TEXT("SFX_Reaction")); break;
		case EYJJSimEvent::ChainArc:
		case EYJJSimEvent::SkyBolt: YJJAudio::Play(this, TEXT("SFX_Zap"), 0.8f); break;
		case EYJJSimEvent::MeleeHit: YJJAudio::Play(this, TEXT("SFX_Hit"), 0.4f); break;
		case EYJJSimEvent::WaveStarted:
			UE_LOG(LogYJJDefense, Display, TEXT("[Battle] wave %d started lives=%d gold=%d"), Event.Int + 1, Sim.Lives, Sim.Gold);
			YJJAudio::Play(this, TEXT("SFX_Horn"));
			break;
		case EYJJSimEvent::WaveBonus: YJJAudio::Play(this, TEXT("SFX_Coin")); break;
		case EYJJSimEvent::Victory: YJJAudio::Play(this, TEXT("SFX_Victory")); break;
		case EYJJSimEvent::Defeat: YJJAudio::Play(this, TEXT("SFX_Defeat")); break;
		default: break;
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
		if (AutoPlayScreenTime > 1.5f && AutoPlayShotIndex == 0)
		{
			TakeShot(TEXT("AutoPlay_Loadout"));
			AutoPlayShotIndex = 1;
		}
		if (AutoPlayScreenTime > 2.5f)
		{
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
			AutoPlayNextReport += 20.f;
			UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] t=%.0f wave=%d/%d lives=%d gold=%d enemies=%d kills=%d casts=%d reactions=%d combos=%d"),
				Sim.Time, Sim.WaveIndex + 1, YJJData::NumWaves(), Sim.Lives, Sim.Gold, Sim.Enemies.Num(), Sim.Kills, Sim.SpellsCast, Sim.Reactions, Sim.CombosCast.Num());
		}
		const float Interval = CVarAutoPlayShotInterval.GetValueOnGameThread();
		if (Interval > 0.f && AutoPlayRuns == 0 && Sim.Time >= Interval * AutoPlayShotIndex - Interval * 0.5f && AutoPlayShotIndex < 12)
		{
			TakeShot(FString::Printf(TEXT("AutoPlay_Battle_%d"), AutoPlayShotIndex));
			AutoPlayShotIndex++;
		}
		AutoPlayScreenTime = 0.f;
		break;
	}

	case EYJJScreen::Result:
		if (AutoPlayScreenTime > 1.5f && !bAutoPlayResultShot)
		{
			bAutoPlayResultShot = true;
			TakeShot(FString::Printf(TEXT("AutoPlay_Result_%d"), AutoPlayRuns));
		}
		if (AutoPlayScreenTime > 3.f)
		{
			AutoPlayRuns++;
			if (ExitAfterRuns > 0 && AutoPlayRuns >= ExitAfterRuns)
			{
				UE_LOG(LogYJJDefense, Display, TEXT("[AutoPlay] finished runs=%d points=%d"), AutoPlayRuns, SaveGame ? SaveGame->Points : 0);
				FPlatformMisc::RequestExit(false, TEXT("yjj.AutoPlayExit"));
				return;
			}
			ReturnToLoadout();
			AutoPlayScreenTime = 1.6f;   // 두 번째 판부터는 준비 화면을 다시 찍지 않는다
		}
		break;
	}
}
