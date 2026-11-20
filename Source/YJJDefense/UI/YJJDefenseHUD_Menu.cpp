// 준비 화면(수집품, 들고 나갈 룬·병졸 고르기)과 결과 화면(수집 점수, 해금)

#include "UI/YJJDefenseHUD.h"
#include "Data/YJJLoc.h"
#include "Data/YJJTuning.h"
#include "Data/YJJUILayout.h"
#include "Engine/Texture2D.h"
#include "Game/YJJGameData.h"
#include "Game/YJJSpellForge.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefenseGameMode.h"

FVector2f AYJJDefenseHUD::LoadoutRuneCenter(int32 Index)
{
	const int32 Columns = FMath::RoundToInt(UI().RuneCardColumns);
	const float X = UI().RunePanelX + UI().CardIndentX + (Index % Columns) * UI().RuneCardStepX;
	const float Y = UI().LoadoutPanelY + UI().CardTop + (Index / Columns) * UI().RuneCardStepY;
	return FVector2f(X + UI().RuneCardW * 0.5f, Y + UI().RuneCardH * 0.5f);
}

FVector2f AYJJDefenseHUD::StartButtonCenter()
{
	return FVector2f(YJJ::ViewW * 0.5f, UI().StartButtonY + UI().StartButtonH * 0.5f);
}

int32 AYJJDefenseHUD::UnlockPoints(bool bTower, int32 Index) const
{
	for (const FYJJUnlock& Unlock : YJJData::UnlockTrack())
	{
		if (Unlock.bTower == bTower && Unlock.Index == Index)
		{
			return Unlock.PointsNeeded;
		}
	}
	return 0;
}

void AYJJDefenseHUD::DrawLoadout()
{
	UYJJSaveGame* Save = Game->GetSave();
	if (!Save)
	{
		return;
	}

	DrawDimmedMap(UI().LoadoutDim, Save->SelectedMap);

	// 제목 옆에서 까딱거리는 녀석들
	const float Hop = FMath::Abs(FMath::Sin(UITime * UI().TitleHopSpeed)) * UI().TitleHop;
	const float ImpScale = UI().TitleImpScale;
	const float PotatoScale = UI().TitlePotatoScale;
	DrawSpriteFeet(EnemyTextures[static_cast<int32>(EYJJEnemy::Imp)][FMath::FloorToInt(UITime * UI().TitleImpFrameRate) % EnemyFrames],
		FVector2f(UI().TitleImpX, UI().TitleImpY - Hop), FVector2f(ImpScale, ImpScale));
	DrawSpriteFeet(EnemyTextures[static_cast<int32>(EYJJEnemy::Potato)][FMath::FloorToInt(UITime * UI().TitlePotatoFrameRate) % EnemyFrames],
		FVector2f(UI().TitlePotatoX, UI().TitlePotatoY), FVector2f(PotatoScale, PotatoScale), FLinearColor::White, true);
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Title")), YJJ::ViewW * 0.5f, UI().TitleY, UI().TitleSize, FLinearColor::White, EAlign::Center, UI().TitleOutline);
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Subtitle")), YJJ::ViewW * 0.5f, UI().SubtitleY, UI().SubtitleSize, YJJUI::Yellow, EAlign::Center, UI().SubtitleOutline);

	DrawLoadoutRunes(*Save);
	DrawLoadoutTowers(*Save);
	DrawLoadoutProgress(*Save);

	// 출전
	const float SW = UI().StartButtonW;
	const float SH = UI().StartButtonH;
	const float SX = (YJJ::ViewW - SW) * 0.5f;
	const float SY = UI().StartButtonY - FMath::Abs(FMath::Sin(UITime * UI().StartBounceSpeed)) * UI().StartBounce;
	const bool bStartHover = DrawButton(SX, SY, SW, SH, YJJUI::Green, EAction::StartRun);
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Start")), YJJ::ViewW * 0.5f, SY + UI().StartTextY - (bStartHover ? UI().ButtonHoverLift : 0.f), UI().StartTextSize, FLinearColor::White, EAlign::Center, UI().StartTextOutline);
	DrawLabel(YJJLoc::Get(TEXT("Loadout.StartKey")), YJJ::ViewW * 0.5f, SY + UI().StartKeyY, UI().StartKeySize, FLinearColor::White, EAlign::Center, UI().ButtonTextOutline);

	DrawMapPicker(*Save);
	DrawLoadoutParade(*Save);
}

void AYJJDefenseHUD::DrawLoadoutRunes(const UYJJSaveGame& Save)
{
	const float AX = UI().RunePanelX;
	const float AY = UI().LoadoutPanelY;
	const float W = UI().RuneCardW;
	const float H = UI().RuneCardH;
	const float Icon = UI().RuneCardIcon;
	const float Lock = UI().RuneCardLock;
	const float Check = UI().CardCheckSize;
	const float CheckInset = UI().CardCheckInset;
	const float IconY = UI().RuneCardIconY;
	const float NameY = UI().RuneCardNameY;

	DrawPanel(AX, AY, UI().RunePanelW, UI().LoadoutPanelH, YJJUI::WithAlpha(YJJUI::Cream, UI().PanelAlpha));
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Runes")), AX + UI().PanelTitleX, AY + UI().PanelTitleY, UI().PanelTitleSize, YJJUI::Ink);
	DrawLabel(YJJLoc::Format(TEXT("Loadout.RuneBag"), { FMath::CountBits(static_cast<uint32>(Save.SelectedRunes)), YJJData::Tune().MaxBagRunes }),
		AX + UI().RuneCountX, AY + UI().PanelCountY, UI().PanelCountSize, YJJUI::DarkGray);

	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		const EYJJRune Rune = static_cast<EYJJRune>(Index);
		const FYJJRuneDef& Def = YJJData::Rune(Rune);
		const bool bUnlocked = (Save.UnlockedRunes & (1 << Index)) != 0;
		const bool bSelected = (Save.SelectedRunes & (1 << Index)) != 0;
		const FVector2f Center = LoadoutRuneCenter(Index);
		const float X = Center.X - W * 0.5f;
		const float Y = Center.Y - H * 0.5f;

		const bool bHover = DrawButton(X, Y, W, H, bSelected ? YJJUI::CardPicked : (bUnlocked ? FLinearColor::White : YJJUI::CardLocked), EAction::ToggleRune, Index, 0, bUnlocked);
		const float Lift = (bHover && bUnlocked) ? UI().ButtonHoverLift : 0.f;
		DrawSprite(RuneTex(Rune), FVector2f(Center.X, Y + IconY - Lift), FVector2f(Icon, Icon), bUnlocked ? FLinearColor::White : YJJUI::IconLocked);
		if (bUnlocked)
		{
			DrawLabel(YJJLoc::Get(Def.SpellKey), Center.X, Y + NameY - Lift, UI().CardNameSize, YJJUI::Ink, EAlign::Center, 0.f, W - UI().TextFitPad);
			if (bSelected)
			{
				DrawSprite(CheckIcon, FVector2f(X + W - CheckInset, Y + CheckInset - Lift), FVector2f(Check, Check));
			}
		}
		else
		{
			DrawSprite(LockIcon, FVector2f(Center.X, Y + IconY), FVector2f(Lock, Lock));
			DrawLabel(YJJLoc::Format(TEXT("Loadout.UnlockAt"), { UnlockPoints(false, Index) }), Center.X, Y + NameY, UI().CardLockedSize, YJJUI::DarkGray, EAlign::Center);
		}

		if (bHover)
		{
			const FString Kind = YJJData::IsElementRune(Rune) ? YJJLoc::Get(TEXT("Loadout.ElementRune")) : YJJLoc::Get(TEXT("Loadout.FormRune"));
			const FString Hint = !bUnlocked ? YJJLoc::Get(TEXT("Loadout.LockedHint"))
				: (bSelected ? YJJLoc::Get(TEXT("Loadout.RemoveHint")) : YJJLoc::Get(TEXT("Loadout.AddHint")));
			SetTooltip(YJJLoc::Format(TEXT("Loadout.RuneTip"), { YJJLoc::Get(Def.SpellKey), Kind }), YJJLoc::Get(Def.DescKey) + TEXT("\n") + Hint, YJJUI::Ink);
		}
	}
}

void AYJJDefenseHUD::DrawLoadoutTowers(const UYJJSaveGame& Save)
{
	const float BX = UI().TowerPanelX;
	const float AY = UI().LoadoutPanelY;
	const float W = UI().TowerCardW;
	const float H = UI().TowerCardH;
	const int32 Columns = FMath::RoundToInt(UI().TowerCardColumns);
	const float Lock = UI().TowerCardLock;
	const float Check = UI().CardCheckSize;
	const float CheckInset = UI().CardCheckInset;
	const float IconY = UI().TowerCardIconY;
	const float NameY = UI().TowerCardNameY;

	DrawPanel(BX, AY, UI().TowerPanelW, UI().LoadoutPanelH, YJJUI::WithAlpha(YJJUI::Cream, UI().PanelAlpha));
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Towers")), BX + UI().PanelTitleX, AY + UI().PanelTitleY, UI().PanelTitleSize, YJJUI::Ink);
	DrawLabel(YJJLoc::Format(TEXT("Loadout.TowerBag"), { FMath::CountBits(static_cast<uint32>(Save.SelectedTowers)), YJJData::Tune().MaxBagTowers }),
		BX + UI().TowerCountX, AY + UI().PanelCountY, UI().PanelCountSize, YJJUI::DarkGray);

	for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count); ++Index)
	{
		const EYJJTower Type = static_cast<EYJJTower>(Index);
		const FYJJTowerDef& Def = YJJData::Tower(Type);
		const bool bUnlocked = (Save.UnlockedTowers & (1 << Index)) != 0;
		const bool bSelected = (Save.SelectedTowers & (1 << Index)) != 0;
		// 윗줄 셋, 아랫줄 둘(가운데 정렬)
		const int32 Row = Index / Columns;
		const float X = BX + UI().TowerCardIndentX + (Index % Columns) * UI().TowerCardStepX + (Row > 0 ? UI().TowerSecondRowShift : 0.f);
		const float Y = AY + UI().CardTop + Row * UI().TowerCardStepY;

		const bool bHover = DrawButton(X, Y, W, H, bSelected ? YJJUI::CardPicked : (bUnlocked ? FLinearColor::White : YJJUI::CardLocked), EAction::ToggleTower, Index, 0, bUnlocked);
		const float Lift = (bHover && bUnlocked) ? UI().ButtonHoverLift : 0.f;
		UTexture2D* Texture = TowerTextures[Index][bUnlocked ? 1 : 0];
		DrawSprite(Texture, FVector2f(X + W * 0.5f, Y + IconY - Lift), TexSize(Texture) * UI().TowerCardArtScale, bUnlocked ? FLinearColor::White : YJJUI::IconLocked);
		if (bUnlocked)
		{
			DrawLabel(YJJLoc::Get(Def.NameKey), X + W * 0.5f, Y + NameY - Lift, UI().CardNameSize, YJJUI::Ink, EAlign::Center, 0.f, W - UI().TextFitPad);
			if (bSelected)
			{
				DrawSprite(CheckIcon, FVector2f(X + W - CheckInset, Y + CheckInset - Lift), FVector2f(Check, Check));
			}
		}
		else
		{
			DrawSprite(LockIcon, FVector2f(X + W * 0.5f, Y + IconY), FVector2f(Lock, Lock));
			DrawLabel(YJJLoc::Format(TEXT("Loadout.UnlockAt"), { UnlockPoints(true, Index) }), X + W * 0.5f, Y + NameY, UI().CardLockedSize, YJJUI::DarkGray, EAlign::Center);
		}

		if (bHover)
		{
			FString Body = DescribeTower(Type, 0);
			if (!bUnlocked)
			{
				Body += TEXT("\n");
				Body += YJJLoc::Get(TEXT("Loadout.LockedHint"));
			}
			SetTooltip(YJJLoc::Get(Def.NameKey), Body, YJJUI::Ink);
		}
	}
}

void AYJJDefenseHUD::DrawLoadoutProgress(const UYJJSaveGame& Save)
{
	const FYJJTuning& Tune = YJJData::Tune();
	const TConstArrayView<FYJJUnlock> Track = YJJData::UnlockTrack();
	const float CX = UI().ProgressPanelX;
	const float CW = UI().ProgressPanelW;
	const float AY = UI().LoadoutPanelY;
	const float TextX = CX + UI().PanelTitleX;
	const float Inner = CW - UI().PanelTitleX * 2.f;
	DrawPanel(CX, AY, CW, UI().LoadoutPanelH, YJJUI::WithAlpha(YJJUI::Cream, UI().PanelAlpha));

	// 도전 단계
	DrawLabel(YJJLoc::Get(TEXT("Loadout.Tier")), TextX, AY + UI().PanelTitleY, UI().PanelTitleSize, YJJUI::Ink);
	const bool bPrev = Save.SelectedTier > 1;
	const bool bNext = Save.SelectedTier < Save.MaxTier;
	const float Arrow = UI().TierArrowSize;
	const float ArrowIcon = UI().TierArrowIcon;
	const float ArrowY = AY + UI().TierArrowY;
	const float ArrowInset = UI().TierArrowInset;
	DrawButton(CX + ArrowInset, ArrowY, Arrow, Arrow, YJJUI::Sky, EAction::TierPrev, 0, 0, bPrev);
	DrawSprite(PlayIcon, FVector2f(CX + ArrowInset + Arrow * 0.5f, ArrowY + Arrow * 0.5f), FVector2f(ArrowIcon, ArrowIcon), FLinearColor::White, 0.f, true);
	DrawButton(CX + CW - ArrowInset - Arrow, ArrowY, Arrow, Arrow, YJJUI::Sky, EAction::TierNext, 0, 0, bNext);
	DrawSprite(PlayIcon, FVector2f(CX + CW - ArrowInset - Arrow * 0.5f, ArrowY + Arrow * 0.5f), FVector2f(ArrowIcon, ArrowIcon));
	DrawLabel(YJJLoc::Format(TEXT("Loadout.TierValue"), { Save.SelectedTier }), CX + CW * 0.5f, AY + UI().TierValueY, UI().TierValueSize, YJJUI::Ink, EAlign::Center);
	DrawLabel(Save.SelectedTier == 1 ? YJJLoc::Get(TEXT("Loadout.TierBase"))
		: YJJLoc::Format(TEXT("Loadout.TierHP"), { Percent((Save.SelectedTier - 1) * Tune.TierHPStep) }),
		CX + CW * 0.5f, AY + UI().TierHPY, UI().TierHPSize, YJJUI::DarkGray, EAlign::Center);
	if (!bNext && Save.MaxTier < Tune.MaxTier)
	{
		DrawLabel(YJJLoc::Get(TEXT("Loadout.TierHint")), CX + CW * 0.5f, AY + UI().TierHintY, UI().TierHintSize, YJJUI::Orange, EAlign::Center);
	}

	// 수집 점수 → 다음 해금
	const float DividerH = UI().DividerH;
	DrawBox(TextX, AY + UI().Divider1Y, Inner, DividerH, YJJUI::Divider);
	DrawLabel(YJJLoc::Format(TEXT("Loadout.Points"), { Save.Points }), TextX, AY + UI().PointsY, UI().PointsSize, YJJUI::Ink);
	if (Track.IsValidIndex(Save.UnlockIndex))
	{
		const FYJJUnlock& Next = Track[Save.UnlockIndex];
		const int32 Previous = Save.UnlockIndex > 0 ? Track[Save.UnlockIndex - 1].PointsNeeded : 0;
		const float Fraction = FMath::Clamp(static_cast<float>(Save.Points - Previous) / FMath::Max(Next.PointsNeeded - Previous, 1), 0.f, 1.f);

		const float BarW = CW - UI().ProgressBarRight;
		const float BarY = AY + UI().ProgressBarY;
		const float BarH = UI().ProgressBarH;
		const float Pad = UI().ProgressBarPad;
		DrawPanel(TextX, BarY, BarW, BarH, YJJUI::DarkGray);
		if (Fraction > UI().ProgressBarMinFraction)
		{
			DrawPanel(TextX + Pad, BarY + Pad, FMath::Max((BarW - Pad * 2.f) * Fraction, UI().ProgressBarMinFill), BarH - Pad * 2.f, YJJUI::Green, false);
		}
		DrawLabel(YJJLoc::Format(TEXT("Loadout.PointsOf"), { Save.Points, Next.PointsNeeded }), TextX + BarW * 0.5f, BarY + BarH * 0.5f - UI().ProgressTextUp, UI().ProgressTextSize, FLinearColor::White, EAlign::Center, UI().ButtonTextOutline);

		UTexture2D* Icon = Next.bTower ? TowerTextures[Next.Index][0] : RuneTextures[Next.Index];
		const float RuneIcon = UI().NextRuneIcon;
		const FVector2f IconSize = Next.bTower ? TexSize(Icon) * UI().NextTowerScale : FVector2f(RuneIcon, RuneIcon);
		DrawSprite(Icon, FVector2f(CX + CW - UI().NextIconRight, AY + UI().NextIconY), IconSize);
		DrawLabel(YJJLoc::Format(TEXT("Loadout.NextUnlock"), { FYJJProgression::UnlockName(Next) }), TextX, AY + UI().NextUnlockY, UI().NextUnlockSize, YJJUI::DarkGray);
	}
	else
	{
		DrawLabel(YJJLoc::Get(TEXT("Loadout.AllUnlocked")), TextX, AY + UI().AllUnlockedY, UI().NextUnlockSize, YJJUI::Orange);
	}

	// 조합 도감
	DrawBox(TextX, AY + UI().Divider2Y, Inner, DividerH, YJJUI::Divider);
	const int32 Possible = FYJJSpellForge::CountCombos(static_cast<uint32>(Save.UnlockedRunes));
	const int32 All = FYJJSpellForge::CountAllCombos();
	DrawLabel(YJJLoc::Get(TEXT("Loadout.ComboBook")), TextX, AY + UI().ComboTitleY, UI().PointsSize, YJJUI::Ink);
	DrawLabel(YJJLoc::Format(TEXT("Loadout.PointsOf"), { Save.DiscoveredCombos.Num(), Possible }), CX + CW - UI().ComboCountRight, AY + UI().ComboTitleY, UI().ComboCountSize, YJJUI::Purple);
	DrawLabel(YJJLoc::Format(TEXT("Loadout.ComboHint"), { All }),
		TextX, AY + UI().ComboHintY, UI().ComboHintSize, YJJUI::DarkGray);

	// 발견한 조합을 별로
	const int32 PerRow = FMath::RoundToInt(UI().ComboStarsPerRow);
	const float Star = UI().ComboStarSize;
	const float StarHalf = UI().ComboStarHalf;
	for (int32 Index = 0; Index < All; ++Index)
	{
		const float X = CX + UI().ComboStarsX + (Index % PerRow) * UI().ComboStarStepX;
		const float Y = AY + UI().ComboStarsY + (Index / PerRow) * UI().ComboStarStepY;
		const bool bFound = Index < Save.DiscoveredCombos.Num();
		const bool bPossible = Index < Possible;
		DrawSprite(bFound ? StarIcon : StarEmptyIcon, FVector2f(X + StarHalf, Y + StarHalf), FVector2f(Star, Star),
			bPossible ? FLinearColor::White : YJJUI::WithAlpha(FLinearColor::White, UI().ComboStarLockedAlpha));
	}
	DrawLabel(YJJLoc::Format(TEXT("Loadout.Record"), { Save.RunsPlayed, Save.Wins }), TextX, AY + UI().RecordY, UI().RecordSize, YJJUI::DarkGray);
}

void AYJJDefenseHUD::DrawLoadoutParade(const UYJJSaveGame& Save)
{
	// 아래를 줄지어 지나가는 몬스터들 (고른 지도에 나오는 녀석들)
	const TArray<EYJJEnemy>& Parade = YJJMap::Def(Save.SelectedMap).Showcase;
	const float Width = UI().ParadeWidth;
	const float Fade = UI().ParadeFade;
	for (int32 Index = 0; Index < Parade.Num(); ++Index)
	{
		const float X = FMath::Fmod(UITime * UI().ParadeSpeed + Index * (Width / Parade.Num()), Width);
		const float Alpha = FMath::Clamp(FMath::Min(X, Width - X) / Fade, 0.f, 1.f);
		const float Scale = YJJData::Enemy(Parade[Index]).bBoss ? UI().ParadeBossScale : UI().ParadeScale;
		DrawSpriteFeet(EnemyTextures[static_cast<int32>(Parade[Index])][FMath::FloorToInt(UITime * UI().ParadeFrameRate + Index) % EnemyFrames],
			FVector2f(UI().ParadeX + X, UI().ParadeY), FVector2f(Scale, Scale), YJJUI::WithAlpha(FLinearColor::White, Alpha));
	}

	const float WizardScale = UI().ParadeWizardScale;
	const float SoldierScale = UI().ParadeSoldierScale;
	const float SoldierRate = UI().ParadeSoldierFrameRate;
	DrawSpriteFeet(WizardTextures[FMath::FloorToInt(UITime * UI().ParadeWizardFrameRate) % WizardFrames], FVector2f(UI().ParadeWizardX, UI().ParadeWizardY), FVector2f(WizardScale, WizardScale));
	DrawSpriteFeet(SoldierTextures[FMath::FloorToInt(UITime * SoldierRate) % SoldierFrames], FVector2f(UI().ParadeSoldier1X, UI().ParadeSoldier1Y), FVector2f(SoldierScale, SoldierScale), FLinearColor::White, true);
	DrawSpriteFeet(SoldierTextures[FMath::FloorToInt(UITime * SoldierRate + 1.f) % SoldierFrames], FVector2f(UI().ParadeSoldier2X, UI().ParadeSoldier2Y), FVector2f(SoldierScale, SoldierScale), FLinearColor::White, true);
}

void AYJJDefenseHUD::DrawMapPicker(UYJJSaveGame& Save)
{
	// 출전 버튼 왼쪽: [<] 지도 그림 [>]
	const float X = UI().MapPickerX;
	const float Y = UI().MapPickerY;
	const float W = UI().MapPickerW;
	const float H = UI().MapPickerH;
	DrawPanel(X, Y, W, H, YJJUI::WithAlpha(YJJUI::Cream, UI().PanelAlpha));
	AddHit(X, Y, W, H, EAction::Blocker);

	const int32 Map = Save.SelectedMap;
	const bool bPrev = Map > 0;
	const bool bNext = Map + 1 < Save.UnlockedMaps;
	const bool bNextLocked = Map + 1 < YJJMap::NumMaps() && !bNext;

	const float ArrowW = UI().MapArrowW;
	const float ArrowH = UI().MapArrowH;
	const float ArrowInset = UI().MapArrowInset;
	const float ArrowY = Y + UI().MapArrowY;
	const float ArrowIcon = UI().MapArrowIcon;
	const float Lift = UI().ButtonHoverLift;
	const bool bPrevHover = DrawButton(X + ArrowInset, ArrowY, ArrowW, ArrowH, YJJUI::Sky, EAction::MapPrev, 0, 0, bPrev);
	DrawSprite(PlayIcon, FVector2f(X + ArrowInset + ArrowW * 0.5f, ArrowY + ArrowH * 0.5f - (bPrevHover && bPrev ? Lift : 0.f)), FVector2f(ArrowIcon, ArrowIcon), FLinearColor::White, 0.f, true);
	const bool bNextHover = DrawButton(X + W - ArrowInset - ArrowW, ArrowY, ArrowW, ArrowH, YJJUI::Sky, EAction::MapNext, 0, 0, bNext);
	DrawSprite(bNextLocked ? LockIcon : PlayIcon, FVector2f(X + W - ArrowInset - ArrowW * 0.5f, ArrowY + ArrowH * 0.5f - (bNextHover && bNext ? Lift : 0.f)), FVector2f(ArrowIcon, ArrowIcon));

	// 지도 미리보기 + 이름
	const float TW = UI().MapThumbW;
	const float TH = UI().MapThumbH;
	const float TX = X + (W - TW) * 0.5f;
	const float TY = Y + UI().MapThumbY;
	const float Frame = UI().MapThumbFrame;
	DrawPanel(TX - Frame, TY - Frame, TW + Frame * 2.f, TH + Frame * 2.f, YJJUI::Ink, false);
	DrawSprite(MapThumb(Map), FVector2f(TX + TW * 0.5f, TY + TH * 0.5f), FVector2f(TW, TH));
	DrawLabel(YJJLoc::Format(TEXT("Loadout.MapName"), { Map + 1, YJJMap::Name(Map) }), X + W * 0.5f, Y + H - UI().MapNameUp, UI().MapNameSize, YJJUI::Ink, EAlign::Center, 0.f, W - UI().TextFitPad);

	if (IsHover(TX, TY, TW, TH))
	{
		TArray<FString> Lines;
		Lines.Add(YJJMap::Desc(Map));
		for (const EYJJEnemy Enemy : YJJMap::Def(Map).Showcase)
		{
			Lines.Add(DescribeEnemy(Enemy));
		}
		SetTooltip(YJJMap::Name(Map), FString::Join(Lines, TEXT("\n")), YJJUI::Ink);
	}
	else if (bNextHover && bNextLocked)
	{
		SetTooltip(YJJLoc::Format(TEXT("Loadout.MapLocked"), { YJJMap::Name(Map + 1) }), YJJLoc::Get(TEXT("Loadout.MapLockedHint")), YJJUI::Ink);
	}
}

// ---------------------------------------------------------------------------------------------
// 결과

float AYJJDefenseHUD::DrawResultRows(float LineY)
{
	const FYJJRunStats& Stats = Game->GetLastStats();
	const FYJJRunReward& Reward = Game->GetLastReward();
	const UYJJSaveGame* Save = Game->GetSave();
	const float X = (YJJ::ViewW - UI().ResultW) * 0.5f;
	const float W = UI().ResultW;
	const float LabelX = X + UI().ResultRowX;
	const float RowStep = UI().ResultRowStep;

	const FString Labels[] = {
		YJJLoc::Format(TEXT("Result.Waves"), { Stats.WavesCleared, YJJData::NumWaves() }),
		YJJLoc::Get(TEXT("Result.Victory")),
		YJJLoc::Format(TEXT("Result.Stars"), { Stats.Stars }),
		YJJLoc::Format(TEXT("Result.Combos"), { Reward.NewCombos }),
	};
	const int32 Points[] = { Reward.WavePoints, Reward.VictoryPoints, Reward.StarPoints, Reward.ComboPoints };
	constexpr int32 NumRows = UE_ARRAY_COUNT(Points);
	static_assert(UE_ARRAY_COUNT(Labels) == NumRows, "결과 줄 수가 맞아야 한다");

	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		DrawLabel(Labels[Row], LabelX, LineY, UI().ResultRowSize, YJJUI::Ink);
		DrawLabel(YJJLoc::Format(TEXT("Fx.Gold"), { Points[Row] }), X + W - UI().ResultPointsRight, LineY, UI().ResultPointsSize, Points[Row] > 0 ? YJJUI::Gain : YJJUI::Gray);
		LineY += RowStep;
	}

	const float Inset = UI().ResultDividerInset;
	DrawBox(X + Inset, LineY - UI().ResultDividerUp, W - Inset * 2.f, UI().ResultDividerH, YJJUI::Divider);
	LineY += UI().ResultTotalGap;
	DrawLabel(YJJLoc::Get(TEXT("Result.Total")), LabelX, LineY, UI().ResultTotalSize, YJJUI::Ink);
	DrawLabel(YJJLoc::Format(TEXT("Result.TotalValue"), { Reward.TotalPoints, Save ? Save->Points : 0 }), X + W - UI().ResultTotalRight, LineY, UI().ResultTotalSize, YJJUI::Purple);
	return LineY + UI().ResultCardsGap;
}

void AYJJDefenseHUD::DrawResultCards(float LineY)
{
	const FYJJRunReward& Reward = Game->GetLastReward();
	const UYJJSaveGame* Save = Game->GetSave();
	const float CenterX = YJJ::ViewW * 0.5f;
	const bool bNewMap = Reward.NewMap != INDEX_NONE;

	if (Reward.NewUnlocks.Num() == 0 && !Reward.bNewTier && !bNewMap)
	{
		const TConstArrayView<FYJJUnlock> Track = YJJData::UnlockTrack();
		if (Save && Track.IsValidIndex(Save->UnlockIndex))
		{
			const FYJJUnlock& Next = Track[Save->UnlockIndex];
			DrawLabel(YJJLoc::Format(TEXT("Result.NextUnlock"), { FYJJProgression::UnlockName(Next), FMath::Max(Next.PointsNeeded - Save->Points, 0) }),
				CenterX, LineY + UI().ResultNextY, UI().ResultNextSize, YJJUI::DarkGray, EAlign::Center);
		}
		return;
	}

	// 새로 얻은 것
	const int32 NumCards = Reward.NewUnlocks.Num() + (Reward.bNewTier ? 1 : 0) + (bNewMap ? 1 : 0);
	const float CardW = UI().ResultCardW;
	const float CardH = UI().ResultCardH;
	const float Gap = UI().ResultCardGap;
	const float Outline = UI().ButtonTextOutline;
	float CardX = CenterX - (NumCards * CardW + (NumCards - 1) * Gap) * 0.5f;
	const float Y = LineY - FMath::Abs(FMath::Sin(UITime * UI().ResultCardBounceSpeed)) * UI().ResultCardBounce;

	for (const FYJJUnlock& Unlock : Reward.NewUnlocks)
	{
		DrawPanel(CardX, Y, CardW, CardH, YJJUI::CardPicked);
		UTexture2D* Icon = Unlock.bTower ? TowerTextures[Unlock.Index][0] : RuneTextures[Unlock.Index];
		const float RuneIcon = UI().ResultRuneIcon;
		DrawSprite(Icon, FVector2f(CardX + UI().ResultCardIconX, Y + CardH * 0.5f), Unlock.bTower ? TexSize(Icon) * UI().ResultTowerScale : FVector2f(RuneIcon, RuneIcon));
		DrawLabel(YJJLoc::Get(TEXT("Result.NewItem")), CardX + UI().ResultCardTextX, Y + UI().ResultCardLine1, UI().ResultCardSmall, YJJUI::Orange);
		DrawLabel(FYJJProgression::UnlockName(Unlock), CardX + UI().ResultCardTextX, Y + UI().ResultCardLine2, UI().ResultCardName, YJJUI::Ink, EAlign::Left, 0.f, CardW - UI().ResultCardTextX - UI().TextFitPad);
		CardX += CardW + Gap;
	}
	if (bNewMap)
	{
		const float Inset = UI().ResultMapInset;
		const float FrameW = UI().ResultMapFrameW;
		const float FrameH = UI().ResultMapFrameH;
		const float ThumbPad = UI().ResultMapThumbPad;
		DrawPanel(CardX, Y, CardW, CardH, YJJUI::Green);
		DrawPanel(CardX + Inset, Y + Inset, FrameW, FrameH, FLinearColor::White, false);
		DrawSprite(MapThumb(Reward.NewMap), FVector2f(CardX + Inset + FrameW * 0.5f, Y + Inset + FrameH * 0.5f), FVector2f(FrameW - ThumbPad * 2.f, FrameH - ThumbPad * 2.f));
		DrawLabel(YJJLoc::Get(TEXT("Result.NewMap")), CardX + UI().ResultMapTextX, Y + UI().ResultMapTextY, UI().ResultCardName, FLinearColor::White, EAlign::Left, Outline);
		DrawLabel(YJJMap::Name(Reward.NewMap), CardX + CardW * 0.5f, Y + UI().ResultMapNameY, UI().ResultMapNameSize, FLinearColor::White, EAlign::Center, Outline);
		CardX += CardW + Gap;
	}
	if (Reward.bNewTier)
	{
		DrawPanel(CardX, Y, CardW, CardH, YJJUI::Sky);
		DrawLabel(YJJLoc::Get(TEXT("Result.NewTier")), CardX + CardW * 0.5f, Y + UI().ResultCardLine1, UI().ResultTierLabelSize, FLinearColor::White, EAlign::Center, Outline);
		DrawLabel(YJJLoc::Format(TEXT("Loadout.TierValue"), { Save ? Save->MaxTier : 0 }), CardX + CardW * 0.5f, Y + UI().ResultTierValueY, UI().ResultTierValueSize, FLinearColor::White, EAlign::Center, UI().ResultTierOutline);
	}
}

void AYJJDefenseHUD::DrawResult()
{
	const FYJJRunStats& Stats = Game->GetLastStats();

	DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, UI().ResultDim));

	const float W = UI().ResultW;
	const float H = UI().ResultH;
	const float X = (YJJ::ViewW - W) * 0.5f;
	const float Y = UI().ResultY;
	DrawPanel(X, Y, W, H, YJJUI::Cream);
	AddHit(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, EAction::Blocker);

	const float CenterX = YJJ::ViewW * 0.5f;
	DrawLabel(Stats.bVictory ? YJJLoc::Get(TEXT("Banner.Victory")) : YJJLoc::Get(TEXT("Banner.Defeat")),
		CenterX, Y + UI().ResultTitleY, UI().ResultTitleSize, Stats.bVictory ? YJJUI::Gain : YJJUI::Red, EAlign::Center);
	DrawLabel(YJJLoc::Format(TEXT("Top.MapTier"), { YJJMap::Name(Stats.Map), Stats.Tier }), CenterX, Y + UI().ResultMapY, UI().ResultMapSize, YJJUI::DarkGray, EAlign::Center);

	// 별: 하나씩 흔들거린다 (가운데 별이 조금 위)
	const float Star = UI().ResultStarSize;
	for (int32 Index = 0; Index < FYJJDefenseSim::ThreeStars; ++Index)
	{
		const bool bEarned = Index < Stats.Stars;
		const bool bMiddle = Index == FYJJDefenseSim::ThreeStars / 2;
		const float Wobble = bEarned ? FMath::Sin(UITime * UI().ResultStarWobbleSpeed + Index) * UI().ResultStarWobble : 0.f;
		const FVector2f Pos(CenterX + (Index - FYJJDefenseSim::ThreeStars / 2) * UI().ResultStarStep, Y + UI().ResultStarY - (bMiddle ? UI().ResultStarMiddleUp : 0.f));
		DrawSprite(bEarned ? StarIcon : StarEmptyIcon, Pos, FVector2f(Star, Star), FLinearColor::White, Wobble);
	}

	const float LineY = DrawResultRows(Y + UI().ResultRowsY);
	DrawResultCards(LineY);

	const float BW = UI().ResultButtonW;
	const float BH = UI().ResultButtonH;
	const float BY = Y + H - BH - UI().ResultButtonBottom;
	const bool bHover = DrawButton(CenterX - BW * 0.5f, BY, BW, BH, YJJUI::Green, EAction::ResultContinue);
	DrawLabel(YJJLoc::Get(TEXT("Result.Again")), CenterX, BY + BH * 0.5f - UI().ResultButtonTextUp - (bHover ? UI().ButtonHoverLift : 0.f), UI().ResultButtonSize, FLinearColor::White, EAlign::Center, UI().ResultButtonOutline);
}
