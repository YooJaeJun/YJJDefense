#pragma once

#include "Game/YJJDefenseTypes.h"
#include "Game/YJJGameData.h"
#include "Game/YJJSpellForge.h"
#include "Math/RandomStream.h"
#include "YJJDefenseSim.generated.h"

struct FYJJEnemyState
{
	int32 Id = INDEX_NONE;
	EYJJEnemy Type = EYJJEnemy::Imp;
	float HP = 0.f;
	float MaxHP = 0.f;
	float Dist = 0.f;      // 길을 따라 온 거리
	float Lateral = 0.f;   // 길 중심에서 옆으로 벗어난 정도
	FVector2f Pos = FVector2f::ZeroVector;
	FVector2f Dir = FVector2f(1.f, 0.f);

	// 묻어 있는 원소. 다른 원소가 닿으면 반응이 일어나고 사라진다
	EYJJElement Aura = EYJJElement::None;
	float AuraTime = 0.f;

	float BurnTime = 0.f;
	float BurnDps = 0.f;
	float PoisonTime = 0.f;
	float PoisonDps = 0.f;
	float SlowTime = 0.f;
	float SlowFactor = 1.f;
	float StunTime = 0.f;
	float FreezeTime = 0.f;
	float VulnTime = 0.f;
	float VulnMult = 1.f;
	float ImmuneTextCooldown = 0.f;   // "면역!" 표시를 너무 자주 띄우지 않게

	int32 BlockerId = INDEX_NONE;  // 길을 막고 있는 병졸
	float AttackTimer = 0.f;
	float HealTimer = 0.f;

	// 그리기용
	float HitFlash = 0.f;
	float AnimTime = 0.f;

	bool IsAlive() const { return HP > 0.f; }
	bool IsDisabled() const { return StunTime > 0.f || FreezeTime > 0.f; }
};

struct FYJJSoldierState
{
	int32 Id = INDEX_NONE;
	int32 Spot = INDEX_NONE;
	int32 Slot = 0;
	FVector2f Pos = FVector2f::ZeroVector;
	FVector2f Home = FVector2f::ZeroVector;
	float HP = 0.f;
	float MaxHP = 0.f;
	int32 TargetId = INDEX_NONE;
	float AttackTimer = 0.f;
	float RespawnTimer = 0.f;  // 0보다 크면 쓰러져서 대기 중
	float IdleTime = 0.f;

	float AnimTime = 0.f;
	float AttackAnim = 0.f;
	float HitFlash = 0.f;
	bool bFacingLeft = false;
	bool bMoving = false;

	bool IsAlive() const { return RespawnTimer <= 0.f; }
};

struct FYJJTowerState
{
	bool bBuilt = false;
	EYJJTower Type = EYJJTower::Archer;
	int32 Level = 0;
	float Cooldown = 0.f;
	int32 Spent = 0;
	EYJJElement ShrineElement = EYJJElement::Fire;
	FVector2f Rally = FVector2f::ZeroVector;

	float FireAnim = 0.f;
	float BuildAnim = 0.f;
	bool bAimLeft = false;
};

struct FYJJProjectileState
{
	EYJJProjectile Kind = EYJJProjectile::Arrow;
	FVector2f Start = FVector2f::ZeroVector;
	FVector2f Pos = FVector2f::ZeroVector;
	FVector2f PrevPos = FVector2f::ZeroVector;
	FVector2f Target = FVector2f::ZeroVector;
	int32 TargetId = INDEX_NONE;  // 유도 대상 (없으면 Target 지점으로)
	float Time = 0.f;
	float Duration = 0.3f;
	float ArcHeight = 0.f;
	float Damage = 0.f;
	float Splash = 0.f;
	bool bMagic = false;
	EYJJElement Element = EYJJElement::None;
	EYJJElement Element2 = EYJJElement::None;
	FYJJSpellSpec Spell;
};

// 바닥에 남아 계속 효과를 주는 장판 (증기, 화염 폭풍, 눈보라, 독늪)
struct FYJJZoneState
{
	EYJJReaction Kind = EYJJReaction::None;
	FVector2f Pos = FVector2f::ZeroVector;
	float Radius = 0.f;
	float Time = 0.f;
	float Duration = 0.f;
	float Dps = 0.f;
	float SlowFactor = 1.f;
	float BurnDps = 0.f;
	float PoisonDps = 0.f;
	bool bGroundOnly = false;
	float TickTimer = 0.f;
};

struct FYJJDiskState
{
	EYJJRune Sockets[YJJ::NumSockets] = { EYJJRune::None, EYJJRune::None };
	FYJJSpellSpec Spell;
	float Cooldown = 0.f;
	float CooldownMax = 1.f;
};

// 화면 연출·소리에 넘기는 사건. 이름이 Sounds.csv 행 이름이 된다
UENUM()
enum class EYJJSimEvent : uint8
{
	EnemyKilled,     // Pos, Int=골드, Value=몬스터 종류
	EnemyLeaked,     // Int=잃은 목숨
	EnemySplit,      // Pos, Value=쪼개진 몬스터 종류, Int=개수
	EnemyImmune,     // Pos, Element=막아 낸 원소
	TowerBuilt,      // Pos
	TowerUpgraded,   // Pos
	TowerSold,       // Pos, Int=환불
	TowerFired,      // Pos, Int=EYJJProjectile
	ProjectileHit,   // Pos, Int=EYJJProjectile, Value=반경
	SpellCast,       // Pos=마법사, Pos2=목표, Int=ComboId, Element
	SpellImpact,     // Pos, Value=반경, Element, Int=두 번째 원소
	Reaction,        // Pos, Reaction, Value=반경
	ChainArc,        // Pos→Pos2, Element
	SkyBolt,         // Pos2에 번개가 떨어짐
	SoldierDied,     // Pos
	MeleeHit,        // Pos
	Heal,            // Pos, Value=반경
	WaveStarted,     // Int=웨이브 번호(0부터)
	WaveBonus,       // Int=골드
	WizardUpgraded,
	Victory,
	Defeat
};

struct FYJJSimEvent
{
	EYJJSimEvent Type = EYJJSimEvent::EnemyKilled;
	FVector2f Pos = FVector2f::ZeroVector;
	FVector2f Pos2 = FVector2f::ZeroVector;
	float Value = 0.f;
	int32 Int = 0;
	EYJJElement Element = EYJJElement::None;
	EYJJReaction Reaction = EYJJReaction::None;
};

enum class EYJJRunResult : uint8
{
	Playing,
	Victory,
	Defeat
};

// 한 판을 시작하는 조건. 기본값 = 처음 시작한 사람의 조건 (Unlocks.csv 의 처음부터 가진 것, StartDisks.csv)
struct YJJDEFENSE_API FYJJRunConfig
{
	FYJJRunConfig();

	int32 Map = 0;
	int32 Tier = 1;
	int32 Seed = 0;
	uint32 RuneMask = 0;    // 들고 나온 룬 (비트 = EYJJRune)
	uint32 TowerMask = 0;   // 들고 나온 병졸 (비트 = EYJJTower)
	EYJJRune Disks[YJJ::NumDisks][YJJ::NumSockets];
};

// 타워 디펜스 한 판의 규칙 전부. UObject·월드와 무관한 순수 계산이라 테스트에서 그대로 돌린다.
// 고정 스텝(FixedStep)으로만 진행해서 같은 입력이면 항상 같은 결과가 나온다
class YJJDEFENSE_API FYJJDefenseSim
{
public:
	/** 시뮬레이션 1스텝 (초). 결정적이려면 늘 이 간격으로만 돈다 */
	static constexpr float FixedStep = 1.f / 60.f;

	void Reset(const FYJJRunConfig& InConfig);
	void Step();

	// --- 플레이어 명령 (성공하면 true) ---
	bool BuildTower(int32 Spot, EYJJTower Type);
	bool UpgradeTower(int32 Spot);
	bool SellTower(int32 Spot);
	bool CycleShrineElement(int32 Spot);
	bool UpgradeWizard();
	bool SetSocket(int32 Disk, int32 Socket, EYJJRune Rune);
	bool CastSpell(int32 Disk, const FVector2f& Target);
	bool CallNextWave();

	// 테스트용: 길 위 Dist 지점에 적 하나를 바로 세운다. 적 Id를 돌려준다
	int32 DebugSpawnEnemy(EYJJEnemy Type, float Dist);

	// --- 조회 ---
	bool CanBuild(int32 Spot, EYJJTower Type) const;
	bool CanUpgrade(int32 Spot) const;
	int32 UpgradeCost(int32 Spot) const;
	int32 SellValue(int32 Spot) const;
	bool CanCallWave() const;
	bool CanCast(int32 Disk) const;
	bool HasRune(EYJJRune Rune) const { return Rune < EYJJRune::Count && (Config.RuneMask & (1u << static_cast<uint32>(Rune))) != 0; }
	bool HasTower(EYJJTower Tower) const { return (Config.TowerMask & (1u << static_cast<uint32>(Tower))) != 0; }
	int32 WizardUpgradeCost() const;
	int32 Stars() const;
	bool IsWaveSpawning() const { return SpawnQueue.Num() > 0; }
	const FYJJEnemyState* FindEnemy(int32 Id) const;
	const FYJJTowerLevel& TowerLevel(int32 Spot) const;

	FYJJRunConfig Config;
	EYJJRunResult Result = EYJJRunResult::Playing;
	float Time = 0.f;
	int32 Gold = 0;
	int32 Lives = 0;
	int32 WaveIndex = INDEX_NONE;   // 진행 중인 웨이브 (시작 전 -1)
	float NextWaveTimer = 0.f;
	int32 WizardLevel = 0;
	float WizardCastAnim = 0.f;

	TArray<FYJJEnemyState> Enemies;
	TArray<FYJJSoldierState> Soldiers;
	TArray<FYJJTowerState> Towers;      // 건설 자리와 1:1
	TArray<FYJJProjectileState> Projectiles;
	TArray<FYJJZoneState> Zones;
	FYJJDiskState Disks[YJJ::NumDisks];

	// 그리기·소리용. 호출자가 읽고 비운다
	TArray<FYJJSimEvent> Events;

	// 통계
	int32 Kills = 0;
	int32 SpellsCast = 0;
	int32 Reactions = 0;
	TArray<int32> CombosCast;   // 이번 판에 쓴 2룬 조합 (중복 없음)

private:
	struct FSpawn
	{
		float Time;
		EYJJEnemy Type;
	};

	FYJJEnemyState* FindEnemyMutable(int32 Id);
	FYJJSoldierState* FindSoldier(int32 Id);
	FYJJSimEvent& Emit(EYJJSimEvent Type, const FVector2f& Pos);

	void StartWave(int32 Index);
	FYJJEnemyState& SpawnEnemy(EYJJEnemy Type);
	void SpawnSplits();
	// 둔화·빙결을 막는 몬스터면 true (표시도 띄운다)
	bool ResistsChill(FYJJEnemyState& Enemy);
	void StepWaves();
	void StepEnemies();
	void StepSoldiers();
	void StepTowers();
	void StepProjectiles();
	void StepZones();

	void ResolveProjectile(const FYJJProjectileState& Projectile);
	void DamageEnemy(FYJJEnemyState& Enemy, float Amount, bool bMagic);
	void AddBurn(FYJJEnemyState& Enemy, float Dps, float Duration);
	void AddPoison(FYJJEnemyState& Enemy, float Dps, float Duration);
	void AddSlow(FYJJEnemyState& Enemy, float Factor, float Duration);
	void AddStun(FYJJEnemyState& Enemy, float Duration);
	void AddFreeze(FYJJEnemyState& Enemy, float Duration);
	void Knockback(FYJJEnemyState& Enemy, float Amount);

	// 원소 고유 효과만 준다 (오라는 건드리지 않음)
	void ApplyIntrinsic(FYJJEnemyState& Enemy, EYJJElement Element, float Power);
	// 고유 효과 + 오라 처리. 기존 오라와 반응하면 그 반응을 돌려준다
	EYJJReaction ApplyElement(FYJJEnemyState& Enemy, EYJJElement Element, float Power);

	// 스펠이 Pos에 닿았을 때. OnlyEnemyId가 있으면 그 적만 맞는다 (연쇄)
	void HitWithSpell(const FYJJSpellSpec& Spell, const FVector2f& Pos, float Radius, float Damage, int32 OnlyEnemyId, TArray<int32>& HitIds);
	void ImpactSpell(const FYJJSpellSpec& Spell, const FVector2f& Pos, float Damage);
	void TriggerReaction(EYJJReaction Reaction, const FVector2f& Pos, float Power);
	void ApplyReactionTo(const FYJJReactionDef& Def, FYJJEnemyState& Enemy, float Power);
	void FireTower(FYJJTowerState& Tower, const FYJJTowerDef& Def, const FYJJTowerLevel& Level, const FVector2f& SpotPos, const FYJJEnemyState& Target);
	void StepSoldierFight(FYJJSoldierState& Soldier, FYJJEnemyState& Target, const FYJJTowerLevel& Level);
	void StepSoldierRest(FYJJSoldierState& Soldier);
	void StepEnemyFight(FYJJEnemyState& Enemy, FYJJSoldierState& Blocker);
	void StepEnemyHeal(FYJJEnemyState& Enemy);

	/** 반경 안의 산 적마다 Func. Func 안에서 적이 죽거나 밀려날 수 있지만 배열 크기는 스텝 끝에서만 줄어든다 */
	template <typename FuncType>
	void ForEnemiesInRadius(const FVector2f& Pos, float Radius, FuncType&& Func)
	{
		for (int32 Index = 0; Index < Enemies.Num(); ++Index)
		{
			FYJJEnemyState& Enemy = Enemies[Index];
			if (Enemy.IsAlive() && YJJ::RangeDist(Enemy.Pos, Pos) <= Radius + YJJData::Enemy(Enemy.Type).Radius * EnemyHitRadiusShare)
			{
				Func(Enemy);
			}
		}
	}

	static FVector2f TowerMuzzle(const FVector2f& SpotPos);
	static FVector2f WizardMuzzle();
	/** 적 반경의 이만큼까지 겹치면 범위 공격에 맞는다 */
	static constexpr float EnemyHitRadiusShare = 0.5f;
	/** 막지 않은 적을 먼저 노리게 하는 가산점 (길 거리보다 훨씬 크게) */
	static constexpr float UnblockedTargetBonus = 100000.f;
	/** 포물선 4t(1-t): 가운데에서 최댓값 1이 되게 하는 배율 */
	static constexpr float ArcPeakScale = 4.f;

public:
	static constexpr int32 ThreeStars = 3;
	static constexpr int32 TwoStars = 2;
	static constexpr int32 OneStar = 1;

private:

	FRandomStream Rng;
	TArray<FSpawn> SpawnQueue;   // 뒤에서부터 꺼낸다 (시간 내림차순)

	// 죽어서 쪼개질 몬스터. 적 배열을 도는 중에 늘리면 참조가 깨지므로 스텝 끝에 한꺼번에 만든다
	struct FSplit
	{
		EYJJEnemy Type;
		int32 Count;
		float Dist;
		float Lateral;
		FVector2f Pos;
	};
	TArray<FSplit> PendingSplits;
	float WaveTime = 0.f;
	float HPScale = 1.f;
	int32 NextId = 1;
};
