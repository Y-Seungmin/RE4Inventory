#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaseInventoryGameMode.generated.h"

/** 케이스 화면 전용 게임 모드. 폰 없이 컨트롤러가 케이스 카메라로 본다. 레벨에 케이스가 없으면 하나 놓는다 */
UCLASS()
class RE4INVENTORY_API ACaseInventoryGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACaseInventoryGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
