#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UStaticMesh;

/** 자동 정리에서 '종류 우선'일 때 이 순서로 묶인다 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Weapon,
	Ammo,
	Recovery,
	Material,
	Throwable
};

/** 아이템 한 종류의 기획 데이터. 코드 수정 없이 에디터에서 에셋으로 추가한다 */
UCLASS(BlueprintType)
class RE4INVENTORY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** 회전하지 않았을 때 가로(X) x 세로(Y) 칸 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	FIntPoint Size = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EItemCategory Category = EItemCategory::Material;

	/** 한 칸(한 묶음)에 쌓을 수 있는 최대 개수. 1이면 쌓이지 않음 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	/** 3D 케이스 화면에서 보여 줄 모델. 비어 있으면 기본 큐브 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 모델의 기본 방향 보정. 회전 0일 때 모델의 긴 쪽이 칸의 가로(X)를 향하고 바닥에 눕게 맞춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator MeshRotation = FRotator::ZeroRotator;

	/** 켜면 모델 크기와 피벗 위치에 상관없이 차지하는 칸 안에 맞춰 크기·위치를 자동으로 정한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bFitToCells = true;

	/** 칸 영역 대비 모델이 차지할 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.1", ClampMax = "1.0", EditCondition = "bFitToCells"))
	float FitRatio = 0.9f;

	/** 자동 맞춤을 끈 경우의 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.001", EditCondition = "!bFitToCells"))
	float MeshScale = 1.0f;

	/**
	 * 켜면 비율을 무시하고 칸 영역(x FitRatio)과 StretchHeight에 꽉 차게 늘린다.
	 * 기본 도형(큐브, 원기둥)으로 만든 임시 아이템용. 실제 모델에는 끈다
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (EditCondition = "bFitToCells"))
	bool bStretchToCells = false;

	/** bStretchToCells일 때의 높이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.1", EditCondition = "bFitToCells && bStretchToCells"))
	float StretchHeight = 2.0f;

	/** 켜면 머티리얼의 "Color" 파라미터를 Color로 바꾼다 (엔진 BasicShapeMaterial 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bOverrideColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (EditCondition = "bOverrideColor"))
	FLinearColor Color = FLinearColor::White;
};
