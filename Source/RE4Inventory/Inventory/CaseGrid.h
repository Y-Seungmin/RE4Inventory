#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/InventoryTypes.h"
#include "Inventory/InventoryCommands.h"
#include "CaseGrid.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCaseChanged);

/**
 * 아타셰 케이스 격자. 각 칸에 그 칸을 차지한 아이템 번호를 저장하고 빈칸은 0으로 둔다.
 * 화면(3D 케이스, UMG)은 이 클래스를 직접 들여다보지 않고 OnChanged를 받아서 다시 그린다.
 */
UCLASS(BlueprintType)
class RE4INVENTORY_API UCaseGrid : public UObject
{
	GENERATED_BODY()

public:
	/** 본작 시작 크기: 가로 10 x 세로 7 = 70칸 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Initialize(int32 InWidth = 10, int32 InHeight = 7);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetWidth() const { return Width; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetHeight() const { return Height; }

	/** 바뀔 때마다 1씩 오른다. 화면 쪽에서 다시 그릴지 판단할 때 쓴다 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetVersion() const { return Version; }

	/** 해당 칸을 차지한 아이템 번호. 빈칸이나 격자 밖이면 0 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemAt(FIntPoint Cell) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItem(int32 InstanceId, FInventoryItem& OutItem) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FInventoryItem> GetItems() const;

	/**
	 * 놓을 수 있는지 검사. 점유할 칸이 모두 격자 안이고, 비어 있거나 IgnoreInstanceId(옮기는 자기 자신)여야 한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanPlace(const UItemDefinition* Definition, FIntPoint Origin, int32 Rotation, int32 IgnoreInstanceId = 0) const;

	/** 케이스 확장. 줄이는 것은 거부하고 기존 배치는 그대로 둔다 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Resize(int32 NewWidth, int32 NewHeight);

	/**
	 * 아이템 획득. 같은 종류의 묶음에 먼저 채우고, 남은 개수는 최대 묶음 단위로 나눠 빈자리에 넣는다.
	 * 반환값은 자리가 없어 못 넣은 개수. 획득이 일어나면 되돌리기 기록은 비운다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UItemDefinition* Definition, int32 Count = 1);

	/** 이동·회전 (되돌리기 가능) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 InstanceId, FIntPoint NewOrigin, int32 NewRotation);

	/** 버리기 (되돌리기 가능) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DiscardItem(int32 InstanceId);

	/** 자동 정리. 하나라도 못 넣으면 아무것도 바꾸지 않고 false (되돌리기 가능) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AutoArrange(EArrangeMode Mode = EArrangeMode::SpaceFirst);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Undo();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanUndo() const { return History.CanUndo(); }

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnCaseChanged OnChanged;

	// ---- 커맨드가 쓰는 기본 동작 (기록을 남기지 않음) ----

	/** Item.InstanceId 그대로 Item.Origin/Rotation 위치에 넣는다 */
	bool PlaceItemRaw(const FInventoryItem& Item);
	bool MoveItemRaw(int32 InstanceId, FIntPoint NewOrigin, int32 NewRotation);
	bool RemoveItemRaw(int32 InstanceId, FInventoryItem* OutRemoved = nullptr);

	TArray<FItemPlacement> CaptureLayout() const;

	/** 모든 아이템의 배치를 한 번에 바꾼다. 빈 임시 격자에서 먼저 검사하므로 실패하면 아무것도 안 바뀐다 */
	bool ApplyLayout(const TArray<FItemPlacement>& Layout);

	/** 자동 정리 결과를 계산만 한다. 아이템과 격자는 건드리지 않는다 */
	bool ComputeArrangement(EArrangeMode Mode, TArray<FItemPlacement>& OutLayout) const;

private:
	int32 ToIndex(FIntPoint Cell) const { return Cell.Y * Width + Cell.X; }
	void FillCells(TArray<int32>& InCells, FIntPoint Origin, FIntPoint Footprint, int32 Value) const;
	bool CanPlaceOn(const TArray<int32>& InCells, FIntPoint Origin, FIntPoint Footprint, int32 IgnoreInstanceId) const;
	/** 기본 방향으로 왼쪽 위부터 행 단위로 훑고, 없으면 90도 돌려서 다시 훑는다 (first-fit) */
	bool FindFirstFit(const TArray<int32>& InCells, FIntPoint BaseSize, FIntPoint& OutOrigin, int32& OutRotation) const;
	void MarkChanged();

	int32 Width = 0;
	int32 Height = 0;
	int32 NextInstanceId = 1;
	int32 Version = 0;

	/** 칸마다 차지한 아이템 번호. 빈칸은 0 */
	TArray<int32> Cells;

	UPROPERTY()
	TMap<int32, FInventoryItem> Items;

	/** 버려서 되돌리기 기록에만 남은 아이템의 정의가 GC로 사라지지 않게 잡아 둔다 */
	UPROPERTY()
	TSet<TObjectPtr<UItemDefinition>> KnownDefinitions;

	FInventoryHistory History;
};
