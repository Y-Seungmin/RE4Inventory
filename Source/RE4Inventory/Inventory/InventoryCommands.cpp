#include "Inventory/InventoryCommands.h"
#include "Inventory/CaseGrid.h"

bool FMoveItemCommand::Execute(UCaseGrid& Grid)
{
	FInventoryItem Item;
	if (!Grid.GetItem(Id, Item))
	{
		return false;
	}
	FromOrigin = Item.Origin;
	FromRotation = Item.Rotation;
	return Grid.MoveItemRaw(Id, ToOrigin, ToRotation);
}

bool FMoveItemCommand::Undo(UCaseGrid& Grid)
{
	return Grid.MoveItemRaw(Id, FromOrigin, FromRotation);
}

bool FDiscardItemCommand::Execute(UCaseGrid& Grid)
{
	return Grid.RemoveItemRaw(Id, &Removed);
}

bool FDiscardItemCommand::Undo(UCaseGrid& Grid)
{
	return Grid.PlaceItemRaw(Removed);
}

bool FAutoArrangeCommand::Execute(UCaseGrid& Grid)
{
	TArray<FItemPlacement> After;
	if (!Grid.ComputeArrangement(Mode, After))
	{
		// 하나라도 못 넣으면 원래 배치를 그대로 둔다
		return false;
	}
	Before = Grid.CaptureLayout();
	return Grid.ApplyLayout(After);
}

bool FAutoArrangeCommand::Undo(UCaseGrid& Grid)
{
	return Grid.ApplyLayout(Before);
}

bool FInventoryHistory::Run(TUniquePtr<IInventoryCommand> Command, UCaseGrid& Grid)
{
	if (!Command.IsValid() || !Command->Execute(Grid))
	{
		return false;
	}
	Stack.Add(MoveTemp(Command));
	if (Stack.Num() > MaxDepth)
	{
		Stack.RemoveAt(0);
	}
	return true;
}

bool FInventoryHistory::Undo(UCaseGrid& Grid)
{
	if (Stack.Num() == 0)
	{
		return false;
	}
	TUniquePtr<IInventoryCommand> Command = Stack.Pop();
	if (!Command->Undo(Grid))
	{
		// 기록과 실제 상태가 어긋났다는 뜻이므로 남은 기록은 믿을 수 없다
		Clear();
		return false;
	}
	return true;
}
