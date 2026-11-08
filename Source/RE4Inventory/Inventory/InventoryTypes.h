#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UItemDefinition;

/** 케이스에 들어간 아이템 하나(한 묶음) */
USTRUCT(BlueprintType)
struct RE4INVENTORY_API FInventoryItem
{
	GENERATED_BODY()

	/** 0은 빈칸을 뜻하므로 1부터 쓴다 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 InstanceId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Count = 1;

	/** 차지한 영역의 왼쪽 위 칸 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint Origin = FIntPoint::ZeroValue;

	/** 0~3, 90도 단위 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Rotation = 0;
};

/** 배치만 기록한 값. 자동 정리 계산과 되돌리기에 쓴다 */
USTRUCT(BlueprintType)
struct RE4INVENTORY_API FItemPlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 InstanceId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint Origin = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Rotation = 0;
};

/** 자동 정리 기준 (전략) */
UENUM(BlueprintType)
enum class EArrangeMode : uint8
{
	/** 큰 아이템부터 넣어 공간을 아낀다 (본작과 같은 방식) */
	SpaceFirst,
	/** 같은 종류끼리 모아서 넣는다 (개선안) */
	CategoryFirst
};
