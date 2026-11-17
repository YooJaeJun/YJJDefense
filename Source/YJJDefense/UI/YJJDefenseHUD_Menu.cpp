// 준비 화면(수집품, 들고 나갈 룬·병졸 고르기)과 결과 화면(수집 점수, 해금)

#include "UI/YJJDefenseHUD.h"
#include "Engine/Texture2D.h"
#include "Game/YJJGameData.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefenseGameMode.h"

void AYJJDefenseHUD::DrawLoadout()
{
	UYJJSaveGame* Save = Game->GetSave();
	if (!Save)
	{
		return;
	}
	using namespace YJJUI;

	DrawDimmedMap(0.3f);

	// 제목 옆에서 까딱거리는 녀석들
	const float Hop = FMath::Abs(FMath::Sin(UITime * 5.f)) * 10.f;
	DrawSpriteFeet(EnemyTextures[static_cast<int32>(EYJJEnemy::Imp)][FMath::FloorToInt(UITime * 5.f) & 1], FVector2f(560.f, 128.f - Hop), FVector2f(1.2f, 1.2f));
	DrawSpriteFeet(EnemyTextures[static_cast<int32>(EYJJEnemy::Potato)][FMath::FloorToInt(UITime * 4.f) & 1], FVector2f(1370.f, 132.f), FVector2f(1.1f, 1.1f), FLinearColor::White, true);
	DrawLabel(TEXT("YJJ 디펜스"), YJJ::ViewW * 0.5f, 70.f, 92.f, FLinearColor::White, EAlign::Center, 9.f);
	DrawLabel(TEXT("룬을 모으고, 섞고, 한 판 더!"), YJJ::ViewW * 0.5f, 136.f, 30.f, Yellow, EAlign::Center, 5.f);

	const int32 NumBagRunes = FMath::CountBits(static_cast<uint32>(Save->SelectedRunes));
	const int32 NumBagTowers = FMath::CountBits(static_cast<uint32>(Save->SelectedTowers));
	const TConstArrayView<FYJJUnlock> Track = YJJProgression::UnlockTrack();

	auto UnlockPoints = [&Track](bool bTower, int32 Index)
	{
		for (const FYJJUnlock& Unlock : Track)
		{
			if (Unlock.bTower == bTower && Unlock.Index == Index)
			{
				return Unlock.PointsNeeded;
			}
		}
		return 0;
	};

	// ---------------- 룬 수집 ----------------
	const float AX = 70.f;
	const float AY = 186.f;
	DrawPanel(AX, AY, 640.f, 600.f, WithAlpha(Cream, 0.97f));
	DrawLabel(TEXT("룬 수집"), AX + 30.f, AY + 40.f, 36.f, Ink);
	DrawLabel(FString::Printf(TEXT("가방 %d/%d"), NumBagRunes, YJJProgression::MaxBagRunes), AX + 470.f, AY + 42.f, 26.f, DarkGray);

	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		const EYJJRune Rune = static_cast<EYJJRune>(Index);
		const FYJJRuneDef& Def = YJJData::Rune(Rune);
		const bool bUnlocked = (Save->UnlockedRunes & (1 << Index)) != 0;
		const bool bSelected = (Save->SelectedRunes & (1 << Index)) != 0;
		const float X = AX + 26.f + (Index % 3) * 198.f;
		const float Y = AY + 84.f + (Index / 3) * 168.f;
		const float W = 186.f;
		const float H = 156.f;

		const bool bHover = DrawButton(X, Y, W, H, bSelected ? Hex(0xFFE98A) : (bUnlocked ? FLinearColor::White : Hex(0xCFCBDA)), EAction::ToggleRune, Index, 0, bUnlocked);
		const float Lift = (bHover && bUnlocked) ? 2.f : 0.f;
		DrawSprite(RuneTex(Rune), FVector2f(X + W * 0.5f, Y + 62.f - Lift), FVector2f(92.f, 92.f), bUnlocked ? FLinearColor::White : Hex(0x7C7888));
		if (bUnlocked)
		{
			DrawLabel(Def.SpellName, X + W * 0.5f, Y + 130.f - Lift, 24.f, Ink, EAlign::Center);
			if (bSelected)
			{
				DrawSprite(CheckIcon, FVector2f(X + W - 26.f, Y + 26.f - Lift), FVector2f(36.f, 36.f));
			}
		}
		else
		{
			DrawSprite(LockIcon, FVector2f(X + W * 0.5f, Y + 62.f), FVector2f(48.f, 48.f));
			DrawLabel(FString::Printf(TEXT("수집 %d점"), UnlockPoints(false, Index)), X + W * 0.5f, Y + 130.f, 22.f, DarkGray, EAlign::Center);
		}

		if (bHover)
		{
			const FString Kind = YJJData::IsElementRune(Rune) ? TEXT("원소 룬") : TEXT("형태 룬");
			SetTooltip(FString::Printf(TEXT("%s (%s)"), Def.SpellName, *Kind),
				FString(Def.Desc) + (bUnlocked ? (bSelected ? TEXT("\n눌러서 가방에서 빼기") : TEXT("\n눌러서 가방에 넣기")) : TEXT("\n수집 점수를 모으면 풀린다")), Ink);
		}
	}

	// ---------------- 병졸 수집 ----------------
	const float BX = 730.f;
	DrawPanel(BX, AY, 560.f, 600.f, WithAlpha(Cream, 0.97f));
	DrawLabel(TEXT("병졸 수집"), BX + 30.f, AY + 40.f, 36.f, Ink);
	DrawLabel(FString::Printf(TEXT("출전 %d/%d"), NumBagTowers, YJJProgression::MaxBagTowers), BX + 390.f, AY + 42.f, 26.f, DarkGray);

	for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count); ++Index)
	{
		const EYJJTower Type = static_cast<EYJJTower>(Index);
		const FYJJTowerDef& Def = YJJData::Tower(Type);
		const bool bUnlocked = (Save->UnlockedTowers & (1 << Index)) != 0;
		const bool bSelected = (Save->SelectedTowers & (1 << Index)) != 0;
		// 윗줄 셋, 아랫줄 둘(가운데 정렬)
		const int32 Row = Index / 3;
		const float X = BX + 22.f + (Index % 3) * 174.f + (Row == 1 ? 87.f : 0.f);
		const float Y = AY + 84.f + Row * 254.f;
		const float W = 164.f;
		const float H = 242.f;

		const bool bHover = DrawButton(X, Y, W, H, bSelected ? Hex(0xFFE98A) : (bUnlocked ? FLinearColor::White : Hex(0xCFCBDA)), EAction::ToggleTower, Index, 0, bUnlocked);
		const float Lift = (bHover && bUnlocked) ? 2.f : 0.f;
		UTexture2D* Texture = TowerTextures[Index][bUnlocked ? 1 : 0];
		DrawSprite(Texture, FVector2f(X + W * 0.5f, Y + 104.f - Lift), TexSize(Texture) * 0.5f * 1.05f, bUnlocked ? FLinearColor::White : Hex(0x7C7888));
		if (bUnlocked)
		{
			DrawLabel(Def.Name, X + W * 0.5f, Y + 208.f - Lift, 24.f, Ink, EAlign::Center);
			if (bSelected)
			{
				DrawSprite(CheckIcon, FVector2f(X + W - 26.f, Y + 26.f - Lift), FVector2f(36.f, 36.f));
			}
		}
		else
		{
			DrawSprite(LockIcon, FVector2f(X + W * 0.5f, Y + 104.f), FVector2f(54.f, 54.f));
			DrawLabel(FString::Printf(TEXT("수집 %d점"), UnlockPoints(true, Index)), X + W * 0.5f, Y + 208.f, 22.f, DarkGray, EAlign::Center);
		}

		if (bHover)
		{
			SetTooltip(Def.Name, DescribeTower(Type, 0) + (bUnlocked ? TEXT("") : TEXT("\n수집 점수를 모으면 풀린다")), Ink);
		}
	}

	// ---------------- 진행 상황 ----------------
	const float CX = 1310.f;
	const float CW = 540.f;
	DrawPanel(CX, AY, CW, 600.f, WithAlpha(Cream, 0.97f));

	// 도전 단계
	DrawLabel(TEXT("도전 단계"), CX + 30.f, AY + 40.f, 36.f, Ink);
	const bool bPrev = Save->SelectedTier > 1;
	const bool bNext = Save->SelectedTier < Save->MaxTier;
	DrawButton(CX + 40.f, AY + 78.f, 76.f, 76.f, Sky, EAction::TierPrev, 0, 0, bPrev);
	DrawSprite(PlayIcon, FVector2f(CX + 78.f, AY + 114.f), FVector2f(34.f, 34.f), FLinearColor::White, 0.f, true);
	DrawButton(CX + CW - 116.f, AY + 78.f, 76.f, 76.f, Sky, EAction::TierNext, 0, 0, bNext);
	DrawSprite(PlayIcon, FVector2f(CX + CW - 78.f, AY + 114.f), FVector2f(34.f, 34.f));
	DrawLabel(FString::Printf(TEXT("%d단계"), Save->SelectedTier), CX + CW * 0.5f, AY + 106.f, 52.f, Ink, EAlign::Center);
	DrawLabel(Save->SelectedTier == 1 ? FString(TEXT("몬스터 체력 기본")) : FString::Printf(TEXT("몬스터 체력 +%d%%"), FMath::RoundToInt((Save->SelectedTier - 1) * YJJData::TierHPStep * 100.f)),
		CX + CW * 0.5f, AY + 152.f, 22.f, DarkGray, EAlign::Center);
	if (!bNext && Save->MaxTier < YJJProgression::MaxTier)
	{
		DrawLabel(TEXT("이 단계를 깨면 다음 단계가 열린다"), CX + CW * 0.5f, AY + 182.f, 20.f, Orange, EAlign::Center);
	}

	// 수집 점수 → 다음 해금
	DrawBox(CX + 30.f, AY + 208.f, CW - 60.f, 3.f, Hex(0xE3DCC4));
	DrawLabel(FString::Printf(TEXT("수집 점수 %d"), Save->Points), CX + 30.f, AY + 244.f, 32.f, Ink);
	if (Track.IsValidIndex(Save->UnlockIndex))
	{
		const FYJJUnlock& Next = Track[Save->UnlockIndex];
		const int32 Previous = Save->UnlockIndex > 0 ? Track[Save->UnlockIndex - 1].PointsNeeded : 0;
		const float Fraction = FMath::Clamp(static_cast<float>(Save->Points - Previous) / FMath::Max(Next.PointsNeeded - Previous, 1), 0.f, 1.f);

		const float BarX0 = CX + 30.f;
		const float BarW0 = CW - 160.f;
		DrawPanel(BarX0, AY + 276.f, BarW0, 40.f, Hex(0x6E6A7C));
		if (Fraction > 0.02f)
		{
			DrawPanel(BarX0 + 5.f, AY + 281.f, FMath::Max((BarW0 - 10.f) * Fraction, 22.f), 30.f, Green, false);
		}
		DrawLabel(FString::Printf(TEXT("%d / %d"), Save->Points, Next.PointsNeeded), BarX0 + BarW0 * 0.5f, AY + 294.f, 22.f, FLinearColor::White, EAlign::Center, 4.f);

		UTexture2D* Icon = Next.bTower ? TowerTextures[Next.Index][0] : RuneTextures[Next.Index];
		const FVector2f IconSize = Next.bTower ? TexSize(Icon) * 0.32f : FVector2f(72.f, 72.f);
		DrawSprite(Icon, FVector2f(CX + CW - 76.f, AY + 290.f), IconSize);
		DrawLabel(FString::Printf(TEXT("다음 수집: %s"), YJJProgression::UnlockName(Next)), CX + 30.f, AY + 342.f, 24.f, DarkGray);
	}
	else
	{
		DrawLabel(TEXT("전부 모았다! 이제 실력으로 승부"), CX + 30.f, AY + 296.f, 24.f, Orange);
	}

	// 조합 도감
	DrawBox(CX + 30.f, AY + 374.f, CW - 60.f, 3.f, Hex(0xE3DCC4));
	const int32 Possible = YJJSpellForge::CountCombos(static_cast<uint32>(Save->UnlockedRunes));
	DrawLabel(TEXT("조합 도감"), CX + 30.f, AY + 410.f, 32.f, Ink);
	DrawLabel(FString::Printf(TEXT("%d / %d"), Save->DiscoveredCombos.Num(), Possible), CX + CW - 150.f, AY + 410.f, 36.f, Purple);
	DrawLabel(FString::Printf(TEXT("룬을 다 모으면 %d가지 · 새 조합을 쓰면 점수 +1"), YJJSpellForge::CountCombos(0x1FF)), CX + 30.f, AY + 448.f, 21.f, DarkGray);

	// 발견한 조합을 점으로
	for (int32 Index = 0; Index < YJJSpellForge::CountCombos(0x1FF); ++Index)
	{
		const float X = CX + 34.f + (Index % 15) * 32.f;
		const float Y = AY + 470.f + (Index / 15) * 28.f;
		const bool bFound = Index < Save->DiscoveredCombos.Num();
		const bool bPossible = Index < Possible;
		DrawSprite(bFound ? StarIcon : StarEmptyIcon, FVector2f(X + 12.f, Y + 12.f), FVector2f(26.f, 26.f), bPossible ? FLinearColor::White : WithAlpha(FLinearColor::White, 0.35f));
	}
	DrawLabel(FString::Printf(TEXT("플레이 %d판 · 승리 %d번"), Save->RunsPlayed, Save->Wins), CX + 30.f, AY + 572.f, 20.f, DarkGray);

	// ---------------- 출전 ----------------
	const float SW = 420.f;
	const float SH = 132.f;
	const float SX = (YJJ::ViewW - SW) * 0.5f;
	const float SY = 838.f - FMath::Abs(FMath::Sin(UITime * 3.f)) * 8.f;
	const bool bStartHover = DrawButton(SX, SY, SW, SH, Green, EAction::StartRun);
	DrawLabel(TEXT("출전!"), YJJ::ViewW * 0.5f, SY + 56.f - (bStartHover ? 2.f : 0.f), 72.f, FLinearColor::White, EAlign::Center, 8.f);
	DrawLabel(TEXT("엔터 / 스페이스"), YJJ::ViewW * 0.5f, SY + 108.f, 20.f, FLinearColor::White, EAlign::Center, 4.f);

	// 아래를 줄지어 지나가는 몬스터들
	static const EYJJEnemy Parade[] = { EYJJEnemy::Chicken, EYJJEnemy::Imp, EYJJEnemy::Mushroom, EYJJEnemy::Toast, EYJJEnemy::Imp, EYJJEnemy::PuddingKing };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Parade); ++Index)
	{
		const float X = FMath::Fmod(UITime * 60.f + Index * 170.f, 620.f);
		const float Alpha = FMath::Clamp(FMath::Min(X, 620.f - X) / 60.f, 0.f, 1.f);
		const float Scale = Parade[Index] == EYJJEnemy::PuddingKing ? 0.7f : 1.f;
		DrawSpriteFeet(EnemyTextures[static_cast<int32>(Parade[Index])][FMath::FloorToInt(UITime * 5.f + Index) & 1], FVector2f(40.f + X, 1040.f), FVector2f(Scale, Scale), WithAlpha(FLinearColor::White, Alpha));
	}
	DrawSpriteFeet(WizardTextures[FMath::FloorToInt(UITime * 1.5f) & 1], FVector2f(1640.f, 1062.f), FVector2f(1.25f, 1.25f));
	DrawSpriteFeet(SoldierTextures[FMath::FloorToInt(UITime * 4.f) % 3], FVector2f(1500.f, 1050.f), FVector2f(1.3f, 1.3f), FLinearColor::White, true);
	DrawSpriteFeet(SoldierTextures[FMath::FloorToInt(UITime * 4.f + 1.f) % 3], FVector2f(1430.f, 1056.f), FVector2f(1.3f, 1.3f), FLinearColor::White, true);
}

void AYJJDefenseHUD::DrawResult()
{
	const FYJJRunStats& Stats = Game->GetLastStats();
	const FYJJRunReward& Reward = Game->GetLastReward();
	const UYJJSaveGame* Save = Game->GetSave();
	using namespace YJJUI;

	DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, WithAlpha(Ink, 0.55f));

	const float W = 860.f;
	const float H = 860.f;
	const float X = (YJJ::ViewW - W) * 0.5f;
	const float Y = 110.f;
	DrawPanel(X, Y, W, H, Cream);
	AddHit(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, EAction::Blocker);

	const float CenterX = YJJ::ViewW * 0.5f;
	DrawLabel(Stats.bVictory ? TEXT("막아냈다!") : TEXT("성문이 뚫렸다..."), CenterX, Y + 70.f, 76.f, Stats.bVictory ? Hex(0x3E9B2B) : Red, EAlign::Center);
	DrawLabel(FString::Printf(TEXT("%d단계"), Stats.Tier), CenterX, Y + 128.f, 26.f, DarkGray, EAlign::Center);

	// 별: 하나씩 통통 튀며 나타난다
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bEarned = Index < Stats.Stars;
		const float Wobble = bEarned ? FMath::Sin(UITime * 4.f + Index) * 6.f : 0.f;
		DrawSprite(bEarned ? StarIcon : StarEmptyIcon, FVector2f(CenterX + (Index - 1) * 130.f, Y + 216.f - (Index == 1 ? 14.f : 0.f)), FVector2f(116.f, 116.f), FLinearColor::White, Wobble);
	}

	// 수집 점수 내역
	float LineY = Y + 318.f;
	auto Row = [this, X, W, &LineY](const FString& Label, int32 Points)
	{
		DrawLabel(Label, X + 120.f, LineY, 30.f, YJJUI::Ink);
		DrawLabel(FString::Printf(TEXT("+%d"), Points), X + W - 200.f, LineY, 32.f, Points > 0 ? YJJUI::Hex(0x3E9B2B) : YJJUI::Gray);
		LineY += 46.f;
	};
	Row(FString::Printf(TEXT("버틴 웨이브 %d/%d"), Stats.WavesCleared, YJJData::NumWaves()), Reward.WavePoints);
	Row(TEXT("승리"), Reward.VictoryPoints);
	Row(FString::Printf(TEXT("별 %d개"), Stats.Stars), Reward.StarPoints);
	Row(FString::Printf(TEXT("새로 발견한 조합 %d가지"), Reward.NewCombos), Reward.ComboPoints);
	DrawBox(X + 110.f, LineY - 14.f, W - 220.f, 4.f, Hex(0xE3DCC4));
	LineY += 16.f;
	DrawLabel(TEXT("수집 점수"), X + 120.f, LineY, 36.f, Ink);
	DrawLabel(FString::Printf(TEXT("+%d  (총 %d)"), Reward.TotalPoints, Save ? Save->Points : 0), X + W - 330.f, LineY, 36.f, Purple);
	LineY += 62.f;

	// 새로 얻은 것
	if (Reward.NewUnlocks.Num() > 0 || Reward.bNewTier)
	{
		const int32 NumCards = Reward.NewUnlocks.Num() + (Reward.bNewTier ? 1 : 0);
		const float CardW = 250.f;
		const float CardH = 150.f;
		float CardX = CenterX - (NumCards * CardW + (NumCards - 1) * 16.f) * 0.5f;
		const float Bounce = FMath::Abs(FMath::Sin(UITime * 4.f)) * 6.f;

		for (const FYJJUnlock& Unlock : Reward.NewUnlocks)
		{
			DrawPanel(CardX, LineY - Bounce, CardW, CardH, Hex(0xFFE98A));
			UTexture2D* Icon = Unlock.bTower ? TowerTextures[Unlock.Index][0] : RuneTextures[Unlock.Index];
			DrawSprite(Icon, FVector2f(CardX + 62.f, LineY + CardH * 0.5f - Bounce), Unlock.bTower ? TexSize(Icon) * 0.4f : FVector2f(86.f, 86.f));
			DrawLabel(TEXT("새로 얻었다!"), CardX + 118.f, LineY + 52.f - Bounce, 22.f, Orange);
			DrawLabel(YJJProgression::UnlockName(Unlock), CardX + 118.f, LineY + 90.f - Bounce, 26.f, Ink);
			CardX += CardW + 16.f;
		}
		if (Reward.bNewTier)
		{
			DrawPanel(CardX, LineY - Bounce, CardW, CardH, Sky);
			DrawLabel(TEXT("다음 단계 열림"), CardX + CardW * 0.5f, LineY + 52.f - Bounce, 24.f, FLinearColor::White, EAlign::Center, 4.f);
			DrawLabel(FString::Printf(TEXT("%d단계"), Save ? Save->MaxTier : 0), CardX + CardW * 0.5f, LineY + 96.f - Bounce, 40.f, FLinearColor::White, EAlign::Center, 5.f);
		}
	}
	else if (Save && YJJProgression::UnlockTrack().IsValidIndex(Save->UnlockIndex))
	{
		const FYJJUnlock& Next = YJJProgression::UnlockTrack()[Save->UnlockIndex];
		DrawLabel(FString::Printf(TEXT("다음 수집 \"%s\"까지 %d점"), YJJProgression::UnlockName(Next), FMath::Max(Next.PointsNeeded - Save->Points, 0)), CenterX, LineY + 60.f, 28.f, DarkGray, EAlign::Center);
	}

	const float BW = 320.f;
	const float BH = 96.f;
	const bool bHover = DrawButton(CenterX - BW * 0.5f, Y + H - BH - 28.f, BW, BH, Green, EAction::ResultContinue);
	DrawLabel(TEXT("한 판 더"), CenterX, Y + H - BH * 0.5f - 32.f - (bHover ? 2.f : 0.f), 48.f, FLinearColor::White, EAlign::Center, 6.f);
}
