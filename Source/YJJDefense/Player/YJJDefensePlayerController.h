#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "YJJDefensePlayerController.generated.h"

// 마우스·키 입력을 HUD(화면 전체를 그리고 클릭 영역을 아는 쪽)에 넘긴다
UCLASS()
class YJJDEFENSE_API AYJJDefensePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AYJJDefensePlayerController();

	virtual void BeginPlay() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
};
