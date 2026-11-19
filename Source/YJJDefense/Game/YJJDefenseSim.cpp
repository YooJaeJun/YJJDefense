#include "Game/YJJDefenseSim.h"
#include "Game/YJJGameData.h"

FYJJRunConfig::FYJJRunConfig()
{
	RuneMask = YJJData::StartRuneMask();
	TowerMask = YJJData::StartTowerMask();
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
		{
			Disks[Disk][Socket] = YJJData::StartDisk(Disk, Socket);
		}
	}
}

FVector2f FYJJDefenseSim::TowerMuzzle(const FVector2f& SpotPos)
{
	return SpotPos - FVector2f(0.f, YJJData::Tune().TowerMuzzleHeight);
}

FVector2f FYJJDefenseSim::WizardMuzzle()
{
	return YJJMap::WizardPos() + YJJData::Tune().WizardMuzzle();
}

void FYJJDefenseSim::Reset(const FYJJRunConfig& InConfig)
{
	const FYJJTuning& Tune = YJJData::Tune();
	Config = InConfig;
	Config.Tier = FMath::Max(Config.Tier, 1);
	Config.Map = FMath::Clamp(Config.Map, 0, YJJMap::NumMaps() - 1);
	YJJMap::SetActive(Config.Map);

	Result = EYJJRunResult::Playing;
	Time = 0.f;
	Gold = Tune.StartGold;
	Lives = Tune.StartLives;
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
	PendingSplits.Reset();
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
		Disks[Disk].Spell = FYJJSpellForge::Combine(Disks[Disk].Sockets);
	}
}

void FYJJDefenseSim::Step()
{
	if (Result != EYJJRunResult::Playing)
	{
		return;
	}

	Time += FixedStep;
	WizardCastAnim = FMath::Max(WizardCastAnim - FixedStep, 0.f);
	for (FYJJDiskState& Disk : Disks)
	{
		Disk.Cooldown = FMath::Max(Disk.Cooldown - FixedStep, 0.f);
	}

	StepWaves();
	StepEnemies();
	StepSoldiers();
	StepTowers();
	StepProjectiles();
	StepZones();

	SpawnSplits();
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
	return Towers.IsValidIndex(Spot) && Towers[Spot].bBuilt ? FMath::FloorToInt(Towers[Spot].Spent * YJJData::Tune().SellRefund) : 0;
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
	return YJJData::Tune().WizardCost(WizardLevel);
}

int32 FYJJDefenseSim::Stars() const
{
	if (Result != EYJJRunResult::Victory)
	{
		return 0;
	}
	const FYJJTuning& Tune = YJJData::Tune();
	return Lives >= Tune.Star3Lives ? ThreeStars : (Lives >= Tune.Star2Lives ? TwoStars : OneStar);
}

// ---------------------------------------------------------------------------------------------
// 명령

bool FYJJDefenseSim::BuildTower(int32 Spot, EYJJTower Type)
{
	if (!CanBuild(Spot, Type))
	{
		return false;
	}

	const FYJJTuning& Tune = YJJData::Tune();
	const FVector2f SpotPos = YJJMap::Spots()[Spot];
	const FYJJTowerDef& Def = YJJData::Tower(Type);

	FYJJTowerState& Tower = Towers[Spot];
	Tower = FYJJTowerState();
	Tower.bBuilt = true;
	Tower.Type = Type;
	Tower.Spent = Def.Levels[0].Cost;
	Tower.Cooldown = Tune.TowerFirstShotDelay;
	Tower.BuildAnim = Tune.TowerBuildAnim;
	Tower.Rally = YJJMap::PointAt(YJJMap::NearestDist(SpotPos));
	Gold -= Def.Levels[0].Cost;

	if (Type == EYJJTower::Shrine)
	{
		// 들고 나온 원소 중 첫 번째
		Tower.ShrineElement = EYJJElement::None;
		for (int32 Index = 0; Index <= static_cast<int32>(EYJJRune::Wind) && Tower.ShrineElement == EYJJElement::None; ++Index)
		{
			if (HasRune(static_cast<EYJJRune>(Index)))
			{
				Tower.ShrineElement = YJJData::RuneElement(static_cast<EYJJRune>(Index));
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
			Soldier.Home = Tower.Rally + Tune.SoldierSlot(Slot);
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
	Tower.BuildAnim = YJJData::Tune().TowerBuildAnim;

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
			if (Soldier.Spot != Spot)
			{
				continue;
			}
			for (FYJJEnemyState& Enemy : Enemies)
			{
				if (Enemy.BlockerId == Soldier.Id)
				{
					Enemy.BlockerId = INDEX_NONE;
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
	State.Spell = FYJJSpellForge::Combine(State.Sockets);

	// 전투 중에 바꾸면 잠깐 식혀야 한다 (바꿔 끼우며 연사하는 것 방지)
	if (WaveIndex >= 0)
	{
		State.Cooldown = FMath::Max(State.Cooldown, YJJData::Tune().SocketChangeCooldown);
		State.CooldownMax = FMath::Max(State.Cooldown, KINDA_SMALL_NUMBER);
	}
	return true;
}

bool FYJJDefenseSim::CastSpell(int32 Disk, const FVector2f& Target)
{
	if (!CanCast(Disk))
	{
		return false;
	}

	const FYJJTuning& Tune = YJJData::Tune();
	FYJJDiskState& State = Disks[Disk];
	const FYJJSpellSpec& Spell = State.Spell;
	const float Damage = Spell.Damage * Tune.WizardDamage(WizardLevel);

	// 분열: 첫 발은 목표 지점, 나머지는 근처의 다른 적 (없으면 주변에 흩뿌린다)
	TArray<FVector2f> Targets;
	Targets.Add(Target);
	if (Spell.Projectiles > 1)
	{
		TArray<const FYJJEnemyState*> Nearby;
		for (const FYJJEnemyState& Enemy : Enemies)
		{
			const float Distance = YJJ::RangeDist(Enemy.Pos, Target);
			if (Enemy.IsAlive() && Distance > Spell.Radius * Tune.SplitTargetMinScale && Distance < Tune.SplitTargetMaxDistance)
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
				continue;
			}
			const float Angle = UE_TWO_PI * Index / Spell.Projectiles + Rng.FRandRange(-Tune.SplitScatterJitter, Tune.SplitScatterJitter);
			Targets.Add(Target + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle) / YJJ::YStretch) * Tune.SplitScatterRadius);
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
		Ball.Duration = FMath::Max(FVector2f::Distance(Muzzle, Targets[Index]) / Tune.SpellBallSpeed, Tune.SpellBallMinTime) + Tune.SpellBallStagger * Index;
		Ball.ArcHeight = Tune.SpellBallArc + Tune.SpellBallArcStep * Index;
		Ball.Damage = Damage;
		Ball.bMagic = true;
		Ball.Element = Spell.Elements[0];
		Ball.Element2 = Spell.NumElements > 1 ? Spell.Elements[1] : (Spell.bAmplified ? Spell.Elements[0] : EYJJElement::None);
		Ball.Spell = Spell;
	}

	State.CooldownMax = Spell.Cooldown * Tune.WizardCooldown(WizardLevel);
	State.Cooldown = State.CooldownMax;
	WizardCastAnim = Tune.WizardCastAnim;
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
	const FYJJTuning& Tune = YJJData::Tune();
	if (WaveIndex >= 0 && NextWaveTimer > Tune.EarlyCallMinTime)
	{
		const int32 Bonus = FMath::RoundToInt(NextWaveTimer * Tune.EarlyCallGoldPerSecond);
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
	const FYJJTuning& Tune = YJJData::Tune();
	WaveIndex = Index;
	WaveTime = 0.f;
	NextWaveTimer = 0.f;
	HPScale = (1.f + Tune.TierHPStep * (Config.Tier - 1)) * (1.f + Tune.WaveHPStep * Index);

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
	const FYJJTuning& Tune = YJJData::Tune();
	const FYJJEnemyDef& Def = YJJData::Enemy(Type);
	FYJJEnemyState& Enemy = Enemies.AddDefaulted_GetRef();
	Enemy.Id = NextId++;
	Enemy.Type = Type;
	Enemy.MaxHP = Def.HP * HPScale;
	Enemy.HP = Enemy.MaxHP;
	Enemy.Lateral = Def.bBoss ? 0.f : Rng.FRandRange(-Tune.EnemyLateralSpread, Tune.EnemyLateralSpread);
	Enemy.AnimTime = Rng.FRand();
	Enemy.AttackTimer = Def.MeleeInterval * Tune.EnemyFirstAttackDelay;
	Enemy.Pos = YJJMap::PointAt(0.f, Enemy.Lateral, &Enemy.Dir);
	return Enemy;
}

void FYJJDefenseSim::StepWaves()
{
	if (WaveIndex < 0)
	{
		return;
	}

	const FYJJTuning& Tune = YJJData::Tune();
	if (SpawnQueue.Num() > 0)
	{
		WaveTime += FixedStep;
		while (SpawnQueue.Num() > 0 && SpawnQueue.Last().Time <= WaveTime)
		{
			SpawnEnemy(SpawnQueue.Pop(EAllowShrinking::No).Type);
		}
		if (SpawnQueue.IsEmpty())
		{
			NextWaveTimer = Tune.WaveGap;
		}
	}
	else if (WaveIndex + 1 < YJJData::NumWaves())
	{
		NextWaveTimer -= FixedStep;
		if (Enemies.IsEmpty())
		{
			// 다 잡았으면 오래 기다리지 않는다
			NextWaveTimer = FMath::Min(NextWaveTimer, Tune.WaveGapWhenClear);
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

	const FYJJTuning& Tune = YJJData::Tune();
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	float Final = Amount * (1.f - (bMagic ? Def.MagicResist : Def.Armor));
	if (Enemy.VulnTime > 0.f)
	{
		Final *= Enemy.VulnMult;
	}

	Enemy.HP -= Final;
	if (Amount > Tune.HitFlashMinDamage)
	{
		Enemy.HitFlash = Tune.HitFlashTime;
	}

	if (Enemy.HP > 0.f)
	{
		return;
	}
	Enemy.HP = 0.f;
	Gold += Def.Gold;
	Kills++;
	FYJJSimEvent& Event = Emit(EYJJSimEvent::EnemyKilled, Enemy.Pos);
	Event.Int = Def.Gold;
	Event.Value = static_cast<float>(Enemy.Type);

	if (Def.HasSplit())
	{
		PendingSplits.Add({ Def.SplitInto, Def.SplitCount, Enemy.Dist, Enemy.Lateral, Enemy.Pos });
	}
}

void FYJJDefenseSim::SpawnSplits()
{
	const FYJJTuning& Tune = YJJData::Tune();
	for (const FSplit& Split : PendingSplits)
	{
		for (int32 Index = 0; Index < Split.Count; ++Index)
		{
			// 앞뒤·좌우로 조금씩 벌려 놓는다 (Side: -1 ~ 1)
			FYJJEnemyState& Kid = SpawnEnemy(Split.Type);
			const float Side = Split.Count > 1 ? FMath::Lerp(-1.f, 1.f, static_cast<float>(Index) / (Split.Count - 1)) : 0.f;
			Kid.Dist = FMath::Max(Split.Dist - Tune.SplitBackStep * Index, 0.f);
			Kid.Lateral = FMath::Clamp(Split.Lateral + Side * Tune.SplitSideSpread, -Tune.SplitLateralLimit, Tune.SplitLateralLimit);
			Kid.Pos = YJJMap::PointAt(Kid.Dist, Kid.Lateral, &Kid.Dir);
			Kid.HitFlash = Tune.HitFlashTime;
		}
		FYJJSimEvent& Event = Emit(EYJJSimEvent::EnemySplit, Split.Pos);
		Event.Value = static_cast<float>(Split.Type);
		Event.Int = Split.Count;
	}
	PendingSplits.Reset();
}

bool FYJJDefenseSim::ResistsChill(FYJJEnemyState& Enemy)
{
	if (YJJData::Enemy(Enemy.Type).Immune != EYJJElement::Ice)
	{
		return false;
	}
	if (Enemy.ImmuneTextCooldown <= 0.f)
	{
		Enemy.ImmuneTextCooldown = YJJData::Tune().ImmuneTextCooldown;
		Emit(EYJJSimEvent::EnemyImmune, Enemy.Pos).Element = EYJJElement::Ice;
	}
	return true;
}

void FYJJDefenseSim::AddBurn(FYJJEnemyState& Enemy, float Dps, float Duration)
{
	if (Dps <= 0.f || Duration <= 0.f)
	{
		return;
	}
	Enemy.BurnDps = Enemy.BurnTime > 0.f ? FMath::Max(Enemy.BurnDps, Dps) : Dps;
	Enemy.BurnTime = FMath::Max(Enemy.BurnTime, Duration);
}

void FYJJDefenseSim::AddPoison(FYJJEnemyState& Enemy, float Dps, float Duration)
{
	if (Dps <= 0.f || Duration <= 0.f)
	{
		return;
	}
	Enemy.PoisonDps = Enemy.PoisonTime > 0.f ? FMath::Max(Enemy.PoisonDps, Dps) : Dps;
	Enemy.PoisonTime = FMath::Max(Enemy.PoisonTime, Duration);
}

void FYJJDefenseSim::AddSlow(FYJJEnemyState& Enemy, float Factor, float Duration)
{
	if (Factor >= 1.f || Duration <= 0.f || ResistsChill(Enemy))
	{
		return;
	}
	if (YJJData::Enemy(Enemy.Type).bBoss)
	{
		Factor = FMath::Lerp(1.f, Factor, YJJData::Tune().BossSlowScale);
	}
	Enemy.SlowFactor = Enemy.SlowTime > 0.f ? FMath::Min(Enemy.SlowFactor, Factor) : Factor;
	Enemy.SlowTime = FMath::Max(Enemy.SlowTime, Duration);
}

void FYJJDefenseSim::AddStun(FYJJEnemyState& Enemy, float Duration)
{
	if (Duration <= 0.f)
	{
		return;
	}
	const float Scale = YJJData::Enemy(Enemy.Type).bBoss ? YJJData::Tune().BossControlScale : 1.f;
	Enemy.StunTime = FMath::Max(Enemy.StunTime, Duration * Scale);
}

void FYJJDefenseSim::AddFreeze(FYJJEnemyState& Enemy, float Duration)
{
	if (Duration <= 0.f || ResistsChill(Enemy))
	{
		return;
	}
	const float Scale = YJJData::Enemy(Enemy.Type).bBoss ? YJJData::Tune().BossControlScale : 1.f;
	Enemy.FreezeTime = FMath::Max(Enemy.FreezeTime, Duration * Scale);
}

void FYJJDefenseSim::Knockback(FYJJEnemyState& Enemy, float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	if (YJJData::Enemy(Enemy.Type).bBoss)
	{
		Amount *= YJJData::Tune().BossControlScale;
	}
	Enemy.Dist = FMath::Max(Enemy.Dist - Amount, 0.f);
	Enemy.BlockerId = INDEX_NONE;
	Enemy.Pos = YJJMap::PointAt(Enemy.Dist, Enemy.Lateral, &Enemy.Dir);
}

void FYJJDefenseSim::ApplyIntrinsic(FYJJEnemyState& Enemy, EYJJElement Element, float Power)
{
	if (!Enemy.IsAlive() || Element == EYJJElement::None)
	{
		return;
	}
	// 원소마다 받는 효과는 Runes.csv (안 쓰는 칸은 0이라 아무 일도 안 일어난다)
	const FYJJRuneDef& Def = YJJData::Rune(YJJData::ElementRune(Element));
	AddBurn(Enemy, Power * Def.BurnScale, Def.BurnTime);
	AddSlow(Enemy, Def.SlowFactor, Def.SlowTime);
	AddStun(Enemy, Def.StunTime);
	AddPoison(Enemy, Power * Def.PoisonScale, Def.PoisonTime);
	Knockback(Enemy, Def.Knockback);
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
		const EYJJReaction Reaction = YJJData::ReactionOf(Enemy.Aura, Element);
		Enemy.Aura = EYJJElement::None;
		Enemy.AuraTime = 0.f;
		return Reaction;
	}

	Enemy.Aura = Element;
	Enemy.AuraTime = YJJData::Tune().AuraDuration;
	return EYJJReaction::None;
}

void FYJJDefenseSim::StepEnemyHeal(FYJJEnemyState& Enemy)
{
	const FYJJTuning& Tune = YJJData::Tune();
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	Enemy.HealTimer += FixedStep;
	if (Enemy.HealTimer < Tune.HealInterval)
	{
		return;
	}
	Enemy.HealTimer = 0.f;

	bool bHealed = false;
	for (FYJJEnemyState& Other : Enemies)
	{
		if (Other.IsAlive() && Other.HP < Other.MaxHP && YJJ::RangeDist(Other.Pos, Enemy.Pos) <= Tune.HealRadius)
		{
			Other.HP = FMath::Min(Other.HP + Def.HealPerSecond * Tune.HealInterval, Other.MaxHP);
			bHealed = true;
		}
	}
	if (bHealed)
	{
		Emit(EYJJSimEvent::Heal, Enemy.Pos).Value = Tune.HealRadius;
	}
}

void FYJJDefenseSim::StepEnemyFight(FYJJEnemyState& Enemy, FYJJSoldierState& Blocker)
{
	const FYJJTuning& Tune = YJJData::Tune();
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	Enemy.AnimTime += FixedStep;
	Enemy.AttackTimer -= FixedStep;
	if (Enemy.AttackTimer > 0.f)
	{
		return;
	}

	Enemy.AttackTimer = Def.MeleeInterval;
	const float Armor = YJJData::Tower(EYJJTower::Barracks).Levels[Towers[Blocker.Spot].Level].SoldierArmor;
	Blocker.HP -= Def.MeleeDamage * (1.f - Armor);
	Blocker.HitFlash = Tune.HitFlashTime;
	Blocker.IdleTime = 0.f;
	Emit(EYJJSimEvent::MeleeHit, Blocker.Pos);

	if (Blocker.HP <= 0.f)
	{
		Blocker.HP = 0.f;
		Blocker.RespawnTimer = Tune.SoldierRespawn;
		Blocker.TargetId = INDEX_NONE;
		Enemy.BlockerId = INDEX_NONE;
		Emit(EYJJSimEvent::SoldierDied, Blocker.Pos);
	}
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

		Enemy.HitFlash = FMath::Max(Enemy.HitFlash - FixedStep, 0.f);
		Enemy.AuraTime -= FixedStep;
		if (Enemy.AuraTime <= 0.f)
		{
			Enemy.Aura = EYJJElement::None;
		}
		Enemy.SlowTime = FMath::Max(Enemy.SlowTime - FixedStep, 0.f);
		Enemy.StunTime = FMath::Max(Enemy.StunTime - FixedStep, 0.f);
		Enemy.FreezeTime = FMath::Max(Enemy.FreezeTime - FixedStep, 0.f);
		Enemy.VulnTime = FMath::Max(Enemy.VulnTime - FixedStep, 0.f);
		Enemy.ImmuneTextCooldown = FMath::Max(Enemy.ImmuneTextCooldown - FixedStep, 0.f);

		// 지속 피해
		if (Enemy.BurnTime > 0.f)
		{
			Enemy.BurnTime -= FixedStep;
			DamageEnemy(Enemy, Enemy.BurnDps * FixedStep, true);
		}
		if (Enemy.PoisonTime > 0.f)
		{
			Enemy.PoisonTime -= FixedStep;
			DamageEnemy(Enemy, Enemy.PoisonDps * FixedStep, true);
		}
		if (!Enemy.IsAlive())
		{
			continue;
		}

		if (Def.HealPerSecond > 0.f && !Enemy.IsDisabled())
		{
			StepEnemyHeal(Enemy);
		}
		if (Enemy.IsDisabled())
		{
			continue;
		}

		// 병졸에게 막혀 있으면 멈춰서 싸운다
		if (Enemy.BlockerId != INDEX_NONE)
		{
			FYJJSoldierState* Blocker = FindSoldier(Enemy.BlockerId);
			if (Blocker && Blocker->IsAlive() && Blocker->TargetId == Enemy.Id)
			{
				StepEnemyFight(Enemy, *Blocker);
				continue;
			}
			Enemy.BlockerId = INDEX_NONE;
		}

		const float Speed = Def.Speed * (Enemy.SlowTime > 0.f ? Enemy.SlowFactor : 1.f);
		Enemy.Dist += Speed * FixedStep;
		Enemy.AnimTime += FixedStep * (Speed / FMath::Max(Def.Speed, KINDA_SMALL_NUMBER));
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

void FYJJDefenseSim::StepSoldierFight(FYJJSoldierState& Soldier, FYJJEnemyState& Target, const FYJJTowerLevel& Level)
{
	const FYJJTuning& Tune = YJJData::Tune();
	Soldier.IdleTime = 0.f;

	// 적의 앞을 막아선다
	const FVector2f Desired = Target.Pos + Target.Dir * (YJJData::Enemy(Target.Type).Radius + Tune.SoldierFrontGap) + Tune.SoldierSlot(Soldier.Slot) * Tune.SoldierSlotSpread;
	const FVector2f Delta = Desired - Soldier.Pos;
	const float Distance = Delta.Size();
	Soldier.bFacingLeft = Target.Pos.X < Soldier.Pos.X;

	if (Distance < Tune.SoldierBlockDistance && Target.BlockerId == INDEX_NONE)
	{
		Target.BlockerId = Soldier.Id;
	}
	if (Distance > Tune.SoldierArriveDistance)
	{
		Soldier.Pos += Delta / Distance * FMath::Min(Tune.SoldierSpeed * FixedStep, Distance);
		Soldier.bMoving = true;
	}
	if (Distance >= Tune.SoldierAttackDistance)
	{
		return;
	}
	Soldier.AttackTimer -= FixedStep;
	if (Soldier.AttackTimer <= 0.f)
	{
		Soldier.AttackTimer = Level.Interval;
		Soldier.AttackAnim = Tune.SoldierAttackAnim;
		DamageEnemy(Target, Level.Damage, false);
		Emit(EYJJSimEvent::MeleeHit, Target.Pos);
	}
}

void FYJJDefenseSim::StepSoldierRest(FYJJSoldierState& Soldier)
{
	const FYJJTuning& Tune = YJJData::Tune();
	const FVector2f Delta = Soldier.Home - Soldier.Pos;
	const float Distance = Delta.Size();
	if (Distance > Tune.SoldierHomeArrive)
	{
		Soldier.Pos += Delta / Distance * FMath::Min(Tune.SoldierSpeed * FixedStep, Distance);
		Soldier.bFacingLeft = Delta.X < 0.f;
		Soldier.bMoving = true;
	}

	// 쉬는 동안 회복
	Soldier.IdleTime += FixedStep;
	if (Soldier.IdleTime > Tune.SoldierIdleHealDelay)
	{
		Soldier.HP = FMath::Min(Soldier.HP + Soldier.MaxHP * Tune.SoldierIdleHealRate * FixedStep, Soldier.MaxHP);
	}
}

void FYJJDefenseSim::StepSoldiers()
{
	const FYJJTuning& Tune = YJJData::Tune();
	for (FYJJSoldierState& Soldier : Soldiers)
	{
		const FVector2f SpotPos = YJJMap::Spots()[Soldier.Spot];
		const FYJJTowerLevel& Level = TowerLevel(Soldier.Spot);

		Soldier.HitFlash = FMath::Max(Soldier.HitFlash - FixedStep, 0.f);
		Soldier.AttackAnim = FMath::Max(Soldier.AttackAnim - FixedStep, 0.f);
		Soldier.AnimTime += FixedStep;
		Soldier.bMoving = false;

		if (!Soldier.IsAlive())
		{
			Soldier.RespawnTimer -= FixedStep;
			if (Soldier.RespawnTimer <= 0.f)
			{
				Soldier.RespawnTimer = 0.f;
				Soldier.HP = Soldier.MaxHP;
				Soldier.Pos = SpotPos;
				Soldier.TargetId = INDEX_NONE;
			}
			continue;
		}

		// 목표가 날아갔거나 멀어졌으면 놓는다
		FYJJEnemyState* Target = FindEnemyMutable(Soldier.TargetId);
		if (Target && (YJJData::Enemy(Target->Type).bFlying || FVector2f::Distance(Target->Pos, Soldier.Home) > Tune.SoldierEngageRange + Tune.SoldierReleaseSlack))
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
				if (!Enemy.IsAlive() || YJJData::Enemy(Enemy.Type).bFlying || FVector2f::Distance(Enemy.Pos, Soldier.Home) > Tune.SoldierEngageRange)
				{
					continue;
				}
				const float Score = Enemy.Dist + (Enemy.BlockerId == INDEX_NONE ? UnblockedTargetBonus : 0.f);
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
			StepSoldierFight(Soldier, *Target, Level);
		}
		else
		{
			StepSoldierRest(Soldier);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 타워

void FYJJDefenseSim::FireTower(FYJJTowerState& Tower, const FYJJTowerDef& Def, const FYJJTowerLevel& Level, const FVector2f& SpotPos, const FYJJEnemyState& Target)
{
	const FYJJTuning& Tune = YJJData::Tune();
	Tower.Cooldown = Level.Interval;
	Tower.FireAnim = Tune.TowerFireAnim;
	Tower.bAimLeft = Target.Pos.X < SpotPos.X;

	const FVector2f Muzzle = TowerMuzzle(SpotPos);
	const float Distance = FVector2f::Distance(Muzzle, Target.Pos);

	FYJJProjectileState& Shot = Projectiles.AddDefaulted_GetRef();
	Shot.Kind = Def.Projectile;
	Shot.Start = Muzzle;
	Shot.Pos = Muzzle;
	Shot.PrevPos = Muzzle;
	Shot.Target = Target.Pos;
	Shot.TargetId = Def.bHoming ? Target.Id : INDEX_NONE;
	Shot.Damage = Level.Damage;
	Shot.Splash = Level.Splash;
	Shot.bMagic = Def.bMagic;
	Shot.Element = Def.Type == EYJJTower::Shrine ? Tower.ShrineElement : EYJJElement::None;
	Shot.Duration = Def.ShotSpeed > 0.f ? FMath::Max(Distance / Def.ShotSpeed, Def.ShotTime) : Def.ShotTime;
	Shot.ArcHeight = Distance * Def.ShotArcRatio + Def.ShotArcHeight;

	if (Def.bLeadTarget)
	{
		// 적이 걸어올 자리를 미리 겨냥한다 (멈춰 있으면 그 자리)
		const bool bStanding = Target.BlockerId != INDEX_NONE || Target.IsDisabled();
		const float Speed = YJJData::Enemy(Target.Type).Speed * (Target.SlowTime > 0.f ? Target.SlowFactor : 1.f);
		Shot.Target = bStanding ? Target.Pos : YJJMap::PointAt(Target.Dist + Speed * Shot.Duration, Target.Lateral);
	}

	Emit(EYJJSimEvent::TowerFired, Muzzle).Int = static_cast<int32>(Shot.Kind);
}

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

		Tower.Cooldown -= FixedStep;
		Tower.FireAnim = FMath::Max(Tower.FireAnim - FixedStep, 0.f);
		Tower.BuildAnim = FMath::Max(Tower.BuildAnim - FixedStep, 0.f);

		const FYJJTowerDef& Def = YJJData::Tower(Tower.Type);
		if (Def.Projectile == EYJJProjectile::None || Tower.Cooldown > 0.f)
		{
			continue;
		}

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
		if (Target)
		{
			FireTower(Tower, Def, Level, SpotPos, *Target);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 투사체

void FYJJDefenseSim::StepProjectiles()
{
	for (int32 Index = Projectiles.Num() - 1; Index >= 0; --Index)
	{
		FYJJProjectileState& Projectile = Projectiles[Index];
		Projectile.Time += FixedStep;

		if (const FYJJEnemyState* Target = FindEnemy(Projectile.TargetId))
		{
			Projectile.Target = Target->Pos;
		}

		const float Alpha = FMath::Clamp(Projectile.Time / Projectile.Duration, 0.f, 1.f);
		Projectile.PrevPos = Projectile.Pos;
		Projectile.Pos = FMath::Lerp(Projectile.Start, Projectile.Target, Alpha);
		Projectile.Pos.Y -= Projectile.ArcHeight * ArcPeakScale * Alpha * (1.f - Alpha);

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
				DamageEnemy(Enemy, Projectile.Damage, Projectile.bMagic);
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
		const float Power = Projectile.Damage * YJJData::Tune().ShrinePowerScale;
		EYJJReaction Pending = EYJJReaction::None;
		FVector2f PendingPos = Projectile.Target;
		ForEnemiesInRadius(Projectile.Target, Projectile.Splash, [&](FYJJEnemyState& Enemy)
		{
			DamageEnemy(Enemy, Projectile.Damage, Projectile.bMagic);
			const EYJJReaction Reaction = ApplyElement(Enemy, Projectile.Element, Power);
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
			TriggerReaction(Pending, PendingPos, Power * YJJData::Tune().WizardDamage(WizardLevel));
		}
		break;
	}

	case EYJJProjectile::SpellBall:
		ImpactSpell(Projectile.Spell, Projectile.Target, Projectile.Damage);
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------------------------------------
// 스펠

void FYJJDefenseSim::HitWithSpell(const FYJJSpellSpec& Spell, const FVector2f& Pos, float Radius, float Damage, int32 OnlyEnemyId, TArray<int32>& HitIds)
{
	EYJJReaction Pending = EYJJReaction::None;
	FVector2f PendingPos = Pos;
	const bool bTwoElements = Spell.NumElements == YJJ::NumSockets;

	auto HitOne = [&](FYJJEnemyState& Enemy)
	{
		HitIds.AddUnique(Enemy.Id);
		DamageEnemy(Enemy, Damage, true);

		if (bTwoElements)
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

	if (bTwoElements)
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
	const FYJJTuning& Tune = YJJData::Tune();
	TArray<int32> HitIds;
	HitWithSpell(Spell, Pos, Spell.Radius, Damage, INDEX_NONE, HitIds);

	// 연쇄: 아직 안 맞은 가장 가까운 적으로 옮겨 붙는다
	FVector2f From = Pos;
	float HopDamage = Damage * Tune.ChainHopDamage;
	for (int32 Jump = 0; Jump < Spell.ChainJumps; ++Jump)
	{
		const FYJJEnemyState* Next = nullptr;
		float BestDistance = Tune.ChainSearchRange;
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
		HopDamage *= Tune.ChainHopFalloff;
	}
}

void FYJJDefenseSim::ApplyReactionTo(const FYJJReactionDef& Def, FYJJEnemyState& Enemy, float Power)
{
	DamageEnemy(Enemy, Power * Def.Damage, true);
	if (!Enemy.IsAlive())
	{
		return;
	}
	Knockback(Enemy, Def.Knockback);
	AddBurn(Enemy, Power * Def.BurnScale, Def.BurnTime);
	AddPoison(Enemy, Power * Def.PoisonScale, Def.PoisonTime);
	AddSlow(Enemy, Def.SlowFactor, Def.SlowTime);
	AddStun(Enemy, Def.StunTime);
	AddFreeze(Enemy, Def.FreezeTime);
	if (Def.VulnTime > 0.f)
	{
		Enemy.VulnMult = Def.VulnMult;
		Enemy.VulnTime = Def.VulnTime;
	}
}

void FYJJDefenseSim::TriggerReaction(EYJJReaction Reaction, const FVector2f& Pos, float Power)
{
	if (Reaction == EYJJReaction::None)
	{
		return;
	}
	Reactions++;

	// 효과 종류는 Reactions.csv 의 칸으로 정해진다 (즉시 효과 / 가까운 몇 명에게 번개 / 장판)
	const FYJJReactionDef& Def = YJJData::Reaction(Reaction);
	if (Def.Targets > 0)
	{
		TArray<FYJJEnemyState*> Nearby;
		ForEnemiesInRadius(Pos, Def.Radius, [&Nearby](FYJJEnemyState& Enemy) { Nearby.Add(&Enemy); });
		Nearby.Sort([&Pos](const FYJJEnemyState& A, const FYJJEnemyState& B) { return YJJ::RangeDist(A.Pos, Pos) < YJJ::RangeDist(B.Pos, Pos); });
		for (int32 Index = 0; Index < FMath::Min(Nearby.Num(), Def.Targets); ++Index)
		{
			Emit(EYJJSimEvent::SkyBolt, Pos).Pos2 = Nearby[Index]->Pos;
			ApplyReactionTo(Def, *Nearby[Index], Power);
		}
	}
	else
	{
		ForEnemiesInRadius(Pos, Def.Radius, [this, &Def, Power](FYJJEnemyState& Enemy) { ApplyReactionTo(Def, Enemy, Power); });
	}

	if (Def.ZoneTime > 0.f)
	{
		FYJJZoneState& Zone = Zones.AddDefaulted_GetRef();
		Zone.Kind = Reaction;
		Zone.Pos = Pos;
		Zone.Radius = Def.Radius;
		Zone.Duration = Def.ZoneTime;
		Zone.Dps = Power * Def.ZoneDps;
		Zone.SlowFactor = Def.ZoneSlow;
		Zone.BurnDps = Power * Def.ZoneBurn;
		Zone.bGroundOnly = Def.bZoneGroundOnly;
	}

	FYJJSimEvent& Event = Emit(EYJJSimEvent::Reaction, Pos);
	Event.Reaction = Reaction;
	Event.Value = Def.DisplayRadius();
}

// ---------------------------------------------------------------------------------------------
// 장판

void FYJJDefenseSim::StepZones()
{
	const FYJJTuning& Tune = YJJData::Tune();
	for (int32 Index = Zones.Num() - 1; Index >= 0; --Index)
	{
		FYJJZoneState& Zone = Zones[Index];
		Zone.Time += FixedStep;
		Zone.TickTimer += FixedStep;

		if (Zone.TickTimer >= Tune.ZoneTickInterval)
		{
			Zone.TickTimer -= Tune.ZoneTickInterval;
			const FYJJZoneState Tick = Zone;
			ForEnemiesInRadius(Tick.Pos, Tick.Radius, [this, &Tick, &Tune](FYJJEnemyState& Enemy)
			{
				if (Tick.bGroundOnly && YJJData::Enemy(Enemy.Type).bFlying)
				{
					return;
				}
				DamageEnemy(Enemy, Tick.Dps * Tune.ZoneTickInterval, true);
				AddSlow(Enemy, Tick.SlowFactor, Tune.ZoneSlowTime);
				AddBurn(Enemy, Tick.BurnDps, Tune.ZoneBurnTime);
				AddPoison(Enemy, Tick.PoisonDps, Tune.ZonePoisonTime);
			});
		}

		if (Zones[Index].Time >= Zones[Index].Duration)
		{
			Zones.RemoveAtSwap(Index, EAllowShrinking::No);
		}
	}
}
