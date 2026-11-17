#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class AYJJDefenseGameMode;
class AYJJDefenseHUD;

// 실제 게임 화면에서 마우스·키 조작을 차례로 흉내 내며 UI가 제대로 반응하는지 확인한다 (yjj.UITest 1).
// 준비 화면 → 출전 → 건설 메뉴 → 강화 → 룬 끌어다 꽂기 → 웨이브 호출 → 스펠 조준·시전 → 일시정지.
//
// 입력은 Slate에 마우스·키 이벤트를 넣어서 만든다. 그래서 실제 플레이와 같은 길
// (Slate → 게임 뷰포트 → 플레이어 컨트롤러 → HUD)을 지나가면서도 OS 마우스 커서는 건드리지 않는다.
// 단계마다 [UITest] PASS/FAIL 로그와 스크린샷(Saved/Screenshots/UITest_*.png)을 남긴다. Tools/RunUITest.ps1
class FYJJUITestDriver
{
public:
	// 매 프레임 호출. 모든 단계를 마치면 true
	bool Tick(AYJJDefenseGameMode& Game, AYJJDefenseHUD& HUD, float DeltaSeconds);

	int32 NumFailed() const { return Failed; }

private:
	void Check(const TCHAR* Name, bool bPassed);

	// 가상 화면(1920x1080) 좌표로 마우스 조작
	void MoveTo(const AYJJDefenseHUD& HUD, const FVector2f& Virtual);
	void Button(const FKey& Key, bool bDown);
	void Click(const AYJJDefenseHUD& HUD, const FVector2f& Virtual, const FKey& Key = EKeys::LeftMouseButton);
	void PressKey(const FKey& Key);

	int32 Step = 0;
	float Wait = 0.5f;
	int32 Passed = 0;
	int32 Failed = 0;

	FVector2f LastScreen = FVector2f::ZeroVector;
	TSet<FKey> PressedButtons;
};
