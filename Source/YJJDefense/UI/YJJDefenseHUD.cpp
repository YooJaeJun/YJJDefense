#include "UI/YJJDefenseHUD.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Game/YJJGameData.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefenseGameMode.h"

DEFINE_LOG_CATEGORY_STATIC(LogYJJHUD, Log, All);

// ---------------------------------------------------------------------------------------------
// 준비

UTexture2D* AYJJDefenseHUD::LoadTex(const FString& Name)
{
	const FString Path = FString::Printf(TEXT("/Game/YJJDefense/Textures/%s.%s"), *Name, *Name);
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
	if (Texture)
	{
		LoadedTextures.Add(Texture);
	}
	return Texture;
}

void AYJJDefenseHUD::BeginPlay()
{
	Super::BeginPlay();

	static const TCHAR* EnemyNames[] = { TEXT("Imp"), TEXT("Chicken"), TEXT("Potato"), TEXT("Toast"), TEXT("Mushroom"), TEXT("PuddingKing"),
		TEXT("Crab"), TEXT("Jellyfish"), TEXT("Coconut"), TEXT("CocoKid"), TEXT("KingCrab"), TEXT("Snowman"), TEXT("Snowball"), TEXT("Penguin"), TEXT("Yeti") };
	static_assert(UE_ARRAY_COUNT(EnemyNames) == static_cast<int32>(EYJJEnemy::Count));
	static const TCHAR* TowerNames[] = { TEXT("Archer"), TEXT("Barracks"), TEXT("Bombard"), TEXT("Mage"), TEXT("Shrine") };
	static const TCHAR* RuneNames[] = { TEXT("Fire"), TEXT("Ice"), TEXT("Water"), TEXT("Lightning"), TEXT("Poison"), TEXT("Wind"), TEXT("Explosion"), TEXT("Chain"), TEXT("Split") };
	static const TCHAR* BallNames[] = { TEXT("Arcane"), TEXT("Fire"), TEXT("Ice"), TEXT("Water"), TEXT("Lightning"), TEXT("Poison"), TEXT("Wind") };

	for (int32 Map = 0; Map < FMath::Min(YJJMap::NumMaps(), static_cast<int32>(UE_ARRAY_COUNT(MapTextures))); ++Map)
	{
		MapTextures[Map] = LoadTex(YJJMap::Texture(Map));
		MapThumbs[Map] = LoadTex(FString(YJJMap::Texture(Map)) + TEXT("_Thumb"));
	}
	for (int32 Type = 0; Type < UE_ARRAY_COUNT(EnemyNames); ++Type)
	{
		for (int32 Frame = 0; Frame < 2; ++Frame)
		{
			EnemyTextures[Type][Frame] = LoadTex(FString::Printf(TEXT("T_Enemy_%s_%d"), EnemyNames[Type], Frame));
		}
	}
	for (int32 Type = 0; Type < UE_ARRAY_COUNT(TowerNames); ++Type)
	{
		for (int32 Level = 0; Level <= YJJ::MaxTowerLevel; ++Level)
		{
			TowerTextures[Type][Level] = LoadTex(FString::Printf(TEXT("T_Tower_%s_%d"), TowerNames[Type], Level));
		}
	}
	for (int32 Frame = 0; Frame < 3; ++Frame)
	{
		SoldierTextures[Frame] = LoadTex(FString::Printf(TEXT("T_Soldier_%d"), Frame));
	}
	for (int32 Frame = 0; Frame < 2; ++Frame)
	{
		WizardTextures[Frame] = LoadTex(FString::Printf(TEXT("T_Wizard_%d"), Frame));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(RuneNames); ++Index)
	{
		RuneTextures[Index] = LoadTex(FString::Printf(TEXT("T_Rune_%s"), RuneNames[Index]));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(BallNames); ++Index)
	{
		BallTextures[Index] = LoadTex(FString::Printf(TEXT("T_Ball_%s"), BallNames[Index]));
	}
	for (int32 Frame = 0; Frame < 4; ++Frame)
	{
		PuffTextures[Frame] = LoadTex(FString::Printf(TEXT("T_FX_Puff%d"), Frame));
	}

	SpotTexture = LoadTex(TEXT("T_BuildSpot"));
	ArrowTexture = LoadTex(TEXT("T_Arrow"));
	BombTexture = LoadTex(TEXT("T_Bomb"));
	BoltTexture = LoadTex(TEXT("T_MagicBolt"));
	BurstTexture = LoadTex(TEXT("T_FX_Burst"));
	RingTexture = LoadTex(TEXT("T_FX_Ring"));
	DashRingTexture = LoadTex(TEXT("T_FX_DashRing"));
	DiscTexture = LoadTex(TEXT("T_FX_Disc"));
	ShadowTexture = LoadTex(TEXT("T_FX_Shadow"));
	StarFxTexture = LoadTex(TEXT("T_FX_Star"));
	IceTexture = LoadTex(TEXT("T_FX_Ice"));
	FlameTexture = LoadTex(TEXT("T_FX_Flame"));
	PanelTexture = LoadTex(TEXT("T_UI_Panel"));
	PanelFlatTexture = LoadTex(TEXT("T_UI_PanelFlat"));
	DiskTexture = LoadTex(TEXT("T_UI_Disk"));
	SocketTexture = LoadTex(TEXT("T_UI_Socket"));
	HeartIcon = LoadTex(TEXT("T_UI_Heart"));
	CoinIcon = LoadTex(TEXT("T_UI_Coin"));
	FlagIcon = LoadTex(TEXT("T_UI_Flag"));
	LockIcon = LoadTex(TEXT("T_UI_Lock"));
	StarIcon = LoadTex(TEXT("T_UI_Star"));
	StarEmptyIcon = LoadTex(TEXT("T_UI_StarEmpty"));
	UpIcon = LoadTex(TEXT("T_UI_Up"));
	SellIcon = LoadTex(TEXT("T_UI_Sell"));
	PauseIcon = LoadTex(TEXT("T_UI_Pause"));
	PlayIcon = LoadTex(TEXT("T_UI_Play"));
	FastIcon = LoadTex(TEXT("T_UI_Fast"));
	CycleIcon = LoadTex(TEXT("T_UI_Cycle"));
	WandIcon = LoadTex(TEXT("T_UI_Wand"));
	CheckIcon = LoadTex(TEXT("T_UI_Check"));
	SkullIcon = LoadTex(TEXT("T_UI_Skull"));

	FxRandom.Initialize(7);
}

void AYJJDefenseHUD::OnRunStarted()
{
	Effects.Reset();
	CancelSelection();
	SelectedRune = EYJJRune::None;
	DragRune = EYJJRune::None;
	BannerTime = 0.f;
}

// ---------------------------------------------------------------------------------------------
// 프레임

void AYJJDefenseHUD::DrawHUD()
{
	Super::DrawHUD();

	Game = GetWorld() ? GetWorld()->GetAuthGameMode<AYJJDefenseGameMode>() : nullptr;
	if (!Canvas || !Game)
	{
		return;
	}

	ViewScale = FMath::Max(FMath::Min(Canvas->ClipX / YJJ::ViewW, Canvas->ClipY / YJJ::ViewH), 0.1f);
	ViewOffset = FVector2f((Canvas->ClipX - YJJ::ViewW * ViewScale) * 0.5f, (Canvas->ClipY - YJJ::ViewH * ViewScale) * 0.5f);

	UpdateMouse();

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	UITime += DeltaSeconds;
	HitRegions.Reset();
	bHasTooltip = false;

	switch (Game->GetScreen())
	{
	case EYJJScreen::Loadout:
		DrawLoadout();
		break;
	case EYJJScreen::Battle:
		DrawBattle(true);
		break;
	case EYJJScreen::Result:
		DrawBattle(false);
		DrawResult();
		break;
	}

	DrawTooltip();

	// 화면비가 16:9가 아니면 남는 자리를 가린다
	const FLinearColor Bar = YJJUI::Ink;
	if (ViewOffset.X > 0.5f)
	{
		FCanvasTileItem Left(FVector2D(0.0, 0.0), FVector2D(ViewOffset.X, Canvas->ClipY), Bar);
		Canvas->DrawItem(Left);
		FCanvasTileItem Right(FVector2D(Canvas->ClipX - ViewOffset.X, 0.0), FVector2D(ViewOffset.X, Canvas->ClipY), Bar);
		Canvas->DrawItem(Right);
	}
	if (ViewOffset.Y > 0.5f)
	{
		FCanvasTileItem Top(FVector2D(0.0, 0.0), FVector2D(Canvas->ClipX, ViewOffset.Y), Bar);
		Canvas->DrawItem(Top);
		FCanvasTileItem Bottom(FVector2D(0.0, Canvas->ClipY - ViewOffset.Y), FVector2D(Canvas->ClipX, ViewOffset.Y), Bar);
		Canvas->DrawItem(Bottom);
	}
}

// ---------------------------------------------------------------------------------------------
// 기본 그리기

FVector2f AYJJDefenseHUD::TexSize(const UTexture2D* Texture) const
{
	return Texture ? FVector2f(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY())) : FVector2f::ZeroVector;
}

void AYJJDefenseHUD::DrawSprite(UTexture2D* Texture, const FVector2f& Center, const FVector2f& Size, const FLinearColor& Color, float Rotation, bool bFlipX)
{
	if (!Texture || !Texture->GetResource() || Color.A <= 0.f)
	{
		return;
	}

	const FVector2D ScreenSize(Size.X * ViewScale, Size.Y * ViewScale);
	const FVector2D Position(ViewOffset.X + Center.X * ViewScale - ScreenSize.X * 0.5, ViewOffset.Y + Center.Y * ViewScale - ScreenSize.Y * 0.5);
	FCanvasTileItem Item(Position, Texture->GetResource(), ScreenSize, FVector2D(bFlipX ? 1.0 : 0.0, 0.0), FVector2D(bFlipX ? 0.0 : 1.0, 1.0), Color);
	Item.BlendMode = SE_BLEND_Translucent;
	if (Rotation != 0.f)
	{
		Item.Rotation = FRotator(0.0, Rotation, 0.0);
		Item.PivotPoint = FVector2D(0.5, 0.5);
	}
	Canvas->DrawItem(Item);
}

void AYJJDefenseHUD::DrawSpriteFeet(UTexture2D* Texture, const FVector2f& Feet, const FVector2f& Scale, const FLinearColor& Color, bool bFlipX, float Rotation)
{
	const FVector2f Size = TexSize(Texture) * 0.5f * Scale;
	DrawSprite(Texture, FVector2f(Feet.X, Feet.Y - Size.Y * 0.5f), Size, Color, Rotation, bFlipX);
}

void AYJJDefenseHUD::DrawBox(float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Item(FVector2D(ViewOffset.X + X * ViewScale, ViewOffset.Y + Y * ViewScale), FVector2D(W * ViewScale, H * ViewScale), Color);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void AYJJDefenseHUD::DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color, bool bOutlined)
{
	UTexture2D* Texture = bOutlined ? PanelTexture : PanelFlatTexture;
	if (!Texture || !Texture->GetResource())
	{
		DrawBox(X, Y, W, H, Color);
		return;
	}

	// 64px 텍스처의 모서리 20px은 그대로, 가운데만 늘린다
	const float SourceCorner = 20.f / 64.f;
	const float Corner = FMath::Min(16.f, FMath::Min(W, H) * 0.5f);
	const float Xs[3] = { X, X + Corner, X + W - Corner };
	const float Ws[3] = { Corner, W - Corner * 2.f, Corner };
	const float Ys[3] = { Y, Y + Corner, Y + H - Corner };
	const float Hs[3] = { Corner, H - Corner * 2.f, Corner };
	const float Us[3] = { 0.f, SourceCorner, 1.f - SourceCorner };
	const float UWs[3] = { SourceCorner, 1.f - SourceCorner * 2.f, SourceCorner };

	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Column = 0; Column < 3; ++Column)
		{
			if (Ws[Column] <= 0.f || Hs[Row] <= 0.f)
			{
				continue;
			}
			// 조각 사이에 틈이 보이지 않게 화면 픽셀에 맞춘다
			const double X0 = FMath::RoundToDouble(ViewOffset.X + Xs[Column] * ViewScale);
			const double Y0 = FMath::RoundToDouble(ViewOffset.Y + Ys[Row] * ViewScale);
			const double X1 = FMath::RoundToDouble(ViewOffset.X + (Xs[Column] + Ws[Column]) * ViewScale);
			const double Y1 = FMath::RoundToDouble(ViewOffset.Y + (Ys[Row] + Hs[Row]) * ViewScale);
			FCanvasTileItem Item(FVector2D(X0, Y0), Texture->GetResource(), FVector2D(X1 - X0, Y1 - Y0),
				FVector2D(Us[Column], Us[Row]), FVector2D(Us[Column] + UWs[Column], Us[Row] + UWs[Row]), Color);
			Item.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Item);
		}
	}
}

void AYJJDefenseHUD::DrawStroke(const FVector2f& A, const FVector2f& B, const FLinearColor& Color, float Thickness)
{
	FCanvasLineItem Item(FVector2D(ViewOffset.X + A.X * ViewScale, ViewOffset.Y + A.Y * ViewScale), FVector2D(ViewOffset.X + B.X * ViewScale, ViewOffset.Y + B.Y * ViewScale));
	Item.SetColor(Color);
	Item.LineThickness = FMath::Max(Thickness * ViewScale, 1.f);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

TSharedPtr<const FCompositeFont> AYJJDefenseHUD::GetGameFont(const UFont* FallbackFont)
{
	if (GameFont.IsValid())
	{
		return GameFont;
	}

	// 커맨드릿에서는 TTF 임포트가 안 돼서 에셋 대신 파일을 직접 읽는다 (패키징 시 DefaultGame.ini에서 스테이징)
	const FString FontPath = FPaths::ProjectContentDir() / GameFontFile;
	if (!FPaths::FileExists(FontPath))
	{
		return nullptr;
	}

	TSharedRef<FCompositeFont> Composite = MakeShared<FCompositeFont>();
	Composite->DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"), FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	if (const FCompositeFont* Fallback = FallbackFont ? FallbackFont->GetCompositeFont() : nullptr)
	{
		Composite->FallbackTypeface.Typeface = Fallback->DefaultTypeface;
	}
	GameFont = Composite;
	return GameFont;
}

void AYJJDefenseHUD::DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align, float Outline)
{
	// FCanvasTextItem은 UFont가 있어야 그려진다. 실제 글꼴은 주아체 컴포지트
	static const UFont* BaseFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
	if (!BaseFont || Text.IsEmpty() || Color.A <= 0.f)
	{
		return;
	}

	const int32 FontSize = FMath::Max(FMath::RoundToInt(Size * ViewScale * 0.75f), 6);
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont(BaseFont);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(BaseFont, FontSize, TEXT("Bold"));
	if (Outline > 0.f)
	{
		Font.OutlineSettings.OutlineSize = FMath::Max(FMath::RoundToInt(Outline * ViewScale), 1);
		Font.OutlineSettings.OutlineColor = YJJUI::WithAlpha(YJJUI::Ink, Color.A);
	}

	FCanvasTextItem Item(FVector2D(ViewOffset.X + X * ViewScale, ViewOffset.Y + Y * ViewScale), FText::FromString(Text), Font, Color);
	Item.Font = BaseFont;
	Item.bCentreX = Align == EAlign::Center;
	Item.bCentreY = true;
	Canvas->DrawItem(Item);
}

float AYJJDefenseHUD::DrawParagraph(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, float LineHeight)
{
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	for (const FString& Line : Lines)
	{
		DrawLabel(Line, X, Y, Size, Color);
		Y += LineHeight;
	}
	return Y;
}

void AYJJDefenseHUD::DrawEllipseRange(const FVector2f& Center, float Radius, const FLinearColor& Color)
{
	if (Radius <= 0.f)
	{
		return;
	}
	const FVector2f Size(Radius * 2.f, Radius * 2.f / YJJ::YStretch);
	DrawSprite(DiscTexture, Center, Size * 1.12f, YJJUI::WithAlpha(Color, Color.A * 0.18f));
	DrawSprite(DashRingTexture, Center, Size, Color);
}

void AYJJDefenseHUD::DrawBar(const FVector2f& Center, float Width, float Fraction, const FLinearColor& Color)
{
	const float Height = 7.f;
	const float X = Center.X - Width * 0.5f;
	const float Y = Center.Y - Height * 0.5f;
	DrawBox(X - 2.f, Y - 2.f, Width + 4.f, Height + 4.f, YJJUI::Ink);
	DrawBox(X, Y, Width, Height, YJJUI::Hex(0x4A4456));
	DrawBox(X, Y, Width * FMath::Clamp(Fraction, 0.f, 1.f), Height, Color);
}

void AYJJDefenseHUD::AddHit(float X, float Y, float W, float H, EAction Action, int32 A, int32 B)
{
	HitRegions.Add({ FBox2f(FVector2f(X, Y), FVector2f(X + W, Y + H)), Action, A, B });
}

bool AYJJDefenseHUD::IsHover(float X, float Y, float W, float H) const
{
	return Mouse.X >= X && Mouse.X <= X + W && Mouse.Y >= Y && Mouse.Y <= Y + H;
}

const AYJJDefenseHUD::FHitRegion* AYJJDefenseHUD::HitAt(const FVector2f& Point) const
{
	// 나중에 그린 것이 위에 있다
	for (int32 Index = HitRegions.Num() - 1; Index >= 0; --Index)
	{
		if (HitRegions[Index].Box.IsInside(Point))
		{
			return &HitRegions[Index];
		}
	}
	return nullptr;
}

bool AYJJDefenseHUD::DrawButton(float X, float Y, float W, float H, const FLinearColor& Color, EAction Action, int32 A, int32 B, bool bEnabled)
{
	const bool bHover = IsHover(X, Y, W, H);
	FLinearColor Tint = bEnabled ? Color : YJJUI::Hex(0xB9B5C4);
	if (bHover && bEnabled)
	{
		// 살짝 밝게 + 위로 들썩
		Tint = FMath::Lerp(Tint, FLinearColor::White, 0.25f);
		Y -= 2.f;
	}
	DrawPanel(X, Y, W, H, Tint);
	AddHit(X, Y, W, H, Action, A, B);
	return bHover;
}

// ---------------------------------------------------------------------------------------------
// 이펙트

AYJJDefenseHUD::FFx& AYJJDefenseHUD::AddFx(EFx Kind, const FVector2f& Pos, float Life, float Size, const FLinearColor& Color)
{
	FFx& Fx = Effects.AddDefaulted_GetRef();
	Fx.Kind = Kind;
	Fx.Pos = Pos;
	Fx.Life = Life;
	Fx.Size = Size;
	Fx.Color = Color;
	Fx.Seed = FxRandom.RandRange(0, 9999);
	Fx.Rotation = FxRandom.FRandRange(0.f, 360.f);
	return Fx;
}

void AYJJDefenseHUD::AddText(const FVector2f& Pos, const FString& Text, float Size, const FLinearColor& Color, float Life)
{
	FFx& Fx = AddFx(EFx::Text, Pos, Life, Size, Color);
	Fx.Text = Text;
	Fx.Vel = FVector2f(0.f, -46.f);
}

void AYJJDefenseHUD::AddSparks(const FVector2f& Pos, int32 Count, const FLinearColor& Color, float Speed)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = FxRandom.FRandRange(0.f, UE_TWO_PI);
		FFx& Fx = AddFx(EFx::Spark, Pos, FxRandom.FRandRange(0.35f, 0.65f), FxRandom.FRandRange(10.f, 20.f), Color);
		Fx.Vel = FVector2f(FMath::Cos(Angle), FMath::Sin(Angle) * 0.7f - 0.5f) * Speed * FxRandom.FRandRange(0.5f, 1.2f);
	}
}

void AYJJDefenseHUD::TickAndDrawFx(float DeltaSeconds)
{
	for (int32 Index = Effects.Num() - 1; Index >= 0; --Index)
	{
		FFx& Fx = Effects[Index];
		Fx.Age += DeltaSeconds;
		if (Fx.Age >= Fx.Life)
		{
			Effects.RemoveAtSwap(Index, EAllowShrinking::No);
			continue;
		}

		const float T = Fx.Age / Fx.Life;
		switch (Fx.Kind)
		{
		case EFx::Puff:
		{
			const int32 Frame = FMath::Clamp(FMath::FloorToInt(T * 4.f), 0, 3);
			DrawSprite(PuffTextures[Frame], Fx.Pos, FVector2f(Fx.Size, Fx.Size), YJJUI::WithAlpha(Fx.Color, T > 0.75f ? (1.f - T) * 4.f : 1.f));
			break;
		}
		case EFx::Burst:
		{
			// 확 커졌다가 사라진다
			const float Scale = FMath::Lerp(0.55f, 1.15f, FMath::Sqrt(T));
			DrawSprite(BurstTexture, Fx.Pos, FVector2f(Fx.Size, Fx.Size) * Scale, YJJUI::WithAlpha(Fx.Color, T < 0.6f ? 1.f : (1.f - T) / 0.4f), Fx.Rotation);
			break;
		}
		case EFx::Ring:
		{
			const float Radius = FMath::Lerp(Fx.Size, Fx.Size2, 1.f - FMath::Square(1.f - T));
			DrawSprite(RingTexture, Fx.Pos, FVector2f(Radius * 2.f, Radius * 2.f / YJJ::YStretch), YJJUI::WithAlpha(Fx.Color, 1.f - T));
			break;
		}
		case EFx::Text:
		{
			Fx.Pos += Fx.Vel * DeltaSeconds;
			const float Punch = 1.f + 0.5f * FMath::Max(1.f - Fx.Age * 7.f, 0.f);
			const float Alpha = 1.f - FMath::Clamp((T - 0.65f) / 0.35f, 0.f, 1.f);
			DrawLabel(Fx.Text, Fx.Pos.X, Fx.Pos.Y, Fx.Size * Punch, YJJUI::WithAlpha(Fx.Color, Alpha), EAlign::Center, 4.f);
			break;
		}
		case EFx::Bolt:
		{
			// 지그재그 번개 (잠깐씩 모양이 바뀐다)
			FRandomStream Random(Fx.Seed + FMath::FloorToInt(Fx.Age * 30.f));
			const FVector2f Delta = Fx.Pos2 - Fx.Pos;
			const float Length = Delta.Size();
			const FVector2f Normal = Length > 1.f ? FVector2f(-Delta.Y, Delta.X) / Length : FVector2f::ZeroVector;
			const int32 Segments = FMath::Clamp(FMath::RoundToInt(Length / 36.f), 2, 14);
			FVector2f Previous = Fx.Pos;
			for (int32 Segment = 1; Segment <= Segments; ++Segment)
			{
				FVector2f Point = Fx.Pos + Delta * (static_cast<float>(Segment) / Segments);
				if (Segment < Segments)
				{
					Point += Normal * Random.FRandRange(-16.f, 16.f);
				}
				DrawStroke(Previous, Point, YJJUI::WithAlpha(YJJUI::Ink, 1.f - T), Fx.Size + 5.f);
				DrawStroke(Previous, Point, YJJUI::WithAlpha(Fx.Color, 1.f - T), Fx.Size);
				Previous = Point;
			}
			break;
		}
		case EFx::Spark:
		{
			Fx.Vel.Y += 260.f * DeltaSeconds;
			Fx.Pos += Fx.Vel * DeltaSeconds;
			Fx.Rotation += 240.f * DeltaSeconds;
			DrawSprite(StarFxTexture, Fx.Pos, FVector2f(Fx.Size, Fx.Size) * (1.f - T * 0.6f), YJJUI::WithAlpha(Fx.Color, 1.f - T * T), Fx.Rotation);
			break;
		}
		}
	}
}

void AYJJDefenseHUD::OnSimEvents(TConstArrayView<FYJJSimEvent> Events)
{
	const FVector2f Up(0.f, -24.f);

	for (const FYJJSimEvent& Event : Events)
	{
		switch (Event.Type)
		{
		case EYJJSimEvent::EnemyKilled:
			AddFx(EFx::Puff, Event.Pos + Up, 0.36f, 84.f, FLinearColor::White);
			AddText(Event.Pos + FVector2f(0.f, -52.f), FString::Printf(TEXT("+%d"), Event.Int), 24.f, YJJUI::Yellow, 0.8f);
			break;

		case EYJJSimEvent::EnemySplit:
			AddFx(EFx::Puff, Event.Pos + Up, 0.4f, 110.f, FLinearColor::White);
			AddText(Event.Pos + FVector2f(0.f, -70.f), FString::Printf(TEXT("%d개로 쪼개짐!"), Event.Int), 24.f, YJJUI::Orange, 0.9f);
			break;

		case EYJJSimEvent::EnemyImmune:
			AddText(Event.Pos + FVector2f(0.f, -76.f), TEXT("안 얼어!"), 22.f, YJJUI::Sky, 0.8f);
			break;

		case EYJJSimEvent::EnemyLeaked:
			AddText(Event.Pos + FVector2f(-40.f, -80.f), FString::Printf(TEXT("목숨 -%d"), Event.Int), 34.f, YJJUI::Red, 1.2f);
			AddFx(EFx::Burst, Event.Pos + Up, 0.3f, 110.f, YJJUI::Red);
			break;

		case EYJJSimEvent::TowerBuilt:
		case EYJJSimEvent::TowerUpgraded:
			AddFx(EFx::Puff, Event.Pos + FVector2f(0.f, -10.f), 0.42f, 170.f, FLinearColor::White);
			AddSparks(Event.Pos + FVector2f(0.f, -60.f), 8, YJJUI::Yellow);
			break;

		case EYJJSimEvent::TowerSold:
			AddFx(EFx::Puff, Event.Pos + FVector2f(0.f, -10.f), 0.42f, 150.f, FLinearColor::White);
			AddText(Event.Pos + FVector2f(0.f, -70.f), FString::Printf(TEXT("+%d"), Event.Int), 30.f, YJJUI::Yellow);
			break;

		case EYJJSimEvent::WizardUpgraded:
			AddSparks(Event.Pos + FVector2f(0.f, -120.f), 14, YJJUI::Yellow, 220.f);
			AddText(Event.Pos + FVector2f(0.f, -180.f), TEXT("마력 강화!"), 32.f, YJJUI::Yellow, 1.2f);
			break;

		case EYJJSimEvent::ProjectileHit:
			switch (static_cast<EYJJProjectile>(Event.Int))
			{
			case EYJJProjectile::Arrow:
				AddFx(EFx::Burst, Event.Pos + Up, 0.14f, 34.f, FLinearColor::White);
				break;
			case EYJJProjectile::MagicBolt:
				AddFx(EFx::Burst, Event.Pos + Up, 0.2f, 58.f, YJJUI::Purple);
				break;
			case EYJJProjectile::Bomb:
			{
				AddFx(EFx::Puff, Event.Pos + FVector2f(0.f, -16.f), 0.4f, Event.Value * 1.9f, YJJUI::Hex(0xFFE9C4));
				AddFx(EFx::Burst, Event.Pos + FVector2f(0.f, -16.f), 0.22f, Event.Value * 1.4f, YJJUI::Orange);
				FFx& Ring = AddFx(EFx::Ring, Event.Pos, 0.3f, Event.Value * 0.4f, YJJUI::Orange);
				Ring.Size2 = Event.Value;
				break;
			}
			case EYJJProjectile::ShrineOrb:
			{
				FFx& Ring = AddFx(EFx::Ring, Event.Pos, 0.3f, 10.f, YJJData::ElementColor(Event.Element));
				Ring.Size2 = Event.Value;
				break;
			}
			default:
				break;
			}
			break;

		case EYJJSimEvent::SpellCast:
			AddSparks(Event.Pos, 6, YJJData::ElementColor(Event.Element), 140.f);
			break;

		case EYJJSimEvent::SpellImpact:
		{
			const FLinearColor Color = YJJData::ElementColor(Event.Element);
			const float Radius = FMath::Max(Event.Value, 34.f);
			AddFx(EFx::Burst, Event.Pos + FVector2f(0.f, -14.f), 0.2f, Radius * 1.5f, Color);
			AddFx(EFx::Puff, Event.Pos + FVector2f(0.f, -14.f), 0.34f, Radius * 1.25f, FLinearColor::White);
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, 0.34f, Radius * 0.3f, Color);
			Ring.Size2 = Radius;
			AddSparks(Event.Pos + Up, 5, Color);
			if (Event.Int != 0)
			{
				AddSparks(Event.Pos + Up, 5, YJJData::ElementColor(static_cast<EYJJElement>(Event.Int)));
			}
			break;
		}

		case EYJJSimEvent::Reaction:
		{
			const FYJJReactionInfo& Info = YJJSpellForge::ReactionInfo(Event.Reaction);
			AddFx(EFx::Burst, Event.Pos + FVector2f(0.f, -20.f), 0.28f, FMath::Min(Event.Value * 1.3f, 190.f), Info.Color);
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, 0.5f, Event.Value * 0.2f, Info.Color);
			Ring.Size2 = Event.Value;
			FFx& Ring2 = AddFx(EFx::Ring, Event.Pos, 0.7f, Event.Value * 0.1f, FLinearColor::White);
			Ring2.Size2 = Event.Value * 1.15f;
			AddSparks(Event.Pos + Up, 10, Info.Color, 240.f);
			AddText(Event.Pos + FVector2f(0.f, -84.f), FString(Info.Name) + TEXT("!"), 40.f, Info.Color, 1.1f);
			break;
		}

		case EYJJSimEvent::ChainArc:
		{
			FFx& Bolt = AddFx(EFx::Bolt, Event.Pos + Up, 0.22f, 5.f, YJJData::ElementColor(Event.Element));
			Bolt.Pos2 = Event.Pos2 + Up;
			break;
		}

		case EYJJSimEvent::SkyBolt:
		{
			FFx& Bolt = AddFx(EFx::Bolt, Event.Pos2 + FVector2f(FxRandom.FRandRange(-60.f, 60.f), -520.f), 0.26f, 7.f, YJJUI::Yellow);
			Bolt.Pos2 = Event.Pos2 + FVector2f(0.f, -10.f);
			AddFx(EFx::Burst, Event.Pos2 + Up, 0.2f, 80.f, YJJUI::Yellow);
			break;
		}

		case EYJJSimEvent::SoldierDied:
			AddFx(EFx::Puff, Event.Pos + Up, 0.36f, 70.f, FLinearColor::White);
			AddText(Event.Pos + FVector2f(0.f, -56.f), TEXT("으악"), 22.f, FLinearColor::White, 0.8f);
			break;

		case EYJJSimEvent::MeleeHit:
			AddFx(EFx::Burst, Event.Pos + FVector2f(FxRandom.FRandRange(-8.f, 8.f), -22.f), 0.12f, 28.f, FLinearColor::White);
			break;

		case EYJJSimEvent::Heal:
		{
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, 0.5f, 10.f, YJJUI::Pink);
			Ring.Size2 = Event.Value;
			AddText(Event.Pos + FVector2f(0.f, -74.f), TEXT("힘내라!"), 20.f, YJJUI::Pink, 0.8f);
			break;
		}

		case EYJJSimEvent::WaveStarted:
			BannerText = Event.Int == YJJData::NumWaves() - 1 ? FString(TEXT("마지막 웨이브!")) : FString::Printf(TEXT("웨이브 %d"), Event.Int + 1);
			BannerColor = Event.Int == YJJData::NumWaves() - 1 ? YJJUI::Red : FLinearColor::White;
			BannerTime = 2.2f;
			break;

		case EYJJSimEvent::WaveBonus:
			AddText(FVector2f(1800.f, 930.f), FString::Printf(TEXT("일찍 불러서 +%d"), Event.Int), 26.f, YJJUI::Yellow, 1.4f);
			break;

		case EYJJSimEvent::Victory:
			BannerText = TEXT("막아냈다!");
			BannerColor = YJJUI::Yellow;
			BannerTime = 3.f;
			break;

		case EYJJSimEvent::Defeat:
			BannerText = TEXT("성문이 뚫렸다...");
			BannerColor = YJJUI::Red;
			BannerTime = 3.f;
			break;

		default:
			break;
		}
	}
}

// ---------------------------------------------------------------------------------------------
// 입력

void AYJJDefenseHUD::CancelSelection()
{
	SelectedSpot = INDEX_NONE;
	ArmedDisk = INDEX_NONE;
}

int32 AYJJDefenseHUD::FindSpotAt(const FVector2f& Point) const
{
	const TConstArrayView<FVector2f> Spots = YJJMap::Spots();
	for (int32 Spot = 0; Spot < Spots.Num(); ++Spot)
	{
		// 지어진 타워는 위로 길쭉하다
		const bool bBuilt = Game && Game->GetSim().Towers.IsValidIndex(Spot) && Game->GetSim().Towers[Spot].bBuilt;
		const float Top = bBuilt ? -130.f : -52.f;
		const FVector2f Delta = Point - Spots[Spot];
		if (FMath::Abs(Delta.X) <= 62.f && Delta.Y >= Top && Delta.Y <= 34.f)
		{
			return Spot;
		}
	}
	return INDEX_NONE;
}

void AYJJDefenseHUD::DoAction(const FHitRegion& Region, bool bRightButton)
{
	FYJJDefenseSim& Sim = Game->GetSim();
	UYJJSaveGame* Save = Game->GetSave();

	if (bRightButton)
	{
		// 우클릭: 칸 비우기, 그 밖에는 선택 취소
		if (Region.Action == EAction::Socket)
		{
			Sim.SetSocket(Region.A, Region.B, EYJJRune::None);
		}
		CancelSelection();
		SelectedRune = EYJJRune::None;
		return;
	}

	switch (Region.Action)
	{
	case EAction::StartRun:
		Game->StartRun();
		break;
	case EAction::ToggleRune:
		if (Save && YJJProgression::ToggleRune(*Save, static_cast<EYJJRune>(Region.A)))
		{
			Game->SaveProgress();
		}
		break;
	case EAction::ToggleTower:
		if (Save && YJJProgression::ToggleTower(*Save, static_cast<EYJJTower>(Region.A)))
		{
			Game->SaveProgress();
		}
		break;
	case EAction::TierPrev:
	case EAction::TierNext:
		if (Save)
		{
			Save->SelectedTier = FMath::Clamp(Save->SelectedTier + (Region.Action == EAction::TierNext ? 1 : -1), 1, Save->MaxTier);
			Game->SaveProgress();
		}
		break;
	case EAction::MapPrev:
	case EAction::MapNext:
		if (Save)
		{
			Save->SelectedMap = FMath::Clamp(Save->SelectedMap + (Region.Action == EAction::MapNext ? 1 : -1), 0, Save->UnlockedMaps - 1);
			Game->SaveProgress();
		}
		break;
	case EAction::ResultContinue:
		Game->ReturnToLoadout();
		break;

	case EAction::BuildTower:
		if (Sim.BuildTower(Region.A, static_cast<EYJJTower>(Region.B)))
		{
			SelectedSpot = INDEX_NONE;
		}
		break;
	case EAction::UpgradeTower:
		Sim.UpgradeTower(Region.A);
		break;
	case EAction::SellTower:
		if (Sim.SellTower(Region.A))
		{
			SelectedSpot = INDEX_NONE;
		}
		break;
	case EAction::CycleShrine:
		Sim.CycleShrineElement(Region.A);
		break;

	case EAction::Socket:
		if (SelectedRune != EYJJRune::None)
		{
			Sim.SetSocket(Region.A, Region.B, SelectedRune);
			break;
		}
		// 고른 룬이 없으면 디스크를 누른 것으로 친다
		[[fallthrough]];
	case EAction::Disk:
		SelectedSpot = INDEX_NONE;
		ArmedDisk = (ArmedDisk == Region.A || !Sim.CanCast(Region.A)) ? INDEX_NONE : Region.A;
		break;
	case EAction::BagRune:
		DragRune = static_cast<EYJJRune>(Region.A);
		DragStart = Mouse;
		SelectedRune = SelectedRune == DragRune ? EYJJRune::None : DragRune;
		ArmedDisk = INDEX_NONE;
		break;

	case EAction::CallWave:
		Sim.CallNextWave();
		break;
	case EAction::Pause:
		Game->TogglePause();
		break;
	case EAction::Speed:
		Game->ToggleSpeed();
		break;
	case EAction::UpgradeWizard:
		Sim.UpgradeWizard();
		break;
	default:
		break;
	}
}

void AYJJDefenseHUD::UpdateMouse()
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (PlayerOwner && PlayerOwner->GetMousePosition(MouseX, MouseY))
	{
		Mouse = FVector2f((MouseX - ViewOffset.X) / ViewScale, (MouseY - ViewOffset.Y) / ViewScale);
	}
}

void AYJJDefenseHUD::HandleMouseDown(bool bRightButton)
{
	if (!Game)
	{
		return;
	}
	// 마지막으로 그린 프레임 이후 마우스가 움직였을 수 있다 (움직이자마자 누르는 경우)
	UpdateMouse();
	UE_LOG(LogYJJHUD, Verbose, TEXT("[Input] mouse %s at (%.0f, %.0f) region=%d"), bRightButton ? TEXT("right") : TEXT("left"), Mouse.X, Mouse.Y,
		HitAt(Mouse) ? static_cast<int32>(HitAt(Mouse)->Action) : -1);

	if (const FHitRegion* Region = HitAt(Mouse))
	{
		const FHitRegion Copy = *Region;
		DoAction(Copy, bRightButton);
		return;
	}

	if (Game->GetScreen() != EYJJScreen::Battle)
	{
		return;
	}

	// 맵을 누른 경우
	if (bRightButton)
	{
		CancelSelection();
		SelectedRune = EYJJRune::None;
		return;
	}

	FYJJDefenseSim& Sim = Game->GetSim();
	if (ArmedDisk != INDEX_NONE)
	{
		Sim.CastSpell(ArmedDisk, Mouse);
		ArmedDisk = INDEX_NONE;
		return;
	}

	const int32 Spot = FindSpotAt(Mouse);
	SelectedSpot = Spot == SelectedSpot ? INDEX_NONE : Spot;
	SelectedRune = EYJJRune::None;
}

void AYJJDefenseHUD::HandleMouseUp()
{
	if (!Game || DragRune == EYJJRune::None)
	{
		return;
	}
	UpdateMouse();

	// 가방에서 끌어다 칸에 놓기
	if (FVector2f::Distance(Mouse, DragStart) > 14.f)
	{
		const FHitRegion* Region = HitAt(Mouse);
		if (Region && Region->Action == EAction::Socket)
		{
			Game->GetSim().SetSocket(Region->A, Region->B, DragRune);
		}
		SelectedRune = EYJJRune::None;
	}
	DragRune = EYJJRune::None;
}

void AYJJDefenseHUD::HandleKey(const FKey& Key)
{
	if (!Game)
	{
		return;
	}

	if (Game->GetScreen() == EYJJScreen::Loadout)
	{
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
		{
			Game->StartRun();
		}
		return;
	}
	if (Game->GetScreen() == EYJJScreen::Result)
	{
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
		{
			Game->ReturnToLoadout();
		}
		return;
	}

	FYJJDefenseSim& Sim = Game->GetSim();
	const FKey DiskKeys[YJJ::NumDisks] = { EKeys::One, EKeys::Two, EKeys::Three };
	for (int32 Disk = 0; Disk < YJJ::NumDisks; ++Disk)
	{
		if (Key == DiskKeys[Disk])
		{
			SelectedSpot = INDEX_NONE;
			ArmedDisk = (ArmedDisk == Disk || !Sim.CanCast(Disk)) ? INDEX_NONE : Disk;
			return;
		}
	}

	if (Key == EKeys::SpaceBar)
	{
		Sim.CallNextWave();
	}
	else if (Key == EKeys::P)
	{
		Game->TogglePause();
	}
	else if (Key == EKeys::F)
	{
		Game->ToggleSpeed();
	}
	else if (Key == EKeys::Escape)
	{
		CancelSelection();
		SelectedRune = EYJJRune::None;
	}
}
