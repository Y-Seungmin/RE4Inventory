#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/InventoryTypes.h"
#include "CaseViewActor.generated.h"

class UCaseGrid;
class UCaseCursor;
class UItemDefinition;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UCameraComponent;
class UPointLightComponent;
class UTextRenderComponent;

/** 머티리얼 슬롯 이름 하나에 줄 PBR 값. M_CaseSurface의 파라미터로 들어간다 */
USTRUCT(BlueprintType)
struct FSurfaceLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	FLinearColor Color = FLinearColor(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0", ClampMax = "1"))
	float Metallic = 0.0f;

	/** 표면 결(노이즈) 세기. 가죽·종이는 크게, 금속은 작게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look", meta = (ClampMin = "0", ClampMax = "1"))
	float Grain = 0.2f;

	/** 지정하면 위 값 대신 이 머티리얼(텍스처 있는 외부 에셋 등)을 그대로 쓴다. 로드하지 못하면 위 값으로 돌아간다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Look")
	TSoftObjectPtr<UMaterialInterface> Material;
};

USTRUCT(BlueprintType)
struct FStartingItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UItemDefinition> Definition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * 3D 아타셰 케이스 화면. UCaseGrid의 상태를 그대로 보여 주기만 한다.
 * 격자가 바뀌면(OnChanged) 아이템 메시를 다시 맞춰 놓고, 커서(UCaseCursor) 상태로 미리보기를 그린다.
 * 액터 로컬 좌표에서 칸 (0,0)의 왼쪽 위 모서리가 원점, 가로(X 칸)는 +X, 세로(Y 칸)는 +Y 방향이다.
 */
UCLASS()
class RE4INVENTORY_API ACaseViewActor : public AActor
{
	GENERATED_BODY()

public:
	ACaseViewActor();

	/** 칸 한 변의 길이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case", meta = (ClampMin = "1"))
	float CellSize = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case")
	FIntPoint GridSize = FIntPoint(10, 7);

	/** 시작할 때 넣어 둘 아이템 (시연·테스트용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case")
	TArray<FStartingItem> StartingItems;

	/** StartingItems가 비어 있으면 기본 도형으로 만든 시연용 아이템을 넣는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case")
	bool bUseDemoItems = true;

	/** E 키 확장의 최대 크기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case")
	FIntPoint MaxGridSize = FIntPoint(13, 9);

	/** Mesh가 비어 있는 아이템에 쓰는 대체 모델 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case")
	TObjectPtr<UStaticMesh> FallbackMesh;

	/** PBR 마스터 머티리얼 (Color, Roughness, Metallic, GrainAmount). 없으면 ColorMaterial로 색만 바꾼다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	/** 머티리얼 슬롯 이름별 표면 값. 아이템·케이스 메시의 슬롯 이름이 여기 있으면 이 값으로 입힌다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	TMap<FName, FSurfaceLook> SurfaceLooks;

	/** 색을 바꿀 수 있는 기본 머티리얼 ("Color" 벡터 파라미터) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	TObjectPtr<UMaterialInterface> ColorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	float WallHeight = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	float WallThickness = 1.5f;

	/** 케이스 카메라의 고정 노출(EV100). 자동 노출은 어두운 케이스를 밝게 끌어올려 색이 날아간다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	float ExposureEV100 = 6.0f;

	/** 끄면 애니메이션 없이 격자 상태 자리로 바로 옮긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation")
	bool bAnimate = true;

	/** 아이템이 목표 자리로 따라가는 빠르기. 클수록 빠르다 (지수 감쇠 계수, 1/초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation", meta = (ClampMin = "1"))
	float MoveSharpness = 22.0f;

	/** 90도 회전 보간 빠르기 (1/초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation", meta = (ClampMin = "1"))
	float RotateSharpness = 18.0f;

	/** 못 놓는 자리에서 놓으려 할 때 흔드는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation")
	float FailShakeDuration = 0.3f;

	/** 내려놓았을 때 살짝 눌렸다 돌아오는 시간(초) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation")
	float LandPulseDuration = 0.18f;

	/** 마우스가 올라간 아이템을 띄우는 높이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Animation")
	float HoverLift = 0.4f;

	/** 집은 아이템을 바닥에서 띄우는 높이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	float HeldLift = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor ShellColor = FLinearColor(0.035f, 0.022f, 0.015f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor InteriorColor = FLinearColor(0.012f, 0.02f, 0.018f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor GridLineColor = FLinearColor(0.06f, 0.08f, 0.075f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor MetalColor = FLinearColor(0.35f, 0.3f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor ValidColor = FLinearColor(0.05f, 0.6f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor InvalidColor = FLinearColor(0.7f, 0.04f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Case|Look")
	FLinearColor HoverColor = FLinearColor(0.5f, 0.4f, 0.08f);

	UFUNCTION(BlueprintPure, Category = "Case")
	UCaseGrid* GetGrid() const { return Grid; }

	UFUNCTION(BlueprintPure, Category = "Case")
	UCaseCursor* GetCursor() const { return Cursor; }

	/** 애니메이션이 끝났을 때 아이템이 있어야 할 자리(액터 로컬). 격자 상태와 커서 상태로만 정해진다 */
	bool GetItemTargetTransform(int32 InstanceId, FTransform& OutTransform) const;

	/** 진행 중인 애니메이션을 모두 끝 상태로 맞춘다 (테스트·즉시 갱신용) */
	UFUNCTION(BlueprintCallable, Category = "Case")
	void FinishAnimations();

	UFUNCTION(BlueprintPure, Category = "Case")
	bool IsAnimating() const;

	/** 애니메이션을 Dt초만큼 진행. 컴포넌트를 목표 자리 쪽으로 옮긴다 */
	void AdvanceAnimations(float DeltaSeconds);

	/** 아이템 하나를 그리는 메시 컴포넌트. 없으면 nullptr */
	UFUNCTION(BlueprintPure, Category = "Case")
	UStaticMeshComponent* GetItemComponent(int32 InstanceId) const { return ItemComponents.FindRef(InstanceId); }

	/** 월드 좌표를 칸 좌표로. 케이스 바닥 평면 위의 점을 넘긴다 */
	UFUNCTION(BlueprintPure, Category = "Case")
	bool WorldToCell(FVector WorldLocation, FIntPoint& OutCell) const;

	/** 칸 영역(왼쪽 위 칸 + 크기)의 가운데를 월드 좌표로 */
	UFUNCTION(BlueprintPure, Category = "Case")
	FVector CellAreaCenterToWorld(FIntPoint Origin, FIntPoint Footprint) const;

	// ---- 플레이어 조작. 컨트롤러가 입력을 받아 부른다 ----

	/** 마우스 광선을 케이스 바닥 평면과 만나게 해 커서 칸을 정한다. 평면과 안 만나면 false */
	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool UpdateCursorFromRay(FVector RayOrigin, FVector RayDirection);

	/** 들고 있지 않으면 집고, 들고 있으면 내려놓는다 */
	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool PrimaryAction();

	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool RotateHeld();

	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	void CancelHeld();

	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool DiscardHeld();

	/** 들고 있던 것은 원래 자리로 돌리고 마지막 조작을 되돌린다 */
	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool UndoLast();

	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool Arrange(EArrangeMode Mode);

	/** 가로 세로 1칸씩 확장 (MaxGridSize까지) */
	UFUNCTION(BlueprintCallable, Category = "Case|Input")
	bool ExpandCase();

	/** 마지막 조작 결과 한 줄 (HUD 표시용) */
	UFUNCTION(BlueprintPure, Category = "Case")
	FString GetStatusText() const { return StatusText; }

	/**
	 * 모델을 칸 영역에 맞추는 변환(액터 로컬 기준). 모델의 원래 크기와 피벗 위치에 상관없이
	 * 바운드 가운데를 칸 영역 가운데에, 바운드 바닥을 BaseTopZ에 맞춘다.
	 */
	static FTransform ComputeItemTransform(const FBox& MeshLocalBounds, const UItemDefinition& Definition,
		FIntPoint Origin, int32 ItemRotation, float InCellSize, float BaseTopZ);

	/** 기본 도형으로 만든 시연용 아이템 정의 (원작 에셋 없이 실행) */
	static TArray<FStartingItem> MakeDemoItems(UObject* Outer);

	/**
	 * 시연 아이템 메시 후보 하나를 읽는다. "이름", "/Game/경로/에셋" 둘 다 되고, 뒤에 "@Pitch,Yaw,Roll"을 붙이면 메시 회전.
	 * 이름만 있으면 /Game/Meshes 아래로 본다. 형식이 틀리면 false
	 */
	static bool ParseMeshCandidate(const FString& Candidate, FString& OutObjectPath, FRotator& OutRotation);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleGridChanged();

	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<USceneComponent> Root;

	/** 케이스 바닥(안쪽 판). 윗면이 Z=0 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UStaticMeshComponent> CaseBase;

	/** 겉 케이스 바닥판 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UStaticMeshComponent> Shell;

	/** 위, 아래, 왼쪽, 오른쪽 벽 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TArray<TObjectPtr<UStaticMeshComponent>> Walls;

	/** 뒤쪽(위쪽 벽)에 경첩으로 열려 서 있는 뚜껑 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UStaticMeshComponent> Lid;

	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UStaticMeshComponent> LidLining;

	/** 잠금쇠 2개 + 손잡이 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TArray<TObjectPtr<UStaticMeshComponent>> Latches;

	/** 바닥판 네 모서리와 벽 윗모서리 네 곳의 보호대 */
	UPROPERTY(VisibleAnywhere, Category = "Case")
	TArray<TObjectPtr<UStaticMeshComponent>> Corners;

	UPROPERTY(VisibleAnywhere, Category = "Case")
	TArray<TObjectPtr<UStaticMeshComponent>> Hinges;

	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Case")
	TObjectPtr<UPointLightComponent> CaseLight;

private:
	void LayoutCase();
	void RefreshPreview();
	float GetBaseTopZ() const { return 0.0f; }
	FIntPoint GetCurrentGridSize() const;
	void SetColor(UStaticMeshComponent* Component, const FLinearColor& Color);
	/** 컴포넌트의 모든 슬롯에 SurfaceLooks[Look]을 입힌다 */
	void ApplySurface(UStaticMeshComponent* Component, FName Look);
	/** 메시의 머티리얼 슬롯 이름으로 SurfaceLooks를 찾아 입힌다. 표에 없는 슬롯은 그대로 둔다 */
	void ApplySlotSurfaces(UStaticMeshComponent* Component);
	UMaterialInstanceDynamic* GetSurfaceInstance(UStaticMeshComponent* Component, int32 Slot);
	void ApplyLookToSlot(UStaticMeshComponent* Component, int32 Slot, const FSurfaceLook& Look);
	UStaticMeshComponent* MakeRuntimeMesh(UStaticMesh* Mesh);
	void PlaceBox(UStaticMeshComponent* Component, const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator);
	void SetStatus(const FString& Text);
	/** 아이템 컴포넌트를 Transform에 놓고 개수 글자를 따라 옮긴다 */
	void ApplyItemTransform(int32 InstanceId, UStaticMeshComponent* Component, const FTransform& Transform);
	bool ShouldAnimate() const;

	UPROPERTY(Transient)
	TObjectPtr<UCaseGrid> Grid;

	UPROPERTY(Transient)
	TObjectPtr<UCaseCursor> Cursor;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UStaticMeshComponent>> ItemComponents;

	/** 묶음 개수 표시 (MaxStack > 1인 아이템) */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UTextRenderComponent>> CountTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GridLines;

	/** 칸 강조 판 (커서 칸, 놓일 자리 미리보기) */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Plates;

	/** 시연용 아이템 정의를 잡아 둔다 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UItemDefinition>> DemoDefinitions;

	/** 아이템별 목표 자리 (RefreshPreview가 정함) */
	TMap<int32, FTransform> TargetTransforms;
	/** 한 번이라도 제자리에 놓인 아이템. 처음 나타날 때는 애니메이션 없이 놓는다 */
	TSet<int32> PlacedItems;
	/** 내려놓은 뒤 남은 눌림 시간 */
	TMap<int32, float> LandPulses;
	int32 FailShakeId = 0;
	float FailShakeTime = 0.0f;
	FTransform CameraTarget;
	bool bCameraPlaced = false;

	FIntPoint LaidOutSize = FIntPoint::ZeroValue;
	FString StatusText;
};
