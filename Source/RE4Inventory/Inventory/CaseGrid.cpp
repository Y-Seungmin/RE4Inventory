#include "Inventory/CaseGrid.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemShape.h"

void UCaseGrid::Initialize(int32 InWidth, int32 InHeight)
{
	Width = FMath::Max(1, InWidth);
	Height = FMath::Max(1, InHeight);
	Cells.Init(0, Width * Height);
	Items.Reset();
	History.Clear();
	NextInstanceId = 1;
	MarkChanged();
}

int32 UCaseGrid::GetItemAt(FIntPoint Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Width || Cell.Y >= Height)
	{
		return 0;
	}
	return Cells[ToIndex(Cell)];
}

bool UCaseGrid::GetItem(int32 InstanceId, FInventoryItem& OutItem) const
{
	if (const FInventoryItem* Found = Items.Find(InstanceId))
	{
		OutItem = *Found;
		return true;
	}
	return false;
}

TArray<FInventoryItem> UCaseGrid::GetItems() const
{
	TArray<FInventoryItem> Result;
	Items.GenerateValueArray(Result);
	Result.Sort([](const FInventoryItem& A, const FInventoryItem& B) { return A.InstanceId < B.InstanceId; });
	return Result;
}

bool UCaseGrid::CanPlace(const UItemDefinition* Definition, FIntPoint Origin, int32 Rotation, int32 IgnoreInstanceId) const
{
	if (!Definition)
	{
		return false;
	}
	const FIntPoint Footprint = ItemShape::GetFootprint(Definition->Size, ItemShape::NormalizeRotation(Rotation));
	return CanPlaceOn(Cells, Origin, Footprint, IgnoreInstanceId);
}

bool UCaseGrid::Resize(int32 NewWidth, int32 NewHeight)
{
	if (NewWidth < Width || NewHeight < Height)
	{
		return false;
	}
	if (NewWidth == Width && NewHeight == Height)
	{
		return true;
	}
	TArray<int32> NewCells;
	NewCells.Init(0, NewWidth * NewHeight);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			NewCells[Y * NewWidth + X] = Cells[ToIndex(FIntPoint(X, Y))];
		}
	}
	Cells = MoveTemp(NewCells);
	Width = NewWidth;
	Height = NewHeight;
	MarkChanged();
	return true;
}

int32 UCaseGrid::AddItem(UItemDefinition* Definition, int32 Count)
{
	if (!Definition || Count <= 0)
	{
		return FMath::Max(Count, 0);
	}
	KnownDefinitions.Add(Definition);

	const int32 MaxStack = FMath::Max(1, Definition->MaxStack);
	int32 Remaining = Count;
	bool bChanged = false;

	// 1) 같은 종류 묶음 중 여유 있는 것부터 채운다
	TArray<int32> Ids;
	Items.GenerateKeyArray(Ids);
	Ids.Sort();
	for (int32 Id : Ids)
	{
		FInventoryItem& Item = Items[Id];
		if (Item.Definition == Definition && Item.Count < MaxStack)
		{
			const int32 Added = FMath::Min(Remaining, MaxStack - Item.Count);
			Item.Count += Added;
			Remaining -= Added;
			bChanged = true;
			if (Remaining == 0)
			{
				break;
			}
		}
	}

	// 2) 남은 개수는 새 묶음으로 만들어 빈자리에 넣는다
	while (Remaining > 0)
	{
		FIntPoint Origin;
		int32 Rotation;
		if (!FindFirstFit(Cells, Definition->Size, Origin, Rotation))
		{
			break;
		}
		FInventoryItem NewItem;
		NewItem.InstanceId = NextInstanceId++;
		NewItem.Definition = Definition;
		NewItem.Count = FMath::Min(Remaining, MaxStack);
		NewItem.Origin = Origin;
		NewItem.Rotation = Rotation;
		FillCells(Cells, Origin, ItemShape::GetFootprint(Definition->Size, Rotation), NewItem.InstanceId);
		Items.Add(NewItem.InstanceId, NewItem);
		Remaining -= NewItem.Count;
		bChanged = true;
	}

	if (bChanged)
	{
		// 새로 들어온 아이템이 자리를 차지해서 이전 기록을 되돌리면 겹칠 수 있다
		History.Clear();
		MarkChanged();
	}
	return Remaining;
}

bool UCaseGrid::MoveItem(int32 InstanceId, FIntPoint NewOrigin, int32 NewRotation)
{
	const FInventoryItem* Item = Items.Find(InstanceId);
	if (!Item)
	{
		return false;
	}
	const int32 Rotation = ItemShape::NormalizeRotation(NewRotation);
	if (Item->Origin == NewOrigin && Item->Rotation == Rotation)
	{
		return true; // 제자리에 놓은 것은 기록하지 않는다
	}
	return History.Run(MakeUnique<FMoveItemCommand>(InstanceId, NewOrigin, Rotation), *this);
}

bool UCaseGrid::DiscardItem(int32 InstanceId)
{
	return History.Run(MakeUnique<FDiscardItemCommand>(InstanceId), *this);
}

bool UCaseGrid::AutoArrange(EArrangeMode Mode)
{
	return History.Run(MakeUnique<FAutoArrangeCommand>(Mode), *this);
}

bool UCaseGrid::Undo()
{
	return History.Undo(*this);
}

bool UCaseGrid::PlaceItemRaw(const FInventoryItem& Item)
{
	if (!Item.Definition || Item.InstanceId <= 0 || Items.Contains(Item.InstanceId))
	{
		return false;
	}
	const FIntPoint Footprint = ItemShape::GetFootprint(Item.Definition->Size, Item.Rotation);
	if (!CanPlaceOn(Cells, Item.Origin, Footprint, 0))
	{
		return false;
	}
	KnownDefinitions.Add(Item.Definition);
	FillCells(Cells, Item.Origin, Footprint, Item.InstanceId);
	Items.Add(Item.InstanceId, Item);
	NextInstanceId = FMath::Max(NextInstanceId, Item.InstanceId + 1);
	MarkChanged();
	return true;
}

bool UCaseGrid::MoveItemRaw(int32 InstanceId, FIntPoint NewOrigin, int32 NewRotation)
{
	FInventoryItem* Item = Items.Find(InstanceId);
	if (!Item)
	{
		return false;
	}
	const int32 Rotation = ItemShape::NormalizeRotation(NewRotation);
	const FIntPoint NewFootprint = ItemShape::GetFootprint(Item->Definition->Size, Rotation);
	// 자기 자신이 있던 칸은 빈칸으로 본다. 그래야 한 칸 옆으로 옮길 때 원래 자리와 겹쳐 실패하지 않는다
	if (!CanPlaceOn(Cells, NewOrigin, NewFootprint, InstanceId))
	{
		return false;
	}
	FillCells(Cells, Item->Origin, ItemShape::GetFootprint(Item->Definition->Size, Item->Rotation), 0);
	Item->Origin = NewOrigin;
	Item->Rotation = Rotation;
	FillCells(Cells, NewOrigin, NewFootprint, InstanceId);
	MarkChanged();
	return true;
}

bool UCaseGrid::RemoveItemRaw(int32 InstanceId, FInventoryItem* OutRemoved)
{
	const FInventoryItem* Item = Items.Find(InstanceId);
	if (!Item)
	{
		return false;
	}
	FillCells(Cells, Item->Origin, ItemShape::GetFootprint(Item->Definition->Size, Item->Rotation), 0);
	if (OutRemoved)
	{
		*OutRemoved = *Item;
	}
	Items.Remove(InstanceId);
	MarkChanged();
	return true;
}

TArray<FItemPlacement> UCaseGrid::CaptureLayout() const
{
	TArray<FItemPlacement> Layout;
	for (const FInventoryItem& Item : GetItems())
	{
		FItemPlacement& Placement = Layout.AddDefaulted_GetRef();
		Placement.InstanceId = Item.InstanceId;
		Placement.Origin = Item.Origin;
		Placement.Rotation = Item.Rotation;
	}
	return Layout;
}

bool UCaseGrid::ApplyLayout(const TArray<FItemPlacement>& Layout)
{
	if (Layout.Num() != Items.Num())
	{
		return false;
	}
	TArray<int32> NewCells;
	NewCells.Init(0, Width * Height);
	for (const FItemPlacement& Placement : Layout)
	{
		const FInventoryItem* Item = Items.Find(Placement.InstanceId);
		if (!Item)
		{
			return false;
		}
		const FIntPoint Footprint = ItemShape::GetFootprint(Item->Definition->Size, Placement.Rotation);
		if (!CanPlaceOn(NewCells, Placement.Origin, Footprint, 0))
		{
			return false;
		}
		FillCells(NewCells, Placement.Origin, Footprint, Placement.InstanceId);
	}
	// 검사를 모두 통과한 뒤에만 실제 상태를 바꾼다
	Cells = MoveTemp(NewCells);
	for (const FItemPlacement& Placement : Layout)
	{
		FInventoryItem& Item = Items[Placement.InstanceId];
		Item.Origin = Placement.Origin;
		Item.Rotation = ItemShape::NormalizeRotation(Placement.Rotation);
	}
	MarkChanged();
	return true;
}

bool UCaseGrid::ComputeArrangement(EArrangeMode Mode, TArray<FItemPlacement>& OutLayout) const
{
	TArray<FInventoryItem> Sorted = GetItems();

	auto Area = [](const FInventoryItem& Item) { return Item.Definition->Size.X * Item.Definition->Size.Y; };
	auto TieBreak = [](const FInventoryItem& A, const FInventoryItem& B)
	{
		if (A.Definition->ItemId != B.Definition->ItemId)
		{
			return A.Definition->ItemId.LexicalLess(B.Definition->ItemId);
		}
		return A.InstanceId < B.InstanceId;
	};

	// 전략: 정렬 기준만 바꾸고 배치 방법(first-fit)은 같다
	if (Mode == EArrangeMode::SpaceFirst)
	{
		Sorted.Sort([&](const FInventoryItem& A, const FInventoryItem& B)
		{
			if (Area(A) != Area(B)) return Area(A) > Area(B);
			if (A.Definition->Category != B.Definition->Category) return A.Definition->Category < B.Definition->Category;
			return TieBreak(A, B);
		});
	}
	else
	{
		Sorted.Sort([&](const FInventoryItem& A, const FInventoryItem& B)
		{
			if (A.Definition->Category != B.Definition->Category) return A.Definition->Category < B.Definition->Category;
			if (Area(A) != Area(B)) return Area(A) > Area(B);
			return TieBreak(A, B);
		});
	}

	// 실제 격자는 건드리지 않고 빈 임시 격자에 넣어 본다
	TArray<int32> TempCells;
	TempCells.Init(0, Width * Height);
	OutLayout.Reset();
	for (const FInventoryItem& Item : Sorted)
	{
		FIntPoint Origin;
		int32 Rotation;
		if (!FindFirstFit(TempCells, Item.Definition->Size, Origin, Rotation))
		{
			OutLayout.Reset();
			return false;
		}
		FillCells(TempCells, Origin, ItemShape::GetFootprint(Item.Definition->Size, Rotation), Item.InstanceId);
		FItemPlacement& Placement = OutLayout.AddDefaulted_GetRef();
		Placement.InstanceId = Item.InstanceId;
		Placement.Origin = Origin;
		Placement.Rotation = Rotation;
	}
	return true;
}

void UCaseGrid::FillCells(TArray<int32>& InCells, FIntPoint Origin, FIntPoint Footprint, int32 Value) const
{
	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			InCells[ToIndex(FIntPoint(Origin.X + X, Origin.Y + Y))] = Value;
		}
	}
}

bool UCaseGrid::CanPlaceOn(const TArray<int32>& InCells, FIntPoint Origin, FIntPoint Footprint, int32 IgnoreInstanceId) const
{
	if (Origin.X < 0 || Origin.Y < 0 || Origin.X + Footprint.X > Width || Origin.Y + Footprint.Y > Height)
	{
		return false;
	}
	for (int32 Y = 0; Y < Footprint.Y; ++Y)
	{
		for (int32 X = 0; X < Footprint.X; ++X)
		{
			const int32 Occupant = InCells[ToIndex(FIntPoint(Origin.X + X, Origin.Y + Y))];
			if (Occupant != 0 && Occupant != IgnoreInstanceId)
			{
				return false;
			}
		}
	}
	return true;
}

bool UCaseGrid::FindFirstFit(const TArray<int32>& InCells, FIntPoint BaseSize, FIntPoint& OutOrigin, int32& OutRotation) const
{
	for (int32 Rotation = 0; Rotation < 2; ++Rotation)
	{
		const FIntPoint Footprint = ItemShape::GetFootprint(BaseSize, Rotation);
		if (Rotation == 1 && Footprint == BaseSize)
		{
			break; // 정사각형은 돌려도 같다
		}
		for (int32 Y = 0; Y + Footprint.Y <= Height; ++Y)
		{
			for (int32 X = 0; X + Footprint.X <= Width; ++X)
			{
				if (CanPlaceOn(InCells, FIntPoint(X, Y), Footprint, 0))
				{
					OutOrigin = FIntPoint(X, Y);
					OutRotation = Rotation;
					return true;
				}
			}
		}
	}
	return false;
}

void UCaseGrid::MarkChanged()
{
	++Version;
	OnChanged.Broadcast();
}
