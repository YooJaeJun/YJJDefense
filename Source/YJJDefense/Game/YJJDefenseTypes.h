#pragma once

#include "CoreMinimal.h"
#include "YJJDefenseTypes.generated.h"

// 게임 좌표는 1920x1080 가상 화면 (y는 아래로 증가). 시뮬레이션과 그리기가 같은 좌표를 쓴다.
// 여기 있는 것은 코드 구조에 묶인 값(배열 크기·화면 기준)뿐이고, 밸런스는 Content/YJJDefense/Data/*.csv 에 있다
namespace YJJ
{
	inline constexpr float ViewW = 1920.f;
	inline constexpr float ViewH = 1080.f;

	// 3/4 시점이라 세로 거리는 더 멀게 친다 (사거리가 화면에서 납작한 타원으로 보인다)
	inline constexpr float YStretch = 1.3f;

	inline constexpr int32 NumDisks = 3;
	inline constexpr int32 NumSockets = 2;
	inline constexpr int32 NumTowerLevels = 3;
	inline constexpr int32 MaxTowerLevel = NumTowerLevels - 1;
	inline constexpr int32 SoldiersPerBarracks = 3;
	inline constexpr int32 NumWizardUpgrades = 3;

	inline float RangeDist(const FVector2f& A, const FVector2f& B)
	{
		const float DX = A.X - B.X;
		const float DY = (A.Y - B.Y) * YStretch;
		return FMath::Sqrt(DX * DX + DY * DY);
	}
}

// 룬: 앞의 여섯은 원소 볼, 뒤의 셋은 형태 (Runes.csv 행 이름과 같다)
UENUM()
enum class EYJJRune : uint8
{
	Fire,
	Ice,
	Water,
	Lightning,
	Poison,
	Wind,
	Explosion,
	Chain,
	Split,
	Count UMETA(Hidden),
	None = 254
};

UENUM()
enum class EYJJElement : uint8
{
	None,
	Fire,
	Ice,
	Water,
	Lightning,
	Poison,
	Wind,
	Count UMETA(Hidden)
};

// 서로 다른 두 원소가 만나면 일어나는 반응 (6원소 → 15가지). 수치와 원소 짝은 Reactions.csv
UENUM()
enum class EYJJReaction : uint8
{
	None,
	Evaporate,
	Steam,
	Overload,
	Combust,
	Firestorm,
	Freeze,
	Superconduct,
	Frostbite,
	Blizzard,
	Electrocute,
	Swamp,
	Tidal,
	Neurotoxin,
	Thunderstorm,
	Plague,
	Count UMETA(Hidden)
};

UENUM()
enum class EYJJTower : uint8
{
	Archer,
	Barracks,
	Bombard,
	Mage,
	Shrine,
	Count UMETA(Hidden),
	None
};

// 몬스터 (Enemies.csv 행 이름과 같다)
UENUM()
enum class EYJJEnemy : uint8
{
	Imp,
	Chicken,
	Potato,
	Toast,
	Mushroom,
	PuddingKing,
	Crab,
	Jellyfish,
	Coconut,
	CocoKid,
	KingCrab,
	Snowman,
	Snowball,
	Penguin,
	Yeti,
	Count UMETA(Hidden),
	None
};

UENUM()
enum class EYJJProjectile : uint8
{
	Arrow,
	Bomb,
	MagicBolt,
	ShrineOrb,
	SpellBall,
	None
};

// ---- 데이터 표를 게임이 쓰기 좋게 푼 것 (YJJGameData가 CSV에서 만든다) ----

struct FYJJRuneDef
{
	EYJJRune Rune = EYJJRune::None;
	EYJJElement Element = EYJJElement::None;
	FString NameKey;    // 룬 이름 (불, 얼음, 폭발...)
	FString SpellKey;   // 단독 스펠 이름 (파이어볼...)
	FString DescKey;
	FString Art;        // T_Rune_<Art>
	FLinearColor Color = FLinearColor::White;

	// 이 원소에 맞으면 받는 효과 (형태 룬은 쓰지 않는다)
	float BurnScale = 0.f;
	float BurnTime = 0.f;
	float SlowFactor = 1.f;
	float SlowTime = 0.f;
	float StunTime = 0.f;
	float PoisonScale = 0.f;
	float PoisonTime = 0.f;
	float Knockback = 0.f;
	// 이 원소가 들어간 스펠
	float SpellDamageMult = 1.f;
	float SpellRadiusBonus = 0.f;
};

struct FYJJReactionDef
{
	EYJJReaction Reaction = EYJJReaction::None;
	EYJJElement ElementA = EYJJElement::None;
	EYJJElement ElementB = EYJJElement::None;
	FString NameKey;
	FString SpellKey;
	FString DescKey;
	FLinearColor Color = FLinearColor::White;

	float Radius = 0.f;
	float FxRadius = 0.f;    // 화면 연출 반경 (0이면 Radius)
	float Damage = 0.f;      // 이하 Scale·Dps 값은 반응 세기 x 값
	float Knockback = 0.f;
	float BurnScale = 0.f;
	float BurnTime = 0.f;
	float PoisonScale = 0.f;
	float PoisonTime = 0.f;
	float SlowFactor = 1.f;
	float SlowTime = 0.f;
	float StunTime = 0.f;
	float FreezeTime = 0.f;
	float VulnMult = 1.f;
	float VulnTime = 0.f;
	int32 Targets = 0;       // 0보다 크면 반경 안 가까운 적 이만큼에게만 (번개가 떨어진다)
	float ZoneTime = 0.f;    // 0보다 크면 장판을 남긴다
	float ZoneDps = 0.f;
	float ZoneSlow = 1.f;
	float ZoneBurn = 0.f;
	bool bZoneGroundOnly = false;

	float DisplayRadius() const { return FxRadius > 0.f ? FxRadius : Radius; }
};

struct FYJJTowerLevel
{
	int32 Cost = 0;
	float Damage = 0.f;
	float Interval = 1.f;
	float Range = 0.f;
	float Splash = 0.f;
	float SoldierHP = 0.f;
	float SoldierArmor = 0.f;
};

struct FYJJTowerDef
{
	EYJJTower Type = EYJJTower::None;
	FString NameKey;
	FString DescKey;
	FString Art;        // T_Tower_<Art>_<단계>
	bool bMagic = false;
	bool bHitsAir = false;
	EYJJProjectile Projectile = EYJJProjectile::None;
	float ShotSpeed = 0.f;      // 0이면 늘 ShotTime 만큼 날아간다
	float ShotTime = 0.f;       // 최소 비행 시간
	float ShotArcRatio = 0.f;   // 포물선 높이 = 거리 x 이것
	float ShotArcHeight = 0.f;  // 포물선 높이 (고정)
	bool bHoming = false;
	bool bLeadTarget = false;   // 적이 걸어올 자리를 미리 노린다
	FYJJTowerLevel Levels[YJJ::NumTowerLevels];
};

struct FYJJEnemyDef
{
	EYJJEnemy Type = EYJJEnemy::None;
	FString NameKey;
	FString Art;        // T_Enemy_<Art>_<프레임>
	float HP = 0.f;
	float Speed = 0.f;
	float Armor = 0.f;
	float MagicResist = 0.f;
	int32 Gold = 0;
	int32 LifeCost = 1;
	bool bFlying = false;
	bool bBoss = false;
	float MeleeDamage = 0.f;
	float MeleeInterval = 1.f;
	float Radius = 0.f;
	float HealPerSecond = 0.f;
	EYJJEnemy SplitInto = EYJJEnemy::None;    // 죽으면 이것으로 쪼개진다
	int32 SplitCount = 0;
	EYJJElement Immune = EYJJElement::None;   // 얼음이면 둔화·빙결을 받지 않는다

	bool HasSplit() const { return SplitInto < EYJJEnemy::Count && SplitCount > 0; }
};

struct FYJJWaveGroup
{
	EYJJEnemy Type = EYJJEnemy::Imp;
	int32 Count = 0;
	float Interval = 1.f;
	float Delay = 0.f;
};

struct FYJJMapDef
{
	FName Key;
	FString NameKey;
	FString DescKey;
	FString Texture;
	TArray<EYJJEnemy> Showcase;            // 준비 화면 미리보기에 나오는 몬스터
	TArray<TArray<FYJJWaveGroup>> Waves;   // 웨이브 순서
	TArray<FVector2f> Path;
	TArray<float> PathCumulative;          // 길 누적 거리
	TArray<FVector2f> Spots;
	FVector2f Wizard = FVector2f::ZeroVector;
	FVector2f Gate = FVector2f::ZeroVector;
};

// 수집 트랙 한 칸 (Unlocks.csv)
struct FYJJUnlock
{
	bool bTower = false;
	uint8 Index = 0;         // EYJJRune 또는 EYJJTower
	int32 PointsNeeded = 0;  // 누적 수집 점수
};

struct FYJJSoundDef
{
	FString Sound;
	float Volume = 1.f;
};

// 그림 동작 하나 (Anims.csv)
struct FYJJAnimDef
{
	int32 Frames = 0;
	float Fps = 0.f;
	int32 HitFrame = 0;   // 공격 동작에서 맞는 순간의 프레임
};
