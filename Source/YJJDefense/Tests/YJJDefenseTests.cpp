// 스펠 조합·원소 반응·한 판 진행·수집 자동화 테스트.
// 실행: Tools/RunTests.ps1 (또는 에디터 Session Frontend > Automation 에서 "YJJDefense")

#include "Game/YJJAutoPlayer.h"
#include "Game/YJJDefenseSim.h"
#include "Game/YJJGameData.h"
#include "Game/YJJSpellForge.h"
#include "Meta/YJJSaveGame.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace YJJTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	FYJJSpellSpec Combine(EYJJRune A, EYJJRune B = EYJJRune::None)
	{
		const EYJJRune Runes[] = { A, B };
		return YJJSpellForge::Combine(Runes);
	}

	FYJJRunConfig FullConfig()
	{
		FYJJRunConfig Config;
		Config.RuneMask = (1u << static_cast<uint32>(EYJJRune::Count)) - 1u;
		Config.TowerMask = (1u << static_cast<uint32>(EYJJTower::Count)) - 1u;
		return Config;
	}

	void StepSeconds(FYJJDefenseSim& Sim, float Seconds)
	{
		const int32 Steps = FMath::RoundToInt(Seconds / FYJJDefenseSim::FixedStep);
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Sim.Step();
		}
	}

	// 봇이 한 판을 끝까지 플레이
	FString PlayRun(FYJJDefenseSim& Sim, const FYJJRunConfig& Config, bool bUseSpells)
	{
		Sim.Reset(Config);
		FYJJAutoPlayer Bot;
		Bot.bUseSpells = bUseSpells;

		// 웨이브가 시작될 때마다 목숨을 적어 둔다 (어느 웨이브에서 새는지 보려고)
		FString LivesByWave;
		int32 LastWave = INDEX_NONE;
		for (int32 Step = 0; Step < 60 * 60 * 20 && Sim.Result == EYJJRunResult::Playing; ++Step)
		{
			Bot.Tick(Sim);
			Sim.Step();
			Sim.Events.Reset();
			if (Sim.WaveIndex != LastWave)
			{
				LastWave = Sim.WaveIndex;
				LivesByWave += FString::Printf(TEXT("%d "), Sim.Lives);
			}
		}
		return LivesByWave + FString::Printf(TEXT("→ %d"), Sim.Lives);
	}

	// 스펠 공이 날아가는 동안 적이 걸어갈 만큼 앞을 노린다
	FVector2f LeadTarget(const FYJJDefenseSim& Sim, int32 EnemyId)
	{
		const FYJJEnemyState* Enemy = Sim.FindEnemy(EnemyId);
		if (!Enemy)
		{
			return FVector2f::ZeroVector;
		}
		const FVector2f Muzzle = YJJMap::WizardPos() + FVector2f(-26.f, -118.f);
		float Flight = FMath::Max(FVector2f::Distance(Muzzle, Enemy->Pos) / YJJData::SpellBallSpeed, 0.28f);
		// 한 번 더 고쳐서 도착할 자리까지의 거리로
		const FVector2f Ahead = YJJMap::PointAt(Enemy->Dist + YJJData::Enemy(Enemy->Type).Speed * Flight, Enemy->Lateral);
		Flight = FMath::Max(FVector2f::Distance(Muzzle, Ahead) / YJJData::SpellBallSpeed, 0.28f);
		return YJJMap::PointAt(Enemy->Dist + YJJData::Enemy(Enemy->Type).Speed * Flight, Enemy->Lateral);
	}

	int32 CountEvents(const FYJJDefenseSim& Sim, EYJJSimEvent Type, EYJJReaction Reaction = EYJJReaction::None)
	{
		int32 Count = 0;
		for (const FYJJSimEvent& Event : Sim.Events)
		{
			Count += (Event.Type == Type && (Reaction == EYJJReaction::None || Event.Reaction == Reaction)) ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJForgeRulesTest, "YJJDefense.Spell.CombineRules", YJJTests::Flags)
bool FYJJForgeRulesTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	const FYJJSpellSpec Fireball = Combine(EYJJRune::Fire);
	const FYJJSpellSpec FireExplosion = Combine(EYJJRune::Fire, EYJJRune::Explosion);
	TestEqual(TEXT("파이어볼+익스플로전은 폭발한다"), FireExplosion.Explode, 1);
	TestTrue(TEXT("폭발하면 범위가 넓어진다"), FireExplosion.Radius > Fireball.Radius + 50.f);
	TestEqual(TEXT("원소는 불 하나"), FireExplosion.NumElements, 1);

	const FYJJSpellSpec FireIce = Combine(EYJJRune::Fire, EYJJRune::Ice);
	TestEqual(TEXT("불+얼음 = 증발"), FireIce.Reaction, EYJJReaction::Evaporate);

	const FYJJSpellSpec LightningWater = Combine(EYJJRune::Lightning, EYJJRune::Water);
	TestEqual(TEXT("번개+물 = 감전"), LightningWater.Reaction, EYJJReaction::Electrocute);
	TestEqual(TEXT("순서를 바꿔도 같은 조합"), LightningWater.ComboId, Combine(EYJJRune::Water, EYJJRune::Lightning).ComboId);

	const FYJJSpellSpec FireFire = Combine(EYJJRune::Fire, EYJJRune::Fire);
	TestTrue(TEXT("같은 원소 둘은 강화"), FireFire.bAmplified && FireFire.Damage > Fireball.Damage * 1.5f);

	const FYJJSpellSpec ChainSplit = Combine(EYJJRune::Chain, EYJJRune::Split);
	TestTrue(TEXT("형태 둘도 합쳐진다"), ChainSplit.ChainJumps > 0 && ChainSplit.Projectiles == 3 && ChainSplit.NumElements == 0);

	TestFalse(TEXT("빈 디스크는 스펠이 아니다"), Combine(EYJJRune::None).IsValid());

	// 6원소의 모든 쌍에 서로 다른 반응이 있다
	TSet<EYJJReaction> Seen;
	for (int32 A = 1; A < static_cast<int32>(EYJJElement::Count); ++A)
	{
		for (int32 B = A + 1; B < static_cast<int32>(EYJJElement::Count); ++B)
		{
			const EYJJReaction Reaction = YJJSpellForge::GetReaction(static_cast<EYJJElement>(A), static_cast<EYJJElement>(B));
			TestNotEqual(TEXT("모든 원소 쌍에 반응이 있다"), Reaction, EYJJReaction::None);
			Seen.Add(Reaction);
		}
	}
	TestEqual(TEXT("반응 15가지"), Seen.Num(), 15);

	// 모든 2룬 조합이 이름과 설명을 가진다
	TSet<int32> ComboIds;
	for (int32 A = 0; A < static_cast<int32>(EYJJRune::Count); ++A)
	{
		for (int32 B = A; B < static_cast<int32>(EYJJRune::Count); ++B)
		{
			const FYJJSpellSpec Spec = Combine(static_cast<EYJJRune>(A), static_cast<EYJJRune>(B));
			TestTrue(TEXT("조합에 이름·설명·수치가 있다"), !Spec.Name.IsEmpty() && !Spec.Description.IsEmpty() && Spec.Damage > 0.f && Spec.Cooldown > 0.f);
			ComboIds.Add(Spec.ComboId);
		}
	}
	TestEqual(TEXT("룬 9개 → 조합 45가지"), ComboIds.Num(), 45);
	TestEqual(TEXT("조합 수 계산"), YJJSpellForge::CountCombos(0x1FF), 45);
	TestEqual(TEXT("처음 룬 5개 → 조합 15가지"), YJJSpellForge::CountCombos(0x4F), 15);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJReactionTest, "YJJDefense.Spell.Reactions", YJJTests::Flags)
bool FYJJReactionTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	// 1) 따로 쏜 워터볼 → 라이트닝볼도 감전을 일으킨다 (적에게 묻은 원소와 반응)
	{
		FYJJRunConfig Config = FullConfig();
		Config.Disks[0][0] = EYJJRune::Water;
		Config.Disks[0][1] = EYJJRune::None;
		Config.Disks[1][0] = EYJJRune::Lightning;
		Config.Disks[1][1] = EYJJRune::None;

		FYJJDefenseSim Sim;
		Sim.Reset(Config);
		const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Potato, 2000.f);
		const FVector2f Target = Sim.FindEnemy(Id)->Pos;

		TestTrue(TEXT("워터볼 시전"), Sim.CastSpell(0, Target));
		StepSeconds(Sim, 0.35f);
		TestEqual(TEXT("물만으로는 반응 없음"), Sim.Reactions, 0);
		TestEqual(TEXT("물이 묻는다"), Sim.FindEnemy(Id)->Aura, EYJJElement::Water);

		TestTrue(TEXT("라이트닝볼 시전"), Sim.CastSpell(1, Sim.FindEnemy(Id)->Pos));
		StepSeconds(Sim, 1.5f);
		TestEqual(TEXT("감전 발생"), CountEvents(Sim, EYJJSimEvent::Reaction, EYJJReaction::Electrocute), 1);
		TestEqual(TEXT("반응 후 원소가 사라진다"), Sim.FindEnemy(Id)->Aura, EYJJElement::None);
	}

	// 2) 라이트닝볼+워터볼 한 디스크는 라이트닝볼 단독보다 아프고, 옆의 적도 맞는다
	auto DamageTo = [](EYJJRune A, EYJJRune B, float& OutNeighborDamage)
	{
		FYJJRunConfig Config = FullConfig();
		Config.Disks[0][0] = A;
		Config.Disks[0][1] = B;
		FYJJDefenseSim Sim;
		Sim.Reset(Config);
		const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Potato, 2000.f);
		const int32 Neighbor = Sim.DebugSpawnEnemy(EYJJEnemy::Potato, 2110.f);
		Sim.CastSpell(0, Sim.FindEnemy(Id)->Pos);
		for (int32 Step = 0; Step < 20; ++Step)
		{
			Sim.Step();
		}
		OutNeighborDamage = Sim.FindEnemy(Neighbor)->MaxHP - Sim.FindEnemy(Neighbor)->HP;
		return Sim.FindEnemy(Id)->MaxHP - Sim.FindEnemy(Id)->HP;
	};
	float SoloNeighbor = 0.f;
	float ComboNeighbor = 0.f;
	const float Solo = DamageTo(EYJJRune::Lightning, EYJJRune::None, SoloNeighbor);
	const float Combo = DamageTo(EYJJRune::Lightning, EYJJRune::Water, ComboNeighbor);
	TestTrue(TEXT("감전은 추가 피해를 준다"), Combo > Solo * 1.4f);
	TestTrue(TEXT("단독 볼은 옆의 적을 못 맞힌다"), SoloNeighbor <= 0.f);
	TestTrue(TEXT("감전은 더 넓게 맞힌다"), ComboNeighbor > 0.f);

	// 3) 얼음+물 = 빙결: 적이 멈춘다
	{
		FYJJRunConfig Config = FullConfig();
		Config.Disks[0][0] = EYJJRune::Ice;
		Config.Disks[0][1] = EYJJRune::Water;
		FYJJDefenseSim Sim;
		Sim.Reset(Config);
		const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Imp, 2000.f);
		Sim.CastSpell(0, Sim.FindEnemy(Id)->Pos);
		StepSeconds(Sim, 0.6f);
		const float DistAfterHit = Sim.FindEnemy(Id)->Dist;
		TestTrue(TEXT("얼어붙는다"), Sim.FindEnemy(Id)->FreezeTime > 0.f);
		StepSeconds(Sim, 1.f);
		TestEqual(TEXT("얼어 있는 동안 못 움직인다"), Sim.FindEnemy(Id)->Dist, DistAfterHit);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJTowerTest, "YJJDefense.Battle.Towers", YJJTests::Flags)
bool FYJJTowerTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	FYJJDefenseSim Sim;
	Sim.Reset(FullConfig());
	const int32 StartGold = Sim.Gold;

	TestTrue(TEXT("궁수 초소 건설"), Sim.BuildTower(3, EYJJTower::Archer));
	TestFalse(TEXT("같은 자리에 또 못 짓는다"), Sim.BuildTower(3, EYJJTower::Mage));
	TestEqual(TEXT("건설비 차감"), Sim.Gold, StartGold - YJJData::Tower(EYJJTower::Archer).Levels[0].Cost);
	TestTrue(TEXT("강화"), Sim.UpgradeTower(3) && Sim.Towers[3].Level == 1);

	// 궁수가 사거리를 지나가는 적을 잡는다
	const FVector2f Spot = YJJMap::Spots()[3];
	const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Imp, YJJMap::NearestDist(Spot) - 150.f);
	StepSeconds(Sim, 8.f);
	TestTrue(TEXT("궁수가 도깨비를 처치"), Sim.FindEnemy(Id) == nullptr && Sim.Kills == 1);

	// 보병은 길을 막는다
	TestTrue(TEXT("보병 막사 건설"), Sim.BuildTower(4, EYJJTower::Barracks));
	TestEqual(TEXT("병졸 셋"), Sim.Soldiers.Num(), 3);
	StepSeconds(Sim, 3.f);
	const float Rally = YJJMap::NearestDist(YJJMap::Spots()[4]);
	const int32 PotatoId = Sim.DebugSpawnEnemy(EYJJEnemy::Potato, Rally - 60.f);
	StepSeconds(Sim, 4.f);
	const FYJJEnemyState* Potato = Sim.FindEnemy(PotatoId);
	TestTrue(TEXT("감자가 병졸에게 막힌다"), Potato && Potato->BlockerId != INDEX_NONE && Potato->Dist < Rally + 40.f);

	const int32 GoldBeforeSell = Sim.Gold;
	TestTrue(TEXT("막사 판매"), Sim.SellTower(4));
	TestTrue(TEXT("환불 + 병졸 철수"), Sim.Gold > GoldBeforeSell && Sim.Soldiers.Num() == 0);
	StepSeconds(Sim, 0.5f);
	TestTrue(TEXT("막던 병졸이 없어지면 다시 걷는다"), Sim.FindEnemy(PotatoId) && Sim.FindEnemy(PotatoId)->BlockerId == INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJFullRunTest, "YJJDefense.Battle.FullRun", YJJTests::Flags)
bool FYJJFullRunTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	// 처음 주어지는 룬·병졸만으로
	const FYJJRunConfig Config;

	FYJJDefenseSim WithSpells;
	const FString WithSpellsLives = PlayRun(WithSpells, Config, true);
	AddInfo(FString::Printf(TEXT("스펠 사용: 결과=%d 웨이브=%d 처치=%d 시전=%d 반응=%d 조합=%d 시간=%.0f초 골드=%d 목숨=[%s]"),
		static_cast<int32>(WithSpells.Result), WithSpells.WaveIndex + 1, WithSpells.Kills, WithSpells.SpellsCast, WithSpells.Reactions,
		WithSpells.CombosCast.Num(), WithSpells.Time, WithSpells.Gold, *WithSpellsLives));
	TestEqual(TEXT("봇이 1단계를 깬다"), WithSpells.Result, EYJJRunResult::Victory);
	TestTrue(TEXT("반응이 일어난다"), WithSpells.Reactions > 0);
	TestTrue(TEXT("여러 조합을 써 본다"), WithSpells.CombosCast.Num() >= 5);

	FYJJDefenseSim NoSpells;
	const FString NoSpellsLives = PlayRun(NoSpells, Config, false);
	AddInfo(FString::Printf(TEXT("스펠 없이: 결과=%d 웨이브=%d 처치=%d 목숨=[%s]"), static_cast<int32>(NoSpells.Result), NoSpells.WaveIndex + 1, NoSpells.Kills, *NoSpellsLives));
	TestTrue(TEXT("스펠 없이는 더 힘들다"), NoSpells.Result != EYJJRunResult::Victory || NoSpells.Lives < WithSpells.Lives);

	// 같은 입력이면 같은 결과
	FYJJDefenseSim Again;
	PlayRun(Again, Config, true);
	TestTrue(TEXT("결정적"), Again.Lives == WithSpells.Lives && Again.Kills == WithSpells.Kills && Again.Gold == WithSpells.Gold && Again.Time == WithSpells.Time);

	// 높은 단계는 더 어렵다
	FYJJRunConfig Hard = Config;
	Hard.Tier = 6;
	FYJJDefenseSim HardRun;
	const FString HardLives = PlayRun(HardRun, Hard, true);
	AddInfo(FString::Printf(TEXT("6단계: 결과=%d 웨이브=%d 목숨=[%s]"), static_cast<int32>(HardRun.Result), HardRun.WaveIndex + 1, *HardLives));

	// 2단계·3단계는 참고용 수치만 남긴다
	for (const int32 Tier : { 2, 3 })
	{
		FYJJRunConfig TierConfig = Config;
		TierConfig.Tier = Tier;
		FYJJDefenseSim TierRun;
		const FString TierLives = PlayRun(TierRun, TierConfig, true);
		AddInfo(FString::Printf(TEXT("%d단계: 결과=%d 웨이브=%d 목숨=[%s]"), Tier, static_cast<int32>(TierRun.Result), TierRun.WaveIndex + 1, *TierLives));
	}
	TestTrue(TEXT("6단계는 처음 룬만으로는 못 깬다"), HardRun.Result == EYJJRunResult::Defeat);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJProgressionTest, "YJJDefense.Meta.Progression", YJJTests::Flags)
bool FYJJProgressionTest::RunTest(const FString& Parameters)
{
	UYJJSaveGame* SaveGame = NewObject<UYJJSaveGame>();
	YJJProgression::SanitizeSelection(*SaveGame);
	TestEqual(TEXT("처음엔 룬 5개를 모두 들고 나간다"), FMath::CountBits(static_cast<uint32>(SaveGame->SelectedRunes)), 5);
	TestEqual(TEXT("처음엔 병졸 3종"), FMath::CountBits(static_cast<uint32>(SaveGame->SelectedTowers)), 3);

	FYJJRunStats Stats;
	Stats.bVictory = true;
	Stats.Stars = 3;
	Stats.WavesCleared = 10;
	Stats.CombosCast = { YJJSpellForge::MakeComboId(EYJJRune::Fire, EYJJRune::Explosion), YJJSpellForge::MakeComboId(EYJJRune::Water, EYJJRune::Lightning) };

	TestEqual(TEXT("처음엔 지도 하나"), SaveGame->UnlockedMaps, 1);
	const FYJJRunReward First = YJJProgression::ApplyRun(*SaveGame, Stats);
	TestEqual(TEXT("새 조합 둘"), First.NewCombos, 2);
	TestEqual(TEXT("점수 = 웨이브 5 + 승리 3 + 별 3 + 조합 2"), First.TotalPoints, 13);
	TestTrue(TEXT("첫 해금은 바람 룬"), First.NewUnlocks.Num() == 1 && !First.NewUnlocks[0].bTower && First.NewUnlocks[0].Index == static_cast<uint8>(EYJJRune::Wind));
	TestTrue(TEXT("바람 룬이 풀렸다"), (SaveGame->UnlockedRunes & (1 << static_cast<int32>(EYJJRune::Wind))) != 0);
	TestTrue(TEXT("다음 단계가 열린다"), First.bNewTier && SaveGame->MaxTier == 2);
	TestTrue(TEXT("들판을 깨면 해변이 열린다"), First.NewMap == 1 && SaveGame->UnlockedMaps == 2);

	const FYJJRunReward Second = YJJProgression::ApplyRun(*SaveGame, Stats);
	TestEqual(TEXT("이미 아는 조합은 점수가 없다"), Second.NewCombos, 0);
	TestTrue(TEXT("두 번째 해금은 마법사 탑"), Second.NewUnlocks.Num() == 1 && Second.NewUnlocks[0].bTower);
	TestEqual(TEXT("같은 지도를 또 깨도 새 지도는 없다"), Second.NewMap, static_cast<int32>(INDEX_NONE));

	FYJJRunStats Beach = Stats;
	Beach.Map = 1;
	TestEqual(TEXT("해변을 깨면 설산이 열린다"), YJJProgression::ApplyRun(*SaveGame, Beach).NewMap, 2);
	FYJJRunStats Snow = Stats;
	Snow.Map = 2;
	TestEqual(TEXT("마지막 지도 다음은 없다"), YJJProgression::ApplyRun(*SaveGame, Snow).NewMap, static_cast<int32>(INDEX_NONE));
	SaveGame->SelectedMap = 9;
	YJJProgression::SanitizeSelection(*SaveGame);
	TestEqual(TEXT("없는 지도는 고를 수 없다"), SaveGame->SelectedMap, YJJMap::NumMaps() - 1);

	// 가방은 룬 5개까지: 여섯 번째는 못 넣고, 하나 빼면 넣을 수 있다
	TestFalse(TEXT("가방이 꽉 차면 못 넣는다"), YJJProgression::ToggleRune(*SaveGame, EYJJRune::Wind));
	TestTrue(TEXT("하나 빼기"), YJJProgression::ToggleRune(*SaveGame, EYJJRune::Ice));
	TestTrue(TEXT("빈자리에 넣기"), YJJProgression::ToggleRune(*SaveGame, EYJJRune::Wind));
	TestFalse(TEXT("잠긴 룬은 못 넣는다"), YJJProgression::ToggleRune(*SaveGame, EYJJRune::Split));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJEnemyTraitsTest, "YJJDefense.Battle.EnemyTraits", YJJTests::Flags)
bool FYJJEnemyTraitsTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	// 맵마다 길과 자리가 다르다
	{
		FYJJDefenseSim Sim;
		FYJJRunConfig Config = FullConfig();
		Config.Map = 1;
		Sim.Reset(Config);
		TestEqual(TEXT("해변 맵이 골라진다"), YJJMap::GetActive(), 1);
		TestEqual(TEXT("건설 자리 수 = 해변 자리 수"), Sim.Towers.Num(), YJJMap::Spots().Num());
		TestEqual(TEXT("해변 웨이브 10개"), YJJData::NumWaves(), 10);
	}

	// 코코넛: 깨지면 알맹이 둘
	{
		FYJJRunConfig Config = FullConfig();
		Config.Disks[0][0] = EYJJRune::Lightning;
		Config.Disks[0][1] = EYJJRune::Lightning;
		FYJJDefenseSim Sim;
		Sim.Reset(Config);
		const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Coconut, 1500.f);
		const FVector2f Pos = Sim.FindEnemy(Id)->Pos;
		Sim.WizardLevel = YJJData::MaxWizardLevel;   // 한 방에 깨지게
		Sim.CastSpell(0, Pos);
		StepSeconds(Sim, 1.5f);
		int32 Kids = 0;
		for (const FYJJEnemyState& Enemy : Sim.Enemies)
		{
			Kids += (Enemy.Type == EYJJEnemy::CocoKid && FMath::Abs(Enemy.Dist - 1500.f) < 200.f) ? 1 : 0;
		}
		TestTrue(TEXT("코코넛이 처치됐다"), Sim.FindEnemy(Id) == nullptr && Sim.Kills >= 1);
		TestEqual(TEXT("코코 알맹이 둘이 나온다"), Kids + (Sim.Kills - 1), 2);
	}

	// 펭귄: 얼음 둔화·빙결이 안 통하지만 원소는 묻는다
	{
		FYJJRunConfig Config = FullConfig();
		Config.Disks[0][0] = EYJJRune::Ice;
		Config.Disks[0][1] = EYJJRune::None;
		Config.Disks[1][0] = EYJJRune::Ice;
		Config.Disks[1][1] = EYJJRune::Water;
		FYJJDefenseSim Sim;
		Sim.Reset(Config);
		const int32 Id = Sim.DebugSpawnEnemy(EYJJEnemy::Penguin, 2400.f);
		Sim.CastSpell(0, LeadTarget(Sim, Id));
		bool bImmuneShown = false;
		// 공이 날아가는 시간(거리 700px 남짓 = 0.7초쯤)보다 넉넉히
		for (int32 Step = 0; Step < 90; ++Step)
		{
			Sim.Step();
			bImmuneShown |= CountEvents(Sim, EYJJSimEvent::EnemyImmune) > 0;
			Sim.Events.Reset();
		}
		const FYJJEnemyState* Penguin = Sim.FindEnemy(Id);
		TestTrue(TEXT("아이스볼에 맞아도 안 느려진다"), Penguin && Penguin->SlowTime <= 0.f);
		TestTrue(TEXT("\"안 얼어!\" 표시"), bImmuneShown);
		TestTrue(TEXT("얼음 원소는 묻는다 (반응용)"), Penguin && Penguin->Aura == EYJJElement::Ice);

		Sim.CastSpell(1, LeadTarget(Sim, Id));
		StepSeconds(Sim, 1.5f);
		Penguin = Sim.FindEnemy(Id);
		TestTrue(TEXT("빙결도 안 걸린다"), !Penguin || Penguin->FreezeTime <= 0.f);

		// 같은 자리의 꼬마 도깨비는 느려진다 (비교)
		const int32 ImpId = Sim.DebugSpawnEnemy(EYJJEnemy::Imp, 1200.f);
		Sim.Disks[0].Cooldown = 0.f;
		Sim.CastSpell(0, LeadTarget(Sim, ImpId));
		StepSeconds(Sim, 1.5f);
		TestTrue(TEXT("보통 몬스터는 느려진다"), Sim.FindEnemy(ImpId) && Sim.FindEnemy(ImpId)->SlowTime > 0.f);
	}

	// 모든 몬스터가 어느 맵엔가 나온다
	TSet<EYJJEnemy> Seen;
	for (int32 Map = 0; Map < YJJMap::NumMaps(); ++Map)
	{
		YJJMap::SetActive(Map);
		for (int32 Wave = 0; Wave < YJJData::NumWaves(); ++Wave)
		{
			for (const FYJJWaveGroup& Group : YJJData::Wave(Wave))
			{
				Seen.Add(Group.Type);
				const FYJJEnemyDef& Def = YJJData::Enemy(Group.Type);
				if (Def.HasSplit())
				{
					Seen.Add(Def.SplitInto);
				}
			}
		}
	}
	YJJMap::SetActive(0);
	TestEqual(TEXT("몬스터 15종이 모두 등장"), Seen.Num(), static_cast<int32>(EYJJEnemy::Count));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYJJAllMapsTest, "YJJDefense.Battle.AllMaps", YJJTests::Flags)
bool FYJJAllMapsTest::RunTest(const FString& Parameters)
{
	using namespace YJJTests;

	// 그 지도에 처음 갈 때쯤 가진 것으로 깰 수 있어야 한다
	// 들판: 처음 룬·병졸 / 해변: + 바람(가방 5개라 폭발 대신) / 설산: + 마법사 탑
	const uint32 Start = FYJJRunConfig().RuneMask;
	const uint32 RuneMasks[] = { Start, Start, Start };
	const uint32 TowerMasks[] = { 0x07, 0x07, 0x0F };
	for (int32 Map = 0; Map < YJJMap::NumMaps(); ++Map)
	{
		for (const int32 Tier : { 1, 2 })
		{
			FYJJRunConfig Config;
			Config.Map = Map;
			Config.Tier = Tier;
			Config.RuneMask = RuneMasks[FMath::Min(Map, 2)];
			Config.TowerMask = TowerMasks[FMath::Min(Map, 2)];
			FYJJDefenseSim Sim;
			const FString Lives = PlayRun(Sim, Config, true);
			AddInfo(FString::Printf(TEXT("%s %d단계: 결과=%d 웨이브=%d 처치=%d 반응=%d 시간=%.0f초 목숨=[%s]"),
				YJJMap::Name(Map), Tier, static_cast<int32>(Sim.Result), Sim.WaveIndex + 1, Sim.Kills, Sim.Reactions, Sim.Time, *Lives));
			if (Tier == 1)
			{
				TestEqual(FString::Printf(TEXT("%s 1단계를 봇이 깬다"), YJJMap::Name(Map)), Sim.Result, EYJJRunResult::Victory);
			}
		}
	}
	return true;
}

#endif
