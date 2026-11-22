#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/YJJDefenseTypes.h"
#include "YJJDataRows.generated.h"

/**
 * Content/YJJDefense/Data/*.csv 한 줄씩의 모양. 게임이 시작할 때 CSV를 엔진 DataTable로 읽는다 (YJJGameData).
 * 열 이름 = 아래 변수 이름. 첫 열(Name)은 행 이름. Note 열은 설명일 뿐 게임은 쓰지 않는다.
 * 글자는 모두 Localization.csv 키 (…Key 열).
 */

/** Tuning.csv, UILayout.csv: 이름 하나에 값 하나 */
USTRUCT()
struct FYJJNameValueRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Value = 0.f;

	UPROPERTY(EditAnywhere)
	FString Note;
};

/** Runes.csv */
USTRUCT()
struct FYJJRuneRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EYJJElement Element = EYJJElement::None;
	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString SpellKey;
	UPROPERTY(EditAnywhere) FString DescKey;
	/** 선형 RGB 16진수 */
	UPROPERTY(EditAnywhere) FString Color;
	UPROPERTY(EditAnywhere) FString Art;
	UPROPERTY(EditAnywhere) float BurnScale = 0.f;
	UPROPERTY(EditAnywhere) float BurnTime = 0.f;
	UPROPERTY(EditAnywhere) float SlowFactor = 1.f;
	UPROPERTY(EditAnywhere) float SlowTime = 0.f;
	UPROPERTY(EditAnywhere) float StunTime = 0.f;
	UPROPERTY(EditAnywhere) float PoisonScale = 0.f;
	UPROPERTY(EditAnywhere) float PoisonTime = 0.f;
	UPROPERTY(EditAnywhere) float Knockback = 0.f;
	UPROPERTY(EditAnywhere) float SpellDamageMult = 1.f;
	UPROPERTY(EditAnywhere) float SpellRadiusBonus = 0.f;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Reactions.csv */
USTRUCT()
struct FYJJReactionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EYJJElement ElementA = EYJJElement::None;
	UPROPERTY(EditAnywhere) EYJJElement ElementB = EYJJElement::None;
	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString SpellKey;
	UPROPERTY(EditAnywhere) FString DescKey;
	UPROPERTY(EditAnywhere) FString Color;
	UPROPERTY(EditAnywhere) float Radius = 0.f;
	UPROPERTY(EditAnywhere) float FxRadius = 0.f;
	UPROPERTY(EditAnywhere) float Damage = 0.f;
	UPROPERTY(EditAnywhere) float Knockback = 0.f;
	UPROPERTY(EditAnywhere) float BurnScale = 0.f;
	UPROPERTY(EditAnywhere) float BurnTime = 0.f;
	UPROPERTY(EditAnywhere) float PoisonScale = 0.f;
	UPROPERTY(EditAnywhere) float PoisonTime = 0.f;
	UPROPERTY(EditAnywhere) float SlowFactor = 1.f;
	UPROPERTY(EditAnywhere) float SlowTime = 0.f;
	UPROPERTY(EditAnywhere) float StunTime = 0.f;
	UPROPERTY(EditAnywhere) float FreezeTime = 0.f;
	UPROPERTY(EditAnywhere) float VulnMult = 1.f;
	UPROPERTY(EditAnywhere) float VulnTime = 0.f;
	UPROPERTY(EditAnywhere) int32 Targets = 0;
	UPROPERTY(EditAnywhere) float ZoneTime = 0.f;
	UPROPERTY(EditAnywhere) float ZoneDps = 0.f;
	UPROPERTY(EditAnywhere) float ZoneSlow = 1.f;
	UPROPERTY(EditAnywhere) float ZoneBurn = 0.f;
	UPROPERTY(EditAnywhere) bool ZoneGroundOnly = false;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Towers.csv. 강화 단계(1~3)마다 값을 따로 둔다 */
USTRUCT()
struct FYJJTowerRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString DescKey;
	UPROPERTY(EditAnywhere) FString Art;
	UPROPERTY(EditAnywhere) bool Magic = false;
	UPROPERTY(EditAnywhere) bool HitsAir = false;
	UPROPERTY(EditAnywhere) EYJJProjectile Projectile = EYJJProjectile::None;
	UPROPERTY(EditAnywhere) float ShotSpeed = 0.f;
	UPROPERTY(EditAnywhere) float ShotTime = 0.f;
	UPROPERTY(EditAnywhere) float ShotArcRatio = 0.f;
	UPROPERTY(EditAnywhere) float ShotArcHeight = 0.f;
	UPROPERTY(EditAnywhere) bool Homing = false;
	UPROPERTY(EditAnywhere) bool LeadTarget = false;
	UPROPERTY(EditAnywhere) int32 Cost1 = 0;
	UPROPERTY(EditAnywhere) int32 Cost2 = 0;
	UPROPERTY(EditAnywhere) int32 Cost3 = 0;
	UPROPERTY(EditAnywhere) float Damage1 = 0.f;
	UPROPERTY(EditAnywhere) float Damage2 = 0.f;
	UPROPERTY(EditAnywhere) float Damage3 = 0.f;
	UPROPERTY(EditAnywhere) float Interval1 = 1.f;
	UPROPERTY(EditAnywhere) float Interval2 = 1.f;
	UPROPERTY(EditAnywhere) float Interval3 = 1.f;
	UPROPERTY(EditAnywhere) float Range1 = 0.f;
	UPROPERTY(EditAnywhere) float Range2 = 0.f;
	UPROPERTY(EditAnywhere) float Range3 = 0.f;
	UPROPERTY(EditAnywhere) float Splash1 = 0.f;
	UPROPERTY(EditAnywhere) float Splash2 = 0.f;
	UPROPERTY(EditAnywhere) float Splash3 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierHP1 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierHP2 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierHP3 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierArmor1 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierArmor2 = 0.f;
	UPROPERTY(EditAnywhere) float SoldierArmor3 = 0.f;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Enemies.csv */
USTRUCT()
struct FYJJEnemyRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString Art;
	UPROPERTY(EditAnywhere) float HP = 0.f;
	UPROPERTY(EditAnywhere) float Speed = 0.f;
	UPROPERTY(EditAnywhere) float Armor = 0.f;
	UPROPERTY(EditAnywhere) float MagicResist = 0.f;
	UPROPERTY(EditAnywhere) int32 Gold = 0;
	UPROPERTY(EditAnywhere) int32 LifeCost = 1;
	UPROPERTY(EditAnywhere) bool Flying = false;
	UPROPERTY(EditAnywhere) bool Boss = false;
	UPROPERTY(EditAnywhere) float MeleeDamage = 0.f;
	UPROPERTY(EditAnywhere) float MeleeInterval = 1.f;
	UPROPERTY(EditAnywhere) float Radius = 0.f;
	UPROPERTY(EditAnywhere) float HealPerSecond = 0.f;
	UPROPERTY(EditAnywhere) EYJJEnemy SplitInto = EYJJEnemy::None;
	UPROPERTY(EditAnywhere) int32 SplitCount = 0;
	UPROPERTY(EditAnywhere) EYJJElement Immune = EYJJElement::None;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Waves.csv: 웨이브 하나 안의 몬스터 무리 하나 (같은 Map·Wave 줄은 CSV 순서대로) */
USTRUCT()
struct FYJJWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Maps.csv 행 이름 */
	UPROPERTY(EditAnywhere) FName Map;
	/** 1부터 */
	UPROPERTY(EditAnywhere) int32 Wave = 1;
	UPROPERTY(EditAnywhere) EYJJEnemy Enemy = EYJJEnemy::Imp;
	UPROPERTY(EditAnywhere) int32 Count = 1;
	/** 한 마리씩 나오는 간격(초) */
	UPROPERTY(EditAnywhere) float Interval = 1.f;
	/** 웨이브 시작 후 이 무리가 나오기 시작하는 때(초) */
	UPROPERTY(EditAnywhere) float Delay = 0.f;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Maps.csv */
USTRUCT()
struct FYJJMapRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) int32 Order = 1;
	UPROPERTY(EditAnywhere) FString NameKey;
	UPROPERTY(EditAnywhere) FString DescKey;
	UPROPERTY(EditAnywhere) FString Texture;
	UPROPERTY(EditAnywhere) TArray<EYJJEnemy> Showcase;
	UPROPERTY(EditAnywhere) FString Note;
};

/** MapGeometry.csv (Tools/Art/map_data.py가 배경 그림과 함께 생성한다. 직접 고치지 말 것) */
USTRUCT()
struct FYJJMapGeometryRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) TArray<FVector2D> Path;
	UPROPERTY(EditAnywhere) TArray<FVector2D> Spots;
	UPROPERTY(EditAnywhere) FVector2D Wizard = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere) FVector2D Gate = FVector2D::ZeroVector;
};

/** Unlocks.csv: 수집 트랙 (Rune이나 Tower 중 하나만) */
USTRUCT()
struct FYJJUnlockRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) int32 Order = 0;
	UPROPERTY(EditAnywhere) EYJJRune Rune = EYJJRune::None;
	UPROPERTY(EditAnywhere) EYJJTower Tower = EYJJTower::None;
	/** 0이면 처음부터 가진 것 */
	UPROPERTY(EditAnywhere) int32 Points = 0;
	UPROPERTY(EditAnywhere) FString Note;
};

/** StartDisks.csv */
USTRUCT()
struct FYJJStartDiskRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EYJJRune Rune1 = EYJJRune::None;
	UPROPERTY(EditAnywhere) EYJJRune Rune2 = EYJJRune::None;
	UPROPERTY(EditAnywhere) FString Note;
};

/** BotBuildOrder.csv */
USTRUCT()
struct FYJJBotBuildRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) EYJJTower Tower = EYJJTower::Archer;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Sounds.csv: 행 이름 = 게임 사건 이름 (사건_투사체) */
USTRUCT()
struct FYJJSoundRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FString Sound;
	UPROPERTY(EditAnywhere) float Volume = 1.f;
	UPROPERTY(EditAnywhere) FString Note;
};

/** Anims.csv: 행 이름 = 텍스처 이름 앞부분 (T_<이름>_<번호>) */
USTRUCT()
struct FYJJAnimRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) int32 Frames = 0;
	UPROPERTY(EditAnywhere) float Fps = 0.f;
	UPROPERTY(EditAnywhere) int32 HitFrame = 0;
	UPROPERTY(EditAnywhere) FString Note;
};
