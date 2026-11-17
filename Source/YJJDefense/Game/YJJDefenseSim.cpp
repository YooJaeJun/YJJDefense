#include "Game/YJJDefenseSim.h"
#include "Game/YJJGameData.h"

namespace
{
	constexpr float Dt = FYJJDefenseSim::FixedStep;

	// 보스는 군중 제어를 덜 받는다
	constexpr float BossControlScale = 0.35f;

	const FVector2f SoldierSlotOffsets[YJJ::SoldiersPerBarracks] = { FVector2f(-24.f, -8.f), FVector2f(24.f, -4.f), FVector2f(0.f, 16.f) };

	FVector2f TowerMuzzle(const FVector2f& SpotPos)
	{
		return SpotPos + FVector2f(0.f, -92.f);
	}

	FVector2f WizardMuzzle()
	{
		return YJJMap::WizardPos() + FVector2f(-26.f, -118.f);
	}
}

template <typename FuncType>
void FYJJDefenseSim::ForEnemiesInRadius(const FVector2f& Pos, float Radius, FuncType&& Func)
{
	// Func 안에서 적이 죽거나 밀려날 수 있지만 배열 크기는 스텝 끝에서만 줄어든다
	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		FYJJEnemyState& Enemy = Enemies[Index];
		if (Enemy.IsAlive() && YJJ::RangeDist(Enemy.Pos, Pos) <= Radius + YJJData::Enemy(Enemy.Type).Radius * 0.5f)
		{
			Func(Enemy);
		}
	}
}

void FYJJDefenseSim::Reset(const FYJJRunConfig& InConfig)
{
	Config = InConfig;
	Config.Tier = FMath::Max(Config.Tier, 1);

	Result = EYJJRunResult::Playing;
	Time = 0.f;
	Gold = YJJData::StartGold;
	Lives = YJJData::StartLives;
	WaveIndex = INDEX_NONE;
	NextWaveTimer = 0.f;
	WizardLevel = 0;
	WizardCastAnim = 0.f;
	WaveTime = 0.f;
	HPScale = 1.f;
	NextId = 1;
	Kills = 0;
	SpellsCast = 0;
	Reactions = 0;

	Enemies.Reset();
	Soldiers.Reset();
	Projectiles.Reset();
	Zones.Reset();
	Events.Reset();
	CombosCast.Reset();
	SpawnQueue.Reset();
	Towers.Reset();
	Towers.SetNum(YJJMap::Spots().Num());
	Rng.Initialize(Config.Seed);

	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		Disks[Disk] = FYJJDiskState();
		for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
		{
			const EYJJRune Rune = Config.Disks[Disk][Socket];
			Disks[Disk].Sockets[Socket] = HasRune(Rune) ? Rune : EYJJRune::None;
		}
		Disks[Disk].Spell = YJJSpellForge::Combine(Disks[Disk].Sockets);
	}
}

void FYJJDefenseSim::Step()
{
	if (Result != EYJJRunResult::Playing)
	{
		return;
	}

	Time += Dt;
	WizardCastAnim = FMath::Max(WizardCastAnim - Dt, 0.f);
	for (FYJJDiskState& Disk : Disks)
	{
		Disk.Cooldown = FMath::Max(Disk.Cooldown - Dt, 0.f);
	}

	StepWaves();
	StepEnemies();
	StepSoldiers();
	StepTowers();
	StepProjectiles();
	StepZones();

	Enemies.RemoveAll([](const FYJJEnemyState& Enemy) { return !Enemy.IsAlive(); });

	if (Lives <= 0)
	{
		Lives = 0;
		Result = EYJJRunResult::Defeat;
		Emit(EYJJSimEvent::Defeat, YJJMap::GatePos());
	}
	else if (WaveIndex == YJJData::NumWaves() - 1 && SpawnQueue.IsEmpty() && Enemies.IsEmpty())
	{
		Result = EYJJRunResult::Victory;
		Emit(EYJJSimEvent::Victory, YJJMap::GatePos());
	}
}

// ---------------------------------------------------------------------------------------------
// 조회

const FYJJEnemyState* FYJJDefenseSim::FindEnemy(int32 Id) const
{
	return Id == INDEX_NONE ? nullptr : Enemies.FindByPredicate([Id](const FYJJEnemyState& Enemy) { return Enemy.Id == Id && Enemy.IsAlive(); });
}

FYJJEnemyState* FYJJDefenseSim::FindEnemyMutable(int32 Id)
{
	return const_cast<FYJJEnemyState*>(FindEnemy(Id));
}

FYJJSoldierState* FYJJDefenseSim::FindSoldier(int32 Id)
{
	return Id == INDEX_NONE ? nullptr : Soldiers.FindByPredicate([Id](const FYJJSoldierState& Soldier) { return Soldier.Id == Id; });
}

FYJJSimEvent& FYJJDefenseSim::Emit(EYJJSimEvent Type, const FVector2f& Pos)
{
	FYJJSimEvent& Event = Events.AddDefaulted_GetRef();
	Event.Type = Type;
	Event.Pos = Pos;
	return Event;
}

const FYJJTowerLevel& FYJJDefenseSim::TowerLevel(int32 Spot) const
{
	const FYJJTowerState& Tower = Towers[Spot];
	return YJJData::Tower(Tower.Type).Levels[Tower.Level];
}

bool FYJJDefenseSim::CanBuild(int32 Spot, EYJJTower Type) const
{
	return Result == EYJJRunResult::Playing && Towers.IsValidIndex(Spot) && !Towers[Spot].bBuilt && Type < EYJJTower::Count && HasTower(Type)
		&& Gold >= YJJData::Tower(Type).Levels[0].Cost;
}

int32 FYJJDefenseSim::UpgradeCost(int32 Spot) const
{
	if (!Towers.IsValidIndex(Spot) || !Towers[Spot].bBuilt || Towers[Spot].Level >= YJJ::MaxTowerLevel)
	{
		return 0;
	}
	return YJJData::Tower(Towers[Spot].Type).Levels[Towers[Spot].Level + 1].Cost;
}

bool FYJJDefenseSim::CanUpgrade(int32 Spot) const
{
	const int32 Cost = UpgradeCost(Spot);
	return Result == EYJJRunResult::Playing && Cost > 0 && Gold >= Cost;
}

int32 FYJJDefenseSim::SellValue(int32 Spot) const
{
	return Towers.IsValidIndex(Spot) && Towers[Spot].bBuilt ? FMath::FloorToInt(Towers[Spot].Spent * YJJData::SellRefund) : 0;
}

bool FYJJDefenseSim::CanCallWave() const
{
	return Result == EYJJRunResult::Playing && WaveIndex + 1 < YJJData::NumWaves() && SpawnQueue.IsEmpty();
}

bool FYJJDefenseSim::CanCast(int32 Disk) const
{
	return Result == EYJJRunResult::Playing && Disk >= 0 && Disk < YJJ::NumDisks && Disks[Disk].Spell.IsValid() && Disks[Disk].Cooldown <= 0.f;
}

int32 FYJJDefenseSim::WizardUpgradeCost() const
{
	return WizardLevel < YJJData::MaxWizardLevel ? YJJData::WizardUpgradeCost[WizardLevel] : 0;
}

int32 FYJJDefenseSim::Stars() const
{
	if (Result != EYJJRunResult::Victory)
	{
		return 0;
	}
	return Lives >= 18 ? 3 : (Lives >= 8 ? 2 : 1);
}

// ---------------------------------------------------------------------------------------------
// 명령

bool FYJJDefenseSim::BuildTower(int32 Spot, EYJJTower Type)
{
	if (!CanBuild(Spot, Type))
	{
		return false;
	}

	const FVector2f SpotPos = YJJMap::Spots()[Spot];
	const FYJJTowerDef& Def = YJJData::Tower(Type);

	FYJJTowerState& Tower = Towers[Spot];
	Tower = FYJJTowerState();
	Tower.bBuilt = true;
	Tower.Type = Type;
	Tower.Spent = Def.Levels[0].Cost;
	Tower.Cooldown = 0.4f;
	Tower.BuildAnim = 0.35f;
	Tower.Rally = YJJMap::PointAt(YJJMap::NearestDist(SpotPos));
	Gold -= Def.Levels[0].Cost;

	if (Type == EYJJTower::Shrine)
	{
		// 들고 나온 원소 중 첫 번째
		Tower.ShrineElement = EYJJElement::Fire;
		for (int32 Index = 0; Index <= static_cast<int32>(EYJJRune::Wind); ++Index)
		{
			if (HasRune(static_cast<EYJJRune>(Index)))
			{
				Tower.ShrineElement = YJJData::RuneElement(static_cast<EYJJRune>(Index));
				break;
			}
		}
	}
	else if (Type == EYJJTower::Barracks)
	{
		for (int32 Slot = 0; Slot < YJJ::SoldiersPerBarracks; ++Slot)
		{
			FYJJSoldierState& Soldier = Soldiers.AddDefaulted_GetRef();
			Soldier.Id = NextId++;
			Soldier.Spot = Spot;
			Soldier.Slot = Slot;
			Soldier.Home = Tower.Rally + SoldierSlotOffsets[Slot];
			Soldier.Pos = SpotPos;
			Soldier.MaxHP = Def.Levels[0].SoldierHP;
			Soldier.HP = Soldier.MaxHP;
		}
	}

	Emit(EYJJSimEvent::TowerBuilt, SpotPos);
	return true;
}

bool FYJJDefenseSim::UpgradeTower(int32 Spot)
{
	if (!CanUpgrade(Spot))
	{
		return false;
	}

	FYJJTowerState& Tower = Towers[Spot];
	const int32 Cost = UpgradeCost(Spot);
	Gold -= Cost;
	Tower.Spent += Cost;
	Tower.Level++;
	Tower.BuildAnim = 0.35f;

	if (Tower.Type == EYJJTower::Barracks)
	{
		const FYJJTowerLevel& Level = TowerLevel(Spot);
		for (FYJJSoldierState& Soldier : Soldiers)
		{
			if (Soldier.Spot == Spot)
			{
				Soldier.MaxHP = Level.SoldierHP;
				Soldier.HP = Soldier.MaxHP;
			}
		}
	}

	Emit(EYJJSimEvent::TowerUpgraded, YJJMap::Spots()[Spot]);
	return true;
}

bool FYJJDefenseSim::SellTower(int32 Spot)
{
	if (Result != EYJJRunResult::Playing || !Towers.IsValidIndex(Spot) || !Towers[Spot].bBuilt)
	{
		return false;
	}

	const int32 Refund = SellValue(Spot);
	Gold += Refund;

	if (Towers[Spot].Type == EYJJTower::Barracks)
	{
		for (const FYJJSoldierState& Soldier : Soldiers)
		{
			if (Soldier.Spot == Spot)
			{
				for (FYJJEnemyState& Enemy : Enemies)
				{
					if (Enemy.BlockerId == Soldier.Id)
					{
						Enemy.BlockerId = INDEX_NONE;
					}
				}
			}
		}
		Soldiers.RemoveAll([Spot](const FYJJSoldierState& Soldier) { return Soldier.Spot == Spot; });
	}

	Towers[Spot] = FYJJTowerState();
	Emit(EYJJSimEvent::TowerSold, YJJMap::Spots()[Spot]).Int = Refund;
	return true;
}

bool FYJJDefenseSim::CycleShrineElement(int32 Spot)
{
	if (!Towers.IsValidIndex(Spot) || !Towers[Spot].bBuilt || Towers[Spot].Type != EYJJTower::Shrine)
	{
		return false;
	}

	const int32 NumElements = static_cast<int32>(EYJJRune::Wind) + 1;
	const int32 Current = static_cast<int32>(YJJData::ElementRune(Towers[Spot].ShrineElement));
	for (int32 Step = 1; Step <= NumElements; ++Step)
	{
		const EYJJRune Rune = static_cast<EYJJRune>((Current + Step) % NumElements);
		if (HasRune(Rune))
		{
			Towers[Spot].ShrineElement = YJJData::RuneElement(Rune);
			return true;
		}
	}
	return false;
}

bool FYJJDefenseSim::UpgradeWizard()
{
	const int32 Cost = WizardUpgradeCost();
	if (Result != EYJJRunResult::Playing || Cost <= 0 || Gold < Cost)
	{
		return false;
	}
	Gold -= Cost;
	WizardLevel++;
	Emit(EYJJSimEvent::WizardUpgraded, YJJMap::WizardPos());
	return true;
}

bool FYJJDefenseSim::SetSocket(int32 Disk, int32 Socket, EYJJRune Rune)
{
	if (Disk < 0 || Disk >= YJJ::NumDisks || Socket < 0 || Socket >= YJJ::NumSockets)
	{
		return false;
	}
	if (Rune != EYJJRune::None && !HasRune(Rune))
	{
		return false;
	}

	FYJJDiskState& State = Disks[Disk];
	if (State.Sockets[Socket] == Rune)
	{
		return false;
	}

	State.Sockets[Socket] = Rune;
	State.Spell = YJJSpellForge::Combine(State.Sockets);

	// 전투 중에 바꾸면 잠깐 식혀야 한다 (바꿔 끼우며 연사하는 것 방지)
	if (WaveIndex >= 0)
	{
		State.Cooldown = FMath::Max(State.Cooldown, YJJData::SocketChangeCooldown);
		State.CooldownMax = FMath::Max(State.Cooldown, 0.01f);
	}
	return true;
}

bool FYJJDefenseSim::CastSpell(int32 Disk, const FVector2f& Target)
{
	if (!CanCast(Disk))
	{
		return false;
	}

	FYJJDiskState& State = Disks[Disk];
	const FYJJSpellSpec& Spell = State.Spell;
	const float Damage = Spell.Damage * YJJData::WizardDamageScale[WizardLevel];

	// 분열: 첫 발은 목표 지점, 나머지는 근처의 다른 적 (없으면 주변에 흩뿌린다)
	TArray<FVector2f, TInlineAllocator<5>> Targets;
	Targets.Add(Target);
	if (Spell.Projectiles > 1)
	{
		TArray<const FYJJEnemyState*, TInlineAllocator<32>> Nearby;
		for (const FYJJEnemyState& Enemy : Enemies)
		{
			const float Distance = YJJ::RangeDist(Enemy.Pos, Target);
			if (Enemy.IsAlive() && Distance > Spell.Radius * 0.8f && Distance < 320.f)
			{
				Nearby.Add(&Enemy);
			}
		}
		Nearby.Sort([&Target](const FYJJEnemyState& A, const FYJJEnemyState& B) { return YJJ::RangeDist(A.Pos, Target) < YJJ::RangeDist(B.Pos, Target); });

		for (int32 Index = 1; Index < Spell.Projectiles; ++Index)
		{
			if (Nearby.IsValidIndex(Index - 1))
			{
				Targets.Add(Nearby[Index - 1]->Pos);
			}
			else
			{
				const float Angle = UE_TWO_PI * Index / Spell.Projectiles + Rng.FRandRange(-0.3f, 0.3f);
				Targets.Add(Target + FVector2f(FMath::Cos(Angle) * 105.f, FMath::Sin(Angle) * 105.f / YJJ::YStretch));
			}
		}
	}

	const FVector2f Muzzle = WizardMuzzle();
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		FYJJProjectileState& Ball = Projectiles.AddDefaulted_GetRef();
		Ball.Kind = EYJJProjectile::SpellBall;
		Ball.Start = Muzzle;
		Ball.Pos = Muzzle;
		Ball.PrevPos = Muzzle;
		Ball.Target = Targets[Index];
		Ball.Duration = FMath::Max(FVector2f::Distance(Muzzle, Targets[Index]) / YJJData::SpellBallSpeed, 0.28f) + 0.05f * Index;
		Ball.ArcHeight = 90.f + 25.f * Index;
		Ball.Damage = Damage;
		Ball.bMagic = true;
		Ball.Element = Spell.Elements[0];
		Ball.Element2 = Spell.NumElements > 1 ? Spell.Elements[1] : (Spell.bAmplified ? Spell.Elements[0] : EYJJElement::None);
		Ball.Spell = Spell;
	}

	State.CooldownMax = Spell.Cooldown * YJJData::WizardCooldownScale[WizardLevel];
	State.Cooldown = State.CooldownMax;
	WizardCastAnim = 0.35f;
	SpellsCast++;
	if (Spell.ComboId != INDEX_NONE)
	{
		CombosCast.AddUnique(Spell.ComboId);
	}

	FYJJSimEvent& Event = Emit(EYJJSimEvent::SpellCast, Muzzle);
	Event.Pos2 = Target;
	Event.Int = Spell.ComboId;
	Event.Element = Spell.Elements[0];
	return true;
}

bool FYJJDefenseSim::CallNextWave()
{
	if (!CanCallWave())
	{
		return false;
	}

	// 일찍 부르면 남은 시간만큼 보너스
	if (WaveIndex >= 0 && NextWaveTimer > 1.f)
	{
		const int32 Bonus = FMath::RoundToInt(NextWaveTimer);
		Gold += Bonus;
		Emit(EYJJSimEvent::WaveBonus, YJJMap::PointAt(0.f)).Int = Bonus;
	}
	StartWave(WaveIndex + 1);
	return true;
}

// ---------------------------------------------------------------------------------------------
// 웨이브

void FYJJDefenseSim::StartWave(int32 Index)
{
	WaveIndex = Index;
	WaveTime = 0.f;
	NextWaveTimer = 0.f;
	HPScale = (1.f + YJJData::TierHPStep * (Config.Tier - 1)) * (1.f + 0.07f * Index);

	SpawnQueue.Reset();
	for (const FYJJWaveGroup& Group : YJJData::Wave(Index))
	{
		for (int32 Count = 0; Count < Group.Count; ++Count)
		{
			SpawnQueue.Add({ Group.Delay + Group.Interval * Count, Group.Type });
		}
	}
	SpawnQueue.StableSort([](const FSpawn& A, const FSpawn& B) { return A.Time > B.Time; });

	Emit(EYJJSimEvent::WaveStarted, YJJMap::PointAt(0.f)).Int = Index;
}

int32 FYJJDefenseSim::DebugSpawnEnemy(EYJJEnemy Type, float Dist)
{
	FYJJEnemyState& Enemy = SpawnEnemy(Type);
	Enemy.Dist = Dist;
	Enemy.Lateral = 0.f;
	Enemy.Pos = YJJMap::PointAt(Dist, 0.f, &Enemy.Dir);
	return Enemy.Id;
}

FYJJEnemyState& FYJJDefenseSim::SpawnEnemy(EYJJEnemy Type)
{
	const FYJJEnemyDef& Def = YJJData::Enemy(Type);
	FYJJEnemyState& Enemy = Enemies.AddDefaulted_GetRef();
	Enemy.Id = NextId++;
	Enemy.Type = Type;
	Enemy.MaxHP = Def.HP * HPScale;
	Enemy.HP = Enemy.MaxHP;
	Enemy.Lateral = Def.bBoss ? 0.f : Rng.FRandRange(-20.f, 20.f);
	Enemy.AnimTime = Rng.FRandRange(0.f, 1.f);
	Enemy.AttackTimer = Def.MeleeInterval * 0.5f;
	Enemy.Pos = YJJMap::PointAt(0.f, Enemy.Lateral, &Enemy.Dir);
	return Enemy;
}

void FYJJDefenseSim::StepWaves()
{
	if (WaveIndex < 0)
	{
		return;
	}

	if (SpawnQueue.Num() > 0)
	{
		WaveTime += Dt;
		while (SpawnQueue.Num() > 0 && SpawnQueue.Last().Time <= WaveTime)
		{
			SpawnEnemy(SpawnQueue.Pop(EAllowShrinking::No).Type);
		}
		if (SpawnQueue.IsEmpty())
		{
			NextWaveTimer = YJJData::WaveGap;
		}
	}
	else if (WaveIndex + 1 < YJJData::NumWaves())
	{
		NextWaveTimer -= Dt;
		if (Enemies.IsEmpty())
		{
			// 다 잡았으면 오래 기다리지 않는다
			NextWaveTimer = FMath::Min(NextWaveTimer, 5.f);
		}
		if (NextWaveTimer <= 0.f)
		{
			StartWave(WaveIndex + 1);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 적

void FYJJDefenseSim::DamageEnemy(FYJJEnemyState& Enemy, float Amount, bool bMagic)
{
	if (!Enemy.IsAlive() || Amount <= 0.f)
	{
		return;
	}

	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	float Final = Amount * (1.f - (bMagic ? Def.MagicResist : Def.Armor));
	if (Enemy.VulnTime > 0.f)
	{
		Final *= Enemy.VulnMult;
	}

	Enemy.HP -= Final;
	if (Amount > 2.f)
	{
		Enemy.HitFlash = 0.12f;
	}

	if (Enemy.HP <= 0.f)
	{
		Enemy.HP = 0.f;
		Gold += Def.Gold;
		Kills++;
		FYJJSimEvent& Event = Emit(EYJJSimEvent::EnemyKilled, Enemy.Pos);
		Event.Int = Def.Gold;
		Event.Value = static_cast<float>(Enemy.Type);
	}
}

void FYJJDefenseSim::AddBurn(FYJJEnemyState& Enemy, float Dps, float Duration)
{
	Enemy.BurnDps = Enemy.BurnTime > 0.f ? FMath::Max(Enemy.BurnDps, Dps) : Dps;
	Enemy.BurnTime = FMath::Max(Enemy.BurnTime, Duration);
}

void FYJJDefenseSim::AddPoison(FYJJEnemyState& Enemy, float Dps, float Duration)
{
	Enemy.PoisonDps = Enemy.PoisonTime > 0.f ? FMath::Max(Enemy.PoisonDps, Dps) : Dps;
	Enemy.PoisonTime = FMath::Max(Enemy.PoisonTime, Duration);
}

void FYJJDefenseSim::AddSlow(FYJJEnemyState& Enemy, float Factor, float Duration)
{
	if (YJJData::Enemy(Enemy.Type).bBoss)
	{
		Factor = FMath::Lerp(1.f, Factor, 0.5f);
	}
	Enemy.SlowFactor = Enemy.SlowTime > 0.f ? FMath::Min(Enemy.SlowFactor, Factor) : Factor;
	Enemy.SlowTime = FMath::Max(Enemy.SlowTime, Duration);
}

void FYJJDefenseSim::AddStun(FYJJEnemyState& Enemy, float Duration)
{
	Enemy.StunTime = FMath::Max(Enemy.StunTime, Duration * (YJJData::Enemy(Enemy.Type).bBoss ? BossControlScale : 1.f));
}

void FYJJDefenseSim::AddFreeze(FYJJEnemyState& Enemy, float Duration)
{
	Enemy.FreezeTime = FMath::Max(Enemy.FreezeTime, Duration * (YJJData::Enemy(Enemy.Type).bBoss ? BossControlScale : 1.f));
}

void FYJJDefenseSim::Knockback(FYJJEnemyState& Enemy, float Amount)
{
	if (YJJData::Enemy(Enemy.Type).bBoss)
	{
		Amount *= BossControlScale;
	}
	Enemy.Dist = FMath::Max(Enemy.Dist - Amount, 0.f);
	Enemy.BlockerId = INDEX_NONE;
	Enemy.Pos = YJJMap::PointAt(Enemy.Dist, Enemy.Lateral, &Enemy.Dir);
}

void FYJJDefenseSim::ApplyIntrinsic(FYJJEnemyState& Enemy, EYJJElement Element, float Power)
{
	if (!Enemy.IsAlive())
	{
		return;
	}
	switch (Element)
	{
	case EYJJElement::Fire: AddBurn(Enemy, Power * 0.25f, 3.f); break;
	case EYJJElement::Ice: AddSlow(Enemy, 0.6f, 2.5f); break;
	case EYJJElement::Lightning: AddStun(Enemy, 0.3f); break;
	case EYJJElement::Poison: AddPoison(Enemy, Power * 0.16f, 6.f); break;
	case EYJJElement::Wind: Knockback(Enemy, 48.f); break;
	default: break;
	}
}

EYJJReaction FYJJDefenseSim::ApplyElement(FYJJEnemyState& Enemy, EYJJElement Element, float Power)
{
	if (!Enemy.IsAlive() || Element == EYJJElement::None)
	{
		return EYJJReaction::None;
	}

	ApplyIntrinsic(Enemy, Element, Power);

	if (Enemy.Aura != EYJJElement::None && Enemy.Aura != Element)
	{
		const EYJJReaction Reaction = YJJSpellForge::GetReaction(Enemy.Aura, Element);
		Enemy.Aura = EYJJElement::None;
		Enemy.AuraTime = 0.f;
		return Reaction;
	}

	Enemy.Aura = Element;
	Enemy.AuraTime = YJJData::AuraDuration;
	return EYJJReaction::None;
}

void FYJJDefenseSim::StepEnemies()
{
	const float PathLength = YJJMap::PathLength();

	for (int32 Index = 0; Index < Enemies.Num(); ++Index)
	{
		FYJJEnemyState& Enemy = Enemies[Index];
		if (!Enemy.IsAlive())
		{
			continue;
		}
		const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);

		Enemy.HitFlash = FMath::Max(Enemy.HitFlash - Dt, 0.f);
		Enemy.AuraTime -= Dt;
		if (Enemy.AuraTime <= 0.f)
		{
			Enemy.Aura = EYJJElement::None;
		}
		Enemy.SlowTime = FMath::Max(Enemy.SlowTime - Dt, 0.f);
		Enemy.StunTime = FMath::Max(Enemy.StunTime - Dt, 0.f);
		Enemy.FreezeTime = FMath::Max(Enemy.FreezeTime - Dt, 0.f);
		Enemy.VulnTime = FMath::Max(Enemy.VulnTime - Dt, 0.f);

		// 지속 피해
		if (Enemy.BurnTime > 0.f)
		{
			Enemy.BurnTime -= Dt;
			DamageEnemy(Enemy, Enemy.BurnDps * Dt, true);
		}
		if (Enemy.PoisonTime > 0.f)
		{
			Enemy.PoisonTime -= Dt;
			DamageEnemy(Enemy, Enemy.PoisonDps * Dt, true);
		}
		if (!Enemy.IsAlive())
		{
			continue;
		}

		// 응원 버섯: 주변 회복
		if (Def.HealPerSecond > 0.f && !Enemy.IsDisabled())
		{
			Enemy.HealTimer += Dt;
			if (Enemy.HealTimer >= 1.f)
			{
				Enemy.HealTimer = 0.f;
				bool bHealed = false;
				for (FYJJEnemyState& Other : Enemies)
				{
					if (Other.IsAlive() && Other.HP < Other.MaxHP && YJJ::RangeDist(Other.Pos, Enemy.Pos) <= 135.f)
					{
						Other.HP = FMath::Min(Other.HP + Def.HealPerSecond, Other.MaxHP);
						bHealed = true;
					}
				}
				if (bHealed)
				{
					Emit(EYJJSimEvent::Heal, Enemy.Pos).Value = 135.f;
				}
			}
		}

		if (Enemy.IsDisabled())
		{
			continue;
		}

		// 병졸에게 막혀 있으면 멈춰서 싸운다
		if (Enemy.BlockerId != INDEX_NONE)
		{
			FYJJSoldierState* Blocker = FindSoldier(Enemy.BlockerId);
			if (!Blocker || !Blocker->IsAlive() || Blocker->TargetId != Enemy.Id)
			{
				Enemy.BlockerId = INDEX_NONE;
			}
			else
			{
				Enemy.AnimTime += Dt;
				Enemy.AttackTimer -= Dt;
				if (Enemy.AttackTimer <= 0.f)
				{
					Enemy.AttackTimer = Def.MeleeInterval;
					const float Armor = YJJData::Tower(EYJJTower::Barracks).Levels[Towers[Blocker->Spot].Level].SoldierArmor;
					Blocker->HP -= Def.MeleeDamage * (1.f - Armor);
					Blocker->HitFlash = 0.12f;
					Blocker->IdleTime = 0.f;
					Emit(EYJJSimEvent::MeleeHit, Blocker->Pos);
					if (Blocker->HP <= 0.f)
					{
						Blocker->HP = 0.f;
						Blocker->RespawnTimer = YJJData::SoldierRespawn;
						Blocker->TargetId = INDEX_NONE;
						Enemy.BlockerId = INDEX_NONE;
						Emit(EYJJSimEvent::SoldierDied, Blocker->Pos);
					}
				}
				continue;
			}
		}

		const float Speed = Def.Speed * (Enemy.SlowTime > 0.f ? Enemy.SlowFactor : 1.f);
		Enemy.Dist += Speed * Dt;
		Enemy.AnimTime += Dt * (Speed / FMath::Max(Def.Speed, 1.f));
		Enemy.Pos = YJJMap::PointAt(Enemy.Dist, Enemy.Lateral, &Enemy.Dir);

		if (Enemy.Dist >= PathLength)
		{
			Lives -= Def.LifeCost;
			Enemy.HP = 0.f;
			Emit(EYJJSimEvent::EnemyLeaked, Enemy.Pos).Int = Def.LifeCost;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 병졸

void FYJJDefenseSim::StepSoldiers()
{
	for (FYJJSoldierState& Soldier : Soldiers)
	{
		const FVector2f SpotPos = YJJMap::Spots()[Soldier.Spot];
		const FYJJTowerLevel& Level = TowerLevel(Soldier.Spot);

		Soldier.HitFlash = FMath::Max(Soldier.HitFlash - Dt, 0.f);
		Soldier.AttackAnim = FMath::Max(Soldier.AttackAnim - Dt, 0.f);
		Soldier.AnimTime += Dt;
		Soldier.bMoving = false;

		if (!Soldier.IsAlive())
		{
			Soldier.RespawnTimer -= Dt;
			if (Soldier.RespawnTimer <= 0.f)
			{
				Soldier.RespawnTimer = 0.f;
				Soldier.HP = Soldier.MaxHP;
				Soldier.Pos = SpotPos;
				Soldier.TargetId = INDEX_NONE;
			}
			continue;
		}

		// 목표가 죽었거나 멀어졌으면 놓는다
		FYJJEnemyState* Target = FindEnemyMutable(Soldier.TargetId);
		if (Target && (YJJData::Enemy(Target->Type).bFlying || FVector2f::Distance(Target->Pos, Soldier.Home) > YJJData::SoldierEngageRange + 30.f))
		{
			if (Target->BlockerId == Soldier.Id)
			{
				Target->BlockerId = INDEX_NONE;
			}
			Target = nullptr;
		}
		if (!Target)
		{
			Soldier.TargetId = INDEX_NONE;

			// 아직 아무도 안 막은 적 먼저, 그중에서도 가장 앞선 적
			float BestScore = -1.f;
			for (FYJJEnemyState& Enemy : Enemies)
			{
				if (!Enemy.IsAlive() || YJJData::Enemy(Enemy.Type).bFlying || FVector2f::Distance(Enemy.Pos, Soldier.Home) > YJJData::SoldierEngageRange)
				{
					continue;
				}
				const float Score = Enemy.Dist + (Enemy.BlockerId == INDEX_NONE ? 100000.f : 0.f);
				if (Score > BestScore)
				{
					BestScore = Score;
					Target = &Enemy;
				}
			}
			if (Target)
			{
				Soldier.TargetId = Target->Id;
			}
		}

		if (Target)
		{
			Soldier.IdleTime = 0.f;

			// 적의 앞을 막아선다
			const FVector2f Desired = Target->Pos + Target->Dir * (YJJData::Enemy(Target->Type).Radius + 14.f) + SoldierSlotOffsets[Soldier.Slot] * 0.35f;
			const FVector2f Delta = Desired - Soldier.Pos;
			const float Distance = Delta.Size();
			Soldier.bFacingLeft = Target->Pos.X < Soldier.Pos.X;

			if (Distance < 46.f && Target->BlockerId == INDEX_NONE)
			{
				Target->BlockerId = Soldier.Id;
			}

			if (Distance > 6.f)
			{
				Soldier.Pos += Delta / Distance * FMath::Min(YJJData::SoldierSpeed * Dt, Distance);
				Soldier.bMoving = true;
			}
			if (Distance < 30.f)
			{
				Soldier.AttackTimer -= Dt;
				if (Soldier.AttackTimer <= 0.f)
				{
					Soldier.AttackTimer = Level.Interval;
					Soldier.AttackAnim = 0.22f;
					DamageEnemy(*Target, Level.Damage, false);
					Emit(EYJJSimEvent::MeleeHit, Target->Pos);
				}
			}
		}
		else
		{
			const FVector2f Delta = Soldier.Home - Soldier.Pos;
			const float Distance = Delta.Size();
			if (Distance > 3.f)
			{
				Soldier.Pos += Delta / Distance * FMath::Min(YJJData::SoldierSpeed * Dt, Distance);
				Soldier.bFacingLeft = Delta.X < 0.f;
				Soldier.bMoving = true;
			}

			// 쉬는 동안 회복
			Soldier.IdleTime += Dt;
			if (Soldier.IdleTime > 2.f)
			{
				Soldier.HP = FMath::Min(Soldier.HP + Soldier.MaxHP * 0.12f * Dt, Soldier.MaxHP);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 타워

void FYJJDefenseSim::StepTowers()
{
	const TConstArrayView<FVector2f> Spots = YJJMap::Spots();

	for (int32 Spot = 0; Spot < Towers.Num(); ++Spot)
	{
		FYJJTowerState& Tower = Towers[Spot];
		if (!Tower.bBuilt)
		{
			continue;
		}

		Tower.Cooldown -= Dt;
		Tower.FireAnim = FMath::Max(Tower.FireAnim - Dt, 0.f);
		Tower.BuildAnim = FMath::Max(Tower.BuildAnim - Dt, 0.f);
		if (Tower.Type == EYJJTower::Barracks || Tower.Cooldown > 0.f)
		{
			continue;
		}

		const FYJJTowerDef& Def = YJJData::Tower(Tower.Type);
		const FYJJTowerLevel& Level = Def.Levels[Tower.Level];
		const FVector2f SpotPos = Spots[Spot];

		// 사거리 안에서 가장 앞선 적
		const FYJJEnemyState* Target = nullptr;
		for (const FYJJEnemyState& Enemy : Enemies)
		{
			if (!Enemy.IsAlive() || (!Def.bHitsAir && YJJData::Enemy(Enemy.Type).bFlying) || YJJ::RangeDist(Enemy.Pos, SpotPos) > Level.Range)
			{
				continue;
			}
			if (!Target || Enemy.Dist > Target->Dist)
			{
				Target = &Enemy;
			}
		}
		if (!Target)
		{
			continue;
		}

		Tower.Cooldown = Level.Interval;
		Tower.FireAnim = 0.25f;
		Tower.bAimLeft = Target->Pos.X < SpotPos.X;

		const FVector2f Muzzle = TowerMuzzle(SpotPos);
		const float Distance = FVector2f::Distance(Muzzle, Target->Pos);

		FYJJProjectileState& Shot = Projectiles.AddDefaulted_GetRef();
		Shot.Start = Muzzle;
		Shot.Pos = Muzzle;
		Shot.PrevPos = Muzzle;
		Shot.Target = Target->Pos;
		Shot.TargetId = Target->Id;
		Shot.Damage = Level.Damage;
		Shot.Splash = Level.Splash;
		Shot.bMagic = Def.bMagic;

		switch (Tower.Type)
		{
		case EYJJTower::Archer:
			Shot.Kind = EYJJProjectile::Arrow;
			Shot.Duration = FMath::Max(Distance / 720.f, 0.14f);
			Shot.ArcHeight = Distance * 0.12f;
			break;
		case EYJJTower::Mage:
			Shot.Kind = EYJJProjectile::MagicBolt;
			Shot.Duration = FMath::Max(Distance / 520.f, 0.18f);
			break;
		case EYJJTower::Shrine:
			Shot.Kind = EYJJProjectile::ShrineOrb;
			Shot.Duration = FMath::Max(Distance / 480.f, 0.18f);
			Shot.Element = Tower.ShrineElement;
			break;
		default:
		{
			// 포탄은 유도하지 않고, 적이 걸어올 자리를 미리 겨냥한다
			Shot.Kind = EYJJProjectile::Bomb;
			Shot.Duration = 0.85f;
			Shot.ArcHeight = 150.f;
			Shot.TargetId = INDEX_NONE;
			const bool bStanding = Target->BlockerId != INDEX_NONE || Target->IsDisabled();
			const float Speed = YJJData::Enemy(Target->Type).Speed * (Target->SlowTime > 0.f ? Target->SlowFactor : 1.f);
			Shot.Target = bStanding ? Target->Pos : YJJMap::PointAt(Target->Dist + Speed * Shot.Duration, Target->Lateral);
			break;
		}
		}

		Emit(EYJJSimEvent::TowerFired, Muzzle).Int = static_cast<int32>(Shot.Kind);
	}
}

// ---------------------------------------------------------------------------------------------
// 투사체

void FYJJDefenseSim::StepProjectiles()
{
	// 착탄 처리에서 새 투사체가 생기지는 않지만, 안전하게 인덱스로 돈다
	for (int32 Index = Projectiles.Num() - 1; Index >= 0; --Index)
	{
		FYJJProjectileState& Projectile = Projectiles[Index];
		Projectile.Time += Dt;

		if (const FYJJEnemyState* Target = FindEnemy(Projectile.TargetId))
		{
			Projectile.Target = Target->Pos;
		}

		const float Alpha = FMath::Clamp(Projectile.Time / Projectile.Duration, 0.f, 1.f);
		Projectile.PrevPos = Projectile.Pos;
		Projectile.Pos = FMath::Lerp(Projectile.Start, Projectile.Target, Alpha);
		Projectile.Pos.Y -= Projectile.ArcHeight * 4.f * Alpha * (1.f - Alpha);

		if (Alpha >= 1.f)
		{
			const FYJJProjectileState Landed = Projectile;
			Projectiles.RemoveAtSwap(Index, EAllowShrinking::No);
			ResolveProjectile(Landed);
		}
	}
}

void FYJJDefenseSim::ResolveProjectile(const FYJJProjectileState& Projectile)
{
	switch (Projectile.Kind)
	{
	case EYJJProjectile::Arrow:
	case EYJJProjectile::MagicBolt:
		if (FYJJEnemyState* Target = FindEnemyMutable(Projectile.TargetId))
		{
			DamageEnemy(*Target, Projectile.Damage, Projectile.bMagic);
			Emit(EYJJSimEvent::ProjectileHit, Target->Pos).Int = static_cast<int32>(Projectile.Kind);
		}
		break;

	case EYJJProjectile::Bomb:
	{
		ForEnemiesInRadius(Projectile.Target, Projectile.Splash, [this, &Projectile](FYJJEnemyState& Enemy)
		{
			if (!YJJData::Enemy(Enemy.Type).bFlying)
			{
				DamageEnemy(Enemy, Projectile.Damage, false);
			}
		});
		FYJJSimEvent& Event = Emit(EYJJSimEvent::ProjectileHit, Projectile.Target);
		Event.Int = static_cast<int32>(Projectile.Kind);
		Event.Value = Projectile.Splash;
		break;
	}

	case EYJJProjectile::ShrineOrb:
	{
		// 약한 피해 + 원소를 묻힌다. 이미 다른 원소가 묻어 있으면 반응
		EYJJReaction Pending = EYJJReaction::None;
		FVector2f PendingPos = Projectile.Target;
		ForEnemiesInRadius(Projectile.Target, Projectile.Splash, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Projectile.Damage, true);
			const EYJJReaction Reaction = ApplyElement(Enemy, Projectile.Element, Projectile.Damage * 3.f);
			if (Reaction != EYJJReaction::None && Pending == EYJJReaction::None)
			{
				Pending = Reaction;
				PendingPos = Enemy.Pos;
			}
		});
		FYJJSimEvent& Event = Emit(EYJJSimEvent::ProjectileHit, Projectile.Target);
		Event.Int = static_cast<int32>(Projectile.Kind);
		Event.Value = Projectile.Splash;
		Event.Element = Projectile.Element;
		if (Pending != EYJJReaction::None)
		{
			TriggerReaction(Pending, PendingPos, Projectile.Damage * 3.f * YJJData::WizardDamageScale[WizardLevel]);
		}
		break;
	}

	case EYJJProjectile::SpellBall:
		ImpactSpell(Projectile.Spell, Projectile.Target, Projectile.Damage);
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// 스펠

void FYJJDefenseSim::HitWithSpell(const FYJJSpellSpec& Spell, const FVector2f& Pos, float Radius, float Damage, int32 OnlyEnemyId, TArray<int32>& HitIds)
{
	EYJJReaction Pending = EYJJReaction::None;
	FVector2f PendingPos = Pos;

	auto HitOne = [&](FYJJEnemyState& Enemy)
	{
		HitIds.AddUnique(Enemy.Id);
		DamageEnemy(Enemy, Damage, true);

		if (Spell.NumElements == 2)
		{
			// 두 원소가 한꺼번에 닿는다 → 오라를 남기지 않고 아래에서 반응이 바로 터진다
			ApplyIntrinsic(Enemy, Spell.Elements[0], Damage);
			ApplyIntrinsic(Enemy, Spell.Elements[1], Damage);
			Enemy.Aura = EYJJElement::None;
			Enemy.AuraTime = 0.f;
		}
		else if (Spell.NumElements == 1)
		{
			const EYJJReaction Reaction = ApplyElement(Enemy, Spell.Elements[0], Damage);
			if (Reaction != EYJJReaction::None && Pending == EYJJReaction::None)
			{
				Pending = Reaction;
				PendingPos = Enemy.Pos;
			}
		}
	};

	if (OnlyEnemyId != INDEX_NONE)
	{
		if (FYJJEnemyState* Enemy = FindEnemyMutable(OnlyEnemyId))
		{
			HitOne(*Enemy);
		}
	}
	else
	{
		ForEnemiesInRadius(Pos, Radius, HitOne);
	}

	FYJJSimEvent& Event = Emit(EYJJSimEvent::SpellImpact, Pos);
	Event.Value = Radius;
	Event.Element = Spell.Elements[0];
	Event.Int = static_cast<int32>(Spell.NumElements > 1 ? Spell.Elements[1] : EYJJElement::None);

	if (Spell.NumElements == 2)
	{
		TriggerReaction(Spell.Reaction, Pos, Damage);
	}
	else if (Pending != EYJJReaction::None)
	{
		TriggerReaction(Pending, PendingPos, Damage);
	}
}

void FYJJDefenseSim::ImpactSpell(const FYJJSpellSpec& Spell, const FVector2f& Pos, float Damage)
{
	TArray<int32> HitIds;
	HitWithSpell(Spell, Pos, Spell.Radius, Damage, INDEX_NONE, HitIds);

	// 연쇄: 아직 안 맞은 가장 가까운 적으로 옮겨 붙는다
	FVector2f From = Pos;
	float HopDamage = Damage * 0.7f;
	for (int32 Jump = 0; Jump < Spell.ChainJumps; ++Jump)
	{
		const FYJJEnemyState* Next = nullptr;
		float BestDistance = 240.f;
		for (const FYJJEnemyState& Enemy : Enemies)
		{
			const float Distance = YJJ::RangeDist(Enemy.Pos, From);
			if (Enemy.IsAlive() && Distance < BestDistance && !HitIds.Contains(Enemy.Id))
			{
				BestDistance = Distance;
				Next = &Enemy;
			}
		}
		if (!Next)
		{
			break;
		}

		const FVector2f NextPos = Next->Pos;
		FYJJSimEvent& Arc = Emit(EYJJSimEvent::ChainArc, From);
		Arc.Pos2 = NextPos;
		Arc.Element = Spell.Elements[0];

		if (Spell.ChainRadius > 0.f)
		{
			HitWithSpell(Spell, NextPos, Spell.ChainRadius, HopDamage, INDEX_NONE, HitIds);
		}
		else
		{
			HitWithSpell(Spell, NextPos, 0.f, HopDamage, Next->Id, HitIds);
		}
		From = NextPos;
		HopDamage *= 0.88f;
	}
}

void FYJJDefenseSim::TriggerReaction(EYJJReaction Reaction, const FVector2f& Pos, float Power)
{
	if (Reaction == EYJJReaction::None)
	{
		return;
	}
	Reactions++;

	float Radius = 90.f;
	auto AddZone = [this, Reaction, &Pos](float ZoneRadius, float Duration) -> FYJJZoneState&
	{
		FYJJZoneState& Zone = Zones.AddDefaulted_GetRef();
		Zone.Kind = Reaction;
		Zone.Pos = Pos;
		Zone.Radius = ZoneRadius;
		Zone.Duration = Duration;
		return Zone;
	};

	switch (Reaction)
	{
	case EYJJReaction::Evaporate:
		Radius = 75.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy) { DamageEnemy(Enemy, Power * 0.9f, true); });
		break;

	case EYJJReaction::Steam:
		Radius = 125.f;
		AddZone(Radius, 3.5f).Dps = Power * 0.28f;
		break;

	case EYJJReaction::Overload:
		Radius = 140.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Power * 0.6f, true);
			Knockback(Enemy, 60.f);
		});
		break;

	case EYJJReaction::Combust:
		Radius = 95.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy) { AddBurn(Enemy, Power * 0.55f, 4.f); });
		break;

	case EYJJReaction::Firestorm:
	{
		Radius = 150.f;
		FYJJZoneState& Zone = AddZone(Radius, 3.f);
		Zone.Dps = Power * 0.3f;
		Zone.BurnDps = Power * 0.2f;
		break;
	}

	case EYJJReaction::Freeze:
		Radius = 95.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy) { AddFreeze(Enemy, 2.4f); });
		break;

	case EYJJReaction::Superconduct:
		Radius = 130.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Power * 0.2f, true);
			Enemy.VulnMult = 1.4f;
			Enemy.VulnTime = 6.f;
		});
		break;

	case EYJJReaction::Frostbite:
		Radius = 105.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			AddSlow(Enemy, 0.4f, 4.5f);
			AddPoison(Enemy, Power * 0.2f, 5.f);
		});
		break;

	case EYJJReaction::Blizzard:
	{
		Radius = 170.f;
		FYJJZoneState& Zone = AddZone(Radius, 4.f);
		Zone.Dps = Power * 0.1f;
		Zone.SlowFactor = 0.5f;
		break;
	}

	case EYJJReaction::Electrocute:
		Radius = 160.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Power * 0.75f, true);
			AddStun(Enemy, 0.5f);
		});
		break;

	case EYJJReaction::Swamp:
	{
		Radius = 130.f;
		FYJJZoneState& Zone = AddZone(Radius, 7.f);
		Zone.Dps = Power * 0.18f;
		Zone.SlowFactor = 0.75f;
		Zone.bGroundOnly = true;
		break;
	}

	case EYJJReaction::Tidal:
		Radius = 140.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Power * 0.3f, true);
			Knockback(Enemy, 140.f);
		});
		break;

	case EYJJReaction::Neurotoxin:
		Radius = 95.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy)
		{
			AddStun(Enemy, 1.4f);
			AddPoison(Enemy, Power * 0.2f, 5.f);
		});
		break;

	case EYJJReaction::Thunderstorm:
	{
		// 가까운 순서로 넷
		Radius = 260.f;
		TArray<FYJJEnemyState*, TInlineAllocator<32>> Nearby;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy) { Nearby.Add(&Enemy); });
		Nearby.Sort([&Pos](const FYJJEnemyState& A, const FYJJEnemyState& B) { return YJJ::RangeDist(A.Pos, Pos) < YJJ::RangeDist(B.Pos, Pos); });
		for (int32 Index = 0; Index < FMath::Min(Nearby.Num(), 4); ++Index)
		{
			Emit(EYJJSimEvent::SkyBolt, Pos).Pos2 = Nearby[Index]->Pos;
			DamageEnemy(*Nearby[Index], Power * 0.55f, true);
			AddStun(*Nearby[Index], 0.2f);
		}
		Radius = 110.f;
		break;
	}

	case EYJJReaction::Plague:
		Radius = 170.f;
		ForEnemiesInRadius(Pos, Radius, [&](FYJJEnemyState& Enemy) { AddPoison(Enemy, Power * 0.3f, 6.f); });
		break;

	default:
		break;
	}

	FYJJSimEvent& Event = Emit(EYJJSimEvent::Reaction, Pos);
	Event.Reaction = Reaction;
	Event.Value = Radius;
}

// ---------------------------------------------------------------------------------------------
// 장판

void FYJJDefenseSim::StepZones()
{
	constexpr float TickInterval = 0.25f;

	for (int32 Index = Zones.Num() - 1; Index >= 0; --Index)
	{
		FYJJZoneState& Zone = Zones[Index];
		Zone.Time += Dt;
		Zone.TickTimer += Dt;

		if (Zone.TickTimer >= TickInterval)
		{
			Zone.TickTimer -= TickInterval;
			const FYJJZoneState Tick = Zone;
			ForEnemiesInRadius(Tick.Pos, Tick.Radius, [this, &Tick](FYJJEnemyState& Enemy)
			{
				if (Tick.bGroundOnly && YJJData::Enemy(Enemy.Type).bFlying)
				{
					return;
				}
				DamageEnemy(Enemy, Tick.Dps * TickInterval, true);
				if (Tick.SlowFactor < 1.f)
				{
					AddSlow(Enemy, Tick.SlowFactor, 0.5f);
				}
				if (Tick.BurnDps > 0.f)
				{
					AddBurn(Enemy, Tick.BurnDps, 1.5f);
				}
				if (Tick.PoisonDps > 0.f)
				{
					AddPoison(Enemy, Tick.PoisonDps, 2.f);
				}
			});
		}

		if (Zones[Index].Time >= Zones[Index].Duration)
		{
			Zones.RemoveAtSwap(Index, EAllowShrinking::No);
		}
	}
}
