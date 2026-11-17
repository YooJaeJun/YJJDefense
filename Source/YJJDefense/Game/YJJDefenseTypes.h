#pragma once

#include "CoreMinimal.h"

// 게임 좌표는 1920x1080 가상 화면 (y는 아래로 증가). 시뮬레이션과 그리기가 같은 좌표를 쓴다
namespace YJJ
{
	inline constexpr float ViewW = 1920.f;
	inline constexpr float ViewH = 1080.f;

	// 3/4 시점이라 세로 거리는 더 멀게 친다 (사거리가 화면에서 납작한 타원으로 보인다)
	inline constexpr float YStretch = 1.3f;

	inline constexpr int32 NumDisks = 3;
	inline constexpr int32 NumSockets = 2;
	inline constexpr int32 MaxTowerLevel = 2;
	inline constexpr int32 SoldiersPerBarracks = 3;

	inline float RangeDist(const FVector2f& A, const FVector2f& B)
	{
		const float DX = A.X - B.X;
		const float DY = (A.Y - B.Y) * YStretch;
		return FMath::Sqrt(DX * DX + DY * DY);
	}
}

// 룬: 앞의 여섯은 원소 볼, 뒤의 셋은 형태
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
	Count,
	None = 255
};

enum class EYJJElement : uint8
{
	None,
	Fire,
	Ice,
	Water,
	Lightning,
	Poison,
	Wind,
	Count
};

// 서로 다른 두 원소가 만나면 일어나는 반응 (6원소 → 15가지)
enum class EYJJReaction : uint8
{
	None,
	Evaporate,     // 불+얼음: 증발, 큰 추가 피해
	Steam,         // 불+물: 증기 구름
	Overload,      // 불+번개: 과부하 폭발, 밀쳐냄
	Combust,       // 불+독: 연소, 강한 화상
	Firestorm,     // 불+바람: 화염 폭풍 장판
	Freeze,        // 얼음+물: 빙결
	Superconduct,  // 얼음+번개: 초전도, 받는 피해 증가
	Frostbite,     // 얼음+독: 동상, 강한 둔화 + 독
	Blizzard,      // 얼음+바람: 눈보라 장판
	Electrocute,   // 물+번개: 감전, 넓은 범위 추가 피해
	Swamp,         // 물+독: 독늪 장판
	Tidal,         // 물+바람: 해일, 크게 밀쳐냄
	Neurotoxin,    // 번개+독: 신경독, 긴 기절
	Thunderstorm,  // 번개+바람: 뇌운, 주변 적에게 번개
	Plague,        // 독+바람: 역병, 독 전염
	Count
};

enum class EYJJTower : uint8
{
	Archer,
	Barracks,
	Bombard,
	Mage,
	Shrine,
	Count
};

enum class EYJJEnemy : uint8
{
	Imp,          // 꼬마 도깨비: 기본
	Chicken,      // 돌진 닭: 빠름
	Potato,       // 근육 감자: 단단함
	Toast,        // 날개 식빵: 비행
	Mushroom,     // 응원 버섯: 주변 회복
	PuddingKing,  // 대왕 푸딩: 보스
	Count
};

struct FYJJRuneDef
{
	EYJJRune Rune;
	EYJJElement Element;
	const TCHAR* Name;       // 룬 이름 (불, 얼음, 폭발...)
	const TCHAR* SpellName;  // 단독 스펠 이름 (파이어볼...)
	const TCHAR* Desc;
	FLinearColor Color;
};

struct FYJJTowerLevel
{
	int32 Cost;
	float Damage;
	float Interval;
	float Range;
	float Splash;
	float SoldierHP;
	float SoldierArmor;
};

struct FYJJTowerDef
{
	EYJJTower Type;
	const TCHAR* Name;
	const TCHAR* Desc;
	bool bMagic;
	bool bHitsAir;
	FYJJTowerLevel Levels[YJJ::MaxTowerLevel + 1];
};

struct FYJJEnemyDef
{
	EYJJEnemy Type;
	const TCHAR* Name;
	float HP;
	float Speed;
	float Armor;
	float MagicResist;
	int32 Gold;
	int32 LifeCost;
	bool bFlying;
	bool bBoss;
	float MeleeDamage;
	float MeleeInterval;
	float Radius;
	float HealPerSecond;
};

struct FYJJWaveGroup
{
	EYJJEnemy Type;
	int32 Count;
	float Interval;
	float Delay;
};

struct FYJJBuildSpot
{
	FVector2f Pos;
};
