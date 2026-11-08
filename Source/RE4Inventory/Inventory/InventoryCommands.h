#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryTypes.h"

class UCaseGrid;

/**
 * 커맨드 패턴. 플레이어 조작 하나(이동, 버리기, 자동 정리)를 객체로 만들고
 * 실행(Execute)과 되돌리기(Undo)를 같이 가진다.
 */
class IInventoryCommand
{
public:
	virtual ~IInventoryCommand() = default;
	virtual bool Execute(UCaseGrid& Grid) = 0;
	virtual bool Undo(UCaseGrid& Grid) = 0;
};

/** 이동 + 회전 */
class FMoveItemCommand final : public IInventoryCommand
{
public:
	FMoveItemCommand(int32 InId, FIntPoint InToOrigin, int32 InToRotation)
		: Id(InId), ToOrigin(InToOrigin), ToRotation(InToRotation) {}

	virtual bool Execute(UCaseGrid& Grid) override;
	virtual bool Undo(UCaseGrid& Grid) override;

private:
	int32 Id;
	FIntPoint ToOrigin;
	int32 ToRotation;
	FIntPoint FromOrigin = FIntPoint::ZeroValue;
	int32 FromRotation = 0;
};

/** 버리기. 버린 아이템을 들고 있다가 되돌리면 원래 자리에 다시 놓는다 */
class FDiscardItemCommand final : public IInventoryCommand
{
public:
	explicit FDiscardItemCommand(int32 InId) : Id(InId) {}

	virtual bool Execute(UCaseGrid& Grid) override;
	virtual bool Undo(UCaseGrid& Grid) override;

private:
	int32 Id;
	FInventoryItem Removed;
};

/** 자동 정리. 정리 전 배치를 저장해 두었다가 되돌린다 */
class FAutoArrangeCommand final : public IInventoryCommand
{
public:
	explicit FAutoArrangeCommand(EArrangeMode InMode) : Mode(InMode) {}

	virtual bool Execute(UCaseGrid& Grid) override;
	virtual bool Undo(UCaseGrid& Grid) override;

private:
	EArrangeMode Mode;
	TArray<FItemPlacement> Before;
};

/** 실행한 커맨드를 쌓아 두는 되돌리기 기록 */
class FInventoryHistory
{
public:
	static constexpr int32 MaxDepth = 50;

	/** 실행에 성공한 커맨드만 기록에 쌓는다 */
	bool Run(TUniquePtr<IInventoryCommand> Command, UCaseGrid& Grid);
	bool Undo(UCaseGrid& Grid);
	void Clear() { Stack.Reset(); }
	bool CanUndo() const { return Stack.Num() > 0; }

private:
	TArray<TUniquePtr<IInventoryCommand>> Stack;
};
