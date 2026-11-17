#pragma once

#include "Game/YJJDefenseTypes.h"

// 룬·병졸·몬스터·웨이브 수치표. 밸런스는 여기만 고치면 된다
namespace YJJData
{
	YJJDEFENSE_API const FYJJRuneDef& Rune(EYJJRune Rune);
	YJJDEFENSE_API const FYJJTowerDef& Tower(EYJJTower Tower);
	YJJDEFENSE_API const FYJJEnemyDef& Enemy(EYJJEnemy Enemy);

	// 지금 맵(YJJMap::SetActive)의 웨이브
	YJJDEFENSE_API int32 NumWaves();
	YJJDEFENSE_API TConstArrayView<FYJJWaveGroup> Wave(int32 Index);
	// 몬스터 특수 능력을 짧은 말로 ("갑옷", "쪼개짐"…). 없으면 빈 문자열
	YJJDEFENSE_API FString EnemyTraits(EYJJEnemy Enemy);

	YJJDEFENSE_API const TCHAR* ElementName(EYJJElement Element);
	YJJDEFENSE_API FLinearColor ElementColor(EYJJElement Element);

	inline bool IsElementRune(EYJJRune Rune) { return static_cast<uint8>(Rune) <= static_cast<uint8>(EYJJRune::Wind); }
	inline EYJJElement RuneElement(EYJJRune Rune) { return IsElementRune(Rune) ? static_cast<EYJJElement>(static_cast<uint8>(Rune) + 1) : EYJJElement::None; }
	inline EYJJRune ElementRune(EYJJElement Element) { return Element == EYJJElement::None ? EYJJRune::None : static_cast<EYJJRune>(static_cast<uint8>(Element) - 1); }

	inline constexpr int32 StartGold = 270;
	inline constexpr int32 StartLives = 20;
	inline constexpr float WaveGap = 22.f;            // 한 웨이브가 다 나온 뒤 다음 웨이브까지
	inline constexpr float SellRefund = 0.6f;
	inline constexpr float TierHPStep = 0.3f;         // 도전 단계마다 몬스터 체력 증가
	inline constexpr float SoldierRespawn = 10.f;
	inline constexpr float SoldierSpeed = 95.f;
	inline constexpr float SoldierEngageRange = 78.f;
	inline constexpr float AuraDuration = 6.f;

	// 스펠 기본 수치
	inline constexpr float SpellBaseDamage = 46.f;
	inline constexpr float SpellBaseRadius = 48.f;
	inline constexpr float SpellBaseCooldown = 7.f;
	inline constexpr float SpellBallSpeed = 1100.f;
	inline constexpr float SocketChangeCooldown = 2.5f;

	// 마력 강화 (마법사 레벨): 스펠 피해 배율과 가격
	inline constexpr int32 MaxWizardLevel = 3;
	inline constexpr int32 WizardUpgradeCost[MaxWizardLevel] = { 140, 240, 380 };
	inline constexpr float WizardDamageScale[MaxWizardLevel + 1] = { 1.f, 1.5f, 2.1f, 2.9f };
	inline constexpr float WizardCooldownScale[MaxWizardLevel + 1] = { 1.f, 0.92f, 0.84f, 0.76f };
}

// 맵 (Tools/Art/map_data.py가 원본. 배경 그림과 같은 값으로 Game/YJJMaps.inl을 생성한다).
// 한 번에 한 판만 돌기 때문에 "지금 맵"을 전역으로 둔다. FYJJDefenseSim::Reset이 고른다
namespace YJJMap
{
	YJJDEFENSE_API int32 NumMaps();
	YJJDEFENSE_API void SetActive(int32 Map);
	YJJDEFENSE_API int32 GetActive();
	YJJDEFENSE_API const TCHAR* Name(int32 Map);
	YJJDEFENSE_API const TCHAR* Desc(int32 Map);
	YJJDEFENSE_API const TCHAR* Texture(int32 Map);
	// 이 맵에 처음 나오는 몬스터 (준비 화면 미리보기)
	YJJDEFENSE_API TConstArrayView<EYJJEnemy> Showcase(int32 Map);

	YJJDEFENSE_API TConstArrayView<FVector2f> Path();
	YJJDEFENSE_API float PathLength();
	// 길 위 Dist 지점. Lateral은 진행 방향의 오른쪽으로 벗어난 거리
	YJJDEFENSE_API FVector2f PointAt(float Dist, float Lateral = 0.f, FVector2f* OutDirection = nullptr);
	YJJDEFENSE_API float NearestDist(const FVector2f& Pos);
	YJJDEFENSE_API TConstArrayView<FVector2f> Spots();
	YJJDEFENSE_API FVector2f WizardPos();
	YJJDEFENSE_API FVector2f GatePos();
}
