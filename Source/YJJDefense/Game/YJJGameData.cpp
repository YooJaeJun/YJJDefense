#include "Game/YJJGameData.h"
#include "Algo/BinarySearch.h"

namespace
{
#include "Game/YJJMaps.inl"

	const FYJJRuneDef GRunes[] = {
		{ EYJJRune::Fire, EYJJElement::Fire, TEXT("불"), TEXT("파이어볼"), TEXT("맞은 적을 잠시 불태운다"), FLinearColor(1.f, 0.42f, 0.16f) },
		{ EYJJRune::Ice, EYJJElement::Ice, TEXT("얼음"), TEXT("아이스볼"), TEXT("맞은 적을 느리게 한다"), FLinearColor(0.55f, 0.88f, 1.f) },
		{ EYJJRune::Water, EYJJElement::Water, TEXT("물"), TEXT("워터볼"), TEXT("넓게 튀어 여럿을 적신다"), FLinearColor(0.25f, 0.55f, 1.f) },
		{ EYJJRune::Lightning, EYJJElement::Lightning, TEXT("번개"), TEXT("라이트닝볼"), TEXT("강하게 때리고 잠깐 기절시킨다"), FLinearColor(1.f, 0.9f, 0.25f) },
		{ EYJJRune::Poison, EYJJElement::Poison, TEXT("독"), TEXT("포이즌볼"), TEXT("오래가는 독을 건다"), FLinearColor(0.6f, 0.85f, 0.25f) },
		{ EYJJRune::Wind, EYJJElement::Wind, TEXT("바람"), TEXT("윈드볼"), TEXT("적을 뒤로 밀어낸다"), FLinearColor(0.75f, 1.f, 0.85f) },
		{ EYJJRune::Explosion, EYJJElement::None, TEXT("폭발"), TEXT("익스플로전"), TEXT("닿는 순간 넓게 터진다"), FLinearColor(1.f, 0.6f, 0.75f) },
		{ EYJJRune::Chain, EYJJElement::None, TEXT("연쇄"), TEXT("체인"), TEXT("가까운 적에게 옮겨 붙는다"), FLinearColor(0.8f, 0.65f, 1.f) },
		{ EYJJRune::Split, EYJJElement::None, TEXT("분열"), TEXT("스플릿"), TEXT("여러 발로 갈라져 나간다"), FLinearColor(1.f, 0.8f, 0.5f) },
	};

	const FYJJTowerDef GTowers[] = {
		{ EYJJTower::Archer, TEXT("궁수 초소"), TEXT("빠르게 한 놈씩 쏜다. 하늘도 맞힌다"), false, true,
			{ { 70, 13.f, 0.7f, 270.f, 0.f, 0.f, 0.f }, { 110, 21.f, 0.6f, 295.f, 0.f, 0.f, 0.f }, { 160, 32.f, 0.5f, 320.f, 0.f, 0.f, 0.f } } },
		{ EYJJTower::Barracks, TEXT("보병 막사"), TEXT("병졸 셋이 길을 막고 싸운다"), false, false,
			{ { 70, 5.f, 1.f, 0.f, 0.f, 65.f, 0.f }, { 110, 8.f, 1.f, 0.f, 0.f, 105.f, 0.15f }, { 160, 12.f, 0.9f, 0.f, 0.f, 155.f, 0.3f } } },
		{ EYJJTower::Bombard, TEXT("대포"), TEXT("느리지만 넓게 터진다. 하늘은 못 맞힌다"), false, false,
			{ { 125, 28.f, 2.8f, 290.f, 85.f, 0.f, 0.f }, { 220, 50.f, 2.8f, 310.f, 95.f, 0.f, 0.f }, { 320, 82.f, 2.6f, 330.f, 105.f, 0.f, 0.f } } },
		{ EYJJTower::Mage, TEXT("마법사 탑"), TEXT("갑옷을 무시하는 마법 화살"), true, true,
			{ { 100, 30.f, 1.5f, 250.f, 0.f, 0.f, 0.f }, { 160, 52.f, 1.5f, 270.f, 0.f, 0.f, 0.f }, { 240, 84.f, 1.4f, 290.f, 0.f, 0.f, 0.f } } },
		{ EYJJTower::Shrine, TEXT("정령 제단"), TEXT("고른 원소를 적에게 묻혀 반응을 일으킨다"), true, true,
			{ { 110, 7.f, 1.2f, 250.f, 55.f, 0.f, 0.f }, { 150, 11.f, 1.f, 265.f, 60.f, 0.f, 0.f }, { 200, 16.f, 0.8f, 280.f, 65.f, 0.f, 0.f } } },
	};

	const FYJJEnemyDef GEnemies[] = {
		//                              이름              HP     속도   방어   마저   골드 목숨 비행   보스   근접  간격  반경  회복
		{ EYJJEnemy::Imp, TEXT("꼬마 도깨비"), 55.f, 62.f, 0.f, 0.f, 6, 1, false, false, 4.f, 1.f, 18.f, 0.f },
		{ EYJJEnemy::Chicken, TEXT("돌진 닭"), 38.f, 115.f, 0.f, 0.f, 5, 1, false, false, 3.f, 0.8f, 16.f, 0.f },
		{ EYJJEnemy::Potato, TEXT("근육 감자"), 260.f, 40.f, 0.45f, 0.f, 18, 2, false, false, 14.f, 1.4f, 24.f, 0.f },
		{ EYJJEnemy::Toast, TEXT("날개 식빵"), 60.f, 85.f, 0.f, 0.f, 8, 1, true, false, 0.f, 1.f, 18.f, 0.f },
		{ EYJJEnemy::Mushroom, TEXT("응원 버섯"), 130.f, 48.f, 0.f, 0.4f, 14, 1, false, false, 3.f, 1.f, 20.f, 14.f },
		{ EYJJEnemy::PuddingKing, TEXT("대왕 푸딩"), 3200.f, 26.f, 0.3f, 0.3f, 200, 20, false, true, 40.f, 1.6f, 46.f, 0.f },
		// 해변
		{ EYJJEnemy::Crab, TEXT("꽃게 병장"), 220.f, 52.f, 0.6f, 0.f, 16, 2, false, false, 12.f, 1.2f, 24.f, 0.f },
		{ EYJJEnemy::Jellyfish, TEXT("둥실 해파리"), 85.f, 58.f, 0.f, 0.5f, 10, 1, true, false, 0.f, 1.f, 20.f, 0.f },
		{ EYJJEnemy::Coconut, TEXT("코코넛 형님"), 140.f, 56.f, 0.3f, 0.f, 10, 1, false, false, 8.f, 1.1f, 22.f, 0.f, EYJJEnemy::CocoKid, 2 },
		{ EYJJEnemy::CocoKid, TEXT("코코 알맹이"), 34.f, 100.f, 0.f, 0.f, 3, 1, false, false, 3.f, 0.8f, 15.f, 0.f },
		{ EYJJEnemy::KingCrab, TEXT("왕꽃게"), 3200.f, 24.f, 0.6f, 0.15f, 260, 20, false, true, 45.f, 1.6f, 52.f, 0.f },
		// 설산
		{ EYJJEnemy::Snowman, TEXT("눈사람"), 170.f, 46.f, 0.1f, 0.1f, 10, 1, false, false, 6.f, 1.2f, 24.f, 0.f, EYJJEnemy::Snowball, 2 },
		{ EYJJEnemy::Snowball, TEXT("눈뭉치"), 40.f, 105.f, 0.f, 0.f, 3, 1, false, false, 3.f, 0.8f, 16.f, 0.f },
		{ EYJJEnemy::Penguin, TEXT("썰매 펭귄"), 75.f, 140.f, 0.f, 0.f, 8, 1, false, false, 3.f, 0.8f, 20.f, 0.f, EYJJEnemy::Count, 0, EYJJElement::Ice },
		{ EYJJEnemy::Yeti, TEXT("예티"), 7000.f, 25.f, 0.25f, 0.25f, 300, 20, false, true, 70.f, 1.4f, 52.f, 0.f, EYJJEnemy::Count, 0, EYJJElement::Ice },
	};
	static_assert(UE_ARRAY_COUNT(GEnemies) == static_cast<int32>(EYJJEnemy::Count));

	// ---- 꾸벅 들판 ----
	const FYJJWaveGroup GMeadow1[] = { { EYJJEnemy::Imp, 8, 1.2f, 0.f } };
	const FYJJWaveGroup GMeadow2[] = { { EYJJEnemy::Imp, 10, 1.f, 0.f }, { EYJJEnemy::Chicken, 4, 0.8f, 8.f } };
	const FYJJWaveGroup GMeadow3[] = { { EYJJEnemy::Chicken, 10, 0.6f, 0.f }, { EYJJEnemy::Imp, 8, 1.f, 5.f } };
	const FYJJWaveGroup GMeadow4[] = { { EYJJEnemy::Potato, 4, 3.f, 0.f }, { EYJJEnemy::Imp, 14, 0.8f, 2.f } };
	const FYJJWaveGroup GMeadow5[] = { { EYJJEnemy::Toast, 10, 1.f, 0.f }, { EYJJEnemy::Imp, 12, 0.8f, 6.f }, { EYJJEnemy::Mushroom, 2, 5.f, 8.f } };
	const FYJJWaveGroup GMeadow6[] = { { EYJJEnemy::Potato, 6, 2.5f, 0.f }, { EYJJEnemy::Mushroom, 2, 4.f, 4.f }, { EYJJEnemy::Chicken, 16, 0.5f, 6.f } };
	const FYJJWaveGroup GMeadow7[] = { { EYJJEnemy::Toast, 14, 0.8f, 0.f }, { EYJJEnemy::Potato, 5, 2.5f, 3.f }, { EYJJEnemy::Imp, 20, 0.6f, 8.f } };
	const FYJJWaveGroup GMeadow8[] = { { EYJJEnemy::Imp, 30, 0.45f, 0.f }, { EYJJEnemy::Mushroom, 4, 3.f, 3.f }, { EYJJEnemy::Chicken, 18, 0.45f, 12.f } };
	const FYJJWaveGroup GMeadow9[] = { { EYJJEnemy::Potato, 10, 1.8f, 0.f }, { EYJJEnemy::Mushroom, 4, 3.f, 5.f }, { EYJJEnemy::Toast, 12, 0.7f, 6.f } };
	const FYJJWaveGroup GMeadow10[] = { { EYJJEnemy::Imp, 20, 0.6f, 0.f }, { EYJJEnemy::PuddingKing, 1, 1.f, 6.f }, { EYJJEnemy::Potato, 4, 2.5f, 10.f },
		{ EYJJEnemy::Toast, 8, 0.8f, 16.f }, { EYJJEnemy::Chicken, 10, 0.5f, 22.f } };
	const TConstArrayView<FYJJWaveGroup> GMeadowWaves[] = { GMeadow1, GMeadow2, GMeadow3, GMeadow4, GMeadow5, GMeadow6, GMeadow7, GMeadow8, GMeadow9, GMeadow10 };

	// ---- 꽃게 해변: 갑옷(꽃게)과 비행·마법 저항(해파리), 쪼개지는 코코넛 ----
	const FYJJWaveGroup GBeach1[] = { { EYJJEnemy::Crab, 4, 2.f, 0.f }, { EYJJEnemy::Imp, 6, 1.f, 3.f } };
	const FYJJWaveGroup GBeach2[] = { { EYJJEnemy::Coconut, 6, 1.4f, 0.f }, { EYJJEnemy::Chicken, 6, 0.7f, 4.f } };
	const FYJJWaveGroup GBeach3[] = { { EYJJEnemy::Jellyfish, 8, 1.f, 0.f }, { EYJJEnemy::Crab, 4, 2.f, 4.f } };
	const FYJJWaveGroup GBeach4[] = { { EYJJEnemy::Coconut, 10, 1.f, 0.f }, { EYJJEnemy::Crab, 4, 2.f, 6.f } };
	const FYJJWaveGroup GBeach5[] = { { EYJJEnemy::Jellyfish, 10, 0.8f, 0.f }, { EYJJEnemy::Mushroom, 2, 4.f, 4.f }, { EYJJEnemy::Coconut, 8, 1.f, 8.f } };
	const FYJJWaveGroup GBeach6[] = { { EYJJEnemy::Crab, 8, 1.6f, 0.f }, { EYJJEnemy::Jellyfish, 8, 0.8f, 6.f } };
	const FYJJWaveGroup GBeach7[] = { { EYJJEnemy::Coconut, 14, 0.7f, 0.f }, { EYJJEnemy::Potato, 4, 2.5f, 4.f }, { EYJJEnemy::Jellyfish, 8, 0.8f, 10.f } };
	const FYJJWaveGroup GBeach8[] = { { EYJJEnemy::Crab, 8, 1.4f, 0.f }, { EYJJEnemy::Mushroom, 3, 3.f, 4.f }, { EYJJEnemy::Chicken, 16, 0.45f, 10.f } };
	const FYJJWaveGroup GBeach9[] = { { EYJJEnemy::Jellyfish, 14, 0.7f, 0.f }, { EYJJEnemy::Coconut, 12, 0.8f, 5.f }, { EYJJEnemy::Crab, 6, 1.5f, 10.f } };
	const FYJJWaveGroup GBeach10[] = { { EYJJEnemy::Coconut, 8, 0.9f, 0.f }, { EYJJEnemy::KingCrab, 1, 1.f, 6.f }, { EYJJEnemy::Crab, 4, 2.f, 10.f },
		{ EYJJEnemy::Jellyfish, 10, 0.8f, 16.f } };
	const TConstArrayView<FYJJWaveGroup> GBeachWaves[] = { GBeach1, GBeach2, GBeach3, GBeach4, GBeach5, GBeach6, GBeach7, GBeach8, GBeach9, GBeach10 };

	// ---- 덜덜 설산: 녹으면 둘이 되는 눈사람, 얼음이 안 통하는 펭귄 ----
	const FYJJWaveGroup GSnow1[] = { { EYJJEnemy::Snowman, 6, 1.6f, 0.f } };
	const FYJJWaveGroup GSnow2[] = { { EYJJEnemy::Penguin, 6, 1.f, 0.f }, { EYJJEnemy::Snowman, 4, 1.6f, 4.f } };
	const FYJJWaveGroup GSnow3[] = { { EYJJEnemy::Snowman, 8, 1.2f, 0.f }, { EYJJEnemy::Imp, 10, 0.7f, 3.f } };
	const FYJJWaveGroup GSnow4[] = { { EYJJEnemy::Penguin, 10, 0.6f, 0.f }, { EYJJEnemy::Potato, 4, 2.5f, 4.f } };
	const FYJJWaveGroup GSnow5[] = { { EYJJEnemy::Toast, 10, 0.8f, 0.f }, { EYJJEnemy::Snowman, 8, 1.f, 5.f } };
	const FYJJWaveGroup GSnow6[] = { { EYJJEnemy::Penguin, 18, 0.45f, 0.f }, { EYJJEnemy::Mushroom, 3, 3.f, 3.f }, { EYJJEnemy::Snowman, 6, 1.2f, 6.f } };
	const FYJJWaveGroup GSnow7[] = { { EYJJEnemy::Snowman, 12, 0.9f, 0.f }, { EYJJEnemy::Toast, 10, 0.7f, 6.f }, { EYJJEnemy::Potato, 5, 2.f, 10.f } };
	const FYJJWaveGroup GSnow8[] = { { EYJJEnemy::Penguin, 24, 0.4f, 0.f }, { EYJJEnemy::Snowman, 12, 0.9f, 6.f } };
	const FYJJWaveGroup GSnow9[] = { { EYJJEnemy::Potato, 10, 1.5f, 0.f }, { EYJJEnemy::Mushroom, 4, 3.f, 3.f }, { EYJJEnemy::Penguin, 16, 0.45f, 8.f },
		{ EYJJEnemy::Toast, 10, 0.7f, 12.f } };
	const FYJJWaveGroup GSnow10[] = { { EYJJEnemy::Snowman, 14, 0.8f, 0.f }, { EYJJEnemy::Yeti, 1, 1.f, 6.f }, { EYJJEnemy::Penguin, 20, 0.45f, 10.f },
		{ EYJJEnemy::Snowman, 8, 1.f, 16.f } };
	const TConstArrayView<FYJJWaveGroup> GSnowWaves[] = { GSnow1, GSnow2, GSnow3, GSnow4, GSnow5, GSnow6, GSnow7, GSnow8, GSnow9, GSnow10 };

	const EYJJEnemy GMeadowShowcase[] = { EYJJEnemy::Imp, EYJJEnemy::Chicken, EYJJEnemy::Potato, EYJJEnemy::Toast, EYJJEnemy::Mushroom, EYJJEnemy::PuddingKing };
	const EYJJEnemy GBeachShowcase[] = { EYJJEnemy::Crab, EYJJEnemy::Jellyfish, EYJJEnemy::Coconut, EYJJEnemy::KingCrab };
	const EYJJEnemy GSnowShowcase[] = { EYJJEnemy::Snowman, EYJJEnemy::Penguin, EYJJEnemy::Yeti };

	struct FYJJMapDef
	{
		const TCHAR* Name;
		const TCHAR* Desc;
		TConstArrayView<TConstArrayView<FYJJWaveGroup>> Waves;
		TConstArrayView<EYJJEnemy> Showcase;
	};

	const FYJJMapDef GMaps[] = {
		{ TEXT("꾸벅 들판"), TEXT("졸린 도깨비와 닭이 길을 막는다"), GMeadowWaves, GMeadowShowcase },
		{ TEXT("꽃게 해변"), TEXT("단단한 꽃게는 마법으로, 해파리는 하늘을 노려라"), GBeachWaves, GBeachShowcase },
		{ TEXT("덜덜 설산"), TEXT("눈사람은 녹으면 둘이 된다. 펭귄은 얼지 않는다"), GSnowWaves, GSnowShowcase },
	};
	static_assert(UE_ARRAY_COUNT(GMaps) == UE_ARRAY_COUNT(GMapGeometry), "맵 데이터(GMaps)와 생성된 맵 모양(YJJMaps.inl) 개수가 다르다");

	int32 GActiveMap = 0;

	const FYJJMapGeometry& Geo()
	{
		return GMapGeometry[GActiveMap];
	}

	// 길 누적 거리 (맵마다 처음 쓸 때 한 번 계산)
	const TArray<float>& PathCumulative()
	{
		static TArray<float> Cumulative[UE_ARRAY_COUNT(GMapGeometry)];
		TArray<float>& Result = Cumulative[GActiveMap];
		if (Result.IsEmpty())
		{
			const TConstArrayView<FVector2f> Path = Geo().Path;
			Result.Reserve(Path.Num());
			float Total = 0.f;
			Result.Add(0.f);
			for (int32 Index = 1; Index < Path.Num(); ++Index)
			{
				Total += FVector2f::Distance(Path[Index - 1], Path[Index]);
				Result.Add(Total);
			}
		}
		return Result;
	}
}

namespace YJJData
{
	const FYJJRuneDef& Rune(EYJJRune Rune)
	{
		return GRunes[FMath::Clamp<int32>(static_cast<int32>(Rune), 0, UE_ARRAY_COUNT(GRunes) - 1)];
	}

	const FYJJTowerDef& Tower(EYJJTower Tower)
	{
		return GTowers[FMath::Clamp<int32>(static_cast<int32>(Tower), 0, UE_ARRAY_COUNT(GTowers) - 1)];
	}

	const FYJJEnemyDef& Enemy(EYJJEnemy Enemy)
	{
		return GEnemies[FMath::Clamp<int32>(static_cast<int32>(Enemy), 0, UE_ARRAY_COUNT(GEnemies) - 1)];
	}

	int32 NumWaves()
	{
		return GMaps[GActiveMap].Waves.Num();
	}

	TConstArrayView<FYJJWaveGroup> Wave(int32 Index)
	{
		const TConstArrayView<TConstArrayView<FYJJWaveGroup>> Waves = GMaps[GActiveMap].Waves;
		return Waves[FMath::Clamp<int32>(Index, 0, Waves.Num() - 1)];
	}

	FString EnemyTraits(EYJJEnemy Type)
	{
		const FYJJEnemyDef& Def = Enemy(Type);
		TArray<FString> Traits;
		if (Def.bBoss) { Traits.Add(TEXT("보스")); }
		if (Def.bFlying) { Traits.Add(TEXT("비행")); }
		if (Def.Speed >= 110.f) { Traits.Add(TEXT("빠름")); }
		if (Def.Armor >= 0.4f) { Traits.Add(TEXT("갑옷")); }
		if (Def.MagicResist >= 0.4f) { Traits.Add(TEXT("마법 저항")); }
		if (Def.HealPerSecond > 0.f) { Traits.Add(TEXT("회복")); }
		if (Def.HasSplit()) { Traits.Add(TEXT("쪼개짐")); }
		if (Def.Immune == EYJJElement::Ice) { Traits.Add(TEXT("둔화·빙결 면역")); }
		return FString::Join(Traits, TEXT(", "));
	}

	const TCHAR* ElementName(EYJJElement Element)
	{
		switch (Element)
		{
		case EYJJElement::Fire: return TEXT("불");
		case EYJJElement::Ice: return TEXT("얼음");
		case EYJJElement::Water: return TEXT("물");
		case EYJJElement::Lightning: return TEXT("번개");
		case EYJJElement::Poison: return TEXT("독");
		case EYJJElement::Wind: return TEXT("바람");
		default: return TEXT("비전");
		}
	}

	FLinearColor ElementColor(EYJJElement Element)
	{
		if (Element == EYJJElement::None || Element == EYJJElement::Count)
		{
			return FLinearColor(0.85f, 0.6f, 1.f);
		}
		return Rune(ElementRune(Element)).Color;
	}
}

namespace YJJMap
{
	int32 NumMaps()
	{
		return UE_ARRAY_COUNT(GMaps);
	}

	void SetActive(int32 Map)
	{
		GActiveMap = FMath::Clamp(Map, 0, NumMaps() - 1);
	}

	int32 GetActive()
	{
		return GActiveMap;
	}

	const TCHAR* Name(int32 Map)
	{
		return GMaps[FMath::Clamp(Map, 0, NumMaps() - 1)].Name;
	}

	const TCHAR* Desc(int32 Map)
	{
		return GMaps[FMath::Clamp(Map, 0, NumMaps() - 1)].Desc;
	}

	const TCHAR* Texture(int32 Map)
	{
		return GMapGeometry[FMath::Clamp(Map, 0, NumMaps() - 1)].Texture;
	}

	TConstArrayView<EYJJEnemy> Showcase(int32 Map)
	{
		return GMaps[FMath::Clamp(Map, 0, NumMaps() - 1)].Showcase;
	}

	TConstArrayView<FVector2f> Path()
	{
		return Geo().Path;
	}

	float PathLength()
	{
		return PathCumulative().Last();
	}

	FVector2f PointAt(float Dist, float Lateral, FVector2f* OutDirection)
	{
		const TArray<float>& Cumulative = PathCumulative();
		const int32 Num = Cumulative.Num();
		Dist = FMath::Clamp(Dist, 0.f, Cumulative.Last());

		// Cumulative[Index] <= Dist 인 마지막 구간
		int32 Index = Algo::UpperBound(Cumulative, Dist) - 1;
		Index = FMath::Clamp(Index, 0, Num - 2);

		const TConstArrayView<FVector2f> Path = Geo().Path;
		const FVector2f A = Path[Index];
		const FVector2f B = Path[Index + 1];
		const float SegmentLength = FMath::Max(Cumulative[Index + 1] - Cumulative[Index], KINDA_SMALL_NUMBER);
		const FVector2f Direction = (B - A) / SegmentLength;
		if (OutDirection)
		{
			*OutDirection = Direction;
		}
		const FVector2f Right(-Direction.Y, Direction.X);
		return A + Direction * (Dist - Cumulative[Index]) + Right * Lateral;
	}

	float NearestDist(const FVector2f& Pos)
	{
		const TArray<float>& Cumulative = PathCumulative();
		const TConstArrayView<FVector2f> Path = Geo().Path;
		float BestDist = 0.f;
		float BestSq = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index + 1 < Cumulative.Num(); ++Index)
		{
			const FVector2f A = Path[Index];
			const FVector2f AB = Path[Index + 1] - A;
			const float LengthSq = FMath::Max(AB.SizeSquared(), KINDA_SMALL_NUMBER);
			const float T = FMath::Clamp(FVector2f::DotProduct(Pos - A, AB) / LengthSq, 0.f, 1.f);
			const float Sq = FVector2f::DistSquared(Pos, A + AB * T);
			if (Sq < BestSq)
			{
				BestSq = Sq;
				BestDist = Cumulative[Index] + FMath::Sqrt(LengthSq) * T;
			}
		}
		return BestDist;
	}

	TConstArrayView<FVector2f> Spots()
	{
		return Geo().Spots;
	}

	FVector2f WizardPos()
	{
		return Geo().Wizard;
	}

	FVector2f GatePos()
	{
		return Geo().Gate;
	}
}
