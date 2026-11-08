#include "Inventory/CaseCursor.h"
#include "Inventory/CaseGrid.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemShape.h"

void UCaseCursor::Bind(UCaseGrid* InGrid)
{
	Grid = InGrid;
	Cancel();
}

void UCaseCursor::SetHoverCell(FIntPoint Cell)
{
	HoverCell = Cell;
}

FIntPoint UCaseCursor::GetHeldFootprint() const
{
	FInventoryItem Item;
	if (!Grid || !Grid->GetItem(HeldId, Item) || !Item.Definition)
	{
		return FIntPoint::ZeroValue;
	}
	return ItemShape::GetFootprint(Item.Definition->Size, HeldRotation);
}

bool UCaseCursor::CanDropHere() const
{
	FInventoryItem Item;
	if (!Grid || !Grid->GetItem(HeldId, Item))
	{
		return false;
	}
	return Grid->CanPlace(Item.Definition, GetTargetOrigin(), HeldRotation, HeldId);
}

bool UCaseCursor::PickUp()
{
	if (!Grid || IsHolding())
	{
		return false;
	}
	FInventoryItem Item;
	if (!Grid->GetItem(Grid->GetItemAt(HoverCell), Item))
	{
		return false;
	}
	HeldId = Item.InstanceId;
	HeldRotation = Item.Rotation;
	GrabOffset = HoverCell - Item.Origin;
	return true;
}

bool UCaseCursor::Rotate()
{
	const FIntPoint Before = GetHeldFootprint();
	if (Before == FIntPoint::ZeroValue)
	{
		return false;
	}
	GrabOffset = ItemShape::Rotate90(GrabOffset, Before);
	HeldRotation = ItemShape::NormalizeRotation(HeldRotation + 1);
	return true;
}

bool UCaseCursor::Drop()
{
	if (!IsHolding() || !CanDropHere())
	{
		return false;
	}
	const int32 Id = HeldId;
	const FIntPoint Origin = GetTargetOrigin();
	const int32 Rotation = HeldRotation;
	// MoveItem이 OnChanged를 보내기 전에 손을 비워 둬야 화면이 '들고 있음'으로 그리지 않는다
	Cancel();
	return Grid->MoveItem(Id, Origin, Rotation);
}

void UCaseCursor::Cancel()
{
	HeldId = 0;
	HeldRotation = 0;
	GrabOffset = FIntPoint::ZeroValue;
}

bool UCaseCursor::DiscardHeld()
{
	if (!IsHolding() || !Grid)
	{
		return false;
	}
	const int32 Id = HeldId;
	Cancel();
	return Grid->DiscardItem(Id);
}
