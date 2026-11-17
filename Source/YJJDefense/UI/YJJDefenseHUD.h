#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Game/YJJDefenseSim.h"
#include "YJJDefenseHUD.generated.h"

class AYJJDefenseGameMode;
class UTexture2D;
class UFont;
struct FCompositeFont;

// 화면 전체를 Canvas로 그린다 (맵, 병졸, 몬스터, 이펙트, UI). 위젯·스프라이트 액터 없음.
// 모든 좌표는 1920x1080 가상 화면이고, 창 크기에 맞춰 통째로 확대·축소한다.
//
// 구현은 세 파일로 나뉜다:
//   YJJDefenseHUD.cpp         기본 그리기, 입력, 이펙트
//   YJJDefenseHUD_Battle.cpp  전투 화면 (월드 + 전투 UI)
//   YJJDefenseHUD_Menu.cpp    준비 화면, 결과 화면
UCLASS()
class YJJDEFENSE_API AYJJDefenseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// 입력 (플레이어 컨트롤러가 호출)
	void HandleMouseDown(bool bRightButton);
	void HandleMouseUp();
	void HandleKey(const FKey& Key);

	// 게임모드가 호출
	void OnSimEvents(TConstArrayView<FYJJSimEvent> Events);
	void OnRunStarted();

	// UI 테스트용
	FVector2f VirtualToViewport(const FVector2f& Virtual) const { return ViewOffset + Virtual * ViewScale; }
	int32 GetSelectedSpot() const { return SelectedSpot; }
	int32 GetArmedDisk() const { return ArmedDisk; }

	// UI 글꼴 TTF (Content 기준 상대 경로). 배민 주아체, SIL OFL 1.1 (같은 폴더 OFL.txt)
	UPROPERTY(EditAnywhere, Category = "HUD")
	FString GameFontFile = TEXT("YJJDefense/Fonts/Jua-Regular.ttf");

private:
	enum class EAlign : uint8 { Left, Center };

	enum class EAction : uint8
	{
		Blocker,        // 아래로 클릭이 새지 않게 막기만 한다
		StartRun,
		ToggleRune,     // A=룬
		ToggleTower,    // A=병졸
		TierPrev,
		TierNext,
		ResultContinue,
		BuildTower,     // A=자리, B=병졸
		UpgradeTower,   // A=자리
		SellTower,      // A=자리
		CycleShrine,    // A=자리
		Disk,           // A=디스크
		Socket,         // A=디스크, B=칸
		BagRune,        // A=룬
		CallWave,
		Pause,
		Speed,
		UpgradeWizard,
	};

	struct FHitRegion
	{
		FBox2f Box;
		EAction Action;
		int32 A;
		int32 B;
	};

	enum class EFx : uint8 { Puff, Burst, Ring, Text, Bolt, Spark };

	struct FFx
	{
		EFx Kind = EFx::Puff;
		FVector2f Pos = FVector2f::ZeroVector;
		FVector2f Pos2 = FVector2f::ZeroVector;
		FVector2f Vel = FVector2f::ZeroVector;
		float Age = 0.f;
		float Life = 0.4f;
		float Size = 40.f;
		float Size2 = 0.f;
		float Rotation = 0.f;
		int32 Seed = 0;
		FLinearColor Color = FLinearColor::White;
		FString Text;
	};

	// ---- 기본 그리기 (가상 좌표) ----
	FVector2f TexSize(const UTexture2D* Texture) const;
	void DrawSprite(UTexture2D* Texture, const FVector2f& Center, const FVector2f& Size, const FLinearColor& Color = FLinearColor::White, float Rotation = 0.f, bool bFlipX = false);
	// 발(아래 가운데)을 기준으로, 원본의 절반 크기 x Scale로 그린다
	void DrawSpriteFeet(UTexture2D* Texture, const FVector2f& Feet, const FVector2f& Scale = FVector2f(1.f, 1.f), const FLinearColor& Color = FLinearColor::White, bool bFlipX = false, float Rotation = 0.f);
	void DrawBox(float X, float Y, float W, float H, const FLinearColor& Color);
	void DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color, bool bOutlined = true);
	void DrawStroke(const FVector2f& A, const FVector2f& B, const FLinearColor& Color, float Thickness);
	void DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, EAlign Align = EAlign::Left, float Outline = 0.f);
	// 여러 줄 (\n). 마지막 줄 아래 y를 돌려준다
	float DrawParagraph(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, float LineHeight);
	void DrawEllipseRange(const FVector2f& Center, float Radius, const FLinearColor& Color);
	void DrawBar(const FVector2f& Center, float Width, float Fraction, const FLinearColor& Color);
	// 패널 버튼 하나 (그리기 + 클릭 영역 등록). 마우스가 올라가 있으면 true
	bool DrawButton(float X, float Y, float W, float H, const FLinearColor& Color, EAction Action, int32 A = 0, int32 B = 0, bool bEnabled = true);
	void AddHit(float X, float Y, float W, float H, EAction Action, int32 A = 0, int32 B = 0);
	bool IsHover(float X, float Y, float W, float H) const;
	const FHitRegion* HitAt(const FVector2f& Point) const;
	TSharedPtr<const FCompositeFont> GetGameFont(const UFont* FallbackFont);

	// ---- 이펙트 ----
	FFx& AddFx(EFx Kind, const FVector2f& Pos, float Life, float Size, const FLinearColor& Color);
	void AddText(const FVector2f& Pos, const FString& Text, float Size, const FLinearColor& Color, float Life = 0.9f);
	void AddSparks(const FVector2f& Pos, int32 Count, const FLinearColor& Color, float Speed = 160.f);
	void TickAndDrawFx(float DeltaSeconds);
	void DoAction(const FHitRegion& Region, bool bRightButton);

	// ---- 전투 화면 ----
	void DrawBattle(bool bInteractive);
	void DrawZones();
	void DrawWorldSorted();
	void DrawSpot(int32 Spot);
	void DrawEnemy(const FYJJEnemyState& Enemy);
	void DrawSoldier(const FYJJSoldierState& Soldier);
	void DrawWizard();
	void DrawProjectiles(float DeltaSeconds);
	void DrawTopBar();
	void DrawSpellBar();
	void DrawWaveButton();
	void DrawTowerMenu();
	void DrawTargeting();
	void DrawBanner(float DeltaSeconds);
	void DrawTooltip();
	void SetTooltip(const FString& Title, const FString& Body, const FLinearColor& TitleColor);
	FString DescribeSpell(const FYJJSpellSpec& Spell, bool& bOutNew) const;
	FString DescribeTower(EYJJTower Type, int32 Level) const;
	FString DescribeWave(int32 WaveIndex) const;
	int32 FindSpotAt(const FVector2f& Point) const;
	void CancelSelection();

	// ---- 준비·결과 화면 ----
	void DrawLoadout();
	void DrawResult();
	void DrawDimmedMap(float Darkness);

	UTexture2D* LoadTex(const FString& Name);
	UTexture2D* RuneTex(EYJJRune Rune) const { return RuneTextures[FMath::Min<int32>(static_cast<int32>(Rune), static_cast<int32>(EYJJRune::Count) - 1)]; }
	UTexture2D* BallTex(EYJJElement Element) const { return BallTextures[FMath::Min<int32>(static_cast<int32>(Element), static_cast<int32>(EYJJElement::Count) - 1)]; }

	// 로드한 텍스처를 붙잡아 둔다 (GC 방지)
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> LoadedTextures;

	UTexture2D* MapTexture = nullptr;
	UTexture2D* EnemyTextures[static_cast<int32>(EYJJEnemy::Count)][2] = {};
	UTexture2D* TowerTextures[static_cast<int32>(EYJJTower::Count)][YJJ::MaxTowerLevel + 1] = {};
	UTexture2D* SoldierTextures[3] = {};
	UTexture2D* WizardTextures[2] = {};
	UTexture2D* RuneTextures[static_cast<int32>(EYJJRune::Count)] = {};
	UTexture2D* BallTextures[static_cast<int32>(EYJJElement::Count)] = {};   // [0] = 비전
	UTexture2D* PuffTextures[4] = {};
	UTexture2D* SpotTexture = nullptr;
	UTexture2D* ArrowTexture = nullptr;
	UTexture2D* BombTexture = nullptr;
	UTexture2D* BoltTexture = nullptr;
	UTexture2D* BurstTexture = nullptr;
	UTexture2D* RingTexture = nullptr;
	UTexture2D* DashRingTexture = nullptr;
	UTexture2D* DiscTexture = nullptr;
	UTexture2D* ShadowTexture = nullptr;
	UTexture2D* StarFxTexture = nullptr;
	UTexture2D* IceTexture = nullptr;
	UTexture2D* FlameTexture = nullptr;
	UTexture2D* PanelTexture = nullptr;
	UTexture2D* PanelFlatTexture = nullptr;
	UTexture2D* DiskTexture = nullptr;
	UTexture2D* SocketTexture = nullptr;
	UTexture2D* HeartIcon = nullptr;
	UTexture2D* CoinIcon = nullptr;
	UTexture2D* FlagIcon = nullptr;
	UTexture2D* LockIcon = nullptr;
	UTexture2D* StarIcon = nullptr;
	UTexture2D* StarEmptyIcon = nullptr;
	UTexture2D* UpIcon = nullptr;
	UTexture2D* SellIcon = nullptr;
	UTexture2D* PauseIcon = nullptr;
	UTexture2D* PlayIcon = nullptr;
	UTexture2D* FastIcon = nullptr;
	UTexture2D* CycleIcon = nullptr;
	UTexture2D* WandIcon = nullptr;
	UTexture2D* CheckIcon = nullptr;
	UTexture2D* SkullIcon = nullptr;

	TSharedPtr<const FCompositeFont> GameFont;

	UPROPERTY()
	TObjectPtr<AYJJDefenseGameMode> Game;

	// 가상 좌표 → 화면
	float ViewScale = 1.f;
	FVector2f ViewOffset = FVector2f::ZeroVector;
	// 뷰포트의 마우스 좌표를 가상 좌표로 읽어 Mouse에 넣는다
	void UpdateMouse();
	FVector2f Mouse = FVector2f::ZeroVector;
	float UITime = 0.f;

	TArray<FHitRegion> HitRegions;
	TArray<FFx> Effects;
	FRandomStream FxRandom;

	// 전투 UI 상태
	int32 SelectedSpot = INDEX_NONE;
	int32 ArmedDisk = INDEX_NONE;
	EYJJRune SelectedRune = EYJJRune::None;
	EYJJRune DragRune = EYJJRune::None;
	FVector2f DragStart = FVector2f::ZeroVector;
	int32 HoverBuildType = INDEX_NONE;
	bool bHoverUpgrade = false;

	FString BannerText;
	FLinearColor BannerColor = FLinearColor::White;
	float BannerTime = 0.f;

	FString TooltipTitle;
	FString TooltipBody;
	FLinearColor TooltipColor = FLinearColor::White;
	bool bHasTooltip = false;
};
