#include "Game/YJJAutoPlayer.h"
#include "Game/YJJGameData.h"

namespace
{
	// 길을 많이 덮는 자리 순서대로 이 병졸을 짓는다
	const EYJJTower GBuildPattern[] = {
		EYJJTower::Archer, EYJJTower::Barracks, EYJJTower::Archer, EYJJTower::Bombard, EYJJTower::Mage, EYJJTower::Barracks,
		EYJJTower::Bombard, EYJJTower::Shrine, EYJJTower::Archer, EYJJTower::Mage, EYJJTower::Archer, EYJJTower::Archer,
	};
}

void FYJJAutoPlayer::PlanBuildOrder()
{
	PlannedMap = YJJMap::GetActive();
	BuildOrder.Reset();

	const TConstArrayView<FVector2f> Spots = YJJMap::Spots();
	const float Length = YJJMap::PathLength();
	TArray<TPair<float, int32>> Scored;
	for (int32 Spot = 0; Spot < Spots.Num(); ++Spot)
	{
		// 사거리 260 안에 들어오는 길 길이. 성문에 가까운 쪽에 조금 더 점수
		float Score = 0.f;
		for (float Dist = 0.f; Dist < Length; Dist += 20.f)
		{
			if (YJJ::RangeDist(YJJMap::PointAt(Dist), Spots[Spot]) <= 260.f)
			{
				Score += 1.f + 0.4f * Dist / Length;
			}
		}
		Scored.Add({ Score, Spot });
	}
	Scored.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });

	for (int32 Index = 0; Index < Scored.Num(); ++Index)
	{
		BuildOrder.Add({ Scored[Index].Value, GBuildPattern[Index % UE_ARRAY_COUNT(GBuildPattern)] });
	}
}

void FYJJAutoPlayer::Reset()
{
	ThinkTimer = 0.f;
	ForgeTimer = 0.f;
	BuildIndex = 0;
	ForgeCursor = 0;
	PlannedMap = INDEX_NONE;
}

void FYJJAutoPlayer::Tick(FYJJDefenseSim& Sim)
{
	if (Sim.Result != EYJJRunResult::Playing)
	{
		return;
	}

	ThinkTimer += FYJJDefenseSim::FixedStep;
	if (ThinkTimer < 0.25f)
	{
		return;
	}
	ThinkTimer = 0.f;

	TickEconomy(Sim);

	// 첫 웨이브는 조금 준비한 뒤, 이후는 조금 일찍 부른다
	if (Sim.CanCallWave() && ((Sim.WaveIndex < 0 && Sim.Time > 2.f) || (Sim.WaveIndex >= 0 && Sim.NextWaveTimer < 10.f)))
	{
		Sim.CallNextWave();
	}

	if (bUseSpells)
	{
		if (bExperiment)
		{
			TickForge(Sim);
		}
		TickSpells(Sim);
	}
}

void FYJJAutoPlayer::TickEconomy(FYJJDefenseSim& Sim)
{
	if (PlannedMap != YJJMap::GetActive() || BuildOrder.IsEmpty())
	{
		PlanBuildOrder();
		BuildIndex = 0;
	}

	// 이미 지어진 자리는 건너뛴다
	while (BuildOrder.IsValidIndex(BuildIndex) && Sim.Towers[BuildOrder[BuildIndex].Spot].bBuilt)
	{
		BuildIndex++;
	}

	int32 BuildCost = MAX_int32;
	EYJJTower BuildType = EYJJTower::Archer;
	if (BuildOrder.IsValidIndex(BuildIndex))
	{
		BuildType = BuildOrder[BuildIndex].Preferred;
		if (!Sim.HasTower(BuildType))
		{
			// 못 들고 나왔으면 궁수, 그것도 없으면 아무거나
			BuildType = EYJJTower::Archer;
			for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count) && !Sim.HasTower(BuildType); ++Index)
			{
				BuildType = static_cast<EYJJTower>(Index);
			}
		}
		BuildCost = YJJData::Tower(BuildType).Levels[0].Cost;
	}

	// 가장 싼 강화
	int32 UpgradeSpot = INDEX_NONE;
	int32 UpgradeCost = MAX_int32;
	for (int32 Spot = 0; Spot < Sim.Towers.Num(); ++Spot)
	{
		const int32 Cost = Sim.UpgradeCost(Spot);
		if (Cost > 0 && Cost < UpgradeCost)
		{
			UpgradeCost = Cost;
			UpgradeSpot = Spot;
		}
	}

	// 처음 넷은 무조건 새로 짓고, 그 뒤로는 싼 것부터
	const bool bPreferBuild = BuildIndex < 4 || BuildCost <= UpgradeCost;
	if (bPreferBuild && BuildCost != MAX_int32)
	{
		if (Sim.Gold >= BuildCost)
		{
			Sim.BuildTower(BuildOrder[BuildIndex].Spot, BuildType);
		}
	}
	else if (UpgradeSpot != INDEX_NONE && Sim.Gold >= UpgradeCost)
	{
		Sim.UpgradeTower(UpgradeSpot);
	}

	// 여유가 있으면 마력 강화
	const int32 WizardCost = Sim.WizardUpgradeCost();
	if (bUseSpells && WizardCost > 0 && Sim.WaveIndex >= 2 && Sim.Gold >= WizardCost + 60)
	{
		Sim.UpgradeWizard();
	}
}

void FYJJAutoPlayer::TickSpells(FYJJDefenseSim& Sim)
{
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		if (!Sim.CanCast(Disk))
		{
			continue;
		}

		// 주변에 적이 가장 많이 몰린 적을 노린다 (같으면 더 앞선 적)
		const float Radius = FMath::Max(Sim.Disks[Disk].Spell.Radius, 90.f);
		const FYJJEnemyState* Best = nullptr;
		float BestScore = 0.f;
		for (const FYJJEnemyState& Enemy : Sim.Enemies)
		{
			if (!Enemy.IsAlive() || Enemy.Dist < 120.f)
			{
				continue;
			}
			float Score = Enemy.Dist * 0.0005f;
			for (const FYJJEnemyState& Other : Sim.Enemies)
			{
				if (Other.IsAlive() && YJJ::RangeDist(Other.Pos, Enemy.Pos) <= Radius)
				{
					Score += YJJData::Enemy(Other.Type).bBoss ? 4.f : 1.f;
				}
			}
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = &Enemy;
			}
		}

		if (Best)
		{
			// 날아가는 동안 걸어올 만큼 앞을 겨냥
			const bool bStanding = Best->BlockerId != INDEX_NONE || Best->IsDisabled();
			const float Lead = bStanding ? 0.f : YJJData::Enemy(Best->Type).Speed * 0.45f;
			Sim.CastSpell(Disk, YJJMap::PointAt(Best->Dist + Lead, Best->Lateral));
			return;   // 한 번에 하나씩
		}
	}
}

void FYJJAutoPlayer::TickForge(FYJJDefenseSim& Sim)
{
	ForgeTimer += 0.25f;
	if (ForgeTimer < 18.f || Sim.WaveIndex < 0)
	{
		return;
	}
	ForgeTimer = 0.f;

	// 들고 나온 룬으로 만들 수 있는 조합을 차례로 돌며, 아직 안 써 본 것을 마지막 디스크에 끼운다
	TArray<EYJJRune, TInlineAllocator<9>> Bag;
	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		if (Sim.HasRune(static_cast<EYJJRune>(Index)))
		{
			Bag.Add(static_cast<EYJJRune>(Index));
		}
	}
	if (Bag.Num() == 0)
	{
		return;
	}

	const int32 NumPairs = Bag.Num() * Bag.Num();
	for (int32 Try = 0; Try < NumPairs; ++Try)
	{
		const int32 Cursor = (ForgeCursor + Try) % NumPairs;
		const EYJJRune A = Bag[Cursor / Bag.Num()];
		const EYJJRune B = Bag[Cursor % Bag.Num()];
		if (A > B || Sim.CombosCast.Contains(YJJSpellForge::MakeComboId(A, B)))
		{
			continue;
		}

		const int32 Disk = YJJ::NumDisks - 1;
		Sim.SetSocket(Disk, 0, A);
		Sim.SetSocket(Disk, 1, B);
		ForgeCursor = Cursor + 1;
		return;
	}
}
