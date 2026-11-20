#include "Tests/YJJUITestDriver.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/YJJGameData.h"
#include "Misc/Paths.h"
#include "UI/YJJDefenseHUD.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include "YJJDefense.h"
#include "YJJDefenseGameMode.h"

void FYJJUITestDriver::Shot(const TCHAR* Name)
{
	FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots") / (FString(Name) + TEXT(".png")), true, false);
}

FVector2f FYJJUITestDriver::LeadEnemy(const FYJJDefenseSim& Sim, float Lead)
{
	const FYJJEnemyState* Best = nullptr;
	for (const FYJJEnemyState& Enemy : Sim.Enemies)
	{
		if (Enemy.IsAlive() && (!Best || Enemy.Dist > Best->Dist))
		{
			Best = &Enemy;
		}
	}
	return Best ? YJJMap::PointAt(Best->Dist + YJJData::Enemy(Best->Type).Speed * Lead, Best->Lateral) : FVector2f(NoEnemyX, NoEnemyY);
}

FVector2f FYJJUITestDriver::ToScreen(const AYJJDefenseHUD& HUD, const FVector2f& Virtual)
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

void FYJJUITestDriver::Check(const TCHAR* Name, bool bPassed)
{
	(bPassed ? Passed : Failed)++;
	UE_LOG(LogYJJDefense, Display, TEXT("[UITest] %s %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), Name);
}

void FYJJUITestDriver::MoveTo(const AYJJDefenseHUD& HUD, const FVector2f& Virtual)
{
	const FVector2f Screen = ToScreen(HUD, Virtual);
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
	Wait -= DeltaSeconds;
	if (Wait > 0.f)
	{
		return false;
	}
	Wait = StepWait;

	FYJJDefenseSim& Sim = Game.GetSim();
	UYJJSaveGame* Save = Game.GetSave();
	const FVector2f Spot3 = YJJMap::Spots()[TestSpotA];
	const FVector2f Spot4 = YJJMap::Spots()[TestSpotB];
	const int32 IceBit = 1 << static_cast<int32>(EYJJRune::Ice);

	switch (Step++)
	{
	// ---- 준비 화면 ----
	case 0:
		MoveTo(HUD, AYJJDefenseHUD::LoadoutRuneCenter(0));
		break;
	case 1:
		Shot(TEXT("UITest_1_Loadout"));
		break;
	case 2:
		Click(HUD, AYJJDefenseHUD::LoadoutRuneCenter(1));
		Check(TEXT("준비: 가방에서 룬 빼기"), Save && (Save->SelectedRunes & IceBit) == 0);
		break;
	case 3:
		Click(HUD, AYJJDefenseHUD::LoadoutRuneCenter(1));
		Check(TEXT("준비: 가방에 룬 다시 넣기"), Save && (Save->SelectedRunes & IceBit) != 0);
		break;
	case 4:
		Click(HUD, AYJJDefenseHUD::StartButtonCenter());
		Check(TEXT("준비: 출전 버튼"), Game.GetScreen() == EYJJScreen::Battle);
		Wait = SceneChangeWait;
		break;

	// ---- 건설 ----
	case 5:
		Click(HUD, Spot3 + FVector2f(0.f, -EmptySpotClickUp));
		Check(TEXT("건설: 빈 자리 선택"), HUD.GetSelectedSpot() == TestSpotA);
		break;
	case 6:
		MoveTo(HUD, AYJJDefenseHUD::TowerMenuButtonCenter(Spot3, 3, 0, false));
		break;
	case 7:
		Shot(TEXT("UITest_2_BuildMenu"));
		break;
	case 8:
		Click(HUD, AYJJDefenseHUD::TowerMenuButtonCenter(Spot3, 3, 0, false));
		Check(TEXT("건설: 궁수 초소 짓기"), Sim.Towers[TestSpotA].bBuilt && Sim.Towers[TestSpotA].Type == EYJJTower::Archer && HUD.GetSelectedSpot() == INDEX_NONE);
		Wait = SceneChangeWait;
		break;
	case 9:
		Click(HUD, Spot3 + FVector2f(0.f, -BuiltTowerClickUp));
		Check(TEXT("건설: 지은 타워 선택"), HUD.GetSelectedSpot() == TestSpotA);
		break;
	case 10:
		MoveTo(HUD, AYJJDefenseHUD::TowerMenuButtonCenter(Spot3, 2, 0, true));
		break;
	case 11:
		Shot(TEXT("UITest_3_TowerMenu"));
		break;
	case 12:
		Click(HUD, AYJJDefenseHUD::TowerMenuButtonCenter(Spot3, 2, 0, true));
		Check(TEXT("건설: 강화"), Sim.Towers[TestSpotA].Level == 1);
		break;
	case 13:
		PressKey(EKeys::Escape);
		Check(TEXT("건설: ESC로 선택 취소"), HUD.GetSelectedSpot() == INDEX_NONE);
		Click(HUD, Spot4 + FVector2f(0.f, -EmptySpotClickUp));
		break;
	case 14:
		Click(HUD, AYJJDefenseHUD::TowerMenuButtonCenter(Spot4, 3, 1, false));
		Check(TEXT("건설: 보병 막사 짓기"), Sim.Towers[TestSpotB].bBuilt && Sim.Towers[TestSpotB].Type == EYJJTower::Barracks && Sim.Soldiers.Num() == YJJ::SoldiersPerBarracks);
		break;

	// ---- 룬 조합 ----
	case 15:
		// 가방 네 번째(번개)를 집어서
		MoveTo(HUD, AYJJDefenseHUD::BagRuneCenter(LightningBagIndex));
		Button(EKeys::LeftMouseButton, true);
		break;
	case 16:
		// 세 번째 디스크의 빈 칸 위로 끌고 간다
		MoveTo(HUD, AYJJDefenseHUD::SocketCenter(2, 1) + FVector2f(DragOverX, DragOverY));
		break;
	case 17:
		Shot(TEXT("UITest_4_Drag"));
		break;
	case 18:
		Button(EKeys::LeftMouseButton, false);
		Check(TEXT("조합: 룬 끌어다 꽂기 (얼음+번개=초전도)"), Sim.Disks[2].Sockets[1] == EYJJRune::Lightning && Sim.Disks[2].Spell.Reaction == EYJJReaction::Superconduct);
		break;
	case 19:
		MoveTo(HUD, AYJJDefenseHUD::DiskCenter(2) + FVector2f(0.f, DiskTooltipDown));
		break;
	case 20:
		Shot(TEXT("UITest_5_SpellTooltip"));
		break;

	// ---- 전투 ----
	case 21:
		PressKey(EKeys::SpaceBar);
		Check(TEXT("전투: 스페이스로 웨이브 호출"), Sim.WaveIndex == 0);
		Wait = WaveArriveWait;
		break;
	case 22:
		PressKey(EKeys::One);
		Check(TEXT("전투: 단축키 1로 조준"), HUD.GetArmedDisk() == 0);
		MoveTo(HUD, LeadEnemy(Sim, AimLead));
		break;
	case 23:
		Shot(TEXT("UITest_6_Targeting"));
		break;
	case 24:
		Click(HUD, LeadEnemy(Sim, CastLead));
		Check(TEXT("전투: 맵을 눌러 스펠 시전"), Sim.SpellsCast == 1 && HUD.GetArmedDisk() == INDEX_NONE && Sim.Disks[0].Cooldown > 0.f);
		Wait = SpellFlightWait;
		break;
	case 25:
		Shot(TEXT("UITest_7_Impact"));
		Wait = ImpactShotWait;
		break;
	case 26:
		Click(HUD, AYJJDefenseHUD::DiskCenter(1) + FVector2f(0.f, DiskTooltipDown));
		Check(TEXT("전투: 디스크를 눌러 조준"), HUD.GetArmedDisk() == 1);
		break;
	case 27:
		Click(HUD, LeadEnemy(Sim, CastLead));
		Check(TEXT("전투: 두 번째 스펠 시전"), Sim.SpellsCast == 2);
		Wait = SpellFlightWait;
		break;
	case 28:
		Shot(TEXT("UITest_8_Reaction"));
		Wait = ReactionShotWait;
		break;
	case 29:
		Check(TEXT("전투: 물+번개 감전 반응"), Sim.Reactions >= 1);
		Click(HUD, AYJJDefenseHUD::SocketCenter(0, 1), EKeys::RightMouseButton);
		Check(TEXT("조합: 우클릭으로 룬 빼기"), Sim.Disks[0].Sockets[1] == EYJJRune::None && Sim.Disks[0].Spell.Explode == 0);
		break;
	case 30:
		Click(HUD, AYJJDefenseHUD::PauseButtonCenter());
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
		UE_LOG(LogYJJDefense, Display, TEXT("[UITest] finished passed=%d failed=%d"), Passed, Failed);
		return true;
	}
	return false;
}
