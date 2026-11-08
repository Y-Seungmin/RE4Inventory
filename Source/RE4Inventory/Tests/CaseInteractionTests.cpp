#include "Misc/AutomationTest.h"
#include "Inventory/CaseCursor.h"
#include "Inventory/CaseGrid.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemShape.h"
#include "View/CaseViewActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace CaseInteractionTest
{
	UItemDefinition* MakeDef(FName Id, FIntPoint Size)
	{
		UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
		Def->ItemId = Id;
		Def->Size = Size;
		return Def;
	}

	FInventoryItem ItemOf(UCaseGrid* Grid, int32 Id)
	{
		FInventoryItem Item;
		Grid->GetItem(Id, Item);
		return Item;
	}
}

using namespace CaseInteractionTest;

#define CaseTestFlags (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseCursorRotateDropTest, "RE4Inventory.CaseCursor.PickRotateDropUndo", CaseTestFlags)
bool FCaseCursorRotateDropTest::RunTest(const FString& Parameters)
{
	UCaseGrid* Grid = NewObject<UCaseGrid>(GetTransientPackage());
	Grid->Initialize(10, 7);
	Grid->AddItem(MakeDef("Gun", FIntPoint(3, 2)), 1); // (0,0) 3x2
	UCaseCursor* Cursor = NewObject<UCaseCursor>(GetTransientPackage());
	Cursor->Bind(Grid);

	Cursor->SetHoverCell(FIntPoint(5, 5));
	TestFalse(TEXT("빈칸은 못 집음"), Cursor->PickUp());

	Cursor->SetHoverCell(FIntPoint(2, 1));
	TestTrue(TEXT("권총 오른쪽 아래 칸을 잡아 집음"), Cursor->PickUp());
	TestEqual(TEXT("집은 자리 그대로면 놓일 자리도 원래 자리"), Cursor->GetTargetOrigin(), FIntPoint(0, 0));
	TestTrue(TEXT("제자리는 놓을 수 있음 (자기 자리는 빈칸 취급)"), Cursor->CanDropHere());

	TestTrue(TEXT("90도 회전"), Cursor->Rotate());
	TestEqual(TEXT("회전하면 3x2 -> 2x3"), Cursor->GetHeldFootprint(), FIntPoint(2, 3));
	// 잡은 칸 (2,1)은 2x3 안에서 (0,2)로 간다 -> 왼쪽 위는 (2,-1)
	TestEqual(TEXT("잡은 칸이 커서 아래에 남도록 왼쪽 위가 바뀜"), Cursor->GetTargetOrigin(), FIntPoint(2, -1));
	TestFalse(TEXT("격자 밖으로 나가면 못 놓음"), Cursor->CanDropHere());
	TestFalse(TEXT("못 놓는 자리에서 놓기 실패"), Cursor->Drop());
	TestTrue(TEXT("실패해도 계속 들고 있음"), Cursor->IsHolding());
	TestEqual(TEXT("집고 있는 동안 격자는 그대로"), ItemOf(Grid, 1).Origin, FIntPoint(0, 0));

	Cursor->SetHoverCell(FIntPoint(2, 3));
	TestTrue(TEXT("안쪽으로 옮기면 놓을 수 있음"), Cursor->CanDropHere());
	TestTrue(TEXT("놓기 성공"), Cursor->Drop());
	TestFalse(TEXT("놓으면 손이 빔"), Cursor->IsHolding());
	TestEqual(TEXT("새 위치"), ItemOf(Grid, 1).Origin, FIntPoint(2, 1));
	TestEqual(TEXT("새 회전"), ItemOf(Grid, 1).Rotation, 1);
	TestEqual(TEXT("새로 차지한 칸"), Grid->GetItemAt(FIntPoint(3, 3)), 1);
	TestEqual(TEXT("원래 칸은 비움"), Grid->GetItemAt(FIntPoint(0, 0)), 0);

	TestTrue(TEXT("되돌리기"), Grid->Undo());
	TestEqual(TEXT("원래 위치"), ItemOf(Grid, 1).Origin, FIntPoint(0, 0));
	TestEqual(TEXT("원래 회전"), ItemOf(Grid, 1).Rotation, 0);

	// 네 번 돌리면 처음과 같다
	Cursor->SetHoverCell(FIntPoint(1, 0));
	Cursor->PickUp();
	for (int32 i = 0; i < 4; ++i)
	{
		Cursor->Rotate();
	}
	TestEqual(TEXT("360도 회전 후 같은 자리"), Cursor->GetTargetOrigin(), FIntPoint(0, 0));
	TestEqual(TEXT("360도 회전 후 회전값 0"), Cursor->GetHeldRotation(), 0);
	Cursor->Drop();
	TestFalse(TEXT("제자리에 놓은 것은 기록하지 않음"), Grid->CanUndo());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseCursorBlockedTest, "RE4Inventory.CaseCursor.BlockedAndDiscard", CaseTestFlags)
bool FCaseCursorBlockedTest::RunTest(const FString& Parameters)
{
	UCaseGrid* Grid = NewObject<UCaseGrid>(GetTransientPackage());
	Grid->Initialize(10, 7);
	Grid->AddItem(MakeDef("Gun", FIntPoint(3, 2)), 1);  // 1: (0,0)
	Grid->AddItem(MakeDef("Ammo", FIntPoint(1, 2)), 1); // 2: (3,0)
	UCaseCursor* Cursor = NewObject<UCaseCursor>(GetTransientPackage());
	Cursor->Bind(Grid);

	Cursor->SetHoverCell(FIntPoint(3, 1));
	TestTrue(TEXT("탄약 집음 (아래 칸)"), Cursor->PickUp());
	TestEqual(TEXT("탄약 번호"), Cursor->GetHeldId(), 2);
	Cursor->SetHoverCell(FIntPoint(1, 1)); // 왼쪽 위 (1,0) -> 권총과 겹침
	TestFalse(TEXT("다른 아이템과 겹치면 못 놓음"), Cursor->CanDropHere());
	TestFalse(TEXT("놓기 실패"), Cursor->Drop());

	TestTrue(TEXT("들고 있는 것 버리기"), Cursor->DiscardHeld());
	TestFalse(TEXT("버리면 손이 빔"), Cursor->IsHolding());
	TestEqual(TEXT("버린 칸은 빔"), Grid->GetItemAt(FIntPoint(3, 0)), 0);
	TestTrue(TEXT("버리기 되돌리기"), Grid->Undo());
	TestEqual(TEXT("원래 자리로 돌아옴"), Grid->GetItemAt(FIntPoint(3, 1)), 2);

	Cursor->SetHoverCell(FIntPoint(0, 0));
	Cursor->PickUp();
	Cursor->Cancel();
	TestFalse(TEXT("취소하면 손이 빔"), Cursor->IsHolding());
	TestEqual(TEXT("취소는 격자를 바꾸지 않음"), ItemOf(Grid, 1).Origin, FIntPoint(0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewStretchTest, "RE4Inventory.CaseView.StretchFit", CaseTestFlags)
bool FCaseViewStretchTest::RunTest(const FString& Parameters)
{
	auto Near = [](const FVector& A, const FVector& B) { return A.Equals(B, 0.01f); };
	const FBox Cube(FVector(-50.0f), FVector(50.0f));

	UItemDefinition* Gun = MakeDef("Gun", FIntPoint(3, 2));
	Gun->FitRatio = 1.0f;
	Gun->bStretchToCells = true;
	Gun->StretchHeight = 2.0f;
	const FBox A = Cube.TransformBy(ACaseViewActor::ComputeItemTransform(Cube, *Gun, FIntPoint(1, 1), 0, 10.0f, 0.0f));
	TestTrue(TEXT("3x2칸을 꽉 채운 상자"), Near(A.GetSize(), FVector(30.0f, 20.0f, 2.0f)));
	TestTrue(TEXT("영역 가운데, 바닥에 닿음"), Near(A.GetCenter(), FVector(25.0f, 20.0f, 1.0f)));
	const FBox A1 = Cube.TransformBy(ACaseViewActor::ComputeItemTransform(Cube, *Gun, FIntPoint(1, 1), 1, 10.0f, 0.0f));
	TestTrue(TEXT("돌리면 2x3칸"), Near(A1.GetSize(), FVector(20.0f, 30.0f, 2.0f)));

	// 축이 Z인 원기둥(지름 100, 높이 100, 바닥 피벗)을 Pitch 90으로 눕혀 2x1칸에
	UItemDefinition* Spray = MakeDef("Spray", FIntPoint(2, 1));
	Spray->FitRatio = 1.0f;
	Spray->bStretchToCells = true;
	Spray->StretchHeight = 3.0f;
	Spray->MeshRotation = FRotator(90.0f, 0.0f, 0.0f);
	const FBox Cylinder(FVector(-50.0f, -50.0f, 0.0f), FVector(50.0f, 50.0f, 100.0f));
	const FBox B = Cylinder.TransformBy(ACaseViewActor::ComputeItemTransform(Cylinder, *Spray, FIntPoint(0, 0), 0, 10.0f, 0.0f));
	TestTrue(TEXT("눕힌 원기둥이 2x1칸, 높이 3"), Near(B.GetSize(), FVector(20.0f, 10.0f, 3.0f)));
	TestTrue(TEXT("눕힌 원기둥도 영역 가운데"), Near(B.GetCenter(), FVector(10.0f, 5.0f, 1.5f)));
	return true;
}

/** 실제 월드에 케이스를 띄워 격자 상태와 3D 아이템 위치가 맞는지, 마우스 광선 조작이 격자에 반영되는지 본다 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewWorldSyncTest, "RE4Inventory.CaseView.WorldSync", CaseTestFlags)
bool FCaseViewWorldSyncTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	const FVector CaseLocation(100.0f, -50.0f, 30.0f);
	ACaseViewActor* Case = World->SpawnActor<ACaseViewActor>(CaseLocation, FRotator::ZeroRotator);
	// 게임 모드 없는 테스트 월드는 BeginPlay를 돌리지 않으므로 직접 부른다
	if (Case && !Case->HasActorBegunPlay())
	{
		Case->DispatchBeginPlay();
	}
	UCaseGrid* Grid = Case ? Case->GetGrid() : nullptr;
	if (!TestNotNull(TEXT("케이스 생성"), Grid))
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	auto CheckSync = [&](const TCHAR* When)
	{
		// 화면은 목표 자리로 애니메이션하며 따라가므로 끝 상태에서 대조한다 (수렴 자체는 AnimationConverges에서 확인)
		Case->FinishAnimations();
		for (const FInventoryItem& Item : Grid->GetItems())
		{
			UStaticMeshComponent* Component = Case->GetItemComponent(Item.InstanceId);
			if (!TestNotNull(FString::Printf(TEXT("%s: 아이템 %d 메시"), When, Item.InstanceId), Component))
			{
				continue;
			}
			// 메시 바운드의 가운데가 차지한 칸 영역 가운데 위에 있어야 한다
			const FBox Bounds = Component->GetStaticMesh()->GetBoundingBox().TransformBy(Component->GetComponentTransform());
			const FIntPoint Footprint = ItemShape::GetFootprint(Item.Definition->Size, Item.Rotation);
			const FVector Expected = Case->CellAreaCenterToWorld(Item.Origin, Footprint);
			TestTrue(FString::Printf(TEXT("%s: 아이템 %d가 칸 영역 가운데"), When, Item.InstanceId),
				FVector2D(Bounds.GetCenter()).Equals(FVector2D(Expected), 0.05f));
			TestTrue(FString::Printf(TEXT("%s: 아이템 %d가 칸 영역 안"), When, Item.InstanceId),
				Bounds.GetSize().X <= Footprint.X * Case->CellSize + 0.01f && Bounds.GetSize().Y <= Footprint.Y * Case->CellSize + 0.01f);
		}
	};

	TestTrue(TEXT("시연용 아이템이 들어감"), Grid->GetItems().Num() >= 9);
	CheckSync(TEXT("시작"));

	// 케이스 카메라처럼 앞쪽 위에서 비스듬히 칸 중심을 향해 쏜 광선으로 조작
	auto Aim = [&](FIntPoint Cell)
	{
		// 칸 아래쪽(카메라 쪽) 가장자리 근처를 노린다. 떠 있는 높이로 칸을 찾으면 여기서 다음 줄로 넘어간다
		const FVector Target = Case->CellAreaCenterToWorld(Cell, FIntPoint(1, 1)) + FVector(0.0f, Case->CellSize * 0.35f, 0.0f);
		const FVector From = Target + FVector(0.0f, 60.0f, 90.0f);
		return Case->UpdateCursorFromRay(From, (Target - From).GetSafeNormal());
	};

	const FInventoryItem Shotgun = Grid->GetItems()[0];
	TestTrue(TEXT("광선이 바닥에 닿음"), Aim(Shotgun.Origin));
	TestEqual(TEXT("광선으로 찾은 칸"), Case->GetCursor()->GetHoverCell(), Shotgun.Origin);
	TestTrue(TEXT("클릭으로 집음"), Case->PrimaryAction());
	Aim(Shotgun.Origin);
	TestEqual(TEXT("집은 직후 같은 칸을 가리키면 자리가 밀리지 않음"), Case->GetCursor()->GetTargetOrigin(), Shotgun.Origin);

	// 빈 아래쪽 줄로 옮겨 놓기
	FIntPoint FreeRow(-1, -1);
	for (int32 Y = Grid->GetHeight() - 1; Y >= 0 && FreeRow.Y < 0; --Y)
	{
		const FIntPoint Footprint = ItemShape::GetFootprint(Shotgun.Definition->Size, Shotgun.Rotation);
		if (Grid->CanPlace(Shotgun.Definition, FIntPoint(0, Y - Footprint.Y + 1), Shotgun.Rotation, Shotgun.InstanceId))
		{
			FreeRow = FIntPoint(0, Y - Footprint.Y + 1);
		}
	}
	if (TestTrue(TEXT("옮길 빈자리가 있음"), FreeRow.Y >= 0))
	{
		TestTrue(TEXT("집은 뒤에도 광선이 바닥에 닿음"), Aim(FreeRow));
		TestTrue(TEXT("놓을 수 있다고 표시"), Case->GetCursor()->CanDropHere());
		TestTrue(TEXT("클릭으로 놓음"), Case->PrimaryAction());
		TestEqual(TEXT("격자에 새 위치 반영"), Grid->GetItemAt(FreeRow), Shotgun.InstanceId);
		CheckSync(TEXT("이동 후"));
		TestTrue(TEXT("Undo"), Case->UndoLast());
		TestEqual(TEXT("Undo 후 원래 칸"), Grid->GetItemAt(Shotgun.Origin), Shotgun.InstanceId);
		CheckSync(TEXT("Undo 후"));
	}

	TestTrue(TEXT("자동 정리(종류 우선)"), Case->Arrange(EArrangeMode::CategoryFirst));
	CheckSync(TEXT("자동 정리 후"));
	TestTrue(TEXT("케이스 확장"), Case->ExpandCase());
	TestEqual(TEXT("확장된 가로"), Grid->GetWidth(), 11);
	CheckSync(TEXT("확장 후"));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

namespace CaseInteractionTest
{
	/** 테스트용 게임 월드에 케이스 하나를 띄운다. 소멸자에서 월드를 정리한다 */
	struct FCaseWorld
	{
		UWorld* World = nullptr;
		ACaseViewActor* Case = nullptr;

		explicit FCaseWorld(TFunction<void(ACaseViewActor&)> BeforeBeginPlay = nullptr)
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			FActorSpawnParameters Params;
			Params.bDeferConstruction = true;
			Case = World->SpawnActor<ACaseViewActor>(ACaseViewActor::StaticClass(), FTransform::Identity, Params);
			if (Case)
			{
				if (BeforeBeginPlay)
				{
					BeforeBeginPlay(*Case);
				}
				Case->FinishSpawning(FTransform::Identity);
				if (!Case->HasActorBegunPlay())
				{
					Case->DispatchBeginPlay();
				}
			}
		}

		~FCaseWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		UCaseGrid* Grid() const { return Case ? Case->GetGrid() : nullptr; }

		/** 위에서 칸 가운데로 쏜 광선 */
		bool Aim(FIntPoint Cell) const
		{
			const FVector Target = Case->CellAreaCenterToWorld(Cell, FIntPoint(1, 1));
			return Case->UpdateCursorFromRay(Target + FVector(0.0f, 0.0f, 100.0f), FVector(0.0f, 0.0f, -1.0f));
		}

		void Simulate(float Seconds) const
		{
			for (float T = 0.0f; T < Seconds; T += 1.0f / 60.0f)
			{
				Case->AdvanceAnimations(1.0f / 60.0f);
			}
		}
	};

	/** 격자의 모든 칸이 아이템 목록과 정확히 맞는지 (겹침·빈 기록 없음) */
	bool GridConsistent(UCaseGrid* Grid)
	{
		TArray<int32> Expected;
		Expected.Init(0, Grid->GetWidth() * Grid->GetHeight());
		for (const FInventoryItem& Item : Grid->GetItems())
		{
			const FIntPoint Footprint = ItemShape::GetFootprint(Item.Definition->Size, Item.Rotation);
			for (int32 Y = 0; Y < Footprint.Y; ++Y)
			{
				for (int32 X = 0; X < Footprint.X; ++X)
				{
					const FIntPoint Cell = Item.Origin + FIntPoint(X, Y);
					if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Grid->GetWidth() || Cell.Y >= Grid->GetHeight())
					{
						return false;
					}
					int32& Slot = Expected[Cell.Y * Grid->GetWidth() + Cell.X];
					if (Slot != 0)
					{
						return false;
					}
					Slot = Item.InstanceId;
				}
			}
		}
		for (int32 Y = 0; Y < Grid->GetHeight(); ++Y)
		{
			for (int32 X = 0; X < Grid->GetWidth(); ++X)
			{
				if (Grid->GetItemAt(FIntPoint(X, Y)) != Expected[Y * Grid->GetWidth() + X])
				{
					return false;
				}
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewAnimationTest, "RE4Inventory.CaseView.AnimationConverges", CaseTestFlags)
bool FCaseViewAnimationTest::RunTest(const FString& Parameters)
{
	FCaseWorld W;
	UCaseGrid* Grid = W.Grid();
	if (!TestNotNull(TEXT("케이스"), Grid)) return false;

	const FInventoryItem Handgun = Grid->GetItems()[1];
	{
		FTransform Start;
		W.Case->GetItemTargetTransform(Handgun.InstanceId, Start);
		TestTrue(TEXT("시작할 때는 날아오지 않고 바로 제자리"), W.Case->GetItemComponent(Handgun.InstanceId)->GetRelativeTransform().Equals(Start, 0.001f));
	}
	W.Aim(Handgun.Origin);
	TestTrue(TEXT("집음"), W.Case->PrimaryAction());
	TestTrue(TEXT("회전"), W.Case->RotateHeld());
	UStaticMeshComponent* Component = W.Case->GetItemComponent(Handgun.InstanceId);
	FTransform Target;
	TestTrue(TEXT("목표 자리가 있음"), W.Case->GetItemTargetTransform(Handgun.InstanceId, Target));
	TestFalse(TEXT("회전 직후에는 아직 도는 중"), Component->GetRelativeTransform().Equals(Target, 0.01f));
	TestTrue(TEXT("애니메이션 중"), W.Case->IsAnimating());

	// 반쯤 지나면 처음과 목표 사이 각도
	W.Simulate(0.05f);
	const float Halfway = Component->GetRelativeTransform().GetRotation().AngularDistance(Target.GetRotation());
	TestTrue(TEXT("0.05초 뒤 회전이 진행 중"), Halfway > 0.01f && Halfway < FMath::DegreesToRadians(90.0f));

	W.Simulate(2.0f);
	TestFalse(TEXT("2초 뒤 애니메이션 끝"), W.Case->IsAnimating());
	TestTrue(TEXT("끝나면 목표 자리와 정확히 같음"), Component->GetRelativeTransform().Equals(Target, 0.001f));

	// 못 놓는 자리(다른 아이템 위)에서 놓기 -> 흔들림 후 제자리
	W.Aim(Grid->GetItems()[0].Origin);
	TestFalse(TEXT("겹치는 자리에는 못 놓음"), W.Case->PrimaryAction());
	TestTrue(TEXT("실패 흔들림 중"), W.Case->IsAnimating());
	W.Simulate(2.0f);
	TestFalse(TEXT("흔들림도 끝남"), W.Case->IsAnimating());
	W.Case->CancelHeld();
	W.Simulate(2.0f);
	TestTrue(TEXT("취소 후 원래 자리로 돌아감"), W.Case->GetItemTargetTransform(Handgun.InstanceId, Target)
		&& Component->GetRelativeTransform().Equals(Target, 0.001f));
	TestEqual(TEXT("격자는 그대로"), Grid->GetItemAt(Handgun.Origin), Handgun.InstanceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewRapidInputTest, "RE4Inventory.CaseView.RapidInput", CaseTestFlags)
bool FCaseViewRapidInputTest::RunTest(const FString& Parameters)
{
	FCaseWorld W;
	UCaseGrid* Grid = W.Grid();
	if (!TestNotNull(TEXT("케이스"), Grid)) return false;

	// 애니메이션이 끝나기 전에 클릭·회전을 마구 섞어 보낸다 (프레임마다 조금씩만 진행)
	const TArray<FIntPoint> Cells = { FIntPoint(0, 0), FIntPoint(4, 5), FIntPoint(8, 1), FIntPoint(2, 6), FIntPoint(9, 6), FIntPoint(5, 3) };
	int32 Steps = 0;
	for (int32 Round = 0; Round < 60; ++Round)
	{
		W.Aim(Cells[Round % Cells.Num()]);
		W.Case->PrimaryAction();
		if (Round % 3 == 0)
		{
			W.Case->RotateHeld();
			W.Case->RotateHeld();
		}
		W.Case->AdvanceAnimations(1.0f / 120.0f);
		++Steps;
		if (!GridConsistent(Grid))
		{
			AddError(FString::Printf(TEXT("%d번째 입력 뒤 격자가 어긋남"), Round));
			return false;
		}
	}
	TestEqual(TEXT("60번 다 처리"), Steps, 60);
	W.Case->CancelHeld();
	while (W.Case->UndoLast()) {}
	TestTrue(TEXT("전부 되돌린 뒤에도 격자 일관"), GridConsistent(Grid));
	W.Simulate(3.0f);
	TestFalse(TEXT("결국 멈춤"), W.Case->IsAnimating());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewHoldingCommandsTest, "RE4Inventory.CaseView.HoldingThenArrangeOrExpand", CaseTestFlags)
bool FCaseViewHoldingCommandsTest::RunTest(const FString& Parameters)
{
	FCaseWorld W;
	UCaseGrid* Grid = W.Grid();
	if (!TestNotNull(TEXT("케이스"), Grid)) return false;

	const FInventoryItem First = Grid->GetItems()[0];
	W.Aim(First.Origin);
	W.Case->PrimaryAction();
	W.Case->RotateHeld();
	TestTrue(TEXT("들고 있음"), W.Case->GetCursor()->IsHolding());
	TestTrue(TEXT("들고 있어도 자동 정리 됨"), W.Case->Arrange(EArrangeMode::CategoryFirst));
	TestFalse(TEXT("자동 정리하면 손을 비움"), W.Case->GetCursor()->IsHolding());
	TestTrue(TEXT("정리 후 격자 일관"), GridConsistent(Grid));

	W.Aim(Grid->GetItems()[0].Origin);
	W.Case->PrimaryAction();
	TestTrue(TEXT("다시 들고 있음"), W.Case->GetCursor()->IsHolding());
	TestTrue(TEXT("들고 있어도 확장 됨"), W.Case->ExpandCase());
	TestFalse(TEXT("확장하면 손을 비움"), W.Case->GetCursor()->IsHolding());
	TestEqual(TEXT("확장 크기"), FIntPoint(Grid->GetWidth(), Grid->GetHeight()), FIntPoint(11, 8));

	// 확장 뒤 모든 아이템 목표 자리가 새 격자 기준 칸 가운데
	W.Case->FinishAnimations();
	for (const FInventoryItem& Item : Grid->GetItems())
	{
		UStaticMeshComponent* Component = W.Case->GetItemComponent(Item.InstanceId);
		const FBox Bounds = Component->GetStaticMesh()->GetBoundingBox().TransformBy(Component->GetComponentTransform());
		const FVector Expected = W.Case->CellAreaCenterToWorld(Item.Origin, ItemShape::GetFootprint(Item.Definition->Size, Item.Rotation));
		TestTrue(FString::Printf(TEXT("확장 후 아이템 %d 위치"), Item.InstanceId), FVector2D(Bounds.GetCenter()).Equals(FVector2D(Expected), 0.05f));
	}
	// 최대 크기까지 늘리고 더는 안 늘어남
	while (W.Case->ExpandCase()) {}
	TestEqual(TEXT("최대 크기에서 멈춤"), FIntPoint(Grid->GetWidth(), Grid->GetHeight()), W.Case->MaxGridSize);
	TestTrue(TEXT("최대 확장 후 Undo 가능한 이동도 정상"), GridConsistent(Grid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewMouseOutsideTest, "RE4Inventory.CaseView.MouseOutside", CaseTestFlags)
bool FCaseViewMouseOutsideTest::RunTest(const FString& Parameters)
{
	FCaseWorld W;
	UCaseGrid* Grid = W.Grid();
	if (!TestNotNull(TEXT("케이스"), Grid)) return false;

	const FIntPoint Before = W.Case->GetCursor()->GetHoverCell();
	TestFalse(TEXT("바닥과 평행한 광선은 무시"), W.Case->UpdateCursorFromRay(FVector(0, 0, 50), FVector(1, 0, 0)));
	TestFalse(TEXT("위로 쏜 광선은 무시"), W.Case->UpdateCursorFromRay(FVector(0, 0, 50), FVector(0, 0, 1)));
	TestEqual(TEXT("무시하면 커서 칸 그대로"), W.Case->GetCursor()->GetHoverCell(), Before);

	// 케이스 밖 먼 곳을 클릭해도 아무 일 없음
	W.Case->UpdateCursorFromRay(FVector(-500, -500, 100), FVector(0, 0, -1));
	TestFalse(TEXT("케이스 밖은 못 집음"), W.Case->PrimaryAction());

	// 집은 채로 케이스 밖으로 나가면 못 놓고, 아이템은 케이스 안에 머문다
	const FInventoryItem Item = Grid->GetItems()[1];
	W.Aim(Item.Origin);
	W.Case->PrimaryAction();
	W.Case->UpdateCursorFromRay(FVector(5000, 5000, 100), FVector(0, 0, -1));
	TestFalse(TEXT("밖에서는 못 놓음"), W.Case->GetCursor()->CanDropHere());
	TestFalse(TEXT("놓기 실패"), W.Case->PrimaryAction());
	TestTrue(TEXT("계속 들고 있음"), W.Case->GetCursor()->IsHolding());
	FTransform Target;
	W.Case->GetItemTargetTransform(Item.InstanceId, Target);
	const FBox Bounds = W.Case->GetItemComponent(Item.InstanceId)->GetStaticMesh()->GetBoundingBox().TransformBy(Target);
	const FVector2D CaseSize(Grid->GetWidth() * W.Case->CellSize, Grid->GetHeight() * W.Case->CellSize);
	TestTrue(TEXT("들고 있는 아이템은 케이스 안에 그려짐"), Bounds.Min.X >= -0.01f && Bounds.Min.Y >= -0.01f
		&& Bounds.Max.X <= CaseSize.X + 0.01f && Bounds.Max.Y <= CaseSize.Y + 0.01f);
	W.Case->CancelHeld();
	TestEqual(TEXT("격자는 그대로"), Grid->GetItemAt(Item.Origin), Item.InstanceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewMissingMeshTest, "RE4Inventory.CaseView.MissingMesh", CaseTestFlags)
bool FCaseViewMissingMeshTest::RunTest(const FString& Parameters)
{
	auto MakeBroken = [](ACaseViewActor& Case, bool bNoFallback)
	{
		UItemDefinition* Broken = MakeDef("Broken", FIntPoint(2, 1));
		// 임포트에 실패했거나 지워진 모델을 가리키는 경로
		Broken->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/LocalOnly/DoesNotExist/SM_Missing.SM_Missing")));
		UItemDefinition* Plain = MakeDef("Plain", FIntPoint(1, 1));
		Case.StartingItems = { FStartingItem{ Broken, 1 }, FStartingItem{ Plain, 1 } };
		if (bNoFallback)
		{
			Case.FallbackMesh = nullptr;
		}
	};

	{
		FCaseWorld W([&](ACaseViewActor& Case) { MakeBroken(Case, false); });
		UCaseGrid* Grid = W.Grid();
		if (!TestNotNull(TEXT("케이스"), Grid)) return false;
		UStaticMeshComponent* Component = W.Case->GetItemComponent(1);
		TestTrue(TEXT("모델이 없으면 대체 모델로 그림"), Component && Component->GetStaticMesh() == W.Case->FallbackMesh);
		FTransform Target;
		TestTrue(TEXT("대체 모델도 칸에 맞춰 목표 자리가 있음"), W.Case->GetItemTargetTransform(1, Target));
	}
	{
		FCaseWorld W([&](ACaseViewActor& Case) { MakeBroken(Case, true); });
		UCaseGrid* Grid = W.Grid();
		if (!TestNotNull(TEXT("케이스"), Grid)) return false;
		FTransform Target;
		TestFalse(TEXT("그릴 모델이 전혀 없으면 목표 자리도 없음"), W.Case->GetItemTargetTransform(1, Target));
		// 그래도 격자 조작은 그대로 된다
		W.Aim(FIntPoint(0, 0));
		TestTrue(TEXT("모델 없어도 집기"), W.Case->PrimaryAction());
		W.Aim(FIntPoint(0, 3));
		TestTrue(TEXT("모델 없어도 놓기"), W.Case->PrimaryAction());
		TestEqual(TEXT("격자 반영"), Grid->GetItemAt(FIntPoint(1, 3)), 1);
		W.Simulate(0.5f);
		TestTrue(TEXT("Undo"), W.Case->UndoLast());
		TestTrue(TEXT("격자 일관"), GridConsistent(Grid));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewCountOutlineTest, "RE4Inventory.CaseView.CountOutline", CaseTestFlags)
bool FCaseViewCountOutlineTest::RunTest(const FString& Parameters)
{
	FCaseWorld W([](ACaseViewActor& Case)
	{
		UItemDefinition* Ammo = MakeDef("Ammo", FIntPoint(1, 1));
		Ammo->MaxStack = 50;
		Case.StartingItems = { FStartingItem{ Ammo, 12 } };
	});
	if (!TestNotNull(TEXT("케이스"), W.Grid())) return false;
	auto CountTexts = [&W]()
	{
		TArray<UTextRenderComponent*> Texts;
		W.Case->GetComponents<UTextRenderComponent>(Texts);
		return Texts;
	};
	TArray<UTextRenderComponent*> Texts = CountTexts();
	TestEqual(TEXT("숫자 1개 + 외곽선 4개"), Texts.Num(), 5);
	for (UTextRenderComponent* Text : Texts)
	{
		TestTrue(TEXT("외곽선도 같은 숫자"), Text->Text.EqualTo(FText::AsNumber(12)));
	}
	W.Aim(FIntPoint(0, 0));
	TestTrue(TEXT("집기"), W.Case->PrimaryAction());
	TestTrue(TEXT("버리기"), W.Case->DiscardHeld());
	TestEqual(TEXT("버리면 외곽선까지 전부 사라짐"), CountTexts().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaseViewMeshCandidateTest, "RE4Inventory.CaseView.MeshCandidate", CaseTestFlags)
bool FCaseViewMeshCandidateTest::RunTest(const FString& Parameters)
{
	FString Path;
	FRotator Rotation;
	TestTrue(TEXT("이름만"), ACaseViewActor::ParseMeshCandidate(TEXT("SM_Item_Knife"), Path, Rotation));
	TestEqual(TEXT("이름만 경로"), Path, FString(TEXT("/Game/Meshes/SM_Item_Knife.SM_Item_Knife")));
	TestTrue(TEXT("이름만 회전 없음"), Rotation.IsZero());

	TestTrue(TEXT("전체 경로+회전"), ACaseViewActor::ParseMeshCandidate(TEXT("/Game/A/SM_B@0,-90,90"), Path, Rotation));
	TestEqual(TEXT("전체 경로"), Path, FString(TEXT("/Game/A/SM_B.SM_B")));
	TestTrue(TEXT("회전 값"), Rotation.Equals(FRotator(0.0f, -90.0f, 90.0f)));

	TestFalse(TEXT("회전 값 개수 틀림"), ACaseViewActor::ParseMeshCandidate(TEXT("/Game/A/SM_B@90"), Path, Rotation));
	TestFalse(TEXT("숫자 아님"), ACaseViewActor::ParseMeshCandidate(TEXT("/Game/A/SM_B@a,b,c"), Path, Rotation));
	TestFalse(TEXT("빈 이름"), ACaseViewActor::ParseMeshCandidate(TEXT("@0,0,0"), Path, Rotation));
	return true;
}

#endif
