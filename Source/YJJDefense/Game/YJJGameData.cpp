#include "Game/YJJGameData.h"
#include "Algo/BinarySearch.h"

namespace
{
#include "Game/YJJMap01.inl"

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
	};

	const FYJJWaveGroup GWave1[] = { { EYJJEnemy::Imp, 8, 1.2f, 0.f } };
	const FYJJWaveGroup GWave2[] = { { EYJJEnemy::Imp, 10, 1.f, 0.f }, { EYJJEnemy::Chicken, 4, 0.8f, 8.f } };
	const FYJJWaveGroup GWave3[] = { { EYJJEnemy::Chicken, 10, 0.6f, 0.f }, { EYJJEnemy::Imp, 8, 1.f, 5.f } };
	const FYJJWaveGroup GWave4[] = { { EYJJEnemy::Potato, 4, 3.f, 0.f }, { EYJJEnemy::Imp, 14, 0.8f, 2.f } };
	const FYJJWaveGroup GWave5[] = { { EYJJEnemy::Toast, 10, 1.f, 0.f }, { EYJJEnemy::Imp, 12, 0.8f, 6.f }, { EYJJEnemy::Mushroom, 2, 5.f, 8.f } };
	const FYJJWaveGroup GWave6[] = { { EYJJEnemy::Potato, 6, 2.5f, 0.f }, { EYJJEnemy::Mushroom, 2, 4.f, 4.f }, { EYJJEnemy::Chicken, 16, 0.5f, 6.f } };
	const FYJJWaveGroup GWave7[] = { { EYJJEnemy::Toast, 14, 0.8f, 0.f }, { EYJJEnemy::Potato, 5, 2.5f, 3.f }, { EYJJEnemy::Imp, 20, 0.6f, 8.f } };
	const FYJJWaveGroup GWave8[] = { { EYJJEnemy::Imp, 30, 0.45f, 0.f }, { EYJJEnemy::Mushroom, 4, 3.f, 3.f }, { EYJJEnemy::Chicken, 18, 0.45f, 12.f } };
	const FYJJWaveGroup GWave9[] = { { EYJJEnemy::Potato, 10, 1.8f, 0.f }, { EYJJEnemy::Mushroom, 4, 3.f, 5.f }, { EYJJEnemy::Toast, 12, 0.7f, 6.f } };
	const FYJJWaveGroup GWave10[] = { { EYJJEnemy::Imp, 20, 0.6f, 0.f }, { EYJJEnemy::PuddingKing, 1, 1.f, 6.f }, { EYJJEnemy::Potato, 4, 2.5f, 10.f },
		{ EYJJEnemy::Toast, 8, 0.8f, 16.f }, { EYJJEnemy::Chicken, 10, 0.5f, 22.f } };

	const TConstArrayView<FYJJWaveGroup> GWaves[] = { GWave1, GWave2, GWave3, GWave4, GWave5, GWave6, GWave7, GWave8, GWave9, GWave10 };

	// 길 누적 거리 (처음 쓸 때 한 번 계산)
	const TArray<float>& PathCumulative()
	{
		static const TArray<float> Cumulative = []
		{
			TArray<float> Result;
			Result.Reserve(UE_ARRAY_COUNT(GMap01Path));
			float Total = 0.f;
			Result.Add(0.f);
			for (int32 Index = 1; Index < UE_ARRAY_COUNT(GMap01Path); ++Index)
			{
				Total += FVector2f::Distance(GMap01Path[Index - 1], GMap01Path[Index]);
				Result.Add(Total);
			}
			return Result;
		}();
		return Cumulative;
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
		return UE_ARRAY_COUNT(GWaves);
	}

	TConstArrayView<FYJJWaveGroup> Wave(int32 Index)
	{
		return GWaves[FMath::Clamp<int32>(Index, 0, UE_ARRAY_COUNT(GWaves) - 1)];
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
	TConstArrayView<FVector2f> Path()
	{
		return GMap01Path;
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

		const FVector2f A = GMap01Path[Index];
		const FVector2f B = GMap01Path[Index + 1];
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
		float BestDist = 0.f;
		float BestSq = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index + 1 < Cumulative.Num(); ++Index)
		{
			const FVector2f A = GMap01Path[Index];
			const FVector2f AB = GMap01Path[Index + 1] - A;
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
		return GMap01Spots;
	}

	FVector2f WizardPos()
	{
		return GMap01Wizard;
	}

	FVector2f GatePos()
	{
		return GMap01Gate;
	}
}
