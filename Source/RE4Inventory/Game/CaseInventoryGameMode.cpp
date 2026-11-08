#include "Game/CaseInventoryGameMode.h"
#include "Game/CaseInventoryController.h"
#include "Game/CaseHUD.h"
#include "View/CaseViewActor.h"
#include "EngineUtils.h"

ACaseInventoryGameMode::ACaseInventoryGameMode()
{
	PlayerControllerClass = ACaseInventoryController::StaticClass();
	HUDClass = ACaseHUD::StaticClass();
	DefaultPawnClass = nullptr;
}

void ACaseInventoryGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 컨트롤러 BeginPlay보다 먼저 케이스가 있어야 카메라를 잡을 수 있다
	TActorIterator<ACaseViewActor> It(GetWorld());
	if (!It)
	{
		GetWorld()->SpawnActor<ACaseViewActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	}
}
