#include "Game/CaseInventoryController.h"
#include "View/CaseViewActor.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

ACaseInventoryController::ACaseInventoryController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ACaseInventoryController::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ACaseViewActor> It(GetWorld()); It; ++It)
	{
		Case = *It;
		break;
	}
	if (Case)
	{
		// 케이스 액터의 카메라로 본다
		SetViewTarget(Case);
	}

	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
}

void ACaseInventoryController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACaseInventoryController::OnPrimary);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ACaseInventoryController::OnRotate);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ACaseInventoryController::OnRotate);
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ACaseInventoryController::OnCancel);
	InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ACaseInventoryController::OnDiscard);
	InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ACaseInventoryController::OnUndo);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ACaseInventoryController::OnArrangeSpace);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ACaseInventoryController::OnArrangeCategory);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ACaseInventoryController::OnExpand);
}

void ACaseInventoryController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateCursorFromMouse();
}

void ACaseInventoryController::UpdateCursorFromMouse()
{
	FVector Origin, Direction;
	if (Case && DeprojectMousePositionToWorld(Origin, Direction))
	{
		Case->UpdateCursorFromRay(Origin, Direction);
	}
}

void ACaseInventoryController::OnPrimary() { UpdateCursorFromMouse(); if (Case) Case->PrimaryAction(); }
void ACaseInventoryController::OnRotate() { UpdateCursorFromMouse(); if (Case) Case->RotateHeld(); }
void ACaseInventoryController::OnCancel() { if (Case) Case->CancelHeld(); }
void ACaseInventoryController::OnDiscard() { if (Case) Case->DiscardHeld(); }
void ACaseInventoryController::OnUndo() { if (Case) Case->UndoLast(); }
void ACaseInventoryController::OnArrangeSpace() { if (Case) Case->Arrange(EArrangeMode::SpaceFirst); }
void ACaseInventoryController::OnArrangeCategory() { if (Case) Case->Arrange(EArrangeMode::CategoryFirst); }
void ACaseInventoryController::OnExpand() { if (Case) Case->ExpandCase(); }
