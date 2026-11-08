#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaseInventoryController.generated.h"

class ACaseViewActor;

/**
 * 케이스 화면 조작. 마우스 광선을 케이스에 넘기고 키 입력을 케이스 조작 함수로 연결한다.
 * 좌클릭: 집기/놓기, 우클릭·R: 90도 회전, C: 취소, X: 버리기, Z: 되돌리기,
 * 1/2: 자동 정리(공간 우선/종류 우선), E: 케이스 확장
 */
UCLASS()
class RE4INVENTORY_API ACaseInventoryController : public APlayerController
{
	GENERATED_BODY()

public:
	ACaseInventoryController();

	UFUNCTION(BlueprintPure, Category = "Case")
	ACaseViewActor* GetCase() const { return Case; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	/** 마우스 광선으로 커서 칸을 갱신. 클릭은 PlayerTick보다 먼저 처리되므로 조작 직전에도 부른다 */
	void UpdateCursorFromMouse();

	void OnPrimary();
	void OnRotate();
	void OnCancel();
	void OnDiscard();
	void OnUndo();
	void OnArrangeSpace();
	void OnArrangeCategory();
	void OnExpand();

	UPROPERTY(Transient)
	TObjectPtr<ACaseViewActor> Case;
};
