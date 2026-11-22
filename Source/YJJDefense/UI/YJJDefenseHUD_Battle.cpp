// 전투 화면: 맵, 병졸, 몬스터, 투사체, 그리고 전투 UI (상단 정보, 스펠 바, 타워 메뉴)

#include "UI/YJJDefenseHUD.h"
#include "Data/YJJLoc.h"
#include "Data/YJJTuning.h"
#include "Data/YJJUILayout.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Game/YJJGameData.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefenseGameMode.h"

// ---------------------------------------------------------------------------------------------
// 버튼 자리 (그리기와 UI 테스트가 같이 쓴다)

FVector2f AYJJDefenseHUD::BagRuneCenter(int32 BagIndex)
{
	const int32 Columns = FMath::RoundToInt(UI().BagColumns);
	const float X = UI().BagX + UI().BagSpacing * (BagIndex % Columns);
	const float Y = UI().BagY + UI().BagSpacing * (BagIndex / Columns);
	return FVector2f(X, Y) + FVector2f(UI().BagIcon, UI().BagIcon) * 0.5f;
}

FVector2f AYJJDefenseHUD::DiskCenter(int32 Disk)
{
	return FVector2f(UI().DiskFirstX + UI().DiskSpacing * Disk, UI().DiskY);
}

FVector2f AYJJDefenseHUD::SocketCenter(int32 Disk, int32 Socket)
{
	return DiskCenter(Disk) + FVector2f((Socket == 0 ? -1.f : 1.f) * (UI().SocketSize * 0.5f + UI().SocketGap), 0.f);
}

FVector2f AYJJDefenseHUD::TowerMenuButtonCenter(const FVector2f& Spot, int32 NumOptions, int32 Index, bool bBuilt)
{
	const float ButtonW = UI().MenuButtonW;
	const float Gap = UI().MenuButtonGap;
	const float Margin = UI().MenuScreenMargin;
	const float TotalW = NumOptions * ButtonW + (NumOptions - 1) * Gap;
	const float Top = Spot.Y - (bBuilt ? UI().MenuAboveBuilt : UI().MenuAboveEmpty);
	const float X0 = FMath::Clamp(Spot.X - TotalW * 0.5f, Margin, YJJ::ViewW - TotalW - Margin);
	const float Y0 = Top < UI().MenuMinTop ? Spot.Y + UI().MenuBelowOffset : Top;
	return FVector2f(X0 + Index * (ButtonW + Gap) + ButtonW * 0.5f, Y0 + UI().MenuButtonH * 0.5f);
}

FVector2f AYJJDefenseHUD::WaveButtonCenter()
{
	return FVector2f(UI().WaveButtonX + UI().WaveButtonW * 0.5f, UI().WaveButtonY + UI().WaveButtonH * 0.5f);
}

FVector2f AYJJDefenseHUD::PauseButtonCenter()
{
	return FVector2f(UI().PauseButtonX + UI().TopButtonW * 0.5f, UI().TopBarY + UI().TopBarH * 0.5f);
}

// ---------------------------------------------------------------------------------------------

void AYJJDefenseHUD::DrawDimmedMap(float Darkness, int32 Map)
{
	Map = Map == INDEX_NONE ? YJJMap::GetActive() : Map;
	DrawSprite(MapTex(Map), FVector2f(YJJ::ViewW * 0.5f, YJJ::ViewH * 0.5f), FVector2f(YJJ::ViewW, YJJ::ViewH));
	if (Darkness > 0.f)
	{
		DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, Darkness));
	}
}

void AYJJDefenseHUD::DrawBattle(bool bInteractive)
{
	// 이펙트는 게임 배속을 따라가고, 일시정지하면 멈춘다
	const bool bRunning = bInteractive && !Game->IsBattlePaused();
	const float FxDelta = bRunning ? GetWorld()->GetDeltaSeconds() * FMath::Min(Game->GetSpeed(), UI().FxMaxSpeed) : 0.f;

	DrawDimmedMap(0.f);
	DrawZones();
	if (bInteractive)
	{
		// 타워보다 먼저 그려 바닥에 깔리게
		DrawSelectedRange();
	}

	DrawWorldSorted();
	DrawProjectiles(FxDelta);
	TickAndDrawFx(FxDelta);

	if (!bInteractive)
	{
		return;
	}

	DrawTargeting();
	DrawTowerMenu();
	DrawTopBar();
	DrawSpellBar();
	DrawWaveButton();
	DrawBanner(GetWorld()->GetDeltaSeconds());

	if (Game->IsBattlePaused())
	{
		DrawPauseOverlay();
	}
}

void AYJJDefenseHUD::DrawSelectedRange()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	if (!Sim.Towers.IsValidIndex(SelectedSpot))
	{
		return;
	}

	const FYJJTowerState& Tower = Sim.Towers[SelectedSpot];
	const FVector2f SpotPos = YJJMap::Spots()[SelectedSpot];
	if (Tower.bBuilt)
	{
		const int32 Level = bHoverUpgrade ? FMath::Min(Tower.Level + 1, YJJ::MaxTowerLevel) : Tower.Level;
		DrawEllipseRange(SpotPos, YJJData::Tower(Tower.Type).Levels[Level].Range, bHoverUpgrade ? YJJUI::Green : FLinearColor::White);
		if (Tower.Type == EYJJTower::Barracks)
		{
			DrawEllipseRange(Tower.Rally, YJJData::Tune().SoldierEngageRange, YJJUI::Sky);
		}
	}
	else if (HoverBuildType != INDEX_NONE)
	{
		DrawEllipseRange(SpotPos, YJJData::Tower(static_cast<EYJJTower>(HoverBuildType)).Levels[0].Range, FLinearColor::White);
	}
}

void AYJJDefenseHUD::DrawAnimPreview()
{
	if (CVarAnimPreview.GetValueOnGameThread() == 0)
	{
		return;
	}

	// 동작이 하나라도 있는 캐릭터만 한 줄씩
	TArray<TPair<FString, const FActorAnims*>> Rows;
	if (SoldierAnims.Idle.IsValid() || SoldierAnims.Walk.IsValid() || SoldierAnims.Attack.IsValid())
	{
		Rows.Add({ YJJLoc::Get(TEXT("AnimPreview.Soldier")), &SoldierAnims });
	}
	for (int32 Type = 0; Type < static_cast<int32>(EYJJEnemy::Count); ++Type)
	{
		const FActorAnims& Anims = EnemyAnims[Type];
		if (Anims.Idle.IsValid() || Anims.Walk.IsValid() || Anims.Attack.IsValid())
		{
			Rows.Add({ YJJLoc::Get(YJJData::Enemy(static_cast<EYJJEnemy>(Type)).NameKey), &Anims });
		}
	}

	const float X = UI().AnimPreviewX;
	const float Y = UI().AnimPreviewY;
	const float W = UI().AnimPreviewW;
	const float RowH = UI().AnimPreviewRowH;
	const float LabelW = UI().AnimPreviewLabelW;
	const float ColW = (W - LabelW) / UI().AnimPreviewColumns;
	const float H = UI().AnimPreviewHeaderH + RowH * Rows.Num() + UI().AnimPreviewPad;
	DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, UI().AnimPreviewDim));
	DrawPanel(X, Y, W, H, YJJUI::Cream);

	const TCHAR* ColumnKeys[] = { TEXT("AnimPreview.Idle"), TEXT("AnimPreview.Walk"), TEXT("AnimPreview.Attack") };
	DrawLabel(YJJLoc::Get(TEXT("AnimPreview.Title")), X + UI().AnimPreviewPad, Y + UI().AnimPreviewTitleY, UI().AnimPreviewTitleSize, YJJUI::Ink);
	for (int32 Column = 0; Column < UE_ARRAY_COUNT(ColumnKeys); ++Column)
	{
		DrawLabel(YJJLoc::Get(ColumnKeys[Column]), X + LabelW + ColW * (Column + 0.5f), Y + UI().AnimPreviewColumnY, UI().AnimPreviewLabelSize, YJJUI::DarkGray, EAlign::Center);
	}

	const float Interval = UI().AnimPreviewInterval;
	const float TimeToHit = Interval - FMath::Fmod(UITime, Interval);
	const FVector2f Scale(UI().AnimPreviewScale, UI().AnimPreviewScale);
	for (int32 Row = 0; Row < Rows.Num(); ++Row)
	{
		const FActorAnims& Anims = *Rows[Row].Value;
		const float RowY = Y + UI().AnimPreviewHeaderH + RowH * Row;
		DrawLabel(Rows[Row].Key, X + UI().AnimPreviewPad, RowY + RowH * 0.5f, UI().AnimPreviewLabelSize, YJJUI::Ink, EAlign::Left, 0.f, LabelW - UI().AnimPreviewPad);

		UTexture2D* Frames[] = {
			Anims.Idle.IsValid() ? Anims.Idle.Loop(UITime) : nullptr,
			Anims.Walk.IsValid() ? Anims.Walk.Loop(UITime) : nullptr,
			Anims.Attack.IsValid() ? FightFrame(Anims, TimeToHit, Interval, UITime) : nullptr,
		};
		for (int32 Column = 0; Column < UE_ARRAY_COUNT(Frames); ++Column)
		{
			if (!Frames[Column])
			{
				continue;
			}
			const FVector2f Feet(X + LabelW + ColW * (Column + 0.5f), RowY + UI().AnimPreviewFeetY);
			DrawSprite(ShadowTexture, Feet, FVector2f(UI().AnimPreviewShadowW, UI().AnimPreviewShadowH));
			DrawSpriteFeet(Frames[Column], Feet, Scale);
		}
	}
}

void AYJJDefenseHUD::DrawPauseOverlay()
{
	DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, UI().PauseDim));
	DrawLabel(YJJLoc::Get(TEXT("Pause.Title")), YJJ::ViewW * 0.5f, UI().PauseTitleY, UI().PauseTitleSize, FLinearColor::White, EAlign::Center, UI().PauseTitleOutline);
	DrawLabel(YJJLoc::Get(TEXT("Pause.Hint")), YJJ::ViewW * 0.5f, UI().PauseHintY, UI().PauseHintSize, YJJUI::Cream, EAlign::Center, UI().PauseHintOutline);
}

// ---------------------------------------------------------------------------------------------
// 월드

void AYJJDefenseHUD::DrawZones()
{
	const int32 Blobs = FMath::RoundToInt(UI().ZoneBlobs);
	for (const FYJJZoneState& Zone : Game->GetSim().Zones)
	{
		const FLinearColor Color = YJJData::Reaction(Zone.Kind).Color;
		const float Life = Zone.Time / FMath::Max(Zone.Duration, KINDA_SMALL_NUMBER);
		const float Fade = FMath::Clamp(Zone.Time * UI().ZoneFadeInSpeed, 0.f, 1.f) * FMath::Clamp((1.f - Life) * UI().ZoneFadeOutSpeed, 0.f, 1.f);
		const float Pulse = 1.f + UI().ZonePulse * FMath::Sin(UITime * UI().ZonePulseSpeed + Zone.Pos.X);
		const FVector2f Size(Zone.Radius * 2.f * Pulse, Zone.Radius * 2.f * Pulse / YJJ::YStretch);

		DrawSprite(DiscTexture, Zone.Pos, Size * UI().ZoneFillScale, YJJUI::WithAlpha(Color, UI().ZoneFillAlpha * Fade));
		DrawSprite(RingTexture, Zone.Pos, Size, YJJUI::WithAlpha(Color, UI().ZoneRingAlpha * Fade));

		// 장판 위를 도는 몽실한 덩어리들
		for (int32 Index = 0; Index < Blobs; ++Index)
		{
			const float Angle = UITime * UI().ZoneBlobSpin + Index * UE_TWO_PI / Blobs + Zone.Pos.Y;
			const float Orbit = Zone.Radius * UI().ZoneBlobOrbit;
			const FVector2f Offset(FMath::Cos(Angle) * Orbit, FMath::Sin(Angle) * Orbit / YJJ::YStretch - UI().ZoneBlobLift);
			const float BlobSize = UI().ZoneBlobSize;
			DrawSprite(PuffTextures[0], Zone.Pos + Offset, FVector2f(BlobSize, BlobSize),
				YJJUI::WithAlpha(FMath::Lerp(Color, FLinearColor::White, UI().ZoneBlobWhiten), UI().ZoneBlobAlpha * Fade));
		}
	}
}

void AYJJDefenseHUD::DrawWorldSorted()
{
	const FYJJDefenseSim& Sim = Game->GetSim();

	constexpr int32 InlineItems = 256;
	TArray<FWorldItem, TInlineAllocator<InlineItems>> Items;

	const TConstArrayView<FVector2f> Spots = YJJMap::Spots();
	for (int32 Spot = 0; Spot < Spots.Num(); ++Spot)
	{
		Items.Add({ Spots[Spot].Y + (Sim.Towers[Spot].bBuilt ? UI().SortBuiltTower : -UI().SortEmptySpot), EWorldItem::Spot, Spot });
	}
	for (int32 Index = 0; Index < Sim.Soldiers.Num(); ++Index)
	{
		if (Sim.Soldiers[Index].IsAlive())
		{
			Items.Add({ Sim.Soldiers[Index].Pos.Y, EWorldItem::Soldier, Index });
		}
	}
	for (int32 Index = 0; Index < Sim.Enemies.Num(); ++Index)
	{
		const FYJJEnemyState& Enemy = Sim.Enemies[Index];
		if (Enemy.IsAlive())
		{
			Items.Add({ Enemy.Pos.Y + (YJJData::Enemy(Enemy.Type).bFlying ? UI().SortFlying : 0.f), EWorldItem::Enemy, Index });
		}
	}
	Items.Add({ YJJMap::WizardPos().Y, EWorldItem::Wizard, 0 });

	Items.StableSort([](const FWorldItem& A, const FWorldItem& B) { return A.Y < B.Y; });

	for (const FWorldItem& Item : Items)
	{
		switch (Item.Kind)
		{
		case EWorldItem::Spot: DrawSpot(Item.Index); break;
		case EWorldItem::Soldier: DrawSoldier(Sim.Soldiers[Item.Index]); break;
		case EWorldItem::Enemy: DrawEnemy(Sim.Enemies[Item.Index]); break;
		case EWorldItem::Wizard: DrawWizard(); break;
		}
	}
}

void AYJJDefenseHUD::DrawEmptySpot(int32 Spot, bool bHover, bool bSelected)
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FVector2f SpotPos = YJJMap::Spots()[Spot];
	const bool bActive = bHover || bSelected;

	// 지을 돈이 있으면 노랗게 숨 쉬듯 빛나서 "여기 지을 수 있다"가 바로 보이게
	int32 Cheapest = MAX_int32;
	for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count); ++Index)
	{
		if (Sim.HasTower(static_cast<EYJJTower>(Index)))
		{
			Cheapest = FMath::Min(Cheapest, YJJData::Tower(static_cast<EYJJTower>(Index)).Levels[0].Cost);
		}
	}
	const bool bAffordable = Sim.Gold >= Cheapest;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(UITime * UI().SpotPulseSpeed + Spot * UI().SpotPulsePhase);
	const FLinearColor Glow = bActive ? FLinearColor::White : (bAffordable ? YJJUI::Yellow : YJJUI::SpotIdleGlow);
	const float GlowAlpha = bActive ? UI().SpotGlowActive : (bAffordable ? UI().SpotGlowReady + UI().SpotGlowReadyPulse * Pulse : UI().SpotGlowIdle);
	const float RingAlpha = bActive ? 1.f : (bAffordable ? UI().SpotRingReady + UI().SpotRingReadyPulse * Pulse : UI().SpotRingIdle);
	const FVector2f GlowCenter = SpotPos + FVector2f(0.f, UI().SpotGlowDown);
	DrawSprite(DiscTexture, GlowCenter, FVector2f(UI().SpotGlowW, UI().SpotGlowH) * (1.f + UI().SpotGlowPulse * Pulse), YJJUI::WithAlpha(Glow, GlowAlpha));
	DrawSprite(DashRingTexture, GlowCenter, FVector2f(UI().SpotRingW, UI().SpotRingH) * (1.f + UI().SpotRingPulse * Pulse), YJJUI::WithAlpha(Glow, RingAlpha));

	// 받침 윗면 중심(그림 높이의 SpotTopShare)이 자리 좌표에 오게
	const FVector2f Size = TexSize(SpotTexture) * UI().SpriteScale * (bActive ? UI().SpotHoverGrow : 1.f);
	const float Hop = bActive ? UI().SpotHoverHop : 0.f;
	DrawSprite(SpotTexture, SpotPos + FVector2f(0.f, Size.Y * (0.5f - UI().SpotTopShare) - Hop), Size, bSelected ? YJJUI::SpotSelected : FLinearColor::White);
}

void AYJJDefenseHUD::DrawSpot(int32 Spot)
{
	const FYJJTowerState& Tower = Game->GetSim().Towers[Spot];
	const FVector2f SpotPos = YJJMap::Spots()[Spot];
	const bool bSelected = Spot == SelectedSpot;
	const bool bHover = FindSpotAt(Mouse) == Spot && HitAt(Mouse) == nullptr && ArmedDisk == INDEX_NONE;

	if (!Tower.bBuilt)
	{
		DrawEmptySpot(Spot, bHover, bSelected);
		return;
	}

	// 지을 때 통통 튀고, 쏠 때 살짝 움찔한다
	const FYJJTuning& Tune = YJJData::Tune();
	FVector2f Scale(1.f, 1.f);
	if (Tower.BuildAnim > 0.f)
	{
		const float T = Tower.BuildAnim / Tune.TowerBuildAnim;
		Scale = FVector2f(1.f + UI().TowerBuildStretchX * T, 1.f - UI().TowerBuildSquashY * T * FMath::Cos(T * UI().TowerBuildWobble));
	}
	else if (Tower.FireAnim > 0.f)
	{
		const float T = Tower.FireAnim / Tune.TowerFireAnim;
		Scale = FVector2f(1.f + UI().TowerFireStretchX * T, 1.f - UI().TowerFireSquashY * T);
	}

	const FVector2f Feet = SpotPos + FVector2f(0.f, UI().TowerFeetDown);
	DrawSprite(ShadowTexture, Feet - FVector2f(0.f, UI().TowerShadowUp), FVector2f(UI().TowerShadowW, UI().TowerShadowH));
	UTexture2D* Texture = TowerTextures[static_cast<int32>(Tower.Type)][Tower.Level];
	const FLinearColor Tint = (bHover || bSelected) ? YJJUI::TowerHover : FLinearColor::White;
	DrawSpriteFeet(Texture, Feet, Scale, Tint, Tower.bAimLeft && Tower.Type != EYJJTower::Barracks && Tower.Type != EYJJTower::Shrine);

	if (Tower.Type == EYJJTower::Shrine)
	{
		// 그릇 위에 떠 있는 원소 구슬
		const float Height = (UI().ShrineOrbBase + UI().ShrineOrbPerLevel * (Tower.Level + 1)) * Scale.Y + UI().ShrineOrbLift;
		const float Bob = FMath::Sin(UITime * UI().ShrineOrbBobSpeed + Spot) * UI().ShrineOrbBob;
		const FVector2f Orb = Feet + FVector2f(0.f, -Height + Bob);
		const FLinearColor Color = YJJData::ElementColor(Tower.ShrineElement);
		const float Glow = UI().ShrineOrbGlow;
		const float Ball = UI().ShrineOrbSize;
		DrawSprite(DiscTexture, Orb, FVector2f(Glow, Glow), YJJUI::WithAlpha(Color, UI().ShrineOrbGlowAlpha));
		DrawSprite(BallTex(Tower.ShrineElement), Orb, FVector2f(Ball, Ball));
	}
}

void AYJJDefenseHUD::DrawEnemy(const FYJJEnemyState& Enemy)
{
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	const int32 TypeIndex = static_cast<int32>(Enemy.Type);
	const FActorAnims& Anims = EnemyAnims[TypeIndex];
	UTexture2D* Texture = nullptr;
	if (Enemy.BlockerId != INDEX_NONE && Anims.Attack.IsValid())
	{
		Texture = FightFrame(Anims, Enemy.AttackTimer, Def.MeleeInterval, Enemy.AnimTime);
	}
	else if (Anims.Walk.IsValid())
	{
		Texture = Anims.Walk.Loop(Enemy.AnimTime);
	}
	else
	{
		const float FrameRate = Def.bFlying ? UI().EnemyFlyFrameRate : UI().EnemyWalkFrameRate;
		Texture = EnemyTextures[TypeIndex][FMath::FloorToInt(Enemy.AnimTime * FrameRate) % EnemyFrames];
	}
	// 그림자·체력 막대는 기본 그림 크기로 (동작 그림은 무기가 휘둘러질 여백만큼 캔버스가 넓다)
	const FVector2f Size = TexSize(EnemyTextures[TypeIndex][0]) * UI().SpriteScale;

	FVector2f Feet = Enemy.Pos + FVector2f(0.f, UI().UnitFeetDown);
	DrawSprite(ShadowTexture, Enemy.Pos + FVector2f(0.f, UI().UnitShadowDown), FVector2f(Size.X * UI().EnemyShadowW, Size.X * UI().EnemyShadowH));
	if (Def.bFlying)
	{
		Feet.Y -= UI().FlyHeight + FMath::Sin(Enemy.AnimTime * UI().FlyBobSpeed) * UI().FlyBob;
	}

	// 상태에 따라 색이 변한다
	FLinearColor Tint = FLinearColor::White;
	if (Enemy.FreezeTime > 0.f)
	{
		Tint = YJJUI::FrozenTint;
	}
	else if (Enemy.PoisonTime > 0.f)
	{
		Tint = YJJUI::PoisonTint;
	}
	else if (Enemy.SlowTime > 0.f)
	{
		Tint = YJJUI::SlowTint;
	}

	FVector2f Scale(1.f, 1.f);
	if (Enemy.HitFlash > 0.f)
	{
		const float T = Enemy.HitFlash / YJJData::Tune().HitFlashTime;
		Tint = FMath::Lerp(Tint, YJJUI::HitTint, T);
		Scale = FVector2f(1.f + UI().EnemyHitStretchX * T, 1.f - UI().EnemyHitSquashY * T);
	}
	if (Enemy.StunTime > 0.f && Enemy.FreezeTime <= 0.f)
	{
		Feet.X += FMath::Sin(UITime * UI().StunShakeSpeed) * UI().StunShake;
	}

	if (Enemy.VulnTime > 0.f)
	{
		DrawSprite(RingTexture, Enemy.Pos, FVector2f(Size.X * UI().VulnRingW, Size.X * UI().VulnRingH), YJJUI::WithAlpha(YJJUI::Purple, UI().VulnRingAlpha));
	}

	DrawSpriteFeet(Texture, Feet, Scale, Tint, Enemy.Dir.X < -UI().FacingDeadZone);
	DrawEnemyStatus(Enemy, Feet, Size);
}

void AYJJDefenseHUD::DrawEnemyStatus(const FYJJEnemyState& Enemy, const FVector2f& Feet, const FVector2f& Size)
{
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	const float HeadY = Feet.Y - Size.Y;

	if (Enemy.FreezeTime > 0.f)
	{
		DrawSprite(IceTexture, FVector2f(Feet.X, Feet.Y - Size.Y * 0.5f), Size * UI().IceScale);
	}
	if (Enemy.BurnTime > 0.f)
	{
		const float Flicker = 1.f + UI().FlameFlicker * FMath::Sin(UITime * UI().FlameFlickerSpeed + Enemy.Id);
		DrawSprite(FlameTexture, FVector2f(Feet.X - Size.X * UI().Flame1X, HeadY + UI().Flame1Down), FVector2f(UI().Flame1W, UI().Flame1H) * Flicker, YJJUI::Orange);
		DrawSprite(FlameTexture, FVector2f(Feet.X + Size.X * UI().Flame2X, HeadY + UI().Flame2Down), FVector2f(UI().Flame2W, UI().Flame2H) * (2.f - Flicker), YJJUI::Yellow);
	}
	if (Enemy.StunTime > 0.f && Enemy.FreezeTime <= 0.f)
	{
		// 머리 위를 도는 별
		const int32 Stars = FMath::RoundToInt(UI().StunStars);
		const float StarSize = UI().StunStarSize;
		for (int32 Index = 0; Index < Stars; ++Index)
		{
			const float Angle = UITime * UI().StunStarSpin + Index * UE_TWO_PI / Stars;
			DrawSprite(StarFxTexture, FVector2f(Feet.X + FMath::Cos(Angle) * Size.X * UI().StunStarOrbit, HeadY + FMath::Sin(Angle) * UI().StunStarBob),
				FVector2f(StarSize, StarSize), YJJUI::Yellow, FMath::RadiansToDegrees(Angle));
		}
	}

	// 묻어 있는 원소
	float BarY = HeadY - UI().EnemyBarUp;
	if (Enemy.Aura != EYJJElement::None)
	{
		const float Blink = Enemy.AuraTime < UI().AuraBlinkTime ? UI().AuraBlinkBase + UI().AuraBlinkAmount * FMath::Sin(UITime * UI().AuraBlinkSpeed) : 1.f;
		const float Icon = UI().AuraIconSize;
		DrawSprite(RuneTex(YJJData::ElementRune(Enemy.Aura)), FVector2f(Feet.X, HeadY - UI().AuraIconUp), FVector2f(Icon, Icon), YJJUI::WithAlpha(FLinearColor::White, Blink));
		BarY = HeadY - UI().EnemyBarUpWithAura;
	}

	if (Enemy.HP < Enemy.MaxHP)
	{
		const float Width = Def.bBoss ? UI().BossBarWidth : FMath::Max(Size.X * UI().EnemyBarShare, UI().EnemyBarMinWidth);
		DrawBar(FVector2f(Feet.X, BarY), Width, Enemy.HP / Enemy.MaxHP, Def.bBoss ? YJJUI::Orange : YJJUI::Red);
	}
}

void AYJJDefenseHUD::DrawSoldier(const FYJJSoldierState& Soldier)
{
	constexpr int32 WalkFrames = 2;
	// 병졸마다 숨쉬기 박자를 어긋나게
	const float AnimTime = Soldier.AnimTime + Soldier.Id * UI().AnimIdPhase;
	const bool bFighting = Soldier.TargetId != INDEX_NONE && (!Soldier.bMoving || Soldier.AttackAnim > 0.f);
	UTexture2D* Texture = nullptr;
	if (bFighting && SoldierAnims.Attack.IsValid())
	{
		const FYJJTowerState& Tower = Game->GetSim().Towers[Soldier.Spot];
		const float Interval = YJJData::Tower(EYJJTower::Barracks).Levels[FMath::Clamp(Tower.Level, 0, YJJ::MaxTowerLevel)].Interval;
		Texture = FightFrame(SoldierAnims, Soldier.AttackTimer, Interval, AnimTime);
	}
	else if (Soldier.bMoving && SoldierAnims.Walk.IsValid())
	{
		Texture = SoldierAnims.Walk.Loop(AnimTime);
	}
	else if (!Soldier.bMoving && SoldierAnims.Idle.IsValid())
	{
		Texture = SoldierAnims.Idle.Loop(AnimTime);
	}
	else
	{
		const int32 Frame = Soldier.AttackAnim > 0.f ? SoldierAttackFrame : (Soldier.bMoving ? (FMath::FloorToInt(Soldier.AnimTime * UI().SoldierWalkFrameRate) % WalkFrames) : 0);
		Texture = SoldierTextures[Frame];
	}
	const FVector2f Size = TexSize(SoldierTextures[0]) * UI().SpriteScale;
	const FVector2f Feet = Soldier.Pos + FVector2f(0.f, UI().UnitFeetDown);

	DrawSprite(ShadowTexture, Soldier.Pos + FVector2f(0.f, UI().UnitShadowDown), FVector2f(UI().SoldierShadowW, UI().SoldierShadowH));
	const FLinearColor Tint = Soldier.HitFlash > 0.f ? YJJUI::HitTint : FLinearColor::White;
	DrawSpriteFeet(Texture, Feet, FVector2f(1.f, 1.f), Tint, Soldier.bFacingLeft);

	if (Soldier.HP < Soldier.MaxHP)
	{
		DrawBar(FVector2f(Feet.X, Feet.Y - Size.Y - UI().SoldierBarUp), UI().SoldierBarWidth, Soldier.HP / Soldier.MaxHP, YJJUI::Green);
	}
}

void AYJJDefenseHUD::DrawWizard()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FVector2f Feet = YJJMap::WizardPos() + FVector2f(0.f, UI().WizardFeetDown);
	DrawSprite(ShadowTexture, Feet - FVector2f(0.f, UI().WizardShadowUp), FVector2f(UI().WizardShadowW, UI().WizardShadowH));
	DrawSpriteFeet(WizardTextures[Sim.WizardCastAnim > 0.f ? 1 : 0], Feet);

	// 마력 레벨만큼 별
	const float Star = UI().WizardStarSize;
	for (int32 Index = 0; Index < Sim.WizardLevel; ++Index)
	{
		DrawSprite(StarIcon, Feet + FVector2f((Index - (Sim.WizardLevel - 1) * 0.5f) * Star, -UI().WizardStarUp), FVector2f(Star, Star));
	}
}

void AYJJDefenseHUD::DrawSpellBall(const FYJJProjectileState& Projectile, const FVector2f& Pos, const FVector2f& Velocity, float DeltaSeconds)
{
	const FLinearColor Color = YJJData::ElementColor(Projectile.Element);
	const bool bBig = Projectile.Spell.bAmplified || Projectile.Spell.Explode > 1;
	const float Size = (bBig ? UI().SpellBallBigSize : UI().SpellBallSize) * (1.f + UI().SpellBallPulse * FMath::Sin(UITime * UI().SpellBallPulseSpeed));
	DrawSprite(DiscTexture, Pos, FVector2f(Size, Size) * UI().SpellBallGlow, YJJUI::WithAlpha(Color, UI().SpellBallGlowAlpha));
	DrawSprite(BallTex(Projectile.Element), Pos, FVector2f(Size, Size));

	// 두 번째 원소는 작은 구슬로 주위를 돈다
	const bool bTwoElements = Projectile.Spell.NumElements > 1;
	if (bTwoElements)
	{
		const float Orbit = UITime * UI().SpellBallOrbitSpeed;
		const FVector2f Offset(FMath::Cos(Orbit) * Size * UI().SpellBallOrbitX, FMath::Sin(Orbit) * Size * UI().SpellBallOrbitY);
		DrawSprite(BallTex(Projectile.Element2), Pos + Offset, FVector2f(Size, Size) * UI().SpellBallMoonScale);
	}

	// 꼬리
	if (DeltaSeconds > 0.f && FxRandom.FRand() < UI().SpellTrailChance)
	{
		const float Jitter = UI().SpellTrailJitter;
		const FLinearColor TrailColor = bTwoElements && FxRandom.FRand() < 0.5f ? YJJData::ElementColor(Projectile.Element2) : Color;
		FFx& Spark = AddFx(EFx::Spark, Pos + FVector2f(FxRandom.FRandRange(-Jitter, Jitter), FxRandom.FRandRange(-Jitter, Jitter)), UI().SpellTrailLife, UI().SpellTrailSize, TrailColor);
		Spark.Vel = -Velocity * UI().SpellTrailBackSpeed;
	}
}

void AYJJDefenseHUD::DrawProjectiles(float DeltaSeconds)
{
	for (const FYJJProjectileState& Projectile : Game->GetSim().Projectiles)
	{
		const FVector2f Velocity = Projectile.Pos - Projectile.PrevPos;
		const float Angle = Velocity.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
		const FVector2f Pos = Projectile.Pos - FVector2f(0.f, UI().ProjectileLift);

		switch (Projectile.Kind)
		{
		case EYJJProjectile::Arrow:
			DrawSprite(ArrowTexture, Pos, FVector2f(UI().ArrowW, UI().ArrowH), FLinearColor::White, Angle);
			break;
		case EYJJProjectile::Bomb:
			DrawSprite(BombTexture, Pos, FVector2f(UI().BombW, UI().BombH), FLinearColor::White, Projectile.Time * UI().BombSpin);
			break;
		case EYJJProjectile::MagicBolt:
		{
			const float Glow = UI().MagicBoltGlow;
			const float Bolt = UI().MagicBoltSize;
			DrawSprite(DiscTexture, Pos, FVector2f(Glow, Glow), YJJUI::WithAlpha(YJJUI::Purple, UI().ProjectileGlowAlpha));
			DrawSprite(BoltTexture, Pos, FVector2f(Bolt, Bolt), FLinearColor::White, Projectile.Time * UI().MagicBoltSpin);
			break;
		}
		case EYJJProjectile::ShrineOrb:
		{
			const float Glow = UI().ShrineShotGlow;
			const float Ball = UI().ShrineShotSize;
			DrawSprite(DiscTexture, Pos, FVector2f(Glow, Glow), YJJUI::WithAlpha(YJJData::ElementColor(Projectile.Element), UI().ProjectileGlowAlpha));
			DrawSprite(BallTex(Projectile.Element), Pos, FVector2f(Ball, Ball));
			break;
		}
		case EYJJProjectile::SpellBall:
			DrawSpellBall(Projectile, Pos, Velocity, DeltaSeconds);
			break;
		default:
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 전투 UI

void AYJJDefenseHUD::DrawTopBar()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const float Y = UI().TopBarY;
	const float H = UI().TopBarH;
	const float IconY = Y + H * 0.5f - UI().TopIconUp;
	const float TextY = Y + H * 0.5f - UI().TopTextUp;
	const float Icon = UI().TopIconSize;

	DrawPanel(UI().LivesX, Y, UI().LivesW, H, FLinearColor::White);
	DrawSprite(HeartIcon, FVector2f(UI().LivesIconX, IconY), FVector2f(Icon, Icon));
	DrawLabel(FString::FromInt(Sim.Lives), UI().LivesTextX, TextY, UI().TopNumberSize, Sim.Lives <= FMath::RoundToInt(UI().LivesWarnAt) ? YJJUI::Red : YJJUI::Ink);

	DrawPanel(UI().GoldX, Y, UI().GoldW, H, FLinearColor::White);
	DrawSprite(CoinIcon, FVector2f(UI().GoldIconX, IconY), FVector2f(Icon, Icon));
	DrawLabel(FString::FromInt(Sim.Gold), UI().GoldTextX, TextY, UI().TopNumberSize, YJJUI::Ink);

	DrawPanel(UI().WaveInfoX, Y, UI().WaveInfoW, H, FLinearColor::White);
	DrawSprite(FlagIcon, FVector2f(UI().WaveIconX, IconY), FVector2f(Icon, Icon));
	DrawLabel(YJJLoc::Format(TEXT("Top.Wave"), { FMath::Max(Sim.WaveIndex + 1, 0), YJJData::NumWaves() }), UI().WaveTextX, TextY, UI().TopWaveSize, YJJUI::Ink);

	const float MapX = UI().MapInfoX;
	const float MapW = UI().MapInfoW;
	DrawPanel(MapX, Y, MapW, H, YJJUI::Yellow);
	DrawLabel(YJJLoc::Format(TEXT("Top.MapTier"), { YJJMap::Name(Sim.Config.Map), Sim.Config.Tier }), MapX + MapW * 0.5f, TextY, UI().TopMapSize, YJJUI::Ink, EAlign::Center, 0.f, MapW - UI().TextFitPad);

	// 오른쪽: 일시정지, 배속
	const float ButtonW = UI().TopButtonW;
	const float Button = UI().TopButtonIcon;
	const float Lift = UI().ButtonHoverLift;
	const float PauseX = UI().PauseButtonX;
	const bool bPauseHover = DrawButton(PauseX, Y, ButtonW, H, YJJUI::Sky, EAction::Pause);
	DrawSprite(Game->IsBattlePaused() ? PlayIcon : PauseIcon, FVector2f(PauseX + ButtonW * 0.5f, IconY - (bPauseHover ? Lift : 0.f)), FVector2f(Button, Button));
	const bool bFast = Game->IsFast();
	const float SpeedX = UI().SpeedButtonX;
	const bool bSpeedHover = DrawButton(SpeedX, Y, ButtonW, H, bFast ? YJJUI::Orange : YJJUI::Sky, EAction::Speed);
	DrawSprite(bFast ? FastIcon : PlayIcon, FVector2f(SpeedX + ButtonW * 0.5f, IconY - (bSpeedHover ? Lift : 0.f)), FVector2f(Button, Button));
	if (bPauseHover)
	{
		SetTooltip(YJJLoc::Get(TEXT("Tip.Pause.Title")), YJJLoc::Get(TEXT("Tip.Pause.Body")), YJJUI::Ink);
	}
	if (bSpeedHover)
	{
		SetTooltip(YJJLoc::Get(TEXT("Tip.Speed.Title")),
			YJJLoc::Format(TEXT("Tip.Speed.Body"), { Whole(Game->GetSpeed()) }), YJJUI::Ink);
	}
}

FString AYJJDefenseHUD::DescribeSpell(const FYJJSpellSpec& Spell, bool& bOutNew) const
{
	bOutNew = false;
	if (!Spell.IsValid())
	{
		return YJJLoc::Get(TEXT("Spell.Empty"));
	}

	const FYJJDefenseSim& Sim = Game->GetSim();
	const FYJJTuning& Tune = YJJData::Tune();
	bOutNew = Spell.ComboId != INDEX_NONE && !Game->IsComboKnown(Spell.ComboId);

	FString Body = Spell.Description;
	Body += TEXT("\n");
	Body += YJJLoc::Format(TEXT("Spell.Stats"), { Whole(Spell.Damage * Tune.WizardDamage(Sim.WizardLevel)), Whole(Spell.Radius), OneDecimal(Spell.Cooldown * Tune.WizardCooldown(Sim.WizardLevel)) });
	if (bOutNew)
	{
		Body += TEXT("\n");
		Body += YJJLoc::Get(TEXT("Spell.NewCombo"));
	}
	return Body;
}

void AYJJDefenseHUD::DrawRuneBag()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const float BarY = UI().BarY;
	const float BarH = UI().BarH;
	const float BagX = UI().BagX;
	const float BagIcon = UI().BagIcon;
	const float TextX = BagX + UI().BagTextIndent;

	DrawLabel(YJJLoc::Get(TEXT("Bag.Title")), TextX, BarY + UI().BagTitleDown, UI().BagTitleSize, YJJUI::DarkGray);
	int32 BagIndex = 0;
	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		const EYJJRune Rune = static_cast<EYJJRune>(Index);
		if (!Sim.HasRune(Rune))
		{
			continue;
		}

		const FVector2f Center = BagRuneCenter(BagIndex++);
		const float X = Center.X - BagIcon * 0.5f;
		const float Y = Center.Y - BagIcon * 0.5f;
		const bool bHover = IsHover(X, Y, BagIcon, BagIcon);
		const bool bPicked = SelectedRune == Rune;
		const float Grow = (bHover || bPicked) ? UI().BagHoverGrow : 0.f;
		if (bPicked)
		{
			const float Pad = UI().BagPickedPad;
			DrawPanel(X - Pad, Y - Pad, BagIcon + Pad * 2.f, BagIcon + Pad * 2.f, YJJUI::Yellow, false);
		}
		DrawSprite(RuneTex(Rune), Center - FVector2f(0.f, bPicked ? UI().BagPickedLift : 0.f), FVector2f(BagIcon + Grow, BagIcon + Grow));
		AddHit(X, Y, BagIcon, BagIcon, EAction::BagRune, Index);

		if (bHover && DragRune == EYJJRune::None)
		{
			const FYJJRuneDef& Def = YJJData::Rune(Rune);
			SetTooltip(YJJLoc::Format(TEXT("Tip.BagRune.Title"), { YJJLoc::Get(Def.NameKey), YJJLoc::Get(Def.SpellKey) }),
				YJJLoc::Get(Def.DescKey) + TEXT("\n") + YJJLoc::Get(TEXT("Tip.BagRune.Body")), YJJUI::Ink);
		}
	}

	const bool bPicking = SelectedRune != EYJJRune::None;
	DrawLabel(bPicking ? YJJLoc::Get(TEXT("Bag.PickSocket")) : YJJLoc::Get(TEXT("Bag.Hint")),
		TextX, BarY + BarH - UI().BagHintUp, UI().BagHintSize, bPicking ? YJJUI::Orange : YJJUI::DarkGray);
}

void AYJJDefenseHUD::DrawDisk(int32 Disk)
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FYJJDiskState& State = Sim.Disks[Disk];
	const FVector2f Center = DiskCenter(Disk);
	const float DiskSize = UI().DiskSize;
	const float SocketSize = UI().SocketSize;
	const float Half = DiskSize * 0.5f;
	const bool bReady = Sim.CanCast(Disk);
	const bool bHover = IsHover(Center.X - Half, Center.Y - Half, DiskSize, DiskSize);
	const FLinearColor ElementColor = State.Spell.IsValid() ? YJJData::ElementColor(State.Spell.Elements[0]) : YJJUI::Gray;
	const FString SpellName = State.Spell.IsValid() ? State.Spell.Name : YJJLoc::Get(TEXT("Disk.Empty"));

	if (ArmedDisk == Disk)
	{
		const float Pulse = UI().DiskArmedGlow + UI().DiskArmedPulse * FMath::Sin(UITime * UI().DiskArmedPulseSpeed);
		DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * Pulse * UI().DiskArmedScale, YJJUI::Yellow);
	}
	else if (bReady)
	{
		DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * UI().DiskReadyGlow, YJJUI::WithAlpha(ElementColor, UI().DiskReadyGlowAlpha));
	}

	// 준비되면 원반이 천천히 돈다
	const float Spin = bReady ? UITime * UI().DiskSpinSpeed + Disk * UI().DiskSpinPhase : 0.f;
	const float Lift = (bHover && bReady) ? UI().DiskHoverLift : 0.f;
	DrawSprite(DiskTexture, Center - FVector2f(0.f, Lift), FVector2f(DiskSize, DiskSize), bReady ? FLinearColor::White : YJJUI::DiskCooling, Spin);
	AddHit(Center.X - Half, Center.Y - Half, DiskSize, DiskSize, EAction::Disk, Disk);

	const bool bCanDrop = SelectedRune != EYJJRune::None || DragRune != EYJJRune::None;
	for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
	{
		const FVector2f Slot = SocketCenter(Disk, Socket) - FVector2f(0.f, Lift);
		const float X = Slot.X - SocketSize * 0.5f;
		const float Y = Slot.Y - SocketSize * 0.5f;
		const EYJJRune Rune = State.Sockets[Socket];

		if (bCanDrop)
		{
			const float Pulse = IsHover(X, Y, SocketSize, SocketSize) ? UI().SocketDropHover
				: UI().SocketDropGlow + UI().SocketDropPulse * FMath::Sin(UITime * UI().SocketDropPulseSpeed);
			const float Glow = SocketSize * Pulse;
			DrawPanel(Slot.X - Glow * 0.5f, Slot.Y - Glow * 0.5f, Glow, Glow, YJJUI::Yellow, false);
		}
		DrawSprite(Rune == EYJJRune::None ? SocketTexture : RuneTex(Rune), Slot, FVector2f(SocketSize, SocketSize), bReady ? FLinearColor::White : YJJUI::SocketCooling);
		AddHit(X, Y, SocketSize, SocketSize, EAction::Socket, Disk, Socket);
	}

	if (State.Cooldown > 0.f)
	{
		DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * UI().DiskCooldownCover, YJJUI::WithAlpha(YJJUI::Ink, UI().DiskCooldownAlpha));
		DrawLabel(OneDecimal(State.Cooldown), Center.X, Center.Y - UI().DiskCooldownTextUp, UI().DiskCooldownSize, FLinearColor::White, EAlign::Center, UI().DiskCooldownOutline);
	}

	// 단축키 딱지, 스펠 이름
	const float Key = UI().DiskKeySize;
	const float KeyX = Center.X - Half - UI().DiskKeyOutX;
	const float KeyY = Center.Y - Half - UI().DiskKeyOutY;
	DrawPanel(KeyX, KeyY, Key, Key, bReady ? YJJUI::Yellow : YJJUI::Gray);
	DrawLabel(FString::FromInt(Disk + 1), KeyX + Key * 0.5f, KeyY + Key * 0.5f - UI().DiskKeyTextUp, UI().DiskKeyTextSize, YJJUI::Ink, EAlign::Center);
	DrawLabel(SpellName, Center.X + UI().DiskNameShift, UI().BarY + UI().DiskNameDown, UI().DiskNameSize, YJJUI::Ink, EAlign::Center, 0.f, UI().DiskNameMaxW);

	if (bHover && DragRune == EYJJRune::None)
	{
		bool bNew = false;
		const FString Body = DescribeSpell(State.Spell, bNew);
		SetTooltip(SpellName, Body + TEXT("\n") + YJJLoc::Get(TEXT("Tip.Disk.Controls")), bNew ? YJJUI::Orange : YJJUI::Ink);
	}
}

void AYJJDefenseHUD::DrawWizardUpgrade()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FYJJTuning& Tune = YJJData::Tune();
	const float X = UI().BarX + UI().BarW - UI().WizardButtonRight;
	const float Y = UI().BarY + UI().WizardButtonDown;
	const float W = UI().WizardButtonW;
	const float H = UI().WizardButtonH;
	const float Outline = UI().ButtonTextOutline;
	const int32 Cost = Sim.WizardUpgradeCost();
	const bool bMaxed = Cost == 0;
	const bool bCanUpgrade = !bMaxed && Sim.Gold >= Cost;

	const bool bHover = DrawButton(X, Y, W, H, bMaxed ? YJJUI::Gray : YJJUI::Purple, EAction::UpgradeWizard, 0, 0, bCanUpgrade || bMaxed);
	const float Lift = (bHover && bCanUpgrade) ? UI().ButtonHoverLift : 0.f;
	const float Wand = UI().WizardWandSize;
	const float Coin = UI().WizardCoinSize;
	DrawSprite(WandIcon, FVector2f(X + UI().WizardWandX, Y + UI().WizardWandY - Lift), FVector2f(Wand, Wand));
	DrawLabel(YJJLoc::Format(TEXT("Wizard.Level"), { Sim.WizardLevel + 1 }), X + UI().WizardLevelX, Y + UI().WizardLevelY - Lift, UI().WizardLevelSize, FLinearColor::White, EAlign::Left, Outline, W - UI().WizardLevelX - UI().TextFitPad);
	if (!bMaxed)
	{
		DrawSprite(CoinIcon, FVector2f(X + UI().WizardCoinX, Y + UI().WizardCoinY - Lift), FVector2f(Coin, Coin));
		DrawLabel(FString::FromInt(Cost), X + UI().WizardCostX, Y + UI().WizardCostY - Lift, UI().WizardCostSize, Sim.Gold >= Cost ? FLinearColor::White : YJJUI::CantAfford, EAlign::Left, Outline);
	}
	else
	{
		DrawLabel(YJJLoc::Get(TEXT("Wizard.Max")), X + W * 0.5f, Y + UI().WizardCostY, UI().WizardCostSize, FLinearColor::White, EAlign::Center, Outline);
	}

	if (bHover)
	{
		const FString Body = bMaxed ? YJJLoc::Get(TEXT("Tip.Wizard.Maxed"))
			: YJJLoc::Format(TEXT("Tip.Wizard.Body"), { OneDecimal(Tune.WizardDamage(Sim.WizardLevel)), OneDecimal(Tune.WizardDamage(Sim.WizardLevel + 1)), Percent(1.f - Tune.WizardCooldown(Sim.WizardLevel + 1)) });
		SetTooltip(YJJLoc::Get(TEXT("Tip.Wizard.Title")), Body, YJJUI::Ink);
	}
}

void AYJJDefenseHUD::DrawSpellBar()
{
	const float BarX = UI().BarX;
	const float BarY = UI().BarY;
	const float BarW = UI().BarW;
	const float BarH = UI().BarH;
	DrawPanel(BarX, BarY, BarW, BarH, YJJUI::WithAlpha(YJJUI::Cream, UI().BarAlpha));
	AddHit(BarX, BarY, BarW, BarH, EAction::Blocker);

	DrawRuneBag();
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		DrawDisk(Disk);
	}
	DrawWizardUpgrade();

	// 끌고 있는 룬
	if (DragRune != EYJJRune::None && FVector2f::Distance(Mouse, DragStart) > UI().DragThreshold)
	{
		const float Size = UI().DragIconSize;
		DrawSprite(RuneTex(DragRune), Mouse, FVector2f(Size, Size), YJJUI::WithAlpha(FLinearColor::White, UI().DragIconAlpha), FMath::Sin(UITime * UI().DragWobbleSpeed) * UI().DragWobble);
	}
}

FString AYJJDefenseHUD::DescribeEnemy(EYJJEnemy Enemy) const
{
	const FString Traits = YJJData::EnemyTraits(Enemy);
	const FString& Name = YJJLoc::Get(YJJData::Enemy(Enemy).NameKey);
	return Traits.IsEmpty() ? Name : YJJLoc::Format(TEXT("Enemy.WithTraits"), { Name, Traits });
}

FString AYJJDefenseHUD::DescribeWave(int32 WaveIndex) const
{
	if (WaveIndex < 0 || WaveIndex >= YJJData::NumWaves())
	{
		return FString();
	}

	int32 Counts[static_cast<int32>(EYJJEnemy::Count)] = {};
	for (const FYJJWaveGroup& Group : YJJData::Wave(WaveIndex))
	{
		Counts[static_cast<int32>(Group.Type)] += Group.Count;
	}

	TArray<FString> Parts;
	for (int32 Type = 0; Type < static_cast<int32>(EYJJEnemy::Count); ++Type)
	{
		if (Counts[Type] > 0)
		{
			const EYJJEnemy Enemy = static_cast<EYJJEnemy>(Type);
			const FString Traits = YJJData::EnemyTraits(Enemy);
			const FString& Name = YJJLoc::Get(YJJData::Enemy(Enemy).NameKey);
			Parts.Add(Traits.IsEmpty()
				? YJJLoc::Format(TEXT("Wave.Group"), { Name, Counts[Type] })
				: YJJLoc::Format(TEXT("Wave.GroupTraits"), { Name, Counts[Type], Traits }));
		}
	}
	return FString::Join(Parts, TEXT("\n"));
}

void AYJJDefenseHUD::DrawWaveButton()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const float X = UI().WaveButtonX;
	const float Y = UI().WaveButtonY;
	const float W = UI().WaveButtonW;
	const float H = UI().WaveButtonH;
	const float CenterX = X + W * 0.5f;
	const float BigX = X + UI().WaveBigTextX;
	// 해골 아이콘 오른쪽 칸에 들어가는 너비
	const float BigTextW = UI().WaveBigTextW;
	const float Outline = UI().ButtonTextOutline;
	const float BigOutline = UI().WaveBigOutline;
	const int32 NextWave = Sim.WaveIndex + 1;
	const FString EnemiesLeft = YJJLoc::Format(TEXT("Wave.EnemiesLeft"), { Sim.Enemies.Num() });

	if (NextWave >= YJJData::NumWaves())
	{
		DrawPanel(X, Y, W, H, YJJUI::Gray);
		DrawLabel(YJJLoc::Get(TEXT("Wave.Last")), CenterX, Y + UI().WaveTopTextY, UI().WaveMidSize, FLinearColor::White, EAlign::Center, Outline, W - UI().TextFitPad);
		DrawLabel(EnemiesLeft, CenterX, Y + UI().WaveLastLeftY, UI().WaveSmallSize, FLinearColor::White, EAlign::Center, Outline, W - UI().TextFitPad);
		return;
	}

	const bool bCanCall = Sim.CanCallWave();
	const bool bFirst = Sim.WaveIndex < 0;
	// 첫 웨이브를 기다리는 동안은 눈에 띄게 들썩인다
	const float Bounce = bFirst ? FMath::Abs(FMath::Sin(UITime * UI().WaveBounceSpeed)) * UI().WaveBounce : 0.f;
	const bool bHover = DrawButton(X, Y - Bounce, W, H, bFirst ? YJJUI::Green : YJJUI::Orange, EAction::CallWave, 0, 0, bCanCall);
	const float Lift = Bounce + ((bHover && bCanCall) ? UI().ButtonHoverLift : 0.f);
	const float TopY = Y + UI().WaveTopTextY;
	const float BottomY = Y + UI().WaveBottomTextY;

	const float Skull = UI().WaveSkullSize;
	DrawSprite(SkullIcon, FVector2f(X + UI().WaveSkullX, Y + UI().WaveSkullY - Lift), FVector2f(Skull, Skull));
	if (bFirst)
	{
		DrawLabel(YJJLoc::Get(TEXT("Wave.Go")), BigX, TopY - Lift, UI().WaveBigSize, FLinearColor::White, EAlign::Center, BigOutline, BigTextW);
		DrawLabel(YJJLoc::Get(TEXT("Wave.GoKey")), CenterX, BottomY - Lift, UI().WaveHintSize, FLinearColor::White, EAlign::Center, Outline, W - UI().TextFitPad);
	}
	else if (!bCanCall)
	{
		DrawLabel(YJJLoc::Get(TEXT("Wave.Incoming")), BigX, TopY, UI().WaveMidSize, FLinearColor::White, EAlign::Center, Outline, BigTextW);
		DrawLabel(EnemiesLeft, CenterX, Y + UI().WaveBusyLeftY, UI().WaveSmallSize, FLinearColor::White, EAlign::Center, Outline, W - UI().TextFitPad);
	}
	else
	{
		DrawLabel(YJJLoc::Format(TEXT("Wave.Timer"), { FMath::CeilToInt(Sim.NextWaveTimer) }), BigX, TopY - Lift, UI().WaveBigSize, FLinearColor::White, EAlign::Center, BigOutline, BigTextW);
		DrawLabel(YJJLoc::Format(TEXT("Wave.EarlyBonus"), { FMath::RoundToInt(Sim.NextWaveTimer * YJJData::Tune().EarlyCallGoldPerSecond) }), CenterX, BottomY - Lift, UI().WaveHintSize, FLinearColor::White, EAlign::Center, Outline, W - UI().TextFitPad);
	}

	if (bHover)
	{
		SetTooltip(YJJLoc::Format(TEXT("Tip.NextWave"), { NextWave + 1 }), DescribeWave(NextWave), YJJUI::Ink);
	}
}

FString AYJJDefenseHUD::DescribeTower(EYJJTower Type, int32 Level) const
{
	const FYJJTowerDef& Def = YJJData::Tower(Type);
	const FYJJTowerLevel& Stats = Def.Levels[FMath::Clamp(Level, 0, YJJ::MaxTowerLevel)];

	FString Body = YJJLoc::Get(Def.DescKey);
	Body += TEXT("\n");
	if (Type == EYJJTower::Barracks)
	{
		Body += YJJLoc::Format(TEXT("Tower.SoldierStats"), { Whole(Stats.SoldierHP), Whole(Stats.Damage), Percent(Stats.SoldierArmor) });
	}
	else
	{
		Body += YJJLoc::Format(TEXT("Tower.Stats"), { Whole(Stats.Damage), Def.bMagic ? YJJLoc::Get(TEXT("Tower.MagicTag")) : FString(), OneDecimal(Stats.Interval), Whole(Stats.Range) });
		if (Stats.Splash > 0.f)
		{
			Body += YJJLoc::Format(TEXT("Tower.Splash"), { Whole(Stats.Splash) });
		}
	}
	return Body;
}

void AYJJDefenseHUD::GatherMenuOptions(TArray<FMenuOption>& Options)
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FYJJTowerState& Tower = Sim.Towers[SelectedSpot];

	if (!Tower.bBuilt)
	{
		for (int32 Index = 0; Index < static_cast<int32>(EYJJTower::Count); ++Index)
		{
			const EYJJTower Type = static_cast<EYJJTower>(Index);
			if (!Sim.HasTower(Type))
			{
				continue;
			}
			const FYJJTowerDef& Def = YJJData::Tower(Type);
			UTexture2D* Icon = TowerTextures[Index][0];
			Options.Add({ EAction::BuildTower, Index, Icon, TexSize(Icon) * UI().MenuTowerIconScale, Def.Levels[0].Cost, Sim.CanBuild(SelectedSpot, Type),
				YJJLoc::Get(Def.NameKey), DescribeTower(Type, 0) });
		}
		return;
	}

	const FYJJTowerDef& Def = YJJData::Tower(Tower.Type);
	if (Tower.Level < YJJ::MaxTowerLevel)
	{
		const float Icon = UI().MenuUpIconSize;
		Options.Add({ EAction::UpgradeTower, 0, UpIcon, FVector2f(Icon, Icon), Sim.UpgradeCost(SelectedSpot), Sim.CanUpgrade(SelectedSpot),
			YJJLoc::Format(TEXT("Menu.Upgrade"), { YJJLoc::Get(Def.NameKey), Tower.Level + 1, Tower.Level + 2 }), DescribeTower(Tower.Type, Tower.Level + 1) });
	}
	if (Tower.Type == EYJJTower::Shrine)
	{
		const float Icon = UI().MenuCycleIconSize;
		Options.Add({ EAction::CycleShrine, 0, CycleIcon, FVector2f(Icon, Icon), 0, true,
			YJJLoc::Format(TEXT("Menu.Cycle.Title"), { YJJData::ElementName(Tower.ShrineElement) }),
			YJJLoc::Get(TEXT("Menu.Cycle.Body")) });
	}
	const float Icon = UI().MenuSellIconSize;
	Options.Add({ EAction::SellTower, 0, SellIcon, FVector2f(Icon, Icon), -Sim.SellValue(SelectedSpot), true, YJJLoc::Get(TEXT("Menu.Sell.Title")),
		YJJLoc::Format(TEXT("Menu.Sell.Body"), { Percent(YJJData::Tune().SellRefund) }) });
}

void AYJJDefenseHUD::DrawTowerMenu()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	HoverBuildType = INDEX_NONE;
	bHoverUpgrade = false;
	if (!Sim.Towers.IsValidIndex(SelectedSpot))
	{
		return;
	}

	const FYJJTowerState& Tower = Sim.Towers[SelectedSpot];
	const FVector2f SpotPos = YJJMap::Spots()[SelectedSpot];

	TArray<FMenuOption> Options;
	GatherMenuOptions(Options);
	if (Options.IsEmpty())
	{
		return;
	}

	const float ButtonW = UI().MenuButtonW;
	const float ButtonH = UI().MenuButtonH;
	const float Coin = UI().MenuCoinSize;

	if (Tower.bBuilt)
	{
		const FVector2f First = TowerMenuButtonCenter(SpotPos, Options.Num(), 0, true);
		const FVector2f Last = TowerMenuButtonCenter(SpotPos, Options.Num(), Options.Num() - 1, true);
		const FString Header = YJJLoc::Format(TEXT("Menu.Header"), { YJJLoc::Get(YJJData::Tower(Tower.Type).NameKey), Tower.Level + 1 });
		DrawLabel(Header, (First.X + Last.X) * 0.5f, First.Y - ButtonH * 0.5f - UI().MenuHeaderUp, UI().MenuHeaderSize, FLinearColor::White, EAlign::Center, UI().MenuHeaderOutline);
	}

	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const FMenuOption& Option = Options[Index];
		const FVector2f Center = TowerMenuButtonCenter(SpotPos, Options.Num(), Index, Tower.bBuilt);
		const float X = Center.X - ButtonW * 0.5f;
		const float Y = Center.Y - ButtonH * 0.5f;
		const bool bHover = DrawButton(X, Y, ButtonW, ButtonH, FLinearColor::White, Option.Action, SelectedSpot, Option.B, Option.bEnabled);
		const float Lift = (bHover && Option.bEnabled) ? UI().ButtonHoverLift : 0.f;
		const FLinearColor IconTint = Option.bEnabled ? FLinearColor::White : YJJUI::IconDisabled;

		DrawSprite(Option.Icon, FVector2f(Center.X, Y + UI().MenuIconY - Lift), Option.IconSize, IconTint);
		if (Option.Cost != 0)
		{
			DrawSprite(CoinIcon, FVector2f(X + UI().MenuCoinX, Y + UI().MenuCoinY - Lift), FVector2f(Coin, Coin));
			const FString CostText = Option.Cost > 0 ? FString::FromInt(Option.Cost) : YJJLoc::Format(TEXT("Fx.Gold"), { -Option.Cost });
			const FLinearColor CostColor = Option.Cost < 0 ? YJJUI::Gain : (Option.bEnabled ? YJJUI::Ink : YJJUI::Red);
			DrawLabel(CostText, X + UI().MenuCostX, Y + UI().MenuCostY - Lift, UI().MenuCostSize, CostColor);
		}
		else if (Option.Action == EAction::CycleShrine)
		{
			const float Rune = UI().MenuShrineRuneSize;
			DrawSprite(RuneTex(YJJData::ElementRune(Tower.ShrineElement)), FVector2f(Center.X, Y + UI().MenuCostY - Lift), FVector2f(Rune, Rune));
		}

		if (bHover)
		{
			SetTooltip(Option.Title, Option.Body, YJJUI::Ink);
			if (Option.Action == EAction::BuildTower)
			{
				HoverBuildType = Option.B;
			}
			bHoverUpgrade = Option.Action == EAction::UpgradeTower;
		}
	}
}

void AYJJDefenseHUD::DrawTargeting()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	if (ArmedDisk == INDEX_NONE)
	{
		return;
	}
	if (!Sim.CanCast(ArmedDisk))
	{
		ArmedDisk = INDEX_NONE;
		return;
	}

	// 스펠이 닿을 범위와, 마법사에서 그곳까지의 점선
	const FYJJSpellSpec& Spell = Sim.Disks[ArmedDisk].Spell;
	const FLinearColor Color = YJJData::ElementColor(Spell.Elements[0]);
	const FVector2f From = YJJMap::WizardPos() + FVector2f(UI().TargetFromX, UI().TargetFromY);
	const int32 Dots = FMath::RoundToInt(UI().TargetDots);
	const float Dot = UI().TargetDotSize;
	for (int32 Index = 1; Index < Dots; ++Index)
	{
		const float T = FMath::Fmod(static_cast<float>(Index) / Dots + UITime * UI().TargetDotSpeed, 1.f);
		FVector2f Point = FMath::Lerp(From, Mouse, T);
		Point.Y -= UI().TargetArc * UI().ArcPeakScale * T * (1.f - T);
		DrawSprite(DiscTexture, Point, FVector2f(Dot, Dot), YJJUI::WithAlpha(Color, UI().TargetDotAlpha));
	}
	DrawEllipseRange(Mouse, Spell.Radius, Color);
	if (Spell.Reaction != EYJJReaction::None)
	{
		const FYJJReactionDef& Reaction = YJJData::Reaction(Spell.Reaction);
		DrawLabel(YJJLoc::Get(Reaction.NameKey), Mouse.X, Mouse.Y - Spell.Radius / YJJ::YStretch - UI().TargetReactionUp, UI().TargetReactionSize, Reaction.Color, EAlign::Center, UI().ButtonTextOutline);
	}
}

void AYJJDefenseHUD::DrawBanner(float DeltaSeconds)
{
	if (BannerTime <= 0.f)
	{
		return;
	}
	BannerTime -= DeltaSeconds;

	const float Alpha = FMath::Clamp(BannerTime * UI().BannerFadeSpeed, 0.f, 1.f);
	DrawBox(0.f, UI().BannerY, YJJ::ViewW, UI().BannerH, YJJUI::WithAlpha(YJJUI::Ink, UI().BannerDim * Alpha));
	DrawLabel(BannerText, YJJ::ViewW * 0.5f, UI().BannerTextY, UI().BannerTextSize, YJJUI::WithAlpha(BannerColor, Alpha), EAlign::Center, UI().BannerOutline);
}

// ---------------------------------------------------------------------------------------------
// 툴팁

void AYJJDefenseHUD::SetTooltip(const FString& Title, const FString& Body, const FLinearColor& TitleColor)
{
	TooltipTitle = Title;
	TooltipBody = Body;
	TooltipColor = TitleColor;
	bHasTooltip = true;
}

void AYJJDefenseHUD::DrawTooltip()
{
	if (!bHasTooltip)
	{
		return;
	}

	TArray<FString> Lines;
	TooltipBody.ParseIntoArrayLines(Lines, false);
	int32 MaxChars = TooltipTitle.Len() + FMath::RoundToInt(UI().TooltipTitleExtraChars);
	for (const FString& Line : Lines)
	{
		MaxChars = FMath::Max(MaxChars, Line.Len());
	}

	// 한글 한 글자를 대략 글자 크기만큼으로 잡는다 (공백·숫자가 섞여 조금 넉넉하다)
	const float Margin = UI().TooltipScreenMargin;
	const float Offset = UI().TooltipMouseOffset;
	const float W = FMath::Clamp(MaxChars * UI().TooltipCharWidth + UI().TooltipPadW, UI().TooltipMinW, UI().TooltipMaxW);
	const float LineHeight = UI().TooltipLineHeight;
	const float H = UI().TooltipPadH + Lines.Num() * LineHeight;
	float X = Mouse.X + Offset;
	float Y = Mouse.Y - H - UI().TooltipAbove;
	if (X + W > YJJ::ViewW - Margin)
	{
		X = Mouse.X - W - Offset;
	}
	X = FMath::Clamp(X, Margin, YJJ::ViewW - W - Margin);
	if (Y < Margin)
	{
		Y = Mouse.Y + UI().TooltipBelow;
	}

	const float TextX = X + UI().TooltipTextX;
	DrawPanel(X, Y, W, H, YJJUI::WithAlpha(FLinearColor::White, UI().TooltipAlpha));
	DrawLabel(TooltipTitle, TextX, Y + UI().TooltipTitleY, UI().TooltipTitleSize, TooltipColor);
	DrawParagraph(TooltipBody, TextX, Y + UI().TooltipBodyY, UI().TooltipBodySize, YJJUI::DarkGray, LineHeight);
}
