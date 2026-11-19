#pragma once

#include "Game/YJJDefenseTypes.h"

// 디스크에 꽂은 룬들이 합쳐진 스펠 한 개
struct FYJJSpellSpec
{
	EYJJRune Runes[YJJ::NumSockets] = { EYJJRune::None, EYJJRune::None };
	int32 NumRunes = 0;

	// 서로 다른 원소 (최대 둘). 둘이면 Reaction이 정해진다
	EYJJElement Elements[YJJ::NumSockets] = { EYJJElement::None, EYJJElement::None };
	int32 NumElements = 0;
	bool bAmplified = false;  // 같은 원소 두 개

	// 형태 룬 개수
	int32 Explode = 0;
	int32 Chain = 0;
	int32 Split = 0;

	EYJJReaction Reaction = EYJJReaction::None;

	float Damage = 0.f;
	float Radius = 0.f;
	float Cooldown = 0.f;
	int32 ChainJumps = 0;
	float ChainRadius = 0.f;   // 연쇄가 옮겨 붙을 때의 폭발 반경
	int32 Projectiles = 1;

	// 도감 번호. 룬 두 개일 때만 유효 (순서 무관)
	int32 ComboId = INDEX_NONE;

	FString Name;
	FString Description;

	bool IsValid() const { return NumRunes > 0; }
	EYJJElement PrimaryElement() const { return Elements[0]; }
};

/**
 * 룬 조합 → 스펠. 규칙은 표가 아니라 합성이라 룬이 늘면 조합도 자동으로 늘어난다.
 *  - 원소 둘(다름): 두 원소 효과 + 반응 (Reactions.csv)
 *  - 원소 둘(같음): 강화
 *  - 형태(폭발/연쇄/분열): 날아가는 방식이 바뀐다. 같은 형태 둘이면 더 세게
 * 수치는 Tuning.csv 의 Forge*, Spell*, 원소별 보정은 Runes.csv.
 */
class YJJDEFENSE_API FYJJSpellForge
{
public:
	static FYJJSpellSpec Combine(TConstArrayView<EYJJRune> Runes);

	static int32 MakeComboId(EYJJRune A, EYJJRune B);
	/** 풀린 룬(비트마스크)으로 만들 수 있는 2룬 조합 수 */
	static int32 CountCombos(uint32 RuneMask);
	/** 모든 룬을 모았을 때의 조합 수 */
	static int32 CountAllCombos();

private:
	static void ApplyNumbers(FYJJSpellSpec& Spec);
	static FString MakeName(const FYJJSpellSpec& Spec);
	static FString MakeDescription(const FYJJSpellSpec& Spec);

	/** 조합 번호 = 작은 룬 x 이것 + 큰 룬 (룬 수보다 커야 한다) */
	static constexpr int32 ComboIdStride = 16;
	static_assert(ComboIdStride > static_cast<int32>(EYJJRune::Count), "ComboIdStride는 룬 수보다 커야 한다");
};
