#include "Game/YJJAutoPlayer.h"
#include "Game/YJJGameData.h"

namespace
{
	struct FBuildStep
	{
		int32 Spot;
		EYJJTower Preferred;
		EYJJTower Fallback;
	};

	// 길이 두 번 지나가는 가운데 줄부터 채운다
	const FBuildStep GBuildOrder[] = {
		{ 3, EYJJTower::Archer, EYJJTower::Archer },
		{ 4, EYJJTower::Barracks, EYJJTower::Archer },
		{ 0, EYJJTower::Archer, EYJJTower::Archer },
		{ 8, EYJJTower::Bombard, EYJJTower::Archer },
		{ 5, EYJJTower::Mage, EYJJTower::Archer },
		{ 9, EYJJTower::Barracks, EYJJTower::Archer },
		{ 10, EYJJTower::Bombard, EYJJTower::Archer },
		{ 1, EYJJTower::Shrine, EYJJTower::Archer },
		{ 6, EYJJTower::Archer, EYJJTower::Archer },
		{ 2, EYJJTower::Mage, EYJJTower::Archer },
		{ 7, EYJJTower::Archer, EYJJTower::Archer },
	};
}

void FYJJAutoPlayer::Reset()
{
	ThinkTimer = 0.f;
	ForgeTimer = 0.f;
	BuildIndex = 0;
	ForgeCursor = 0;
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
	// 이미 지어진 자리는 건너뛴다
	while (BuildIndex < UE_ARRAY_COUNT(GBuildOrder) && Sim.Towers.IsValidIndex(GBuildOrder[BuildIndex].Spot) && Sim.Towers[GBuildOrder[BuildIndex].Spot].bBuilt)
	{
		BuildIndex++;
	}

	int32 BuildCost = MAX_int32;
	EYJJTower BuildType = EYJJTower::Archer;
	if (BuildIndex < UE_ARRAY_COUNT(GBuildOrder))
	{
		const FBuildStep& Step = GBuildOrder[BuildIndex];
		BuildType = Sim.HasTower(Step.Preferred) ? Step.Preferred : Step.Fallback;
		if (!Sim.HasTower(BuildType))
		{
			// 들고 나온 것 중 아무거나
			for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count); ++Index)
			{
				if (Sim.HasTower(static_cast<EYJJTower>(Index)))
				{
					BuildType = static_cast<EYJJTower>(Index);
					break;
				}
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
			Sim.BuildTower(GBuildOrder[BuildIndex].Spot, BuildType);
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
