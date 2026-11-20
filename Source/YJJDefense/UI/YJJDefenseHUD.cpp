#include "UI/YJJDefenseHUD.h"
#include "CanvasItem.h"
#include "Data/YJJLoc.h"
#include "Data/YJJUILayout.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/YJJGameData.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rendering/SlateRenderer.h"
#include "UI/YJJUIStyle.h"
#include "YJJDefense.h"
#include "YJJDefenseGameMode.h"

const FYJJUILayout& AYJJDefenseHUD::UI()
{
	return YJJData::Layout();
}

// ---------------------------------------------------------------------------------------------
// 준비

UTexture2D* AYJJDefenseHUD::LoadTex(const FString& Name)
{
	const FString Path = FString::Printf(TEXT("%s/%s.%s"), TextureFolder, *Name, *Name);
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

	BaseFont = LoadObject<UFont>(nullptr, BaseFontPath);

	// 그림 이름은 데이터 표의 Art·Texture 열
	for (int32 Map = 0; Map < YJJMap::NumMaps(); ++Map)
	{
		const FString& Texture = YJJMap::Def(Map).Texture;
		MapTextures.Add(LoadTex(Texture));
		MapThumbs.Add(LoadTex(Texture + ThumbSuffix));
	}
	for (int32 Type = 0; Type < static_cast<int32>(EYJJEnemy::Count); ++Type)
	{
		for (int32 Frame = 0; Frame < EnemyFrames; ++Frame)
		{
			EnemyTextures[Type][Frame] = LoadTex(FString::Printf(TEXT("T_Enemy_%s_%d"), *YJJData::Enemy(static_cast<EYJJEnemy>(Type)).Art, Frame));
		}
	}
	for (int32 Type = 0; Type < static_cast<int32>(EYJJTower::Count); ++Type)
	{
		for (int32 Level = 0; Level < YJJ::NumTowerLevels; ++Level)
		{
			TowerTextures[Type][Level] = LoadTex(FString::Printf(TEXT("T_Tower_%s_%d"), *YJJData::Tower(static_cast<EYJJTower>(Type)).Art, Level));
		}
	}
	for (int32 Frame = 0; Frame < SoldierFrames; ++Frame)
	{
		SoldierTextures[Frame] = LoadTex(FString::Printf(TEXT("T_Soldier_%d"), Frame));
	}
	for (int32 Frame = 0; Frame < WizardFrames; ++Frame)
	{
		WizardTextures[Frame] = LoadTex(FString::Printf(TEXT("T_Wizard_%d"), Frame));
	}
	for (int32 Index = 0; Index < static_cast<int32>(EYJJRune::Count); ++Index)
	{
		RuneTextures[Index] = LoadTex(FString::Printf(TEXT("T_Rune_%s"), *YJJData::Rune(static_cast<EYJJRune>(Index)).Art));
	}
	for (int32 Index = 0; Index < static_cast<int32>(EYJJElement::Count); ++Index)
	{
		const EYJJElement Element = static_cast<EYJJElement>(Index);
		const FString Art = Element == EYJJElement::None ? FString(ArcaneBallArt) : YJJData::Rune(YJJData::ElementRune(Element)).Art;
		BallTextures[Index] = LoadTex(FString::Printf(TEXT("T_Ball_%s"), *Art));
	}
	for (int32 Frame = 0; Frame < PuffFrames; ++Frame)
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

	FxRandom.Initialize(FxRandomSeed);
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

	ViewScale = FMath::Max(FMath::Min(Canvas->ClipX / YJJ::ViewW, Canvas->ClipY / YJJ::ViewH), UI().MinViewScale);
	ViewOffset = FVector2f(Canvas->ClipX - YJJ::ViewW * ViewScale, Canvas->ClipY - YJJ::ViewH * ViewScale) * 0.5f;

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
	if (ViewOffset.X > 0.f)
	{
		FCanvasTileItem Left(FVector2D(0.0, 0.0), FVector2D(ViewOffset.X, Canvas->ClipY), Bar);
		Canvas->DrawItem(Left);
		FCanvasTileItem Right(FVector2D(Canvas->ClipX - ViewOffset.X, 0.0), FVector2D(ViewOffset.X, Canvas->ClipY), Bar);
		Canvas->DrawItem(Right);
	}
	if (ViewOffset.Y > 0.f)
	{
		FCanvasTileItem Top(FVector2D(0.0, 0.0), FVector2D(Canvas->ClipX, ViewOffset.Y), Bar);
		Canvas->DrawItem(Top);
		FCanvasTileItem Bottom(FVector2D(0.0, Canvas->ClipY - ViewOffset.Y), FVector2D(Canvas->ClipX, ViewOffset.Y), Bar);
		Canvas->DrawItem(Bottom);
	}
}

// ---------------------------------------------------------------------------------------------
// 기본 그리기

FString AYJJDefenseHUD::Whole(float Value)
{
	return FString::FromInt(FMath::RoundToInt(Value));
}

FString AYJJDefenseHUD::OneDecimal(float Value)
{
	return FString::Printf(TEXT("%.1f"), Value);
}

int32 AYJJDefenseHUD::Percent(float Ratio)
{
	return FMath::RoundToInt(Ratio * PercentScale);
}

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
	const FVector2f Size = TexSize(Texture) * UI().SpriteScale * Scale;
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

	// 9칸으로 나눠 모서리는 그대로, 가운데만 늘린다
	constexpr int32 Cells = 3;
	const float SourceCorner = UI().PanelSourceCorner / TexSize(Texture).X;
	const float Corner = FMath::Min(UI().PanelCorner, FMath::Min(W, H) * 0.5f);
	const float Xs[Cells] = { X, X + Corner, X + W - Corner };
	const float Ws[Cells] = { Corner, W - Corner * 2.f, Corner };
	const float Ys[Cells] = { Y, Y + Corner, Y + H - Corner };
	const float Hs[Cells] = { Corner, H - Corner * 2.f, Corner };
	const float Us[Cells] = { 0.f, SourceCorner, 1.f - SourceCorner };
	const float UWs[Cells] = { SourceCorner, 1.f - SourceCorner * 2.f, SourceCorner };

	for (int32 Row = 0; Row < Cells; ++Row)
	{
		for (int32 Column = 0; Column < Cells; ++Column)
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
	Composite->DefaultTypeface.Fonts.Add(FTypefaceEntry(GameFontFace, FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad));
	if (const FCompositeFont* Fallback = FallbackFont ? FallbackFont->GetCompositeFont() : nullptr)
	{
		Composite->FallbackTypeface.Typeface = Fallback->DefaultTypeface;
	}
	GameFont = Composite;
	return GameFont;
}

void AYJJDefenseHUD::DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align, float Outline, float MaxWidth)
{
	// FCanvasTextItem은 UFont가 있어야 그려진다. 실제 글꼴은 주아체 컴포지트
	if (!BaseFont || Text.IsEmpty() || Color.A <= 0.f)
	{
		return;
	}

	const int32 FontSize = FMath::Max(FMath::RoundToInt(Size * ViewScale * UI().FontPointScale), FMath::RoundToInt(UI().MinFontSize));
	const TSharedPtr<const FCompositeFont> Composite = GetGameFont(BaseFont);
	FSlateFontInfo Font = Composite.IsValid() ? FSlateFontInfo(Composite, FontSize) : FSlateFontInfo(BaseFont, FontSize, BaseFontStyle);
	if (Outline > 0.f)
	{
		Font.OutlineSettings.OutlineSize = FMath::Max(FMath::RoundToInt(Outline * ViewScale), 1);
		Font.OutlineSettings.OutlineColor = YJJUI::WithAlpha(YJJUI::Ink, Color.A);
	}

	// 칸보다 길면 (주로 다른 언어) 글자를 줄여서 맞춘다
	FSlateRenderer* Renderer = MaxWidth > 0.f && FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetRenderer() : nullptr;
	if (Renderer)
	{
		const float Width = Renderer->GetFontMeasureService()->Measure(Text, Font).X / ViewScale;
		if (Width > MaxWidth)
		{
			Font.Size = FMath::Max(FMath::FloorToFloat(Font.Size * MaxWidth / Width), UI().MinFontSize);
		}
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
	DrawSprite(DiscTexture, Center, Size * UI().RangeFillScale, YJJUI::WithAlpha(Color, Color.A * UI().RangeFillAlpha));
	DrawSprite(DashRingTexture, Center, Size, Color);
}

void AYJJDefenseHUD::DrawBar(const FVector2f& Center, float Width, float Fraction, const FLinearColor& Color)
{
	const float Height = UI().HealthBarHeight;
	const float Border = UI().HealthBarBorder;
	const float X = Center.X - Width * 0.5f;
	const float Y = Center.Y - Height * 0.5f;
	DrawBox(X - Border, Y - Border, Width + Border * 2.f, Height + Border * 2.f, YJJUI::Ink);
	DrawBox(X, Y, Width, Height, YJJUI::BarBack);
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
	FLinearColor Tint = bEnabled ? Color : YJJUI::ButtonDisabled;
	if (bHover && bEnabled)
	{
		// 살짝 밝게 + 위로 들썩
		Tint = FMath::Lerp(Tint, FLinearColor::White, UI().ButtonHoverBrighten);
		Y -= UI().ButtonHoverLift;
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
	Fx.Seed = FxRandom.RandRange(0, FxSeedRange);
	Fx.Rotation = FxRandom.FRandRange(0.f, FullTurnDegrees);
	return Fx;
}

void AYJJDefenseHUD::AddText(const FVector2f& Pos, const FString& Text, float Size, const FLinearColor& Color, float Life)
{
	FFx& Fx = AddFx(EFx::Text, Pos, Life, Size, Color);
	Fx.Text = Text;
	Fx.Vel = FVector2f(0.f, -UI().FxTextRiseSpeed);
}

void AYJJDefenseHUD::AddSparks(const FVector2f& Pos, int32 Count, const FLinearColor& Color, float Speed)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = FxRandom.FRandRange(0.f, UE_TWO_PI);
		FFx& Fx = AddFx(EFx::Spark, Pos, FxRandom.FRandRange(UI().FxSparkLifeMin, UI().FxSparkLifeMax),
			FxRandom.FRandRange(UI().FxSparkSizeMin, UI().FxSparkSizeMax), Color);
		Fx.Vel = FVector2f(FMath::Cos(Angle), FMath::Sin(Angle) * UI().FxSparkFlatten - UI().FxSparkUpward)
			* Speed * FxRandom.FRandRange(UI().FxSparkSpeedMin, UI().FxSparkSpeedMax);
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
		DrawFx(Fx, DeltaSeconds);
	}
}

void AYJJDefenseHUD::DrawFx(FFx& Fx, float DeltaSeconds)
{
	const float T = Fx.Age / Fx.Life;
	switch (Fx.Kind)
	{
	case EFx::Puff:
	{
		const int32 Frame = FMath::Clamp(FMath::FloorToInt(T * PuffFrames), 0, PuffFrames - 1);
		const float FadeStart = UI().FxPuffFadeStart;
		const float Alpha = T > FadeStart ? (1.f - T) / (1.f - FadeStart) : 1.f;
		DrawSprite(PuffTextures[Frame], Fx.Pos, FVector2f(Fx.Size, Fx.Size), YJJUI::WithAlpha(Fx.Color, Alpha));
		break;
	}
	case EFx::Burst:
	{
		// 확 커졌다가 사라진다
		const float Scale = FMath::Lerp(UI().FxBurstScaleStart, UI().FxBurstScaleEnd, FMath::Sqrt(T));
		const float FadeStart = UI().FxBurstFadeStart;
		const float Alpha = T < FadeStart ? 1.f : (1.f - T) / (1.f - FadeStart);
		DrawSprite(BurstTexture, Fx.Pos, FVector2f(Fx.Size, Fx.Size) * Scale, YJJUI::WithAlpha(Fx.Color, Alpha), Fx.Rotation);
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
		const float Punch = 1.f + UI().FxTextPunch * FMath::Max(1.f - Fx.Age * UI().FxTextPunchSpeed, 0.f);
		const float FadeStart = UI().FxTextFadeStart;
		const float Alpha = 1.f - FMath::Clamp((T - FadeStart) / (1.f - FadeStart), 0.f, 1.f);
		DrawLabel(Fx.Text, Fx.Pos.X, Fx.Pos.Y, Fx.Size * Punch, YJJUI::WithAlpha(Fx.Color, Alpha), EAlign::Center, UI().FxTextOutline);
		break;
	}
	case EFx::Bolt:
	{
		// 지그재그 번개 (잠깐씩 모양이 바뀐다)
		FRandomStream Random(Fx.Seed + FMath::FloorToInt(Fx.Age * UI().FxBoltFlickerRate));
		const FVector2f Delta = Fx.Pos2 - Fx.Pos;
		const float Length = Delta.Size();
		const FVector2f Normal = Length > KINDA_SMALL_NUMBER ? FVector2f(-Delta.Y, Delta.X) / Length : FVector2f::ZeroVector;
		const int32 Segments = FMath::Clamp(FMath::RoundToInt(Length / UI().FxBoltSegment), FMath::RoundToInt(UI().FxBoltMinSegments), FMath::RoundToInt(UI().FxBoltMaxSegments));
		const float Jitter = UI().FxBoltJitter;
		FVector2f Previous = Fx.Pos;
		for (int32 Segment = 1; Segment <= Segments; ++Segment)
		{
			FVector2f Point = Fx.Pos + Delta * (static_cast<float>(Segment) / Segments);
			if (Segment < Segments)
			{
				Point += Normal * Random.FRandRange(-Jitter, Jitter);
			}
			DrawStroke(Previous, Point, YJJUI::WithAlpha(YJJUI::Ink, 1.f - T), Fx.Size + UI().FxBoltOutline);
			DrawStroke(Previous, Point, YJJUI::WithAlpha(Fx.Color, 1.f - T), Fx.Size);
			Previous = Point;
		}
		break;
	}
	case EFx::Spark:
	{
		Fx.Vel.Y += UI().FxSparkGravity * DeltaSeconds;
		Fx.Pos += Fx.Vel * DeltaSeconds;
		Fx.Rotation += UI().FxSparkSpin * DeltaSeconds;
		DrawSprite(StarFxTexture, Fx.Pos, FVector2f(Fx.Size, Fx.Size) * (1.f - T * UI().FxSparkShrink), YJJUI::WithAlpha(Fx.Color, 1.f - T * T), Fx.Rotation);
		break;
	}
	}
}

void AYJJDefenseHUD::OnProjectileHit(const FYJJSimEvent& Event)
{
	const FVector2f Up(0.f, -UI().FxGroundLift);
	switch (static_cast<EYJJProjectile>(Event.Int))
	{
	case EYJJProjectile::Arrow:
		AddFx(EFx::Burst, Event.Pos + Up, UI().FxArrowHitLife, UI().FxArrowHitSize, FLinearColor::White);
		break;
	case EYJJProjectile::MagicBolt:
		AddFx(EFx::Burst, Event.Pos + Up, UI().FxBoltHitLife, UI().FxBoltHitSize, YJJUI::Purple);
		break;
	case EYJJProjectile::Bomb:
	{
		const FVector2f Center = Event.Pos - FVector2f(0.f, UI().FxBombLift);
		AddFx(EFx::Puff, Center, UI().FxBombPuffLife, Event.Value * UI().FxBombPuffScale, YJJUI::BombPuff);
		AddFx(EFx::Burst, Center, UI().FxBombBurstLife, Event.Value * UI().FxBombBurstScale, YJJUI::Orange);
		FFx& Ring = AddFx(EFx::Ring, Event.Pos, UI().FxBombRingLife, Event.Value * UI().FxBombRingStart, YJJUI::Orange);
		Ring.Size2 = Event.Value;
		break;
	}
	case EYJJProjectile::ShrineOrb:
	{
		FFx& Ring = AddFx(EFx::Ring, Event.Pos, UI().FxOrbRingLife, UI().FxOrbRingStart, YJJData::ElementColor(Event.Element));
		Ring.Size2 = Event.Value;
		break;
	}
	default:
		break;
	}
}

void AYJJDefenseHUD::OnSimEvents(TConstArrayView<FYJJSimEvent> Events)
{
	const FVector2f Up(0.f, -UI().FxGroundLift);

	for (const FYJJSimEvent& Event : Events)
	{
		switch (Event.Type)
		{
		case EYJJSimEvent::EnemyKilled:
			AddFx(EFx::Puff, Event.Pos + Up, UI().FxKillPuffLife, UI().FxKillPuffSize, FLinearColor::White);
			AddText(Event.Pos - FVector2f(0.f, UI().FxKillTextLift), YJJLoc::Format(TEXT("Fx.Gold"), { Event.Int }), UI().FxKillTextSize, YJJUI::Yellow, UI().FxKillTextLife);
			break;

		case EYJJSimEvent::EnemySplit:
			AddFx(EFx::Puff, Event.Pos + Up, UI().FxSplitPuffLife, UI().FxSplitPuffSize, FLinearColor::White);
			AddText(Event.Pos - FVector2f(0.f, UI().FxSplitTextLift), YJJLoc::Format(TEXT("Fx.Split"), { Event.Int }), UI().FxSplitTextSize, YJJUI::Orange, UI().FxSplitTextLife);
			break;

		case EYJJSimEvent::EnemyImmune:
			AddText(Event.Pos - FVector2f(0.f, UI().FxImmuneTextLift), YJJLoc::Get(TEXT("Fx.ChillImmune")), UI().FxImmuneTextSize, YJJUI::Sky, UI().FxImmuneTextLife);
			break;

		case EYJJSimEvent::EnemyLeaked:
			AddText(Event.Pos + FVector2f(-UI().FxLeakTextShift, -UI().FxLeakTextLift), YJJLoc::Format(TEXT("Fx.LifeLost"), { Event.Int }), UI().FxLeakTextSize, YJJUI::Red, UI().FxLeakTextLife);
			AddFx(EFx::Burst, Event.Pos + Up, UI().FxLeakBurstLife, UI().FxLeakBurstSize, YJJUI::Red);
			break;

		case EYJJSimEvent::TowerBuilt:
		case EYJJSimEvent::TowerUpgraded:
			AddFx(EFx::Puff, Event.Pos - FVector2f(0.f, UI().FxBuildPuffLift), UI().FxBuildPuffLife, UI().FxBuildPuffSize, FLinearColor::White);
			AddSparks(Event.Pos - FVector2f(0.f, UI().FxBuildSparkLift), FMath::RoundToInt(UI().FxBuildSparks), YJJUI::Yellow, UI().FxSparkSpeed);
			break;

		case EYJJSimEvent::TowerSold:
			AddFx(EFx::Puff, Event.Pos - FVector2f(0.f, UI().FxBuildPuffLift), UI().FxBuildPuffLife, UI().FxSellPuffSize, FLinearColor::White);
			AddText(Event.Pos - FVector2f(0.f, UI().FxSellTextLift), YJJLoc::Format(TEXT("Fx.Gold"), { Event.Int }), UI().FxSellTextSize, YJJUI::Yellow, UI().FxSellTextLife);
			break;

		case EYJJSimEvent::WizardUpgraded:
			AddSparks(Event.Pos - FVector2f(0.f, UI().FxWizardSparkLift), FMath::RoundToInt(UI().FxWizardSparks), YJJUI::Yellow, UI().FxWizardSparkSpeed);
			AddText(Event.Pos - FVector2f(0.f, UI().FxWizardTextLift), YJJLoc::Get(TEXT("Fx.WizardUp")), UI().FxWizardTextSize, YJJUI::Yellow, UI().FxWizardTextLife);
			break;

		case EYJJSimEvent::ProjectileHit:
			OnProjectileHit(Event);
			break;

		case EYJJSimEvent::SpellCast:
			AddSparks(Event.Pos, FMath::RoundToInt(UI().FxCastSparks), YJJData::ElementColor(Event.Element), UI().FxCastSparkSpeed);
			break;

		case EYJJSimEvent::SpellImpact:
		{
			const FLinearColor Color = YJJData::ElementColor(Event.Element);
			const float Radius = FMath::Max(Event.Value, UI().FxImpactMinRadius);
			const FVector2f Center = Event.Pos - FVector2f(0.f, UI().FxImpactLift);
			AddFx(EFx::Burst, Center, UI().FxImpactBurstLife, Radius * UI().FxImpactBurstScale, Color);
			AddFx(EFx::Puff, Center, UI().FxImpactPuffLife, Radius * UI().FxImpactPuffScale, FLinearColor::White);
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, UI().FxImpactRingLife, Radius * UI().FxImpactRingStart, Color);
			Ring.Size2 = Radius;
			const int32 Sparks = FMath::RoundToInt(UI().FxImpactSparks);
			AddSparks(Event.Pos + Up, Sparks, Color, UI().FxSparkSpeed);
			if (Event.Int != static_cast<int32>(EYJJElement::None))
			{
				AddSparks(Event.Pos + Up, Sparks, YJJData::ElementColor(static_cast<EYJJElement>(Event.Int)), UI().FxSparkSpeed);
			}
			break;
		}

		case EYJJSimEvent::Reaction:
		{
			const FYJJReactionDef& Def = YJJData::Reaction(Event.Reaction);
			AddFx(EFx::Burst, Event.Pos - FVector2f(0.f, UI().FxReactionLift), UI().FxReactionBurstLife,
				FMath::Min(Event.Value * UI().FxReactionBurstScale, UI().FxReactionBurstMax), Def.Color);
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, UI().FxReactionRingLife, Event.Value * UI().FxReactionRingStart, Def.Color);
			Ring.Size2 = Event.Value;
			FFx& Ring2 = AddFx(EFx::Ring, Event.Pos, UI().FxReactionRing2Life, Event.Value * UI().FxReactionRing2Start, FLinearColor::White);
			Ring2.Size2 = Event.Value * UI().FxReactionRing2End;
			AddSparks(Event.Pos + Up, FMath::RoundToInt(UI().FxReactionSparks), Def.Color, UI().FxReactionSparkSpeed);
			AddText(Event.Pos - FVector2f(0.f, UI().FxReactionTextLift), YJJLoc::Format(TEXT("Fx.Reaction"), { YJJLoc::Get(Def.NameKey) }), UI().FxReactionTextSize, Def.Color, UI().FxReactionTextLife);
			break;
		}

		case EYJJSimEvent::ChainArc:
		{
			FFx& Bolt = AddFx(EFx::Bolt, Event.Pos + Up, UI().FxChainLife, UI().FxChainWidth, YJJData::ElementColor(Event.Element));
			Bolt.Pos2 = Event.Pos2 + Up;
			break;
		}

		case EYJJSimEvent::SkyBolt:
		{
			const float Sway = UI().FxSkyBoltSway;
			FFx& Bolt = AddFx(EFx::Bolt, Event.Pos2 + FVector2f(FxRandom.FRandRange(-Sway, Sway), -UI().FxSkyBoltHeight), UI().FxSkyBoltLife, UI().FxSkyBoltWidth, YJJUI::Yellow);
			Bolt.Pos2 = Event.Pos2 - FVector2f(0.f, UI().FxSkyBoltGround);
			AddFx(EFx::Burst, Event.Pos2 + Up, UI().FxSkyBoltBurstLife, UI().FxSkyBoltBurstSize, YJJUI::Yellow);
			break;
		}

		case EYJJSimEvent::SoldierDied:
			AddFx(EFx::Puff, Event.Pos + Up, UI().FxKillPuffLife, UI().FxSoldierPuffSize, FLinearColor::White);
			AddText(Event.Pos - FVector2f(0.f, UI().FxSoldierTextLift), YJJLoc::Get(TEXT("Fx.SoldierDown")), UI().FxSoldierTextSize, FLinearColor::White, UI().FxSoldierTextLife);
			break;

		case EYJJSimEvent::MeleeHit:
		{
			const float Jitter = UI().FxMeleeJitter;
			AddFx(EFx::Burst, Event.Pos + FVector2f(FxRandom.FRandRange(-Jitter, Jitter), -UI().FxMeleeLift), UI().FxMeleeLife, UI().FxMeleeSize, FLinearColor::White);
			break;
		}

		case EYJJSimEvent::Heal:
		{
			FFx& Ring = AddFx(EFx::Ring, Event.Pos, UI().FxHealRingLife, UI().FxHealRingStart, YJJUI::Pink);
			Ring.Size2 = Event.Value;
			AddText(Event.Pos - FVector2f(0.f, UI().FxHealTextLift), YJJLoc::Get(TEXT("Fx.Heal")), UI().FxHealTextSize, YJJUI::Pink, UI().FxHealTextLife);
			break;
		}

		case EYJJSimEvent::WaveStarted:
		{
			const bool bLast = Event.Int == YJJData::NumWaves() - 1;
			BannerText = bLast ? YJJLoc::Get(TEXT("Banner.LastWave")) : YJJLoc::Format(TEXT("Banner.Wave"), { Event.Int + 1 });
			BannerColor = bLast ? YJJUI::Red : FLinearColor::White;
			BannerTime = UI().BannerWaveTime;
			break;
		}

		case EYJJSimEvent::WaveBonus:
			AddText(FVector2f(UI().FxBonusTextX, UI().FxBonusTextY), YJJLoc::Format(TEXT("Fx.EarlyBonus"), { Event.Int }), UI().FxBonusTextSize, YJJUI::Yellow, UI().FxBonusTextLife);
			break;

		case EYJJSimEvent::Victory:
			BannerText = YJJLoc::Get(TEXT("Banner.Victory"));
			BannerColor = YJJUI::Yellow;
			BannerTime = UI().BannerEndTime;
			break;

		case EYJJSimEvent::Defeat:
			BannerText = YJJLoc::Get(TEXT("Banner.Defeat"));
			BannerColor = YJJUI::Red;
			BannerTime = UI().BannerEndTime;
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
		const float Top = bBuilt ? -UI().SpotPickTopBuilt : -UI().SpotPickTopEmpty;
		const FVector2f Delta = Point - Spots[Spot];
		if (FMath::Abs(Delta.X) <= UI().SpotPickHalfWidth && Delta.Y >= Top && Delta.Y <= UI().SpotPickBottom)
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
		if (Save && FYJJProgression::ToggleRune(*Save, static_cast<EYJJRune>(Region.A)))
		{
			Game->SaveProgress();
		}
		break;
	case EAction::ToggleTower:
		if (Save && FYJJProgression::ToggleTower(*Save, static_cast<EYJJTower>(Region.A)))
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
		Mouse = (FVector2f(MouseX, MouseY) - ViewOffset) / ViewScale;
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
	UE_LOG(LogYJJDefense, Verbose, TEXT("[Input] mouse %s at (%.0f, %.0f)"), bRightButton ? TEXT("right") : TEXT("left"), Mouse.X, Mouse.Y);

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
	if (FVector2f::Distance(Mouse, DragStart) > UI().DragThreshold)
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
