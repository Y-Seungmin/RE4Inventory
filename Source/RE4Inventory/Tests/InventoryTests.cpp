#include "Misc/AutomationTest.h"
#include "Inventory/CaseGrid.h"
#include "Inventory/ItemDefinition.h"
#include "View/CaseViewActor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace InventoryTest
{
	UItemDefinition* MakeDef(FName Id, FIntPoint Size, EItemCategory Category, int32 MaxStack = 1)
	{
		UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
		Def->ItemId = Id;
		Def->Size = Size;
		Def->Category = Category;
		Def->MaxStack = MaxStack;
		return Def;
	}

	UCaseGrid* MakeGrid(int32 Width, int32 Height)
	{
		UCaseGrid* Grid = NewObject<UCaseGrid>(GetTransientPackage());
		Grid->Initialize(Width, Height);
		return Grid;
	}

	FItemPlacement PlacementOf(UCaseGrid* Grid, int32 Id)
	{
		FInventoryItem Item;
		Grid->GetItem(Id, Item);
		FItemPlacement P;
		P.InstanceId = Id;
		P.Origin = Item.Origin;
		P.Rotation = Item.Rotation;
		return P;
	}

	bool SameLayout(const TArray<FItemPlacement>& A, const TArray<FItemPlacement>& B)
	{
		if (A.Num() != B.Num()) return false;
		for (int32 i = 0; i < A.Num(); ++i)
		{
			if (A[i].InstanceId != B[i].InstanceId || A[i].Origin != B[i].Origin || A[i].Rotation != B[i].Rotation) return false;
		}
		return true;
	}
}

using namespace InventoryTest;

#define InventoryTestFlags (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridPlacementTest, "RE4Inventory.CaseGrid.Placement", InventoryTestFlags)
bool FCaseGridPlacementTest::RunTest(const FString& Parameters)
{
	UItemDefinition* Gun = MakeDef("SG09R", FIntPoint(3, 2), EItemCategory::Weapon);
	UCaseGrid* Grid = MakeGrid(10, 7);

	TestEqual(TEXT("빈 케이스에 권총이 들어감"), Grid->AddItem(Gun, 1), 0);
	TestEqual(TEXT("왼쪽 위 칸을 차지"), Grid->GetItemAt(FIntPoint(0, 0)), 1);
	TestEqual(TEXT("3x2 오른쪽 아래 끝 칸"), Grid->GetItemAt(FIntPoint(2, 1)), 1);
	TestEqual(TEXT("그 바깥은 빈칸"), Grid->GetItemAt(FIntPoint(3, 0)), 0);

	TestFalse(TEXT("격자 밖은 거부"), Grid->CanPlace(Gun, FIntPoint(8, 0), 0));
	TestFalse(TEXT("다른 아이템과 겹치면 거부"), Grid->CanPlace(Gun, FIntPoint(1, 1), 0));
	TestTrue(TEXT("자기 자신과 겹치는 건 허용"), Grid->CanPlace(Gun, FIntPoint(1, 1), 0, 1));

	TestTrue(TEXT("한 칸 옆으로 이동"), Grid->MoveItem(1, FIntPoint(1, 0), 0));
	TestEqual(TEXT("원래 자리 (0,0)은 비워짐"), Grid->GetItemAt(FIntPoint(0, 0)), 0);

	TestTrue(TEXT("90도 회전해서 세움"), Grid->MoveItem(1, FIntPoint(5, 3), 1));
	TestEqual(TEXT("세운 모양 2x3의 아래 끝"), Grid->GetItemAt(FIntPoint(6, 5)), 1);
	TestEqual(TEXT("세웠으니 가로 세 번째 칸은 빈칸"), Grid->GetItemAt(FIntPoint(7, 3)), 0);
	TestFalse(TEXT("세운 상태로 아래 끝을 넘으면 거부"), Grid->MoveItem(1, FIntPoint(0, 5), 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridStackTest, "RE4Inventory.CaseGrid.StackAndFull", InventoryTestFlags)
bool FCaseGridStackTest::RunTest(const FString& Parameters)
{
	UItemDefinition* Ammo = MakeDef("HandgunAmmo", FIntPoint(1, 2), EItemCategory::Ammo, 50);
	UCaseGrid* Grid = MakeGrid(10, 7);

	TestEqual(TEXT("70발은 50 + 20 두 묶음"), Grid->AddItem(Ammo, 70), 0);
	TestEqual(TEXT("묶음 2개"), Grid->GetItems().Num(), 2);
	TestEqual(TEXT("40발 더 받으면 20짜리를 먼저 채우고 10발 새 묶음"), Grid->AddItem(Ammo, 40), 0);
	TArray<FInventoryItem> Items = Grid->GetItems();
	TestEqual(TEXT("묶음 3개"), Items.Num(), 3);
	if (Items.Num() == 3)
	{
		TestEqual(TEXT("첫 묶음"), Items[0].Count, 50);
		TestEqual(TEXT("둘째 묶음"), Items[1].Count, 50);
		TestEqual(TEXT("셋째 묶음"), Items[2].Count, 10);
	}

	UItemDefinition* Rifle = MakeDef("Rifle", FIntPoint(3, 2), EItemCategory::Weapon);
	UCaseGrid* Small = MakeGrid(2, 2);
	TestEqual(TEXT("눕혀도 세워도 안 들어가면 그대로 돌려줌"), Small->AddItem(Rifle, 1), 1);
	TestEqual(TEXT("아무것도 안 들어감"), Small->GetItems().Num(), 0);

	UItemDefinition* Tall = MakeDef("Tall", FIntPoint(3, 1), EItemCategory::Material);
	UCaseGrid* Narrow = MakeGrid(1, 3);
	TestEqual(TEXT("눕힌 방향이 안 맞으면 세워서 넣음"), Narrow->AddItem(Tall, 1), 0);
	FInventoryItem Placed;
	Narrow->GetItem(1, Placed);
	TestEqual(TEXT("회전 1단계"), Placed.Rotation, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridUndoTest, "RE4Inventory.CaseGrid.Undo", InventoryTestFlags)
bool FCaseGridUndoTest::RunTest(const FString& Parameters)
{
	UItemDefinition* Gun = MakeDef("SG09R", FIntPoint(3, 2), EItemCategory::Weapon);
	UItemDefinition* Herb = MakeDef("GreenHerb", FIntPoint(1, 1), EItemCategory::Recovery);
	UCaseGrid* Grid = MakeGrid(10, 7);
	Grid->AddItem(Gun, 1);
	Grid->AddItem(Herb, 1);
	TestFalse(TEXT("획득만 했으면 되돌릴 게 없음"), Grid->CanUndo());

	const TArray<FItemPlacement> Start = Grid->CaptureLayout();
	TestTrue(TEXT("이동"), Grid->MoveItem(1, FIntPoint(4, 4), 1));
	TestTrue(TEXT("버리기"), Grid->DiscardItem(2));
	TestEqual(TEXT("버린 뒤 1개"), Grid->GetItems().Num(), 1);

	TestTrue(TEXT("버리기 되돌리기"), Grid->Undo());
	TestEqual(TEXT("다시 2개"), Grid->GetItems().Num(), 2);
	TestTrue(TEXT("이동 되돌리기"), Grid->Undo());
	TestTrue(TEXT("처음 배치와 같음"), SameLayout(Grid->CaptureLayout(), Start));
	TestFalse(TEXT("기록을 다 썼음"), Grid->Undo());

	Grid->MoveItem(1, FIntPoint(4, 4), 0);
	UItemDefinition* Ammo = MakeDef("HandgunAmmo", FIntPoint(1, 2), EItemCategory::Ammo, 50);
	Grid->AddItem(Ammo, 10);
	TestFalse(TEXT("새 아이템을 얻으면 기록을 비움"), Grid->CanUndo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridArrangeTest, "RE4Inventory.CaseGrid.AutoArrange", InventoryTestFlags)
bool FCaseGridArrangeTest::RunTest(const FString& Parameters)
{
	// 정리 후 되돌리기
	UItemDefinition* Gun = MakeDef("SG09R", FIntPoint(3, 2), EItemCategory::Weapon);
	UItemDefinition* Herb = MakeDef("GreenHerb", FIntPoint(1, 1), EItemCategory::Recovery);
	UItemDefinition* Ammo = MakeDef("HandgunAmmo", FIntPoint(1, 2), EItemCategory::Ammo, 50);
	UCaseGrid* Grid = MakeGrid(10, 7);
	Grid->AddItem(Herb, 1);
	Grid->AddItem(Ammo, 30);
	Grid->AddItem(Gun, 1);
	Grid->MoveItem(1, FIntPoint(9, 6), 0);
	Grid->MoveItem(3, FIntPoint(5, 3), 1);
	const TArray<FItemPlacement> Messy = Grid->CaptureLayout();

	TestTrue(TEXT("공간 우선 정리 성공"), Grid->AutoArrange(EArrangeMode::SpaceFirst));
	TestEqual(TEXT("가장 큰 권총이 왼쪽 위"), Grid->GetItemAt(FIntPoint(0, 0)), 3);
	TestTrue(TEXT("정리 되돌리기"), Grid->Undo());
	TestTrue(TEXT("정리 전 배치로 돌아옴"), SameLayout(Grid->CaptureLayout(), Messy));

	// 종류 우선 vs 공간 우선: 5x1 칸에 수류탄(1x1), 허브 묶음(2x1), 탄약(1x1)
	UItemDefinition* Grenade = MakeDef("Grenade", FIntPoint(1, 1), EItemCategory::Throwable);
	UItemDefinition* Herbs = MakeDef("MixedHerb", FIntPoint(2, 1), EItemCategory::Recovery);
	UItemDefinition* Shell = MakeDef("Shell", FIntPoint(1, 1), EItemCategory::Ammo);
	UCaseGrid* Row = MakeGrid(5, 1);
	Row->AddItem(Grenade, 1); // id 1
	Row->AddItem(Herbs, 1);   // id 2
	Row->AddItem(Shell, 1);   // id 3

	TestTrue(TEXT("공간 우선"), Row->AutoArrange(EArrangeMode::SpaceFirst));
	TestEqual(TEXT("공간 우선: 큰 허브가 맨 앞"), PlacementOf(Row, 2).Origin.X, 0);
	TestEqual(TEXT("공간 우선: 그다음 탄약"), PlacementOf(Row, 3).Origin.X, 2);

	TestTrue(TEXT("종류 우선"), Row->AutoArrange(EArrangeMode::CategoryFirst));
	TestEqual(TEXT("종류 우선: 탄약이 맨 앞"), PlacementOf(Row, 3).Origin.X, 0);
	TestEqual(TEXT("종류 우선: 그다음 회복"), PlacementOf(Row, 2).Origin.X, 1);
	TestEqual(TEXT("종류 우선: 투척은 마지막"), PlacementOf(Row, 1).Origin.X, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridGreedyLimitTest, "RE4Inventory.CaseGrid.GreedyLimit", InventoryTestFlags)
bool FCaseGridGreedyLimitTest::RunTest(const FString& Parameters)
{
	// 4x3 칸에 3x2 두 개: 둘 다 세우면 꽉 차지만, 큰 것부터 눕혀 넣는 방식은 두 번째를 못 넣는다
	UItemDefinition* Box = MakeDef("Box", FIntPoint(3, 2), EItemCategory::Material);
	UCaseGrid* Grid = MakeGrid(4, 3);
	Grid->AddItem(Box, 1);
	TestTrue(TEXT("첫 번째를 세움"), Grid->MoveItem(1, FIntPoint(0, 0), 1));
	TestEqual(TEXT("두 번째도 세워서 들어감"), Grid->AddItem(Box, 1), 0);
	const TArray<FItemPlacement> Packed = Grid->CaptureLayout();

	TestFalse(TEXT("자동 정리는 실패"), Grid->AutoArrange(EArrangeMode::SpaceFirst));
	TestTrue(TEXT("실패하면 배치가 그대로"), SameLayout(Grid->CaptureLayout(), Packed));
	TestFalse(TEXT("실패한 정리는 기록에 안 남음"), Grid->CanUndo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseGridResizeTest, "RE4Inventory.CaseGrid.Resize", InventoryTestFlags)
bool FCaseGridResizeTest::RunTest(const FString& Parameters)
{
	UItemDefinition* Gun = MakeDef("SG09R", FIntPoint(3, 2), EItemCategory::Weapon);
	UCaseGrid* Grid = MakeGrid(10, 7);
	Grid->AddItem(Gun, 1);
	Grid->MoveItem(1, FIntPoint(7, 5), 0);

	TestFalse(TEXT("줄이기는 거부"), Grid->Resize(9, 7));
	TestTrue(TEXT("확장"), Grid->Resize(13, 9));
	TestEqual(TEXT("확장 후에도 같은 자리"), Grid->GetItemAt(FIntPoint(9, 6)), 1);
	TestEqual(TEXT("새로 생긴 칸은 빈칸"), Grid->GetItemAt(FIntPoint(12, 8)), 0);
	TestTrue(TEXT("되돌리기 기록도 유지"), Grid->Undo());
	TestEqual(TEXT("이동 전 자리로"), Grid->GetItemAt(FIntPoint(0, 0)), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewFitTest, "RE4Inventory.CaseView.MeshFit", InventoryTestFlags)
bool FCaseViewFitTest::RunTest(const FString& Parameters)
{
	auto Placed = [](const FBox& Bounds, const UItemDefinition& Def, FIntPoint Origin, int32 Rotation)
	{
		return Bounds.TransformBy(ACaseViewActor::ComputeItemTransform(Bounds, Def, Origin, Rotation, 10.0f, 0.0f));
	};
	auto Near = [](const FVector& A, const FVector& B) { return A.Equals(B, 0.01f); };

	// 가운데 피벗 큐브(100cm)를 3x2칸에: 짧은 쪽(20cm)에 맞춰 0.2배
	UItemDefinition* Gun = MakeDef("Gun", FIntPoint(3, 2), EItemCategory::Weapon);
	Gun->FitRatio = 1.0f;
	const FBox Cube(FVector(-50.0f), FVector(50.0f));
	const FBox A = Placed(Cube, *Gun, FIntPoint(0, 0), 0);
	TestTrue(TEXT("칸 영역 가운데"), Near(A.GetCenter(), FVector(15.0f, 10.0f, 10.0f)));
	TestTrue(TEXT("바닥이 케이스 바닥에 닿음"), FMath::IsNearlyZero(A.Min.Z, 0.01f));

	// 모서리 피벗 모델을 세워서(회전 1) (3,4)에: 1x2칸 영역에 딱 맞음
	UItemDefinition* Bar = MakeDef("Bar", FIntPoint(2, 1), EItemCategory::Material);
	Bar->FitRatio = 1.0f;
	const FBox CornerPivot(FVector(0.0f, 0.0f, 0.0f), FVector(200.0f, 100.0f, 50.0f));
	const FBox B = Placed(CornerPivot, *Bar, FIntPoint(3, 4), 1);
	TestTrue(TEXT("피벗이 모서리여도 영역 가운데"), Near(B.GetCenter(), FVector(35.0f, 50.0f, 2.5f)));
	TestTrue(TEXT("세웠으니 가로 10, 세로 20"), Near(B.GetSize(), FVector(10.0f, 20.0f, 5.0f)));

	// 긴 쪽이 Y로 만들어진 모델을 방향 보정(Yaw 90)으로 눕힘
	UItemDefinition* Rifle = MakeDef("Rifle", FIntPoint(4, 1), EItemCategory::Weapon);
	Rifle->FitRatio = 1.0f;
	Rifle->MeshRotation = FRotator(0.0f, 90.0f, 0.0f);
	const FBox AlongY(FVector(-25.0f, -100.0f, 0.0f), FVector(25.0f, 100.0f, 10.0f));
	const FBox C = Placed(AlongY, *Rifle, FIntPoint(0, 0), 0);
	TestTrue(TEXT("보정 후 긴 쪽이 가로 4칸"), Near(C.GetSize(), FVector(40.0f, 10.0f, 2.0f)));
	TestTrue(TEXT("보정 후에도 영역 가운데"), Near(C.GetCenter(), FVector(20.0f, 5.0f, 1.0f)));
	return true;
}

#endif
