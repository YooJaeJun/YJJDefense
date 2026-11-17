#include "Tests/YJJUITestDriver.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/YJJGameData.h"
#include "Misc/Paths.h"
#include "UI/YJJDefenseHUD.h"
#include "UI/YJJUIStyle.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include "YJJDefenseGameMode.h"

DEFINE_LOG_CATEGORY_STATIC(LogYJJUITest, Log, All);

namespace YJJUITest
{
	void Shot(const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / (FString(Name) + TEXT(".png")), true, false);
	}

	// 아래 좌표들은 HUD 배치(YJJDefenseHUD_*.cpp)와 같은 계산이다. 배치를 바꾸면 여기도 맞춘다

	FVector2f LoadoutRune(int32 Index)
	{
		return FVector2f(70.f + 26.f + (Index % 3) * 198.f + 93.f, 186.f + 84.f + (Index / 3) * 168.f + 78.f);
	}

	FVector2f BagRune(int32 BagIndex)
	{
		return FVector2f(YJJUI::BagX + YJJUI::BagSpacing * BagIndex + YJJUI::BagIcon * 0.5f, YJJUI::BagY - 28.f + YJJUI::BagIcon * 0.5f);
	}

	FVector2f DiskCenter(int32 Disk)
	{
		return FVector2f(YJJUI::DiskFirstX + YJJUI::DiskSpacing * Disk, YJJUI::DiskY);
	}

	FVector2f SocketCenter(int32 Disk, int32 Socket)
	{
		return DiskCenter(Disk) + FVector2f((Socket == 0 ? -1.f : 1.f) * (YJJUI::SocketSize * 0.5f + 3.f), 0.f);
	}

	FVector2f MenuOption(const FVector2f& Spot, int32 NumOptions, int32 Index, bool bBuilt)
	{
		const float TotalW = NumOptions * 92.f + (NumOptions - 1) * 8.f;
		const float Top = bBuilt ? Spot.Y - 262.f : Spot.Y - 182.f;
		const float X0 = FMath::Clamp(Spot.X - TotalW * 0.5f, 12.f, YJJ::ViewW - TotalW - 12.f);
		const float Y0 = Top < 84.f ? Spot.Y + 54.f : Top;
		return FVector2f(X0 + Index * 100.f + 46.f, Y0 + 54.f);
	}

	// 가장 앞선 적이 Lead초 뒤에 있을 자리
	FVector2f LeadEnemy(const FYJJDefenseSim& Sim, float Lead)
	{
		const FYJJEnemyState* Best = nullptr;
		for (const FYJJEnemyState& Enemy : Sim.Enemies)
		{
			if (Enemy.IsAlive() && (!Best || Enemy.Dist > Best->Dist))
			{
				Best = &Enemy;
			}
		}
		return Best ? YJJMap::PointAt(Best->Dist + YJJData::Enemy(Best->Type).Speed * Lead, Best->Lateral) : FVector2f(400.f, 270.f);
	}

	// 가상 좌표 → Slate 화면 좌표 (게임 뷰포트 위젯 기준)
	FVector2f ToScreen(const AYJJDefenseHUD& HUD, const FVector2f& Virtual)
	{
		UGameViewportClient* Client = HUD.GetWorld() ? HUD.GetWorld()->GetGameViewport() : nullptr;
		const TSharedPtr<SViewport> Widget = Client ? Client->GetGameViewportWidget() : nullptr;
		if (!Widget.IsValid() || !Client->Viewport)
		{
			return FVector2f::ZeroVector;
		}

		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FIntPoint Size = Client->Viewport->GetSizeXY();
		const FVector2f AbsolutePosition = Geometry.GetAbsolutePosition();
		const FVector2f AbsoluteSize = Geometry.GetAbsoluteSize();
		const FVector2f Pixel = HUD.VirtualToViewport(Virtual);
		return AbsolutePosition + FVector2f(Pixel.X * AbsoluteSize.X / FMath::Max(Size.X, 1), Pixel.Y * AbsoluteSize.Y / FMath::Max(Size.Y, 1));
	}
}

void FYJJUITestDriver::Check(const TCHAR* Name, bool bPassed)
{
	(bPassed ? Passed : Failed)++;
	UE_LOG(LogYJJUITest, Display, TEXT("[UITest] %s %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), Name);
}

void FYJJUITestDriver::MoveTo(const AYJJDefenseHUD& HUD, const FVector2f& Virtual)
{
	const FVector2f Screen = YJJUITest::ToScreen(HUD, Virtual);
	const FPointerEvent Event(FSlateApplicationBase::CursorUserIndex, FSlateApplicationBase::CursorPointerIndex, Screen, LastScreen, PressedButtons,
		EKeys::Invalid, 0.f, FModifierKeysState());
	FSlateApplication::Get().ProcessMouseMoveEvent(Event);
	LastScreen = Screen;
}

void FYJJUITestDriver::Button(const FKey& Key, bool bDown)
{
	if (bDown)
	{
		PressedButtons.Add(Key);
	}
	else
	{
		PressedButtons.Remove(Key);
	}

	const FPointerEvent Event(FSlateApplicationBase::CursorUserIndex, FSlateApplicationBase::CursorPointerIndex, LastScreen, LastScreen, PressedButtons,
		Key, 0.f, FModifierKeysState());
	if (bDown)
	{
		FSlateApplication::Get().ProcessMouseButtonDownEvent(nullptr, Event);
	}
	else
	{
		FSlateApplication::Get().ProcessMouseButtonUpEvent(Event);
	}
}

void FYJJUITestDriver::Click(const AYJJDefenseHUD& HUD, const FVector2f& Virtual, const FKey& Key)
{
	MoveTo(HUD, Virtual);
	Button(Key, true);
	Button(Key, false);
}

void FYJJUITestDriver::PressKey(const FKey& Key)
{
	const FKeyEvent Event(Key, FModifierKeysState(), FSlateApplicationBase::CursorUserIndex, false, 0, 0);
	FSlateApplication::Get().ProcessKeyDownEvent(Event);
	FSlateApplication::Get().ProcessKeyUpEvent(Event);
}

bool FYJJUITestDriver::Tick(AYJJDefenseGameMode& Game, AYJJDefenseHUD& HUD, float DeltaSeconds)
{
	using namespace YJJUITest;

	Wait -= DeltaSeconds;
	if (Wait > 0.f)
	{
		return false;
	}
	Wait = 0.3f;

	FYJJDefenseSim& Sim = Game.GetSim();
	UYJJSaveGame* Save = Game.GetSave();
	const FVector2f Spot3 = YJJMap::Spots()[3];
	const FVector2f Spot4 = YJJMap::Spots()[4];
	const int32 IceBit = 1 << static_cast<int32>(EYJJRune::Ice);

	switch (Step++)
	{
	// ---- 준비 화면 ----
	case 0:
		MoveTo(HUD, LoadoutRune(0));
		break;
	case 1:
		Shot(TEXT("UITest_1_Loadout"));
		break;
	case 2:
		Click(HUD, LoadoutRune(1));
		Check(TEXT("준비: 가방에서 룬 빼기"), Save && (Save->SelectedRunes & IceBit) == 0);
		break;
	case 3:
		Click(HUD, LoadoutRune(1));
		Check(TEXT("준비: 가방에 룬 다시 넣기"), Save && (Save->SelectedRunes & IceBit) != 0);
		break;
	case 4:
		Click(HUD, FVector2f(960.f, 905.f));
		Check(TEXT("준비: 출전 버튼"), Game.GetScreen() == EYJJScreen::Battle);
		Wait = 0.6f;
		break;

	// ---- 건설 ----
	case 5:
		Click(HUD, Spot3 + FVector2f(0.f, -10.f));
		Check(TEXT("건설: 빈 자리 선택"), HUD.GetSelectedSpot() == 3);
		break;
	case 6:
		MoveTo(HUD, MenuOption(Spot3, 3, 0, false));
		break;
	case 7:
		Shot(TEXT("UITest_2_BuildMenu"));
		break;
	case 8:
		Click(HUD, MenuOption(Spot3, 3, 0, false));
		Check(TEXT("건설: 궁수 초소 짓기"), Sim.Towers[3].bBuilt && Sim.Towers[3].Type == EYJJTower::Archer && HUD.GetSelectedSpot() == INDEX_NONE);
		Wait = 0.6f;
		break;
	case 9:
		Click(HUD, Spot3 + FVector2f(0.f, -60.f));
		Check(TEXT("건설: 지은 타워 선택"), HUD.GetSelectedSpot() == 3);
		break;
	case 10:
		MoveTo(HUD, MenuOption(Spot3, 2, 0, true));
		break;
	case 11:
		Shot(TEXT("UITest_3_TowerMenu"));
		break;
	case 12:
		Click(HUD, MenuOption(Spot3, 2, 0, true));
		Check(TEXT("건설: 강화"), Sim.Towers[3].Level == 1);
		break;
	case 13:
		PressKey(EKeys::Escape);
		Check(TEXT("건설: ESC로 선택 취소"), HUD.GetSelectedSpot() == INDEX_NONE);
		Click(HUD, Spot4 + FVector2f(0.f, -10.f));
		break;
	case 14:
		Click(HUD, MenuOption(Spot4, 3, 1, false));
		Check(TEXT("건설: 보병 막사 짓기"), Sim.Towers[4].bBuilt && Sim.Towers[4].Type == EYJJTower::Barracks && Sim.Soldiers.Num() == YJJ::SoldiersPerBarracks);
		break;

	// ---- 룬 조합 ----
	case 15:
		// 가방 네 번째(번개)를 집어서
		MoveTo(HUD, BagRune(3));
		Button(EKeys::LeftMouseButton, true);
		break;
	case 16:
		// 세 번째 디스크의 빈 칸 위로 끌고 간다
		MoveTo(HUD, SocketCenter(2, 1) + FVector2f(6.f, -4.f));
		break;
	case 17:
		Shot(TEXT("UITest_4_Drag"));
		break;
	case 18:
		Button(EKeys::LeftMouseButton, false);
		Check(TEXT("조합: 룬 끌어다 꽂기 (얼음+번개=초전도)"), Sim.Disks[2].Sockets[1] == EYJJRune::Lightning && Sim.Disks[2].Spell.Reaction == EYJJReaction::Superconduct);
		break;
	case 19:
		MoveTo(HUD, DiskCenter(2) + FVector2f(0.f, 42.f));
		break;
	case 20:
		Shot(TEXT("UITest_5_SpellTooltip"));
		break;

	// ---- 전투 ----
	case 21:
		PressKey(EKeys::SpaceBar);
		Check(TEXT("전투: 스페이스로 웨이브 호출"), Sim.WaveIndex == 0);
		Wait = 7.f;
		break;
	case 22:
		PressKey(EKeys::One);
		Check(TEXT("전투: 단축키 1로 조준"), HUD.GetArmedDisk() == 0);
		MoveTo(HUD, LeadEnemy(Sim, 1.4f));
		break;
	case 23:
		Shot(TEXT("UITest_6_Targeting"));
		break;
	case 24:
		Click(HUD, LeadEnemy(Sim, 1.2f));
		Check(TEXT("전투: 맵을 눌러 스펠 시전"), Sim.SpellsCast == 1 && HUD.GetArmedDisk() == INDEX_NONE && Sim.Disks[0].Cooldown > 0.f);
		Wait = 1.3f;
		break;
	case 25:
		Shot(TEXT("UITest_7_Impact"));
		Wait = 0.8f;
		break;
	case 26:
		Click(HUD, DiskCenter(1) + FVector2f(0.f, 42.f));
		Check(TEXT("전투: 디스크를 눌러 조준"), HUD.GetArmedDisk() == 1);
		break;
	case 27:
		Click(HUD, LeadEnemy(Sim, 1.2f));
		Check(TEXT("전투: 두 번째 스펠 시전"), Sim.SpellsCast == 2);
		Wait = 1.3f;
		break;
	case 28:
		Shot(TEXT("UITest_8_Reaction"));
		Wait = 0.5f;
		break;
	case 29:
		Check(TEXT("전투: 물+번개 감전 반응"), Sim.Reactions >= 1);
		Click(HUD, SocketCenter(0, 1), EKeys::RightMouseButton);
		Check(TEXT("조합: 우클릭으로 룬 빼기"), Sim.Disks[0].Sockets[1] == EYJJRune::None && Sim.Disks[0].Spell.Explode == 0);
		break;
	case 30:
		Click(HUD, FVector2f(1804.f, 45.f));
		Check(TEXT("전투: 일시정지 버튼"), Game.IsBattlePaused());
		break;
	case 31:
		Shot(TEXT("UITest_9_Paused"));
		break;
	case 32:
		PressKey(EKeys::P);
		Check(TEXT("전투: P로 다시 진행"), !Game.IsBattlePaused());
		break;

	default:
		UE_LOG(LogYJJUITest, Display, TEXT("[UITest] finished passed=%d failed=%d"), Passed, Failed);
		return true;
	}
	return false;
}
