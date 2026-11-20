#pragma once

#include "CoreMinimal.h"
#include "Game/YJJDefenseSim.h"
#include "Game/YJJSpellForge.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

/** 자동화 테스트(YJJDefenseTests.cpp)가 같이 쓰는 도우미 */
class FYJJTestUtils
{
public:
	static constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	/** 봇 한 판의 최대 길이 (초) */
	static constexpr float MaxRunSeconds = 20.f * 60.f;

	static FYJJSpellSpec Combine(EYJJRune A, EYJJRune B = EYJJRune::None);
	/** 룬·병졸을 모두 가진 판 */
	static FYJJRunConfig FullConfig();
	static void StepSeconds(FYJJDefenseSim& Sim, float Seconds);
	/** 봇이 한 판을 끝까지 플레이. 웨이브마다 남은 목숨을 글로 돌려준다 */
	static FString PlayRun(FYJJDefenseSim& Sim, const FYJJRunConfig& Config, bool bUseSpells);
	/** 스펠 공이 날아가는 동안 적이 걸어갈 만큼 앞을 노린다 */
	static FVector2f LeadTarget(const FYJJDefenseSim& Sim, int32 EnemyId);
	static int32 CountEvents(const FYJJDefenseSim& Sim, EYJJSimEvent Type, EYJJReaction Reaction = EYJJReaction::None);
	/** 디스크 0에 A+B를 꽂고 감자에게 쏜다. 감자가 받은 피해, 옆 감자가 받은 피해 */
	static float SpellDamageToPotato(EYJJRune A, EYJJRune B, float& OutNeighborDamage);
	/** Localization.csv 에 키가 있는지 */
	static void CheckLocKey(FAutomationTestBase& Test, const FString& Key);
	/** 빠진 글자([Key])가 섞여 있는지 */
	static bool HasMissingText(const FString& Text);
};

#endif
