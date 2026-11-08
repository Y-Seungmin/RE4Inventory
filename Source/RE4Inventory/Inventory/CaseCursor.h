#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CaseCursor.generated.h"

class UCaseGrid;

/**
 * 케이스 위의 커서. 아이템을 집고(PickUp) 칸을 옮기고(SetHoverCell) 돌리고(Rotate) 내려놓는(Drop) 조작 상태.
 * 집은 아이템은 내려놓기 전까지 격자 상태를 바꾸지 않는다. 내려놓을 때 UCaseGrid::MoveItem 하나로 기록된다.
 * 화면(ACaseViewActor)은 이 값을 읽어 미리보기만 그린다.
 */
UCLASS(BlueprintType)
class RE4INVENTORY_API UCaseCursor : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Bind(UCaseGrid* InGrid);

	/** 마우스가 가리키는 칸. 격자 밖일 수도 있다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetHoverCell(FIntPoint Cell);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetHoverCell() const { return HoverCell; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsHolding() const { return HeldId != 0; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetHeldId() const { return HeldId; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetHeldRotation() const { return HeldRotation; }

	/** 집은 아이템이 놓일 왼쪽 위 칸 = 가리키는 칸 - 잡은 위치 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetTargetOrigin() const { return HoverCell - GrabOffset; }

	/** 현재 회전을 반영한 집은 아이템의 가로 x 세로 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetHeldFootprint() const;

	/** 지금 자리에 내려놓을 수 있는지 (자기 원래 자리는 빈칸으로 본다) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanDropHere() const;

	/** 가리키는 칸의 아이템을 집는다. 빈칸이면 false */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool PickUp();

	/** 집은 아이템을 시계 방향 90도 돌린다. 잡은 칸이 커서 아래에 그대로 있도록 잡은 위치도 같이 돌린다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Rotate();

	/** 내려놓기. 못 놓는 자리면 계속 들고 있고 false */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Drop();

	/** 집은 것을 취소. 아이템은 원래 자리 그대로 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Cancel();

	/** 집은 아이템을 버린다 (되돌리기 가능) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DiscardHeld();

private:
	UPROPERTY()
	TObjectPtr<UCaseGrid> Grid;

	FIntPoint HoverCell = FIntPoint::ZeroValue;
	int32 HeldId = 0;
	int32 HeldRotation = 0;
	/** 아이템 왼쪽 위에서 잡은 칸까지의 거리. 회전한 모양 기준 */
	FIntPoint GrabOffset = FIntPoint::ZeroValue;
};
