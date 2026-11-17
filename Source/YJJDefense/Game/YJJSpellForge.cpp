#include "Game/YJJSpellForge.h"
#include "Game/YJJGameData.h"

namespace
{
	const FYJJReactionInfo GReactions[] = {
		{ TEXT(""), TEXT(""), TEXT(""), FLinearColor::White },
		{ TEXT("증발"), TEXT("증발탄"), TEXT("증발! 맞은 적에게 큰 추가 피해"), FLinearColor(1.f, 0.75f, 0.6f) },
		{ TEXT("증기"), TEXT("증기 폭탄"), TEXT("뜨거운 증기 구름이 남아 계속 지진다"), FLinearColor(0.9f, 0.9f, 0.95f) },
		{ TEXT("과부하"), TEXT("과부하 구체"), TEXT("펑 터지며 적을 뒤로 날린다"), FLinearColor(1.f, 0.55f, 0.8f) },
		{ TEXT("연소"), TEXT("연소탄"), TEXT("독에 불이 붙어 오래 활활 탄다"), FLinearColor(1.f, 0.6f, 0.1f) },
		{ TEXT("화염 폭풍"), TEXT("화염 폭풍"), TEXT("불 회오리가 남아 지나가는 적을 태운다"), FLinearColor(1.f, 0.45f, 0.1f) },
		{ TEXT("빙결"), TEXT("빙결탄"), TEXT("꽁꽁 얼려 꼼짝 못 하게 한다"), FLinearColor(0.6f, 0.9f, 1.f) },
		{ TEXT("초전도"), TEXT("초전도 구체"), TEXT("주변 적이 한동안 피해를 더 받는다"), FLinearColor(0.75f, 0.7f, 1.f) },
		{ TEXT("동상"), TEXT("동상 독침"), TEXT("아주 느려지고 독까지 퍼진다"), FLinearColor(0.55f, 0.9f, 0.75f) },
		{ TEXT("눈보라"), TEXT("눈보라"), TEXT("넓은 눈보라가 남아 적을 느리게 한다"), FLinearColor(0.85f, 0.95f, 1.f) },
		{ TEXT("감전"), TEXT("감전 물벼락"), TEXT("찌릿! 더 넓은 범위에 추가 피해와 기절"), FLinearColor(0.7f, 0.85f, 1.f) },
		{ TEXT("독늪"), TEXT("독늪"), TEXT("끈적한 독 웅덩이가 오래 남는다"), FLinearColor(0.5f, 0.75f, 0.3f) },
		{ TEXT("해일"), TEXT("해일"), TEXT("물살이 적을 멀리 떠밀어 보낸다"), FLinearColor(0.4f, 0.75f, 1.f) },
		{ TEXT("신경독"), TEXT("신경독 번개"), TEXT("몸이 굳어 오래 기절하고 독에 걸린다"), FLinearColor(0.85f, 0.95f, 0.3f) },
		{ TEXT("뇌운"), TEXT("뇌운"), TEXT("번개가 주변 적 넷에게 떨어진다"), FLinearColor(1.f, 0.95f, 0.5f) },
		{ TEXT("역병"), TEXT("역병 바람"), TEXT("독이 바람을 타고 넓게 옮는다"), FLinearColor(0.7f, 0.8f, 0.35f) },
	};
	static_assert(UE_ARRAY_COUNT(GReactions) == static_cast<int32>(EYJJReaction::Count));

	// [작은 원소][큰 원소] → 반응
	EYJJReaction ReactionOfPair(EYJJElement A, EYJJElement B)
	{
		using E = EYJJElement;
		using R = EYJJReaction;
		if (static_cast<uint8>(A) > static_cast<uint8>(B))
		{
			Swap(A, B);
		}
		switch (A)
		{
		case E::Fire:
			switch (B)
			{
			case E::Ice: return R::Evaporate;
			case E::Water: return R::Steam;
			case E::Lightning: return R::Overload;
			case E::Poison: return R::Combust;
			case E::Wind: return R::Firestorm;
			default: break;
			}
			break;
		case E::Ice:
			switch (B)
			{
			case E::Water: return R::Freeze;
			case E::Lightning: return R::Superconduct;
			case E::Poison: return R::Frostbite;
			case E::Wind: return R::Blizzard;
			default: break;
			}
			break;
		case E::Water:
			switch (B)
			{
			case E::Lightning: return R::Electrocute;
			case E::Poison: return R::Swamp;
			case E::Wind: return R::Tidal;
			default: break;
			}
			break;
		case E::Lightning:
			switch (B)
			{
			case E::Poison: return R::Neurotoxin;
			case E::Wind: return R::Thunderstorm;
			default: break;
			}
			break;
		case E::Poison:
			if (B == E::Wind)
			{
				return R::Plague;
			}
			break;
		default:
			break;
		}
		return R::None;
	}

	FString FormPrefix(const FYJJSpellSpec& Spec)
	{
		if (Spec.Explode > 0)
		{
			return TEXT("폭발 ");
		}
		if (Spec.Chain > 0)
		{
			return TEXT("연쇄 ");
		}
		if (Spec.Split > 0)
		{
			return TEXT("삼연발 ");
		}
		return FString();
	}

	FString MakeName(const FYJJSpellSpec& Spec)
	{
		if (Spec.NumElements == 2)
		{
			return YJJSpellForge::ReactionInfo(Spec.Reaction).SpellName;
		}
		if (Spec.NumElements == 1)
		{
			const FString Ball = YJJData::Rune(YJJData::ElementRune(Spec.Elements[0])).SpellName;
			return Spec.bAmplified ? TEXT("대형 ") + Ball : FormPrefix(Spec) + Ball;
		}

		// 형태 룬만
		if (Spec.Explode == 2) { return TEXT("대폭발"); }
		if (Spec.Chain == 2) { return TEXT("끝없는 연쇄"); }
		if (Spec.Split == 2) { return TEXT("오연발"); }
		if (Spec.Explode && Spec.Chain) { return TEXT("연쇄 폭발"); }
		if (Spec.Explode && Spec.Split) { return TEXT("산탄 폭발"); }
		if (Spec.Chain && Spec.Split) { return TEXT("갈래 연쇄"); }
		if (Spec.Explode) { return TEXT("비전 폭발"); }
		if (Spec.Chain) { return TEXT("연쇄 화살"); }
		return TEXT("삼연발");
	}

	FString MakeDescription(const FYJJSpellSpec& Spec)
	{
		TArray<FString> Lines;
		for (int32 Index = 0; Index < Spec.NumElements; ++Index)
		{
			const FYJJRuneDef& Def = YJJData::Rune(YJJData::ElementRune(Spec.Elements[Index]));
			Lines.Add(FString::Printf(TEXT("%s: %s"), Def.Name, Def.Desc));
		}
		if (Spec.bAmplified)
		{
			Lines.Add(TEXT("강화: 같은 원소 둘이라 더 크고 아프다"));
		}
		if (Spec.Reaction != EYJJReaction::None)
		{
			const FYJJReactionInfo& Info = YJJSpellForge::ReactionInfo(Spec.Reaction);
			Lines.Add(FString::Printf(TEXT("반응 [%s] %s"), Info.Name, Info.Desc));
		}
		if (Spec.Explode > 0)
		{
			Lines.Add(Spec.Explode > 1 ? TEXT("대폭발: 아주 넓게 터진다") : TEXT("폭발: 닿는 순간 넓게 터진다"));
		}
		if (Spec.Chain > 0)
		{
			Lines.Add(FString::Printf(TEXT("연쇄: 가까운 적 %d명에게 옮겨 붙는다"), Spec.ChainJumps));
		}
		if (Spec.Split > 0)
		{
			Lines.Add(FString::Printf(TEXT("분열: %d발로 갈라진다"), Spec.Projectiles));
		}
		if (Spec.NumElements == 0)
		{
			Lines.Add(TEXT("비전: 원소가 없는 순수 마법 피해"));
		}
		return FString::Join(Lines, TEXT("\n"));
	}
}

namespace YJJSpellForge
{
	FYJJSpellSpec Combine(TConstArrayView<EYJJRune> Runes)
	{
		FYJJSpellSpec Spec;
		for (const EYJJRune Rune : Runes)
		{
			if (Rune == EYJJRune::None || Rune >= EYJJRune::Count || Spec.NumRunes >= YJJ::NumSockets)
			{
				continue;
			}
			Spec.Runes[Spec.NumRunes++] = Rune;

			const EYJJElement Element = YJJData::RuneElement(Rune);
			if (Element != EYJJElement::None)
			{
				if (Spec.NumElements > 0 && Spec.Elements[0] == Element)
				{
					Spec.bAmplified = true;
				}
				else if (Spec.NumElements < 2)
				{
					Spec.Elements[Spec.NumElements++] = Element;
				}
			}
			else if (Rune == EYJJRune::Explosion)
			{
				Spec.Explode++;
			}
			else if (Rune == EYJJRune::Chain)
			{
				Spec.Chain++;
			}
			else if (Rune == EYJJRune::Split)
			{
				Spec.Split++;
			}
		}

		if (Spec.NumRunes == 0)
		{
			return Spec;
		}

		if (Spec.NumElements == 2)
		{
			Spec.Reaction = ReactionOfPair(Spec.Elements[0], Spec.Elements[1]);
		}

		float Damage = YJJData::SpellBaseDamage;
		float Radius = YJJData::SpellBaseRadius;
		float Cooldown = YJJData::SpellBaseCooldown;

		// 원소별 기본 성격
		for (int32 Index = 0; Index < Spec.NumElements; ++Index)
		{
			switch (Spec.Elements[Index])
			{
			case EYJJElement::Water: Radius += 30.f; break;
			case EYJJElement::Lightning: Damage *= 1.25f; break;
			default: break;
			}
		}

		if (Spec.NumRunes == 2)
		{
			Cooldown += 1.5f;
		}
		if (Spec.bAmplified)
		{
			Damage *= 1.8f;
			Radius *= 1.35f;
		}
		if (Spec.NumElements == 0)
		{
			// 원소 효과가 없는 대신 기본 피해가 높다
			Damage *= 1.3f;
		}

		if (Spec.Explode > 0)
		{
			Radius += Spec.Explode == 1 ? 85.f : 150.f;
			Damage *= Spec.Explode == 1 ? 1.15f : 1.5f;
			Cooldown += 1.f * Spec.Explode;
		}
		if (Spec.Chain > 0)
		{
			Spec.ChainJumps = Spec.Chain == 1 ? 3 : 7;
			Spec.ChainRadius = Spec.Explode > 0 ? 70.f : 0.f;
			Cooldown += 0.5f * Spec.Chain;
		}
		if (Spec.Split > 0)
		{
			Spec.Projectiles = Spec.Split == 1 ? 3 : 5;
			Damage *= Spec.Split == 1 ? 0.62f : 0.5f;
			Cooldown += 0.5f * Spec.Split;
			if (Spec.Chain > 0)
			{
				Spec.ChainJumps = 2;
			}
		}

		Spec.Damage = Damage;
		Spec.Radius = Radius;
		Spec.Cooldown = Cooldown;
		if (Spec.NumRunes == 2)
		{
			Spec.ComboId = MakeComboId(Spec.Runes[0], Spec.Runes[1]);
		}
		Spec.Name = MakeName(Spec);
		Spec.Description = MakeDescription(Spec);
		return Spec;
	}

	EYJJReaction GetReaction(EYJJElement A, EYJJElement B)
	{
		if (A == EYJJElement::None || B == EYJJElement::None || A == B)
		{
			return EYJJReaction::None;
		}
		return ReactionOfPair(A, B);
	}

	const FYJJReactionInfo& ReactionInfo(EYJJReaction Reaction)
	{
		return GReactions[FMath::Clamp<int32>(static_cast<int32>(Reaction), 0, UE_ARRAY_COUNT(GReactions) - 1)];
	}

	int32 MakeComboId(EYJJRune A, EYJJRune B)
	{
		const int32 Low = FMath::Min(static_cast<int32>(A), static_cast<int32>(B));
		const int32 High = FMath::Max(static_cast<int32>(A), static_cast<int32>(B));
		return Low * 16 + High;
	}

	int32 CountCombos(uint32 RuneMask)
	{
		const int32 Num = FMath::CountBits(static_cast<uint32>(RuneMask & ((1u << static_cast<int32>(EYJJRune::Count)) - 1u)));
		return Num * (Num + 1) / 2;
	}
}
