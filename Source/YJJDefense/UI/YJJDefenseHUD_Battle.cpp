// 전투 화면: 맵, 병졸, 몬스터, 투사체, 그리고 전투 UI (상단 정보, 스펠 바, 타워 메뉴)

#include "UI/YJJDefenseHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Game/YJJGameData.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefenseGameMode.h"

void AYJJDefenseHUD::DrawDimmedMap(float Darkness, int32 Map)
{
	Map = Map == INDEX_NONE ? YJJMap::GetActive() : Map;
	DrawSprite(MapTextures[FMath::Clamp(Map, 0, static_cast<int32>(UE_ARRAY_COUNT(MapTextures)) - 1)], FVector2f(YJJ::ViewW * 0.5f, YJJ::ViewH * 0.5f), FVector2f(YJJ::ViewW, YJJ::ViewH));
	if (Darkness > 0.f)
	{
		DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, Darkness));
	}
}

void AYJJDefenseHUD::DrawBattle(bool bInteractive)
{
	const FYJJDefenseSim& Sim = Game->GetSim();

	// 이펙트는 게임 배속을 따라가고, 일시정지하면 멈춘다
	const bool bRunning = bInteractive && !Game->IsBattlePaused();
	const float FxDelta = bRunning ? GetWorld()->GetDeltaSeconds() * FMath::Min(Game->GetSpeed(), 2.f) : 0.f;

	DrawDimmedMap(0.f);
	DrawZones();

	// 고른 자리의 사거리 (타워보다 먼저 그려 바닥에 깔리게)
	if (bInteractive && Sim.Towers.IsValidIndex(SelectedSpot))
	{
		const FYJJTowerState& Tower = Sim.Towers[SelectedSpot];
		const FVector2f SpotPos = YJJMap::Spots()[SelectedSpot];
		if (Tower.bBuilt)
		{
			const int32 Level = bHoverUpgrade ? FMath::Min(Tower.Level + 1, YJJ::MaxTowerLevel) : Tower.Level;
			DrawEllipseRange(SpotPos, YJJData::Tower(Tower.Type).Levels[Level].Range, bHoverUpgrade ? YJJUI::Green : FLinearColor::White);
			if (Tower.Type == EYJJTower::Barracks)
			{
				DrawEllipseRange(Tower.Rally, YJJData::SoldierEngageRange, YJJUI::Sky);
			}
		}
		else if (HoverBuildType != INDEX_NONE)
		{
			DrawEllipseRange(SpotPos, YJJData::Tower(static_cast<EYJJTower>(HoverBuildType)).Levels[0].Range, FLinearColor::White);
		}
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
		DrawBox(0.f, 0.f, YJJ::ViewW, YJJ::ViewH, YJJUI::WithAlpha(YJJUI::Ink, 0.35f));
		DrawLabel(TEXT("잠깐 멈춤"), YJJ::ViewW * 0.5f, 470.f, 84.f, FLinearColor::White, EAlign::Center, 8.f);
		DrawLabel(TEXT("P를 누르면 계속"), YJJ::ViewW * 0.5f, 550.f, 30.f, YJJUI::Cream, EAlign::Center, 5.f);
	}
}

// ---------------------------------------------------------------------------------------------
// 월드

void AYJJDefenseHUD::DrawZones()
{
	for (const FYJJZoneState& Zone : Game->GetSim().Zones)
	{
		const FLinearColor Color = YJJSpellForge::ReactionInfo(Zone.Kind).Color;
		const float Life = Zone.Time / FMath::Max(Zone.Duration, 0.01f);
		const float Fade = FMath::Clamp(Zone.Time * 5.f, 0.f, 1.f) * FMath::Clamp((1.f - Life) * 4.f, 0.f, 1.f);
		const float Pulse = 1.f + 0.04f * FMath::Sin(UITime * 5.f + Zone.Pos.X);
		const FVector2f Size(Zone.Radius * 2.f * Pulse, Zone.Radius * 2.f * Pulse / YJJ::YStretch);

		DrawSprite(DiscTexture, Zone.Pos, Size * 1.15f, YJJUI::WithAlpha(Color, 0.5f * Fade));
		DrawSprite(RingTexture, Zone.Pos, Size, YJJUI::WithAlpha(Color, 0.9f * Fade));

		// 장판 위를 도는 몽실한 덩어리들
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const float Angle = UITime * 1.6f + Index * UE_TWO_PI / 5.f + Zone.Pos.Y;
			const FVector2f Offset(FMath::Cos(Angle) * Zone.Radius * 0.55f, FMath::Sin(Angle) * Zone.Radius * 0.55f / YJJ::YStretch - 16.f);
			DrawSprite(PuffTextures[0], Zone.Pos + Offset, FVector2f(46.f, 46.f), YJJUI::WithAlpha(FMath::Lerp(Color, FLinearColor::White, 0.35f), 0.85f * Fade));
		}
	}
}

void AYJJDefenseHUD::DrawWorldSorted()
{
	const FYJJDefenseSim& Sim = Game->GetSim();

	// 아래쪽에 있는 것이 위를 덮도록 y순으로 그린다
	enum class EKind : uint8 { Spot, Soldier, Enemy, Wizard };
	struct FItem
	{
		float Y;
		EKind Kind;
		int32 Index;
	};
	TArray<FItem, TInlineAllocator<256>> Items;

	const TConstArrayView<FVector2f> Spots = YJJMap::Spots();
	for (int32 Spot = 0; Spot < Spots.Num(); ++Spot)
	{
		Items.Add({ Spots[Spot].Y + (Sim.Towers[Spot].bBuilt ? 22.f : -30.f), EKind::Spot, Spot });
	}
	for (int32 Index = 0; Index < Sim.Soldiers.Num(); ++Index)
	{
		if (Sim.Soldiers[Index].IsAlive())
		{
			Items.Add({ Sim.Soldiers[Index].Pos.Y, EKind::Soldier, Index });
		}
	}
	for (int32 Index = 0; Index < Sim.Enemies.Num(); ++Index)
	{
		const FYJJEnemyState& Enemy = Sim.Enemies[Index];
		if (Enemy.IsAlive())
		{
			Items.Add({ Enemy.Pos.Y + (YJJData::Enemy(Enemy.Type).bFlying ? 60.f : 0.f), EKind::Enemy, Index });
		}
	}
	Items.Add({ YJJMap::WizardPos().Y, EKind::Wizard, 0 });

	Items.StableSort([](const FItem& A, const FItem& B) { return A.Y < B.Y; });

	for (const FItem& Item : Items)
	{
		switch (Item.Kind)
		{
		case EKind::Spot: DrawSpot(Item.Index); break;
		case EKind::Soldier: DrawSoldier(Sim.Soldiers[Item.Index]); break;
		case EKind::Enemy: DrawEnemy(Sim.Enemies[Item.Index]); break;
		case EKind::Wizard: DrawWizard(); break;
		}
	}
}

void AYJJDefenseHUD::DrawSpot(int32 Spot)
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FYJJTowerState& Tower = Sim.Towers[Spot];
	const FVector2f SpotPos = YJJMap::Spots()[Spot];
	const bool bSelected = Spot == SelectedSpot;
	const bool bHover = FindSpotAt(Mouse) == Spot && HitAt(Mouse) == nullptr && ArmedDisk == INDEX_NONE;

	if (!Tower.bBuilt)
	{
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
		const float Pulse = 0.5f + 0.5f * FMath::Sin(UITime * 3.5f + Spot * 0.7f);
		const FLinearColor Glow = (bHover || bSelected) ? FLinearColor::White : (bAffordable ? YJJUI::Yellow : YJJUI::Hex(0xE8F6D8));
		const float GlowAlpha = (bHover || bSelected) ? 0.85f : (bAffordable ? 0.45f + 0.3f * Pulse : 0.3f);
		DrawSprite(DiscTexture, SpotPos + FVector2f(0.f, 4.f), FVector2f(210.f, 92.f) * (1.f + 0.04f * Pulse), YJJUI::WithAlpha(Glow, GlowAlpha));
		DrawSprite(DashRingTexture, SpotPos + FVector2f(0.f, 4.f), FVector2f(176.f, 74.f) * (1.f + 0.05f * Pulse),
			YJJUI::WithAlpha(Glow, (bHover || bSelected) ? 1.f : (bAffordable ? 0.6f + 0.4f * Pulse : 0.45f)));

		// 받침 윗면 중심(그림의 y=100/156)이 자리 좌표에 오게
		const FVector2f Size = TexSize(SpotTexture) * 0.5f * ((bHover || bSelected) ? 1.08f : 1.f);
		const float Hop = (bHover || bSelected) ? 3.f : 0.f;
		DrawSprite(SpotTexture, SpotPos + FVector2f(0.f, Size.Y * (0.5f - 100.f / 156.f) - Hop), Size, bSelected ? YJJUI::Hex(0xFFF3B0) : FLinearColor::White);
		return;
	}

	// 지을 때 통통 튀고, 쏠 때 살짝 움찔한다
	FVector2f Scale(1.f, 1.f);
	if (Tower.BuildAnim > 0.f)
	{
		const float T = Tower.BuildAnim / 0.35f;
		Scale = FVector2f(1.f + 0.18f * T, 1.f - 0.2f * T * FMath::Cos(T * 9.f));
	}
	else if (Tower.FireAnim > 0.f)
	{
		const float T = Tower.FireAnim / 0.25f;
		Scale = FVector2f(1.f + 0.05f * T, 1.f - 0.06f * T);
	}

	const FVector2f Feet = SpotPos + FVector2f(0.f, 24.f);
	DrawSprite(ShadowTexture, Feet + FVector2f(0.f, -6.f), FVector2f(120.f, 34.f));
	UTexture2D* Texture = TowerTextures[static_cast<int32>(Tower.Type)][Tower.Level];
	const FLinearColor Tint = (bHover || bSelected) ? YJJUI::Hex(0xFFF7D0) : FLinearColor::White;
	DrawSpriteFeet(Texture, Feet, Scale, Tint, Tower.bAimLeft && Tower.Type != EYJJTower::Barracks && Tower.Type != EYJJTower::Shrine);

	if (Tower.Type == EYJJTower::Shrine)
	{
		// 그릇 위에 떠 있는 원소 구슬
		const FVector2f Orb = Feet + FVector2f(0.f, -(21.f + 38.f * (Tower.Level + 1)) * Scale.Y - 8.f + FMath::Sin(UITime * 3.f + Spot) * 4.f);
		const FLinearColor Color = YJJData::ElementColor(Tower.ShrineElement);
		DrawSprite(DiscTexture, Orb, FVector2f(64.f, 64.f), YJJUI::WithAlpha(Color, 0.55f));
		DrawSprite(BallTex(Tower.ShrineElement), Orb, FVector2f(38.f, 38.f));
	}
}

void AYJJDefenseHUD::DrawEnemy(const FYJJEnemyState& Enemy)
{
	const FYJJEnemyDef& Def = YJJData::Enemy(Enemy.Type);
	UTexture2D* Texture = EnemyTextures[static_cast<int32>(Enemy.Type)][FMath::FloorToInt(Enemy.AnimTime * (Def.bFlying ? 7.f : 5.f)) & 1];
	const FVector2f Size = TexSize(Texture) * 0.5f;

	FVector2f Feet = Enemy.Pos + FVector2f(0.f, 6.f);
	DrawSprite(ShadowTexture, Enemy.Pos + FVector2f(0.f, 2.f), FVector2f(Size.X * 0.85f, Size.X * 0.26f));
	if (Def.bFlying)
	{
		Feet.Y -= 46.f + FMath::Sin(Enemy.AnimTime * 4.f) * 5.f;
	}

	// 상태에 따라 색이 변한다
	FLinearColor Tint = FLinearColor::White;
	if (Enemy.FreezeTime > 0.f)
	{
		Tint = YJJUI::Hex(0xA8E4FF);
	}
	else if (Enemy.PoisonTime > 0.f)
	{
		Tint = YJJUI::Hex(0xC9F29A);
	}
	else if (Enemy.SlowTime > 0.f)
	{
		Tint = YJJUI::Hex(0xC4E6FF);
	}
	if (Enemy.HitFlash > 0.f)
	{
		Tint = FMath::Lerp(Tint, YJJUI::Hex(0xFF8A7A), Enemy.HitFlash / 0.12f);
	}

	FVector2f Scale(1.f, 1.f);
	if (Enemy.HitFlash > 0.f)
	{
		const float T = Enemy.HitFlash / 0.12f;
		Scale = FVector2f(1.f + 0.12f * T, 1.f - 0.1f * T);
	}
	if (Enemy.StunTime > 0.f && Enemy.FreezeTime <= 0.f)
	{
		Feet.X += FMath::Sin(UITime * 40.f) * 2.f;
	}

	if (Enemy.VulnTime > 0.f)
	{
		DrawSprite(RingTexture, Enemy.Pos, FVector2f(Size.X * 1.1f, Size.X * 0.42f), YJJUI::WithAlpha(YJJUI::Purple, 0.9f));
	}

	DrawSpriteFeet(Texture, Feet, Scale, Tint, Enemy.Dir.X < -0.05f);

	const float HeadY = Feet.Y - Size.Y;
	if (Enemy.FreezeTime > 0.f)
	{
		DrawSprite(IceTexture, FVector2f(Feet.X, Feet.Y - Size.Y * 0.5f), FVector2f(Size.X * 1.1f, Size.Y * 1.1f));
	}
	if (Enemy.BurnTime > 0.f)
	{
		const float Flicker = 1.f + 0.15f * FMath::Sin(UITime * 22.f + Enemy.Id);
		DrawSprite(FlameTexture, FVector2f(Feet.X - Size.X * 0.2f, HeadY + 6.f), FVector2f(22.f, 26.f) * Flicker, YJJUI::Orange);
		DrawSprite(FlameTexture, FVector2f(Feet.X + Size.X * 0.22f, HeadY + 14.f), FVector2f(18.f, 21.f) * (2.f - Flicker), YJJUI::Yellow);
	}
	if (Enemy.StunTime > 0.f && Enemy.FreezeTime <= 0.f)
	{
		// 머리 위를 도는 별
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Angle = UITime * 7.f + Index * UE_TWO_PI / 3.f;
			DrawSprite(StarFxTexture, FVector2f(Feet.X + FMath::Cos(Angle) * Size.X * 0.36f, HeadY + FMath::Sin(Angle) * 6.f), FVector2f(18.f, 18.f), YJJUI::Yellow, Angle * 60.f);
		}
	}

	// 묻어 있는 원소
	float BarY = HeadY - 8.f;
	if (Enemy.Aura != EYJJElement::None)
	{
		const float Blink = Enemy.AuraTime < 1.5f ? 0.55f + 0.45f * FMath::Sin(UITime * 16.f) : 1.f;
		DrawSprite(RuneTex(YJJData::ElementRune(Enemy.Aura)), FVector2f(Feet.X, HeadY - 20.f), FVector2f(30.f, 30.f), YJJUI::WithAlpha(FLinearColor::White, Blink));
		BarY = HeadY - 42.f;
	}

	if (Enemy.HP < Enemy.MaxHP)
	{
		DrawBar(FVector2f(Feet.X, BarY), Def.bBoss ? 120.f : FMath::Max(Size.X * 0.7f, 34.f), Enemy.HP / Enemy.MaxHP, Def.bBoss ? YJJUI::Orange : YJJUI::Red);
	}
}

void AYJJDefenseHUD::DrawSoldier(const FYJJSoldierState& Soldier)
{
	const int32 Frame = Soldier.AttackAnim > 0.f ? 2 : (Soldier.bMoving ? (FMath::FloorToInt(Soldier.AnimTime * 8.f) & 1) : 0);
	UTexture2D* Texture = SoldierTextures[Frame];
	const FVector2f Size = TexSize(Texture) * 0.5f;
	const FVector2f Feet = Soldier.Pos + FVector2f(0.f, 6.f);

	DrawSprite(ShadowTexture, Soldier.Pos + FVector2f(0.f, 2.f), FVector2f(40.f, 12.f));
	const FLinearColor Tint = Soldier.HitFlash > 0.f ? YJJUI::Hex(0xFF8A7A) : FLinearColor::White;
	DrawSpriteFeet(Texture, Feet, FVector2f(1.f, 1.f), Tint, Soldier.bFacingLeft);

	if (Soldier.HP < Soldier.MaxHP)
	{
		DrawBar(FVector2f(Feet.X, Feet.Y - Size.Y - 6.f), 34.f, Soldier.HP / Soldier.MaxHP, YJJUI::Green);
	}
}

void AYJJDefenseHUD::DrawWizard()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const FVector2f Feet = YJJMap::WizardPos() + FVector2f(0.f, 8.f);
	DrawSprite(ShadowTexture, Feet + FVector2f(0.f, -6.f), FVector2f(96.f, 28.f));
	DrawSpriteFeet(WizardTextures[Sim.WizardCastAnim > 0.f ? 1 : 0], Feet);

	// 마력 레벨만큼 별
	for (int32 Index = 0; Index < Sim.WizardLevel; ++Index)
	{
		DrawSprite(StarIcon, Feet + FVector2f((Index - (Sim.WizardLevel - 1) * 0.5f) * 22.f, -172.f), FVector2f(22.f, 22.f));
	}
}

void AYJJDefenseHUD::DrawProjectiles(float DeltaSeconds)
{
	for (const FYJJProjectileState& Projectile : Game->GetSim().Projectiles)
	{
		const FVector2f Velocity = Projectile.Pos - Projectile.PrevPos;
		const float Angle = Velocity.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(Velocity.Y, Velocity.X));
		const FVector2f Pos = Projectile.Pos + FVector2f(0.f, -16.f);

		switch (Projectile.Kind)
		{
		case EYJJProjectile::Arrow:
			DrawSprite(ArrowTexture, Pos, FVector2f(34.f, 11.f), FLinearColor::White, Angle);
			break;
		case EYJJProjectile::Bomb:
			DrawSprite(BombTexture, Pos, FVector2f(30.f, 32.f), FLinearColor::White, Projectile.Time * 300.f);
			break;
		case EYJJProjectile::MagicBolt:
			DrawSprite(DiscTexture, Pos, FVector2f(44.f, 44.f), YJJUI::WithAlpha(YJJUI::Purple, 0.5f));
			DrawSprite(BoltTexture, Pos, FVector2f(30.f, 30.f), FLinearColor::White, Projectile.Time * 500.f);
			break;
		case EYJJProjectile::ShrineOrb:
			DrawSprite(DiscTexture, Pos, FVector2f(40.f, 40.f), YJJUI::WithAlpha(YJJData::ElementColor(Projectile.Element), 0.5f));
			DrawSprite(BallTex(Projectile.Element), Pos, FVector2f(26.f, 26.f));
			break;
		case EYJJProjectile::SpellBall:
		{
			const FLinearColor Color = YJJData::ElementColor(Projectile.Element);
			const bool bBig = Projectile.Spell.bAmplified || Projectile.Spell.Explode > 1;
			const float Size = (bBig ? 68.f : 50.f) * (1.f + 0.06f * FMath::Sin(UITime * 20.f));
			DrawSprite(DiscTexture, Pos, FVector2f(Size, Size) * 1.6f, YJJUI::WithAlpha(Color, 0.5f));
			DrawSprite(BallTex(Projectile.Element), Pos, FVector2f(Size, Size));

			// 두 번째 원소는 작은 구슬로 주위를 돈다
			if (Projectile.Spell.NumElements > 1)
			{
				const float Orbit = UITime * 14.f;
				const FVector2f Offset(FMath::Cos(Orbit) * Size * 0.55f, FMath::Sin(Orbit) * Size * 0.3f);
				DrawSprite(BallTex(Projectile.Element2), Pos + Offset, FVector2f(Size, Size) * 0.6f);
			}

			// 꼬리
			if (DeltaSeconds > 0.f && FxRandom.FRand() < 0.6f)
			{
				FFx& Spark = AddFx(EFx::Spark, Pos + FVector2f(FxRandom.FRandRange(-8.f, 8.f), FxRandom.FRandRange(-8.f, 8.f)), 0.3f, 16.f,
					Projectile.Spell.NumElements > 1 && FxRandom.FRand() < 0.5f ? YJJData::ElementColor(Projectile.Element2) : Color);
				Spark.Vel = -Velocity * 8.f;
			}
			break;
		}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 전투 UI

void AYJJDefenseHUD::DrawTopBar()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const float Y = 16.f;
	const float H = 58.f;

	DrawPanel(20.f, Y, 138.f, H, FLinearColor::White);
	DrawSprite(HeartIcon, FVector2f(54.f, Y + H * 0.5f - 2.f), FVector2f(38.f, 38.f));
	DrawLabel(FString::FromInt(Sim.Lives), 82.f, Y + H * 0.5f - 3.f, 34.f, Sim.Lives <= 5 ? YJJUI::Red : YJJUI::Ink);

	DrawPanel(168.f, Y, 162.f, H, FLinearColor::White);
	DrawSprite(CoinIcon, FVector2f(202.f, Y + H * 0.5f - 2.f), FVector2f(38.f, 38.f));
	DrawLabel(FString::FromInt(Sim.Gold), 230.f, Y + H * 0.5f - 3.f, 34.f, YJJUI::Ink);

	DrawPanel(340.f, Y, 236.f, H, FLinearColor::White);
	DrawSprite(FlagIcon, FVector2f(374.f, Y + H * 0.5f - 2.f), FVector2f(38.f, 38.f));
	DrawLabel(FString::Printf(TEXT("웨이브 %d/%d"), FMath::Max(Sim.WaveIndex + 1, 0), YJJData::NumWaves()), 400.f, Y + H * 0.5f - 3.f, 30.f, YJJUI::Ink);

	DrawPanel(586.f, Y, 250.f, H, YJJUI::Yellow);
	DrawLabel(FString::Printf(TEXT("%s · %d단계"), YJJMap::Name(Sim.Config.Map), Sim.Config.Tier), 711.f, Y + H * 0.5f - 3.f, 26.f, YJJUI::Ink, EAlign::Center);

	// 오른쪽: 일시정지, 배속
	const bool bPauseHover = DrawButton(1774.f, Y, 60.f, H, YJJUI::Sky, EAction::Pause);
	DrawSprite(Game->IsBattlePaused() ? PlayIcon : PauseIcon, FVector2f(1804.f, Y + H * 0.5f - 2.f - (bPauseHover ? 2.f : 0.f)), FVector2f(32.f, 32.f));
	const bool bFast = Game->GetSpeed() > 1.5f;
	const bool bSpeedHover = DrawButton(1842.f, Y, 60.f, H, bFast ? YJJUI::Orange : YJJUI::Sky, EAction::Speed);
	DrawSprite(bFast ? FastIcon : PlayIcon, FVector2f(1872.f, Y + H * 0.5f - 2.f - (bSpeedHover ? 2.f : 0.f)), FVector2f(32.f, 32.f));
	if (bPauseHover)
	{
		SetTooltip(TEXT("잠깐 멈춤 (P)"), TEXT("멈춘 동안에도 룬은 바꿔 끼울 수 있다"), YJJUI::Ink);
	}
	if (bSpeedHover)
	{
		SetTooltip(TEXT("배속 (F)"), bFast ? TEXT("지금 2배속") : TEXT("지금 1배속"), YJJUI::Ink);
	}
}

FString AYJJDefenseHUD::DescribeSpell(const FYJJSpellSpec& Spell, bool& bOutNew) const
{
	bOutNew = false;
	if (!Spell.IsValid())
	{
		return TEXT("비어 있다. 룬을 끌어다 꽂자");
	}

	const FYJJDefenseSim& Sim = Game->GetSim();
	bOutNew = Spell.ComboId != INDEX_NONE && !Game->IsComboKnown(Spell.ComboId);

	FString Body = Spell.Description;
	Body += FString::Printf(TEXT("\n피해 %.0f · 범위 %.0f · 재사용 %.1f초"),
		Spell.Damage * YJJData::WizardDamageScale[Sim.WizardLevel], Spell.Radius, Spell.Cooldown * YJJData::WizardCooldownScale[Sim.WizardLevel]);
	if (bOutNew)
	{
		Body += TEXT("\n★ 아직 안 써 본 조합! 쓰면 도감에 오른다");
	}
	return Body;
}

void AYJJDefenseHUD::DrawSpellBar()
{
	FYJJDefenseSim& Sim = Game->GetSim();
	using namespace YJJUI;

	DrawPanel(BarX, BarY, BarW, BarH, YJJUI::WithAlpha(Cream, 0.96f));
	AddHit(BarX, BarY, BarW, BarH, EAction::Blocker);

	// --- 룬 가방 ---
	DrawLabel(TEXT("룬 가방"), BagX + 2.f, BarY + 27.f, 25.f, DarkGray);
	int32 BagIndex = 0;
	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		const EYJJRune Rune = static_cast<EYJJRune>(Index);
		if (!Sim.HasRune(Rune))
		{
			continue;
		}

		const float X = BagX + BagSpacing * (BagIndex % 5);
		const float Y = BagY - 28.f + (BagIndex / 5) * BagSpacing;
		BagIndex++;

		const bool bHover = IsHover(X, Y, BagIcon, BagIcon);
		const bool bPicked = SelectedRune == Rune;
		const float Grow = (bHover || bPicked) ? 6.f : 0.f;
		if (bPicked)
		{
			DrawPanel(X - 6.f, Y - 6.f, BagIcon + 12.f, BagIcon + 12.f, Yellow, false);
		}
		DrawSprite(RuneTex(Rune), FVector2f(X + BagIcon * 0.5f, Y + BagIcon * 0.5f - (bPicked ? 3.f : 0.f)), FVector2f(BagIcon + Grow, BagIcon + Grow));
		AddHit(X, Y, BagIcon, BagIcon, EAction::BagRune, Index);

		if (bHover && DragRune == EYJJRune::None)
		{
			const FYJJRuneDef& Def = YJJData::Rune(Rune);
			SetTooltip(FString::Printf(TEXT("%s 룬 · %s"), Def.Name, Def.SpellName), FString(Def.Desc) + TEXT("\n끌어다 디스크 칸에 놓으면 조합된다"), Ink);
		}
	}
	DrawLabel(SelectedRune != EYJJRune::None ? TEXT("꽂을 칸을 누르세요") : TEXT("끌어서 디스크에 꽂기"), BagX + 2.f, BarY + BarH - 24.f, 20.f, SelectedRune != EYJJRune::None ? Orange : DarkGray);

	// --- 스펠 디스크 ---
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		const FYJJDiskState& State = Sim.Disks[Disk];
		const FVector2f Center(DiskFirstX + DiskSpacing * Disk, DiskY);
		const float Half = DiskSize * 0.5f;
		const bool bReady = Sim.CanCast(Disk);
		const bool bArmed = ArmedDisk == Disk;
		const bool bHover = IsHover(Center.X - Half, Center.Y - Half, DiskSize, DiskSize);
		const FLinearColor ElementColor = State.Spell.IsValid() ? YJJData::ElementColor(State.Spell.Elements[0]) : Gray;

		if (bArmed)
		{
			const float Pulse = 1.12f + 0.05f * FMath::Sin(UITime * 10.f);
			DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * Pulse * 1.15f, Yellow);
		}
		else if (bReady)
		{
			DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * 1.2f, YJJUI::WithAlpha(ElementColor, 0.75f));
		}

		// 준비되면 원반이 천천히 돈다
		const float Spin = bReady ? UITime * 30.f + Disk * 40.f : 0.f;
		const float Lift = (bHover && bReady) ? 3.f : 0.f;
		DrawSprite(DiskTexture, Center - FVector2f(0.f, Lift), FVector2f(DiskSize, DiskSize), bReady ? FLinearColor::White : Hex(0xC9C6D4), Spin);
		AddHit(Center.X - Half, Center.Y - Half, DiskSize, DiskSize, EAction::Disk, Disk);

		for (int32 Socket = 0; Socket < YJJ::NumSockets; ++Socket)
		{
			const FVector2f SocketCenter = Center + FVector2f((Socket == 0 ? -1.f : 1.f) * (SocketSize * 0.5f + 3.f), -Lift);
			const float X = SocketCenter.X - SocketSize * 0.5f;
			const float Y = SocketCenter.Y - SocketSize * 0.5f;
			const EYJJRune Rune = State.Sockets[Socket];
			const bool bSocketHover = IsHover(X, Y, SocketSize, SocketSize);
			const bool bCanDrop = (SelectedRune != EYJJRune::None || DragRune != EYJJRune::None);

			if (bCanDrop)
			{
				const float Pulse = bSocketHover ? 1.3f : 1.12f + 0.06f * FMath::Sin(UITime * 9.f);
				DrawPanel(SocketCenter.X - SocketSize * 0.5f * Pulse, SocketCenter.Y - SocketSize * 0.5f * Pulse, SocketSize * Pulse, SocketSize * Pulse, Yellow, false);
			}
			DrawSprite(Rune == EYJJRune::None ? SocketTexture : RuneTex(Rune), SocketCenter, FVector2f(SocketSize, SocketSize), bReady ? FLinearColor::White : Hex(0xD8D5E0));
			AddHit(X, Y, SocketSize, SocketSize, EAction::Socket, Disk, Socket);
		}

		if (State.Cooldown > 0.f)
		{
			DrawSprite(DiscTexture, Center, FVector2f(DiskSize, DiskSize) * 1.02f, YJJUI::WithAlpha(Ink, 0.5f));
			DrawLabel(FString::Printf(TEXT("%.1f"), State.Cooldown), Center.X, Center.Y - 2.f, 40.f, FLinearColor::White, EAlign::Center, 5.f);
		}

		// 단축키 딱지, 스펠 이름
		DrawPanel(Center.X - Half - 4.f, Center.Y - Half - 6.f, 34.f, 34.f, bReady ? Yellow : Gray);
		DrawLabel(FString::FromInt(Disk + 1), Center.X - Half + 13.f, Center.Y - Half + 9.f, 22.f, Ink, EAlign::Center);
		DrawLabel(State.Spell.IsValid() ? State.Spell.Name : FString(TEXT("빈 디스크")), Center.X + 12.f, BarY + 22.f, 24.f, Ink, EAlign::Center);

		if (bHover && DragRune == EYJJRune::None)
		{
			bool bNew = false;
			const FString Body = DescribeSpell(State.Spell, bNew);
			SetTooltip(State.Spell.IsValid() ? State.Spell.Name : FString(TEXT("빈 디스크")), Body + TEXT("\n좌클릭: 조준 · 칸 우클릭: 룬 빼기"), bNew ? Orange : Ink);
		}
	}

	// --- 마력 강화 ---
	const float WX = BarX + BarW - 178.f;
	const float WY = BarY + 34.f;
	const int32 Cost = Sim.WizardUpgradeCost();
	const bool bCanUpgrade = Cost > 0 && Sim.Gold >= Cost;
	const bool bWizardHover = DrawButton(WX, WY, 158.f, 112.f, Cost > 0 ? Purple : Gray, EAction::UpgradeWizard, 0, 0, bCanUpgrade || Cost == 0);
	const float WLift = (bWizardHover && bCanUpgrade) ? 2.f : 0.f;
	DrawSprite(WandIcon, FVector2f(WX + 36.f, WY + 40.f - WLift), FVector2f(44.f, 44.f));
	DrawLabel(FString::Printf(TEXT("마력 Lv%d"), Sim.WizardLevel + 1), WX + 62.f, WY + 38.f - WLift, 24.f, FLinearColor::White, EAlign::Left, 4.f);
	if (Cost > 0)
	{
		DrawSprite(CoinIcon, FVector2f(WX + 54.f, WY + 82.f - WLift), FVector2f(26.f, 26.f));
		DrawLabel(FString::FromInt(Cost), WX + 72.f, WY + 80.f - WLift, 26.f, Sim.Gold >= Cost ? FLinearColor::White : Hex(0xFFB3AB), EAlign::Left, 4.f);
	}
	else
	{
		DrawLabel(TEXT("최고"), WX + 79.f, WY + 80.f, 26.f, FLinearColor::White, EAlign::Center, 4.f);
	}
	if (bWizardHover)
	{
		SetTooltip(TEXT("마력 강화"), Cost > 0
			? FString::Printf(TEXT("모든 스펠 피해 x%.1f > x%.1f\n재사용 대기 %d%% 단축"), YJJData::WizardDamageScale[Sim.WizardLevel], YJJData::WizardDamageScale[Sim.WizardLevel + 1],
				FMath::RoundToInt((1.f - YJJData::WizardCooldownScale[Sim.WizardLevel + 1]) * 100.f))
			: FString(TEXT("더 올릴 수 없다")), Ink);
	}

	// 끌고 있는 룬
	if (DragRune != EYJJRune::None && FVector2f::Distance(Mouse, DragStart) > 14.f)
	{
		DrawSprite(RuneTex(DragRune), Mouse, FVector2f(66.f, 66.f), YJJUI::WithAlpha(FLinearColor::White, 0.92f), FMath::Sin(UITime * 12.f) * 6.f);
	}
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
			const FString Traits = YJJData::EnemyTraits(static_cast<EYJJEnemy>(Type));
			Parts.Add(Traits.IsEmpty()
				? FString::Printf(TEXT("%s x%d"), YJJData::Enemy(static_cast<EYJJEnemy>(Type)).Name, Counts[Type])
				: FString::Printf(TEXT("%s x%d  (%s)"), YJJData::Enemy(static_cast<EYJJEnemy>(Type)).Name, Counts[Type], *Traits));
		}
	}
	return FString::Join(Parts, TEXT("\n"));
}

void AYJJDefenseHUD::DrawWaveButton()
{
	const FYJJDefenseSim& Sim = Game->GetSim();
	const float X = 1694.f;
	const float Y = 954.f;
	const float W = 206.f;
	const float H = 112.f;
	const int32 NextWave = Sim.WaveIndex + 1;

	if (NextWave >= YJJData::NumWaves())
	{
		DrawPanel(X, Y, W, H, YJJUI::Gray);
		DrawLabel(TEXT("마지막 웨이브"), X + W * 0.5f, Y + 40.f, 26.f, FLinearColor::White, EAlign::Center, 4.f);
		DrawLabel(FString::Printf(TEXT("남은 적 %d"), Sim.Enemies.Num()), X + W * 0.5f, Y + 74.f, 22.f, FLinearColor::White, EAlign::Center, 4.f);
		return;
	}

	const bool bCanCall = Sim.CanCallWave();
	const bool bFirst = Sim.WaveIndex < 0;
	// 첫 웨이브를 기다리는 동안은 눈에 띄게 들썩인다
	const float Bounce = bFirst ? FMath::Abs(FMath::Sin(UITime * 4.f)) * 6.f : 0.f;
	const bool bHover = DrawButton(X, Y - Bounce, W, H, bFirst ? YJJUI::Green : YJJUI::Orange, EAction::CallWave, 0, 0, bCanCall);
	const float Lift = Bounce + ((bHover && bCanCall) ? 2.f : 0.f);

	DrawSprite(SkullIcon, FVector2f(X + 40.f, Y + 42.f - Lift), FVector2f(44.f, 44.f));
	if (bFirst)
	{
		DrawLabel(TEXT("출발!"), X + 122.f, Y + 40.f - Lift, 36.f, FLinearColor::White, EAlign::Center, 5.f);
		DrawLabel(TEXT("스페이스"), X + W * 0.5f, Y + 82.f - Lift, 20.f, FLinearColor::White, EAlign::Center, 4.f);
	}
	else if (!bCanCall)
	{
		DrawLabel(TEXT("몰려온다"), X + 122.f, Y + 40.f, 26.f, FLinearColor::White, EAlign::Center, 4.f);
		DrawLabel(FString::Printf(TEXT("남은 적 %d"), Sim.Enemies.Num()), X + W * 0.5f, Y + 80.f, 22.f, FLinearColor::White, EAlign::Center, 4.f);
	}
	else
	{
		DrawLabel(FString::Printf(TEXT("%d초"), FMath::CeilToInt(Sim.NextWaveTimer)), X + 122.f, Y + 40.f - Lift, 36.f, FLinearColor::White, EAlign::Center, 5.f);
		DrawLabel(FString::Printf(TEXT("지금 부르면 +%d"), FMath::RoundToInt(Sim.NextWaveTimer)), X + W * 0.5f, Y + 82.f - Lift, 20.f, FLinearColor::White, EAlign::Center, 4.f);
	}

	if (bHover)
	{
		SetTooltip(FString::Printf(TEXT("다음: 웨이브 %d"), NextWave + 1), DescribeWave(NextWave), YJJUI::Ink);
	}
}

FString AYJJDefenseHUD::DescribeTower(EYJJTower Type, int32 Level) const
{
	const FYJJTowerDef& Def = YJJData::Tower(Type);
	const FYJJTowerLevel& Stats = Def.Levels[FMath::Clamp(Level, 0, YJJ::MaxTowerLevel)];

	FString Body = Def.Desc;
	if (Type == EYJJTower::Barracks)
	{
		Body += FString::Printf(TEXT("\n병졸 체력 %.0f · 공격 %.0f · 방어 %d%%"), Stats.SoldierHP, Stats.Damage, FMath::RoundToInt(Stats.SoldierArmor * 100.f));
	}
	else
	{
		Body += FString::Printf(TEXT("\n피해 %.0f%s · %.1f초마다 · 사거리 %.0f"), Stats.Damage, Def.bMagic ? TEXT("(마법)") : TEXT(""), Stats.Interval, Stats.Range);
		if (Stats.Splash > 0.f)
		{
			Body += FString::Printf(TEXT(" · 폭발 %.0f"), Stats.Splash);
		}
	}
	return Body;
}

void AYJJDefenseHUD::DrawTowerMenu()
{
	FYJJDefenseSim& Sim = Game->GetSim();
	HoverBuildType = INDEX_NONE;
	bHoverUpgrade = false;
	if (!Sim.Towers.IsValidIndex(SelectedSpot))
	{
		return;
	}

	const FYJJTowerState& Tower = Sim.Towers[SelectedSpot];
	const FVector2f SpotPos = YJJMap::Spots()[SelectedSpot];

	struct FOption
	{
		EAction Action;
		int32 B;
		UTexture2D* Icon;
		FVector2f IconSize;
		int32 Cost;        // 0이면 가격 표시 없음, 음수면 받는 돈
		bool bEnabled;
		FString Title;
		FString Body;
	};
	TArray<FOption, TInlineAllocator<5>> Options;

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
			Options.Add({ EAction::BuildTower, Index, Icon, TexSize(Icon) * 0.3f, Def.Levels[0].Cost, Sim.CanBuild(SelectedSpot, Type), Def.Name, DescribeTower(Type, 0) });
		}
	}
	else
	{
		const FYJJTowerDef& Def = YJJData::Tower(Tower.Type);
		if (Tower.Level < YJJ::MaxTowerLevel)
		{
			Options.Add({ EAction::UpgradeTower, 0, UpIcon, FVector2f(48.f, 48.f), Sim.UpgradeCost(SelectedSpot), Sim.CanUpgrade(SelectedSpot),
				FString::Printf(TEXT("%s Lv%d > Lv%d"), Def.Name, Tower.Level + 1, Tower.Level + 2), DescribeTower(Tower.Type, Tower.Level + 1) });
		}
		if (Tower.Type == EYJJTower::Shrine)
		{
			Options.Add({ EAction::CycleShrine, 0, CycleIcon, FVector2f(44.f, 44.f), 0, true,
				FString::Printf(TEXT("원소 바꾸기 (지금: %s)"), YJJData::ElementName(Tower.ShrineElement)), TEXT("제단이 묻히는 원소를 바꾼다.\n스펠과 다른 원소여야 반응이 일어난다") });
		}
		Options.Add({ EAction::SellTower, 0, SellIcon, FVector2f(46.f, 46.f), -Sim.SellValue(SelectedSpot), true, TEXT("팔기"), TEXT("쓴 돈의 60%를 돌려받는다") });
	}

	const float ButtonW = 92.f;
	const float ButtonH = 108.f;
	const float Gap = 8.f;
	const float TotalW = Options.Num() * ButtonW + (Options.Num() - 1) * Gap;
	const float Top = Tower.bBuilt ? SpotPos.Y - 262.f : SpotPos.Y - 182.f;
	const float X0 = FMath::Clamp(SpotPos.X - TotalW * 0.5f, 12.f, YJJ::ViewW - TotalW - 12.f);
	const float Y0 = Top < 84.f ? SpotPos.Y + 54.f : Top;

	if (Tower.bBuilt)
	{
		const FString Header = FString::Printf(TEXT("%s Lv%d"), YJJData::Tower(Tower.Type).Name, Tower.Level + 1);
		DrawLabel(Header, X0 + TotalW * 0.5f, Y0 - 20.f, 26.f, FLinearColor::White, EAlign::Center, 5.f);
	}

	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const FOption& Option = Options[Index];
		const float X = X0 + Index * (ButtonW + Gap);
		const bool bHover = DrawButton(X, Y0, ButtonW, ButtonH, FLinearColor::White, Option.Action, SelectedSpot, Option.B, Option.bEnabled);
		const float Lift = (bHover && Option.bEnabled) ? 2.f : 0.f;
		const FLinearColor IconTint = Option.bEnabled ? FLinearColor::White : YJJUI::Hex(0x9A96A8);

		DrawSprite(Option.Icon, FVector2f(X + ButtonW * 0.5f, Y0 + 40.f - Lift), Option.IconSize, IconTint);
		if (Option.Cost != 0)
		{
			DrawSprite(CoinIcon, FVector2f(X + 24.f, Y0 + 86.f - Lift), FVector2f(22.f, 22.f));
			const FString CostText = Option.Cost > 0 ? FString::FromInt(Option.Cost) : FString::Printf(TEXT("+%d"), -Option.Cost);
			DrawLabel(CostText, X + 40.f, Y0 + 84.f - Lift, 24.f, Option.Cost < 0 ? YJJUI::Hex(0x3E9B2B) : (Option.bEnabled ? YJJUI::Ink : YJJUI::Red));
		}
		else if (Option.Action == EAction::CycleShrine)
		{
			DrawSprite(RuneTex(YJJData::ElementRune(Tower.ShrineElement)), FVector2f(X + ButtonW * 0.5f, Y0 + 84.f - Lift), FVector2f(30.f, 30.f));
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
	const FVector2f From = YJJMap::WizardPos() + FVector2f(-20.f, -128.f);
	const int32 Dots = 14;
	for (int32 Index = 1; Index < Dots; ++Index)
	{
		const float T = FMath::Fmod(static_cast<float>(Index) / Dots + UITime * 0.5f, 1.f);
		FVector2f Point = FMath::Lerp(From, Mouse, T);
		Point.Y -= 90.f * 4.f * T * (1.f - T);
		DrawSprite(DiscTexture, Point, FVector2f(16.f, 16.f), YJJUI::WithAlpha(Color, 0.9f));
	}
	DrawEllipseRange(Mouse, Spell.Radius, Color);
	if (Spell.Reaction != EYJJReaction::None)
	{
		DrawLabel(YJJSpellForge::ReactionInfo(Spell.Reaction).Name, Mouse.X, Mouse.Y - Spell.Radius / YJJ::YStretch - 22.f, 24.f, YJJSpellForge::ReactionInfo(Spell.Reaction).Color, EAlign::Center, 4.f);
	}
}

void AYJJDefenseHUD::DrawBanner(float DeltaSeconds)
{
	if (BannerTime <= 0.f)
	{
		return;
	}
	BannerTime -= DeltaSeconds;

	// 왼쪽에서 미끄러져 들어와 잠깐 머문다
	const float Alpha = FMath::Clamp(BannerTime * 3.f, 0.f, 1.f);
	DrawBox(0.f, 404.f, YJJ::ViewW, 112.f, YJJUI::WithAlpha(YJJUI::Ink, 0.45f * Alpha));
	DrawLabel(BannerText, YJJ::ViewW * 0.5f, 456.f, 78.f, YJJUI::WithAlpha(BannerColor, Alpha), EAlign::Center, 8.f);
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
	int32 MaxChars = TooltipTitle.Len() + 3;
	for (const FString& Line : Lines)
	{
		MaxChars = FMath::Max(MaxChars, Line.Len());
	}

	// 한글 한 글자를 대략 글자 크기만큼으로 잡는다 (공백·숫자가 섞여 조금 넉넉하다)
	const float W = FMath::Clamp(MaxChars * 19.f + 44.f, 240.f, 760.f);
	const float H = 62.f + Lines.Num() * 28.f;
	float X = Mouse.X + 24.f;
	float Y = Mouse.Y - H - 18.f;
	if (X + W > YJJ::ViewW - 10.f)
	{
		X = Mouse.X - W - 24.f;
	}
	X = FMath::Clamp(X, 10.f, YJJ::ViewW - W - 10.f);
	if (Y < 10.f)
	{
		Y = Mouse.Y + 30.f;
	}

	DrawPanel(X, Y, W, H, YJJUI::WithAlpha(FLinearColor::White, 0.97f));
	DrawLabel(TooltipTitle, X + 22.f, Y + 30.f, 28.f, TooltipColor);
	DrawParagraph(TooltipBody, X + 22.f, Y + 64.f, 21.f, YJJUI::DarkGray, 28.f);
}
