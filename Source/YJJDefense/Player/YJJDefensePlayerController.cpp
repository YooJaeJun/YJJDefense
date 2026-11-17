#include "Player/YJJDefensePlayerController.h"
#include "InputKeyEventArgs.h"
#include "UI/YJJDefenseHUD.h"

AYJJDefensePlayerController::AYJJDefensePlayerController()
{
	bShowMouseCursor = true;
}

void AYJJDefensePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

bool AYJJDefensePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	if (AYJJDefenseHUD* HUD = Cast<AYJJDefenseHUD>(GetHUD()))
	{
		const bool bPressed = Params.Event == IE_Pressed || Params.Event == IE_DoubleClick;
		if (Params.Key == EKeys::LeftMouseButton)
		{
			if (bPressed)
			{
				HUD->HandleMouseDown(false);
			}
			else if (Params.Event == IE_Released)
			{
				HUD->HandleMouseUp();
			}
		}
		else if (Params.Key == EKeys::RightMouseButton)
		{
			if (bPressed)
			{
				HUD->HandleMouseDown(true);
			}
		}
		else if (Params.Event == IE_Pressed)
		{
			HUD->HandleKey(Params.Key);
		}
	}

	return Super::InputKey(Params);
}
