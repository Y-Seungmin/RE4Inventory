#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CaseHUD.generated.h"

/** 조작 안내와 현재 상태(집은 아이템, 놓을 수 있는지, 마지막 결과)를 화면 왼쪽 위에 글자로 띄운다 */
UCLASS()
class RE4INVENTORY_API ACaseHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void DrawHUD() override;
};
