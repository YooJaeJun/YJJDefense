#pragma once

#include "CoreMinimal.h"
#include "Game/YJJDefenseTypes.h"
#include "YJJTuning.generated.h"

/**
 * 게임 규칙의 낱개 수치. 값은 Content/YJJDefense/Data/Tuning.csv (행 이름 = 아래 변수 이름).
 * 여기에는 기본값을 두지 않는다 (CSV에 빠진 이름은 로그 경고 + 테스트 실패로 드러난다).
 */
USTRUCT()
struct FYJJTuning
{
	GENERATED_BODY()

	// ---- 판 ----
	UPROPERTY() int32 StartGold = 0;
	UPROPERTY() int32 StartLives = 0;
	UPROPERTY() float WaveGap = 0.f;
	UPROPERTY() float WaveGapWhenClear = 0.f;
	UPROPERTY() float EarlyCallMinTime = 0.f;
	UPROPERTY() float EarlyCallGoldPerSecond = 0.f;
	UPROPERTY() float SellRefund = 0.f;
	UPROPERTY() float TierHPStep = 0.f;
	UPROPERTY() float WaveHPStep = 0.f;
	UPROPERTY() int32 Star3Lives = 0;
	UPROPERTY() int32 Star2Lives = 0;

	// ---- 몬스터 ----
	UPROPERTY() float AuraDuration = 0.f;
	UPROPERTY() float EnemyLateralSpread = 0.f;
	UPROPERTY() float EnemyFirstAttackDelay = 0.f;
	UPROPERTY() float HitFlashTime = 0.f;
	UPROPERTY() float HitFlashMinDamage = 0.f;
	UPROPERTY() float HealInterval = 0.f;
	UPROPERTY() float HealRadius = 0.f;
	UPROPERTY() float BossControlScale = 0.f;
	UPROPERTY() float BossSlowScale = 0.f;
	UPROPERTY() float ImmuneTextCooldown = 0.f;
	UPROPERTY() float SplitBackStep = 0.f;
	UPROPERTY() float SplitSideSpread = 0.f;
	UPROPERTY() float SplitLateralLimit = 0.f;

	// ---- 병졸 ----
	UPROPERTY() float SoldierRespawn = 0.f;
	UPROPERTY() float SoldierSpeed = 0.f;
	UPROPERTY() float SoldierEngageRange = 0.f;
	UPROPERTY() float SoldierReleaseSlack = 0.f;
	UPROPERTY() float SoldierBlockDistance = 0.f;
	UPROPERTY() float SoldierArriveDistance = 0.f;
	UPROPERTY() float SoldierAttackDistance = 0.f;
	UPROPERTY() float SoldierHomeArrive = 0.f;
	UPROPERTY() float SoldierIdleHealDelay = 0.f;
	UPROPERTY() float SoldierIdleHealRate = 0.f;
	UPROPERTY() float SoldierFrontGap = 0.f;
	UPROPERTY() float SoldierSlotSpread = 0.f;
	UPROPERTY() float SoldierAttackAnim = 0.f;
	UPROPERTY() float SoldierSlot0X = 0.f;
	UPROPERTY() float SoldierSlot0Y = 0.f;
	UPROPERTY() float SoldierSlot1X = 0.f;
	UPROPERTY() float SoldierSlot1Y = 0.f;
	UPROPERTY() float SoldierSlot2X = 0.f;
	UPROPERTY() float SoldierSlot2Y = 0.f;

	// ---- 타워 ----
	UPROPERTY() float TowerFirstShotDelay = 0.f;
	UPROPERTY() float TowerBuildAnim = 0.f;
	UPROPERTY() float TowerFireAnim = 0.f;
	UPROPERTY() float TowerMuzzleHeight = 0.f;
	UPROPERTY() float ShrinePowerScale = 0.f;

	// ---- 스펠 ----
	UPROPERTY() float SpellBaseDamage = 0.f;
	UPROPERTY() float SpellBaseRadius = 0.f;
	UPROPERTY() float SpellBaseCooldown = 0.f;
	UPROPERTY() float SpellBallSpeed = 0.f;
	UPROPERTY() float SpellBallMinTime = 0.f;
	UPROPERTY() float SpellBallStagger = 0.f;
	UPROPERTY() float SpellBallArc = 0.f;
	UPROPERTY() float SpellBallArcStep = 0.f;
	UPROPERTY() float SocketChangeCooldown = 0.f;
	UPROPERTY() float WizardMuzzleX = 0.f;
	UPROPERTY() float WizardMuzzleY = 0.f;
	UPROPERTY() float WizardCastAnim = 0.f;
	UPROPERTY() float SplitTargetMinScale = 0.f;
	UPROPERTY() float SplitTargetMaxDistance = 0.f;
	UPROPERTY() float SplitScatterRadius = 0.f;
	UPROPERTY() float SplitScatterJitter = 0.f;
	UPROPERTY() float ChainHopDamage = 0.f;
	UPROPERTY() float ChainHopFalloff = 0.f;
	UPROPERTY() float ChainSearchRange = 0.f;

	// ---- 마력 강화 (Lv2~4) ----
	UPROPERTY() int32 WizardCost1 = 0;
	UPROPERTY() int32 WizardCost2 = 0;
	UPROPERTY() int32 WizardCost3 = 0;
	UPROPERTY() float WizardDamage1 = 0.f;
	UPROPERTY() float WizardDamage2 = 0.f;
	UPROPERTY() float WizardDamage3 = 0.f;
	UPROPERTY() float WizardCooldown1 = 0.f;
	UPROPERTY() float WizardCooldown2 = 0.f;
	UPROPERTY() float WizardCooldown3 = 0.f;

	// ---- 룬 조합 ----
	UPROPERTY() float ForgeTwoRuneCooldown = 0.f;
	UPROPERTY() float ForgeAmplifyDamage = 0.f;
	UPROPERTY() float ForgeAmplifyRadius = 0.f;
	UPROPERTY() float ForgeArcaneDamage = 0.f;
	UPROPERTY() float ForgeExplodeRadius1 = 0.f;
	UPROPERTY() float ForgeExplodeRadius2 = 0.f;
	UPROPERTY() float ForgeExplodeDamage1 = 0.f;
	UPROPERTY() float ForgeExplodeDamage2 = 0.f;
	UPROPERTY() float ForgeExplodeCooldown = 0.f;
	UPROPERTY() int32 ForgeChainJumps1 = 0;
	UPROPERTY() int32 ForgeChainJumps2 = 0;
	UPROPERTY() float ForgeChainExplodeRadius = 0.f;
	UPROPERTY() float ForgeChainCooldown = 0.f;
	UPROPERTY() int32 ForgeSplitProjectiles1 = 0;
	UPROPERTY() int32 ForgeSplitProjectiles2 = 0;
	UPROPERTY() float ForgeSplitDamage1 = 0.f;
	UPROPERTY() float ForgeSplitDamage2 = 0.f;
	UPROPERTY() float ForgeSplitCooldown = 0.f;
	UPROPERTY() int32 ForgeSplitChainJumps = 0;

	// ---- 장판 ----
	UPROPERTY() float ZoneTickInterval = 0.f;
	UPROPERTY() float ZoneSlowTime = 0.f;
	UPROPERTY() float ZoneBurnTime = 0.f;
	UPROPERTY() float ZonePoisonTime = 0.f;

	// ---- 수집 ----
	UPROPERTY() int32 MaxBagRunes = 0;
	UPROPERTY() int32 MaxBagTowers = 0;
	UPROPERTY() int32 MaxTier = 0;
	UPROPERTY() int32 WavesPerPoint = 0;
	UPROPERTY() int32 VictoryPoints = 0;
	UPROPERTY() int32 MaxComboPoints = 0;

	// ---- 흐름 ----
	UPROPERTY() float ResultDelay = 0.f;
	UPROPERTY() float MaxStepBacklog = 0.f;
	UPROPERTY() float FastSpeed = 0.f;
	UPROPERTY() int32 RunSeedBase = 0;
	UPROPERTY() float SoundMinInterval = 0.f;

	// ---- 봇 (자동 플레이·밸런스 테스트) ----
	UPROPERTY() float BotThinkInterval = 0.f;
	UPROPERTY() float BotFirstWaveDelay = 0.f;
	UPROPERTY() float BotCallWhenTimerBelow = 0.f;
	UPROPERTY() int32 BotForceBuildCount = 0;
	UPROPERTY() int32 BotWizardMinWave = 0;
	UPROPERTY() int32 BotWizardGoldReserve = 0;
	UPROPERTY() float BotSpellMinDist = 0.f;
	UPROPERTY() float BotSpellMinRadius = 0.f;
	UPROPERTY() float BotBossWeight = 0.f;
	UPROPERTY() float BotFrontWeight = 0.f;
	UPROPERTY() float BotLeadTime = 0.f;
	UPROPERTY() float BotForgeInterval = 0.f;
	UPROPERTY() float BotCoverageRange = 0.f;
	UPROPERTY() float BotCoverageStep = 0.f;
	UPROPERTY() float BotCoverageGateBias = 0.f;

	// ---- 묶음으로 읽기 ----

	/** 마력 레벨(0 = 강화 전)의 다음 강화 비용. 최고 레벨이면 0 */
	int32 WizardCost(int32 Level) const
	{
		const int32 Costs[YJJ::NumWizardUpgrades] = { WizardCost1, WizardCost2, WizardCost3 };
		return Level >= 0 && Level < YJJ::NumWizardUpgrades ? Costs[Level] : 0;
	}

	/** 마력 레벨의 스펠 피해 배율 (0 = 강화 전 = 1배) */
	float WizardDamage(int32 Level) const
	{
		const float Scales[YJJ::NumWizardUpgrades] = { WizardDamage1, WizardDamage2, WizardDamage3 };
		return Level > 0 ? Scales[FMath::Min(Level, YJJ::NumWizardUpgrades) - 1] : 1.f;
	}

	/** 마력 레벨의 재사용 배율 (0 = 강화 전 = 1배) */
	float WizardCooldown(int32 Level) const
	{
		const float Scales[YJJ::NumWizardUpgrades] = { WizardCooldown1, WizardCooldown2, WizardCooldown3 };
		return Level > 0 ? Scales[FMath::Min(Level, YJJ::NumWizardUpgrades) - 1] : 1.f;
	}

	FVector2f SoldierSlot(int32 Slot) const
	{
		const FVector2f Slots[YJJ::SoldiersPerBarracks] = {
			FVector2f(SoldierSlot0X, SoldierSlot0Y), FVector2f(SoldierSlot1X, SoldierSlot1Y), FVector2f(SoldierSlot2X, SoldierSlot2Y) };
		return Slots[FMath::Clamp(Slot, 0, YJJ::SoldiersPerBarracks - 1)];
	}

	FVector2f WizardMuzzle() const { return FVector2f(WizardMuzzleX, WizardMuzzleY); }
};
