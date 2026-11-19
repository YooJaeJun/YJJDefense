#include "Game/YJJSpellForge.h"
#include "Data/YJJLoc.h"
#include "Game/YJJGameData.h"

FYJJSpellSpec FYJJSpellForge::Combine(TConstArrayView<EYJJRune> Runes)
{
	FYJJSpellSpec Spec;
	for (const EYJJRune Rune : Runes)
	{
		if (Rune >= EYJJRune::Count || Spec.NumRunes >= YJJ::NumSockets)
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
			else if (Spec.NumElements < YJJ::NumSockets)
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

	if (!Spec.IsValid())
	{
		return Spec;
	}

	if (Spec.NumElements == YJJ::NumSockets)
	{
		Spec.Reaction = YJJData::ReactionOf(Spec.Elements[0], Spec.Elements[1]);
	}
	if (Spec.NumRunes == YJJ::NumSockets)
	{
		Spec.ComboId = MakeComboId(Spec.Runes[0], Spec.Runes[1]);
	}
	ApplyNumbers(Spec);
	Spec.Name = MakeName(Spec);
	Spec.Description = MakeDescription(Spec);
	return Spec;
}

void FYJJSpellForge::ApplyNumbers(FYJJSpellSpec& Spec)
{
	const FYJJTuning& Tune = YJJData::Tune();
	float Damage = Tune.SpellBaseDamage;
	float Radius = Tune.SpellBaseRadius;
	float Cooldown = Tune.SpellBaseCooldown;

	// 원소별 성격 (Runes.csv)
	for (int32 Index = 0; Index < Spec.NumElements; ++Index)
	{
		const FYJJRuneDef& Def = YJJData::Rune(YJJData::ElementRune(Spec.Elements[Index]));
		Damage *= Def.SpellDamageMult;
		Radius += Def.SpellRadiusBonus;
	}

	if (Spec.NumRunes == YJJ::NumSockets)
	{
		Cooldown += Tune.ForgeTwoRuneCooldown;
	}
	if (Spec.bAmplified)
	{
		Damage *= Tune.ForgeAmplifyDamage;
		Radius *= Tune.ForgeAmplifyRadius;
	}
	if (Spec.NumElements == 0)
	{
		// 원소 효과가 없는 대신 기본 피해가 높다
		Damage *= Tune.ForgeArcaneDamage;
	}

	const bool bDoubleExplode = Spec.Explode > 1;
	const bool bDoubleChain = Spec.Chain > 1;
	const bool bDoubleSplit = Spec.Split > 1;
	if (Spec.Explode > 0)
	{
		Radius += bDoubleExplode ? Tune.ForgeExplodeRadius2 : Tune.ForgeExplodeRadius1;
		Damage *= bDoubleExplode ? Tune.ForgeExplodeDamage2 : Tune.ForgeExplodeDamage1;
		Cooldown += Tune.ForgeExplodeCooldown * Spec.Explode;
	}
	if (Spec.Chain > 0)
	{
		Spec.ChainJumps = bDoubleChain ? Tune.ForgeChainJumps2 : Tune.ForgeChainJumps1;
		Spec.ChainRadius = Spec.Explode > 0 ? Tune.ForgeChainExplodeRadius : 0.f;
		Cooldown += Tune.ForgeChainCooldown * Spec.Chain;
	}
	if (Spec.Split > 0)
	{
		Spec.Projectiles = bDoubleSplit ? Tune.ForgeSplitProjectiles2 : Tune.ForgeSplitProjectiles1;
		Damage *= bDoubleSplit ? Tune.ForgeSplitDamage2 : Tune.ForgeSplitDamage1;
		Cooldown += Tune.ForgeSplitCooldown * Spec.Split;
		if (Spec.Chain > 0)
		{
			Spec.ChainJumps = Tune.ForgeSplitChainJumps;
		}
	}

	Spec.Damage = Damage;
	Spec.Radius = Radius;
	Spec.Cooldown = Cooldown;
}

FString FYJJSpellForge::MakeName(const FYJJSpellSpec& Spec)
{
	if (Spec.NumElements == YJJ::NumSockets)
	{
		return YJJLoc::Get(YJJData::Reaction(Spec.Reaction).SpellKey);
	}
	if (Spec.NumElements == 1)
	{
		const FString& Ball = YJJLoc::Get(YJJData::Rune(YJJData::ElementRune(Spec.Elements[0])).SpellKey);
		if (Spec.bAmplified) { return YJJLoc::Format(TEXT("Spell.Amplified"), { Ball }); }
		if (Spec.Explode > 0) { return YJJLoc::Format(TEXT("Spell.Form.Explode"), { Ball }); }
		if (Spec.Chain > 0) { return YJJLoc::Format(TEXT("Spell.Form.Chain"), { Ball }); }
		if (Spec.Split > 0) { return YJJLoc::Format(TEXT("Spell.Form.Split"), { Ball }); }
		return Ball;
	}

	// 형태 룬만
	if (Spec.Explode > 1) { return YJJLoc::Get(TEXT("Spell.Pure.Explode2")); }
	if (Spec.Chain > 1) { return YJJLoc::Get(TEXT("Spell.Pure.Chain2")); }
	if (Spec.Split > 1) { return YJJLoc::Get(TEXT("Spell.Pure.Split2")); }
	if (Spec.Explode && Spec.Chain) { return YJJLoc::Get(TEXT("Spell.Pure.ExplodeChain")); }
	if (Spec.Explode && Spec.Split) { return YJJLoc::Get(TEXT("Spell.Pure.ExplodeSplit")); }
	if (Spec.Chain && Spec.Split) { return YJJLoc::Get(TEXT("Spell.Pure.ChainSplit")); }
	if (Spec.Explode) { return YJJLoc::Get(TEXT("Spell.Pure.Explode")); }
	if (Spec.Chain) { return YJJLoc::Get(TEXT("Spell.Pure.Chain")); }
	return YJJLoc::Get(TEXT("Spell.Pure.Split"));
}

FString FYJJSpellForge::MakeDescription(const FYJJSpellSpec& Spec)
{
	TArray<FString> Lines;
	for (int32 Index = 0; Index < Spec.NumElements; ++Index)
	{
		const FYJJRuneDef& Def = YJJData::Rune(YJJData::ElementRune(Spec.Elements[Index]));
		Lines.Add(YJJLoc::Format(TEXT("Spell.Desc.Element"), { YJJLoc::Get(Def.NameKey), YJJLoc::Get(Def.DescKey) }));
	}
	if (Spec.bAmplified)
	{
		Lines.Add(YJJLoc::Get(TEXT("Spell.Desc.Amplified")));
	}
	if (Spec.Reaction != EYJJReaction::None)
	{
		const FYJJReactionDef& Reaction = YJJData::Reaction(Spec.Reaction);
		Lines.Add(YJJLoc::Format(TEXT("Spell.Desc.Reaction"), { YJJLoc::Get(Reaction.NameKey), YJJLoc::Get(Reaction.DescKey) }));
	}
	if (Spec.Explode > 0)
	{
		Lines.Add(YJJLoc::Get(Spec.Explode > 1 ? TEXT("Spell.Desc.Explode2") : TEXT("Spell.Desc.Explode")));
	}
	if (Spec.Chain > 0)
	{
		Lines.Add(YJJLoc::Format(TEXT("Spell.Desc.Chain"), { Spec.ChainJumps }));
	}
	if (Spec.Split > 0)
	{
		Lines.Add(YJJLoc::Format(TEXT("Spell.Desc.Split"), { Spec.Projectiles }));
	}
	if (Spec.NumElements == 0)
	{
		Lines.Add(YJJLoc::Get(TEXT("Spell.Desc.Arcane")));
	}
	return FString::Join(Lines, TEXT("\n"));
}

int32 FYJJSpellForge::MakeComboId(EYJJRune A, EYJJRune B)
{
	const int32 Low = FMath::Min(static_cast<int32>(A), static_cast<int32>(B));
	const int32 High = FMath::Max(static_cast<int32>(A), static_cast<int32>(B));
	return Low * ComboIdStride + High;
}

int32 FYJJSpellForge::CountCombos(uint32 RuneMask)
{
	const uint32 AllRunes = (1u << static_cast<uint32>(EYJJRune::Count)) - 1u;
	const int32 Num = FMath::CountBits(static_cast<uint32>(RuneMask & AllRunes));
	// 같은 룬 둘도 조합이라 n(n+1)/2
	return Num * (Num + 1) / 2;
}

int32 FYJJSpellForge::CountAllCombos()
{
	return CountCombos((1u << static_cast<uint32>(EYJJRune::Count)) - 1u);
}
