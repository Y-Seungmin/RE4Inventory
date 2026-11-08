#include "View/CaseViewActor.h"
#include "Inventory/CaseGrid.h"
#include "Inventory/CaseCursor.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemShape.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float BaseThickness = 1.0f;
	/** 겉 케이스 바닥판 아래 끝 */
	constexpr float ShellBottomZ = -2.5f;
	constexpr float LidThickness = 1.5f;
	/** 뚜껑이 수직에서 뒤로 넘어간 각도 */
	constexpr float LidLeanDegrees = 12.0f;
	constexpr float GridLineWidth = 0.25f;
	constexpr float GridLineHeight = 0.06f;
	constexpr float PlateHeight = 0.08f;
	const FName ColorParam(TEXT("Color"));

	UStaticMesh* LoadEngineMesh(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
}

ACaseViewActor::ACaseViewActor()
{
	// 애니메이션만 틱에서 진행한다. 격자 상태는 틱과 상관없이 즉시 바뀐다
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// 이 프로젝트의 독자 디자인 케이스 부품 (Tools/blender/make_items.py). 없으면 기본 큐브로 대신한다
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> BevelFinder(TEXT("/Game/Meshes/SM_Case_BevelBox.SM_Case_BevelBox"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> CornerFinder(TEXT("/Game/Meshes/SM_Case_Corner.SM_Case_Corner"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> LatchFinder(TEXT("/Game/Meshes/SM_Case_Latch.SM_Case_Latch"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> HandleFinder(TEXT("/Game/Meshes/SM_Case_Handle.SM_Case_Handle"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> HingeFinder(TEXT("/Game/Meshes/SM_Case_Hinge.SM_Case_Hinge"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SurfaceFinder(TEXT("/Game/Materials/M_CaseSurface.M_CaseSurface"));
	UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
	FallbackMesh = Cube;
	ColorMaterial = ColorFinder.Succeeded() ? ColorFinder.Object : nullptr;
	SurfaceMaterial = SurfaceFinder.Get();
	auto OrCube = [Cube](UStaticMesh* Mesh) { return Mesh ? Mesh : Cube; };

	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(Mesh);
		// 선택은 바닥 평면 계산으로 하므로 케이스 부품도 충돌이 필요 없다
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};

	UStaticMesh* Panel = OrCube(BevelFinder.Get());
	CaseBase = MakePart(TEXT("CaseBase"), Cube);
	Shell = MakePart(TEXT("Shell"), Panel);
	Walls.Add(MakePart(TEXT("WallBack"), Panel));
	Walls.Add(MakePart(TEXT("WallFront"), Panel));
	Walls.Add(MakePart(TEXT("WallLeft"), Panel));
	Walls.Add(MakePart(TEXT("WallRight"), Panel));
	Lid = MakePart(TEXT("Lid"), Panel);
	LidLining = MakePart(TEXT("LidLining"), Cube);
	Latches.Add(MakePart(TEXT("LatchLeft"), OrCube(LatchFinder.Get())));
	Latches.Add(MakePart(TEXT("LatchRight"), OrCube(LatchFinder.Get())));
	Latches.Add(MakePart(TEXT("Handle"), OrCube(HandleFinder.Get())));
	for (int32 i = 0; i < 8; ++i)
	{
		Corners.Add(MakePart(*FString::Printf(TEXT("Corner%d"), i), OrCube(CornerFinder.Get())));
	}
	Hinges.Add(MakePart(TEXT("HingeLeft"), OrCube(HingeFinder.Get())));
	Hinges.Add(MakePart(TEXT("HingeRight"), OrCube(HingeFinder.Get())));

	// 슬롯 이름별 표면 값 (선형 색, 거칠기, 금속성, 결). 값은 이 프로젝트에서 정한 것
	auto Look = [this](const TCHAR* Name, FLinearColor Color, float Roughness, float Metallic, float Grain)
	{
		FSurfaceLook& L = SurfaceLooks.Add(Name);
		L.Color = Color;
		L.Roughness = Roughness;
		L.Metallic = Metallic;
		L.Grain = Grain;
	};
	Look(TEXT("Leather"), FLinearColor(0.045f, 0.022f, 0.012f), 0.55f, 0.0f, 0.6f);
	Look(TEXT("LeatherDark"), FLinearColor(0.02f, 0.011f, 0.006f), 0.5f, 0.0f, 0.5f);
	Look(TEXT("Foam"), FLinearColor(0.016f, 0.018f, 0.018f), 0.95f, 0.0f, 0.35f);
	Look(TEXT("FoamLine"), FLinearColor(0.045f, 0.05f, 0.045f), 0.9f, 0.0f, 0.1f);
	Look(TEXT("Brass"), FLinearColor(0.55f, 0.38f, 0.13f), 0.35f, 1.0f, 0.2f);
	Look(TEXT("BrassDark"), FLinearColor(0.3f, 0.2f, 0.07f), 0.45f, 1.0f, 0.2f);
	Look(TEXT("Steel"), FLinearColor(0.6f, 0.6f, 0.62f), 0.3f, 1.0f, 0.1f);
	Look(TEXT("Gunmetal"), FLinearColor(0.06f, 0.06f, 0.065f), 0.35f, 0.9f, 0.1f);
	Look(TEXT("GunmetalDark"), FLinearColor(0.03f, 0.03f, 0.032f), 0.45f, 0.8f, 0.1f);
	Look(TEXT("Polymer"), FLinearColor(0.02f, 0.02f, 0.02f), 0.6f, 0.0f, 0.3f);
	Look(TEXT("PolymerDark"), FLinearColor(0.012f, 0.012f, 0.012f), 0.75f, 0.0f, 0.5f);
	Look(TEXT("Wood"), FLinearColor(0.2f, 0.08f, 0.03f), 0.5f, 0.0f, 0.5f);
	Look(TEXT("WoodDark"), FLinearColor(0.09f, 0.035f, 0.012f), 0.6f, 0.0f, 0.4f);
	Look(TEXT("Rubber"), FLinearColor(0.015f, 0.015f, 0.015f), 0.85f, 0.0f, 0.4f);
	Look(TEXT("RubberDark"), FLinearColor(0.008f, 0.008f, 0.008f), 0.9f, 0.0f, 0.4f);
	Look(TEXT("Cardboard"), FLinearColor(0.33f, 0.2f, 0.09f), 0.85f, 0.0f, 0.6f);
	Look(TEXT("CardboardDark"), FLinearColor(0.18f, 0.1f, 0.04f), 0.85f, 0.0f, 0.5f);
	Look(TEXT("LabelYellow"), FLinearColor(0.6f, 0.42f, 0.05f), 0.6f, 0.0f, 0.2f);
	Look(TEXT("LabelRed"), FLinearColor(0.45f, 0.03f, 0.02f), 0.6f, 0.0f, 0.2f);
	Look(TEXT("LabelDark"), FLinearColor(0.02f, 0.02f, 0.02f), 0.7f, 0.0f, 0.1f);
	Look(TEXT("LabelGreen"), FLinearColor(0.04f, 0.3f, 0.1f), 0.5f, 0.0f, 0.1f);
	Look(TEXT("Stem"), FLinearColor(0.08f, 0.15f, 0.03f), 0.7f, 0.0f, 0.3f);
	Look(TEXT("LeafGreen"), FLinearColor(0.05f, 0.25f, 0.03f), 0.55f, 0.0f, 0.4f);
	Look(TEXT("LeafRed"), FLinearColor(0.35f, 0.03f, 0.02f), 0.55f, 0.0f, 0.4f);
	Look(TEXT("Twine"), FLinearColor(0.35f, 0.25f, 0.12f), 0.9f, 0.0f, 0.6f);
	Look(TEXT("Olive"), FLinearColor(0.08f, 0.09f, 0.04f), 0.6f, 0.2f, 0.4f);
	Look(TEXT("OliveDark"), FLinearColor(0.04f, 0.045f, 0.02f), 0.65f, 0.2f, 0.3f);
	Look(TEXT("Paint"), FLinearColor(0.7f, 0.7f, 0.68f), 0.35f, 0.0f, 0.15f);
	Look(TEXT("PaintDark"), FLinearColor(0.25f, 0.25f, 0.25f), 0.4f, 0.0f, 0.1f);
	Look(TEXT("PlasticWhite"), FLinearColor(0.85f, 0.85f, 0.85f), 0.4f, 0.0f, 0.05f);
	Look(TEXT("Glass"), FLinearColor(0.3f, 0.35f, 0.35f), 0.1f, 0.0f, 0.05f);
	Look(TEXT("Powder"), FLinearColor(0.05f, 0.05f, 0.05f), 0.9f, 0.0f, 0.8f);

	// Fab 무료 에셋 머티리얼 (출처·라이선스는 ASSETS.md). 받지 않았거나 지웠으면 위 값으로 그린다
	SurfaceLooks[TEXT("Leather")].Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MaterialsLeather/Materials/Instances/MI_Leather_Worn_02.MI_Leather_Worn_02")));
	SurfaceLooks[TEXT("Brass")].Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MetalMat/Materials/MI_MetallMat_Inst9.MI_MetallMat_Inst9")));
	SurfaceLooks[TEXT("Steel")].Material = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MetalMat/Materials/MI_MetallMat_Inst6.MI_MetallMat_Inst6")));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetFieldOfView(45.0f);

	CaseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CaseLight"));
	CaseLight->SetupAttachment(Root);
	CaseLight->SetIntensityUnits(ELightUnits::Lumens);
	CaseLight->SetIntensity(400.0f);
	CaseLight->SetAttenuationRadius(400.0f);
	CaseLight->SetSourceRadius(20.0f);
	CaseLight->SetLightColor(FLinearColor(1.0f, 0.9f, 0.78f));
}

void ACaseViewActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutCase();
}

void ACaseViewActor::BeginPlay()
{
	Super::BeginPlay();

	Grid = NewObject<UCaseGrid>(this);
	Grid->Initialize(GridSize.X, GridSize.Y);
	Cursor = NewObject<UCaseCursor>(this);
	Cursor->Bind(Grid);
	LayoutCase();

	Grid->OnChanged.AddDynamic(this, &ACaseViewActor::HandleGridChanged);
	TArray<FStartingItem> Items = StartingItems;
	if (Items.Num() == 0 && bUseDemoItems)
	{
		Items = MakeDemoItems(this);
	}
	for (const FStartingItem& Start : Items)
	{
		if (Start.Definition)
		{
			DemoDefinitions.AddUnique(Start.Definition);
		}
		Grid->AddItem(Start.Definition, Start.Count);
	}
	HandleGridChanged();
}

FIntPoint ACaseViewActor::GetCurrentGridSize() const
{
	return Grid ? FIntPoint(Grid->GetWidth(), Grid->GetHeight()) : GridSize;
}

void ACaseViewActor::SetColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (!Component || !ColorMaterial)
	{
		return;
	}
	UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
	if (!Dynamic || Dynamic->Parent != ColorMaterial)
	{
		Dynamic = UMaterialInstanceDynamic::Create(ColorMaterial, Component);
		Component->SetMaterial(0, Dynamic);
	}
	Dynamic->SetVectorParameterValue(ColorParam, Color);
}

UMaterialInstanceDynamic* ACaseViewActor::GetSurfaceInstance(UStaticMeshComponent* Component, int32 Slot)
{
	UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(Slot));
	if (!Dynamic || Dynamic->Parent != SurfaceMaterial)
	{
		Dynamic = UMaterialInstanceDynamic::Create(SurfaceMaterial, Component);
		Component->SetMaterial(Slot, Dynamic);
	}
	return Dynamic;
}

void ACaseViewActor::ApplyLookToSlot(UStaticMeshComponent* Component, int32 Slot, const FSurfaceLook& Look)
{
	if (!Look.Material.IsNull())
	{
		if (UMaterialInterface* Override = Look.Material.LoadSynchronous())
		{
			Component->SetMaterial(Slot, Override);
			return;
		}
	}
	UMaterialInstanceDynamic* Dynamic = GetSurfaceInstance(Component, Slot);
	Dynamic->SetVectorParameterValue(ColorParam, Look.Color);
	Dynamic->SetScalarParameterValue(TEXT("Roughness"), Look.Roughness);
	Dynamic->SetScalarParameterValue(TEXT("Metallic"), Look.Metallic);
	Dynamic->SetScalarParameterValue(TEXT("GrainAmount"), Look.Grain);
}

void ACaseViewActor::ApplySurface(UStaticMeshComponent* Component, FName Look)
{
	const FSurfaceLook* Found = SurfaceLooks.Find(Look);
	if (!Component || !Found)
	{
		return;
	}
	if (!SurfaceMaterial)
	{
		SetColor(Component, Found->Color);
		return;
	}
	for (int32 Slot = 0; Slot < FMath::Max(1, Component->GetNumMaterials()); ++Slot)
	{
		ApplyLookToSlot(Component, Slot, *Found);
	}
}

void ACaseViewActor::ApplySlotSurfaces(UStaticMeshComponent* Component)
{
	if (!Component || !Component->GetStaticMesh() || !SurfaceMaterial)
	{
		return;
	}
	const TArray<FStaticMaterial>& Slots = Component->GetStaticMesh()->GetStaticMaterials();
	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		// 임포트할 때 슬롯 이름 뒤에 붙는 번호(_1 등)는 떼고 찾는다
		FString Name = Slots[Slot].MaterialSlotName.ToString();
		const FSurfaceLook* Found = SurfaceLooks.Find(*Name);
		int32 Underscore = INDEX_NONE;
		if (!Found && Name.FindLastChar(TEXT('_'), Underscore) && Name.Mid(Underscore + 1).IsNumeric())
		{
			Found = SurfaceLooks.Find(*Name.Left(Underscore));
		}
		if (!Found)
		{
			continue;
		}
		ApplyLookToSlot(Component, Slot, *Found);
	}
}

UStaticMeshComponent* ACaseViewActor::MakeRuntimeMesh(UStaticMesh* Mesh)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
	Component->SetupAttachment(Root);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetStaticMesh(Mesh);
	Component->RegisterComponent();
	return Component;
}

void ACaseViewActor::PlaceBox(UStaticMeshComponent* Component, const FVector& Center, const FVector& Size, const FRotator& Rotation)
{
	if (!Component || !Component->GetStaticMesh())
	{
		return;
	}
	// 메시 크기와 피벗에 상관없이 바운드 가운데를 Center에, 크기를 Size에 맞춘다
	const FBox Bounds = Component->GetStaticMesh()->GetBoundingBox();
	const FVector Scale = Size / Bounds.GetSize().ComponentMax(FVector(KINDA_SMALL_NUMBER));
	const FQuat Quat = Rotation.Quaternion();
	Component->SetRelativeTransform(FTransform(Quat, Center - Quat.RotateVector(Bounds.GetCenter() * Scale), Scale));
}

void ACaseViewActor::LayoutCase()
{
	const FIntPoint Size = GetCurrentGridSize();
	const float InnerX = Size.X * CellSize;
	const float InnerY = Size.Y * CellSize;
	const float T = WallThickness;
	const float WallTall = WallHeight - ShellBottomZ;
	const float WallMidZ = (WallHeight + ShellBottomZ) * 0.5f;

	// 안쪽 바닥: 윗면이 Z=0, 칸 (0,0) 모서리가 원점
	PlaceBox(CaseBase, FVector(InnerX * 0.5f, InnerY * 0.5f, GetBaseTopZ() - BaseThickness * 0.5f), FVector(InnerX, InnerY, BaseThickness));
	const float ShellTop = GetBaseTopZ() - BaseThickness;
	PlaceBox(Shell, FVector(InnerX * 0.5f, InnerY * 0.5f, (ShellTop + ShellBottomZ) * 0.5f),
		FVector(InnerX + T * 2.0f, InnerY + T * 2.0f, ShellTop - ShellBottomZ));

	PlaceBox(Walls[0], FVector(InnerX * 0.5f, -T * 0.5f, WallMidZ), FVector(InnerX + T * 2.0f, T, WallTall));
	PlaceBox(Walls[1], FVector(InnerX * 0.5f, InnerY + T * 0.5f, WallMidZ), FVector(InnerX + T * 2.0f, T, WallTall));
	PlaceBox(Walls[2], FVector(-T * 0.5f, InnerY * 0.5f, WallMidZ), FVector(T, InnerY, WallTall));
	PlaceBox(Walls[3], FVector(InnerX + T * 0.5f, InnerY * 0.5f, WallMidZ), FVector(T, InnerY, WallTall));

	// 뚜껑: 뒤쪽 벽 윗모서리를 경첩으로 열려 조금 뒤로 기대 서 있다
	const float LidLength = InnerY + T * 2.0f;
	const float Lean = FMath::DegreesToRadians(LidLeanDegrees);
	const FVector Hinge(InnerX * 0.5f, -T, WallHeight);
	const FVector Up(0.0f, -FMath::Sin(Lean), FMath::Cos(Lean));     // 뚜껑 길이 방향
	const FVector Inward(0.0f, FMath::Cos(Lean), FMath::Sin(Lean));  // 케이스 쪽을 보는 면
	const FRotator LidRotation = FRotationMatrix::MakeFromYZ(Up, Inward).Rotator();
	PlaceBox(Lid, Hinge + Up * (LidLength * 0.5f) - Inward * (LidThickness * 0.5f), FVector(InnerX + T * 2.0f, LidLength, LidThickness), LidRotation);
	PlaceBox(LidLining, Hinge + Up * (LidLength * 0.5f) + Inward * 0.05f, FVector(InnerX, LidLength - T * 2.0f, 0.1f), LidRotation);

	// 앞쪽 벽 바깥의 잠금쇠 2개와 손잡이. 부품 크기는 케이스 크기와 상관없이 고정이고 위치만 따라간다
	const float FrontY = InnerY + T;
	PlaceBox(Latches[0], FVector(InnerX * 0.2f, FrontY + 0.75f, WallHeight - 1.9f), FVector(5.0f, 1.5f, 3.8f));
	PlaceBox(Latches[1], FVector(InnerX * 0.8f, FrontY + 0.75f, WallHeight - 1.9f), FVector(5.0f, 1.5f, 3.8f));
	// 손잡이는 아치(로컬 +Z)가 앞(+Y)으로 나오도록 X축으로 +90도 눕힌다. Size는 메시 로컬 축 기준이라 SM_Case_Handle 치수(폭 15.2, 벽 방향 2.4, 아치 깊이 5.17) 그대로 준다
	PlaceBox(Latches[2], FVector(InnerX * 0.5f, FrontY + 2.6f, WallMidZ), FVector(15.2f, 2.4f, 5.17f), FRotator(0.0f, 0.0f, 90.0f));

	// 모서리 보호대: 바닥판 아래 네 모서리, 벽 위 네 모서리
	const float CornerSize = 2.6f;
	const float OuterX[2] = { -T + CornerSize * 0.35f, InnerX + T - CornerSize * 0.35f };
	const float OuterY[2] = { -T + CornerSize * 0.35f, InnerY + T - CornerSize * 0.35f };
	const float CornerZ[2] = { ShellBottomZ + CornerSize * 0.35f, WallHeight - CornerSize * 0.35f };
	for (int32 i = 0; i < 8; ++i)
	{
		PlaceBox(Corners[i], FVector(OuterX[i & 1], OuterY[(i >> 1) & 1], CornerZ[(i >> 2) & 1]), FVector(CornerSize));
	}
	// 경첩: 뒤쪽 벽 윗모서리
	PlaceBox(Hinges[0], FVector(InnerX * 0.22f, -T - 0.3f, WallHeight), FVector(6.0f, 1.2f, 2.6f), FRotator(0.0f, 0.0f, 90.0f));
	PlaceBox(Hinges[1], FVector(InnerX * 0.78f, -T - 0.3f, WallHeight), FVector(6.0f, 1.2f, 2.6f), FRotator(0.0f, 0.0f, 90.0f));

	ApplySurface(CaseBase, TEXT("Foam"));
	ApplySurface(Shell, TEXT("Leather"));
	for (UStaticMeshComponent* Wall : Walls)
	{
		ApplySurface(Wall, TEXT("Leather"));
	}
	ApplySurface(Lid, TEXT("Leather"));
	ApplySurface(LidLining, TEXT("Foam"));
	for (UStaticMeshComponent* Part : Latches)
	{
		ApplySlotSurfaces(Part);
	}
	for (UStaticMeshComponent* Part : Corners)
	{
		ApplySlotSurfaces(Part);
	}
	for (UStaticMeshComponent* Part : Hinges)
	{
		ApplySlotSurfaces(Part);
	}

	// 앞쪽 위에서 비스듬히 내려다본다. 화면 위쪽이 0행, 오른쪽이 +X
	const FVector Center(InnerX * 0.5f, InnerY * 0.5f, 0.0f);
	const float Pitch = 58.0f;
	const float Distance = FMath::Max(InnerX * 1.6f, InnerY * 2.6f) + 40.0f;
	const float PitchRad = FMath::DegreesToRadians(Pitch);
	const FVector CameraOffset(0.0f, FMath::Cos(PitchRad) * Distance, FMath::Sin(PitchRad) * Distance);
	// 뚜껑이 화면 위쪽에 보이도록 바라보는 점을 조금 뒤로 둔다
	const FVector LookAt = Center + FVector(0.0f, -InnerY * 0.12f, 0.0f);
	CameraTarget = FTransform(FRotator(-Pitch, -90.0f, 0.0f), LookAt + CameraOffset);
	// 처음 놓을 때와 애니메이션을 끈 경우에는 바로, 실행 중 확장할 때는 틱에서 부드럽게 옮긴다
	if (!bCameraPlaced || !ShouldAnimate())
	{
		Camera->SetRelativeTransform(CameraTarget);
		bCameraPlaced = HasActorBegunPlay();
	}
	CaseLight->SetRelativeLocation(Center + FVector(0.0f, InnerY * 0.6f, 55.0f));

	// 자동 노출 범위를 한 값으로 묶어 고정한다 (프로젝트 설정상 EV100 단위)
	FPostProcessSettings& Post = Camera->PostProcessSettings;
	Post.bOverride_AutoExposureMinBrightness = true;
	Post.bOverride_AutoExposureMaxBrightness = true;
	Post.AutoExposureMinBrightness = ExposureEV100;
	Post.AutoExposureMaxBrightness = ExposureEV100;

	// 격자선은 실행 중에만 만든다 (생성 스크립트에서 만든 컴포넌트는 다시 실행될 때마다 정리가 필요해서)
	if (GetWorld() && GetWorld()->IsGameWorld() && FallbackMesh)
	{
		const int32 LineCount = (Size.X + 1) + (Size.Y + 1);
		while (GridLines.Num() < LineCount)
		{
			GridLines.Add(MakeRuntimeMesh(FallbackMesh));
		}
		int32 Index = 0;
		for (int32 X = 0; X <= Size.X; ++X)
		{
			PlaceBox(GridLines[Index++], FVector(X * CellSize, InnerY * 0.5f, GridLineHeight * 0.5f), FVector(GridLineWidth, InnerY, GridLineHeight));
		}
		for (int32 Y = 0; Y <= Size.Y; ++Y)
		{
			PlaceBox(GridLines[Index++], FVector(InnerX * 0.5f, Y * CellSize, GridLineHeight * 0.5f), FVector(InnerX, GridLineWidth, GridLineHeight));
		}
		for (int32 i = 0; i < GridLines.Num(); ++i)
		{
			ApplySurface(GridLines[i], TEXT("FoamLine"));
			GridLines[i]->SetVisibility(i < LineCount);
		}
	}
	LaidOutSize = Size;
}

void ACaseViewActor::HandleGridChanged()
{
	if (!Grid)
	{
		return;
	}
	if (GetCurrentGridSize() != LaidOutSize)
	{
		LayoutCase();
	}

	TSet<int32> Alive;
	for (const FInventoryItem& Item : Grid->GetItems())
	{
		if (!Item.Definition)
		{
			continue;
		}
		Alive.Add(Item.InstanceId);

		TObjectPtr<UStaticMeshComponent>& Component = ItemComponents.FindOrAdd(Item.InstanceId);
		if (!Component)
		{
			// 마우스 선택은 케이스 바닥 평면에서 칸으로 찾으므로 아이템 자체는 충돌이 필요 없다
			Component = MakeRuntimeMesh(nullptr);
		}

		UStaticMesh* Mesh = Item.Definition->Mesh.LoadSynchronous();
		if (!Mesh)
		{
			Mesh = FallbackMesh;
		}
		if (Component->GetStaticMesh() != Mesh)
		{
			Component->SetStaticMesh(Mesh);
		}
		if (Item.Definition->bOverrideColor)
		{
			SetColor(Component, Item.Definition->Color);
		}
		else
		{
			ApplySlotSurfaces(Component);
		}

		if (Item.Definition->MaxStack > 1)
		{
			TObjectPtr<UTextRenderComponent>& Text = CountTexts.FindOrAdd(Item.InstanceId);
			if (!Text)
			{
				Text = NewObject<UTextRenderComponent>(this);
				Text->SetupAttachment(Root);
				Text->SetHorizontalAlignment(EHTA_Right);
				Text->SetVerticalAlignment(EVRTA_TextBottom);
				Text->SetWorldSize(2.4f);
				Text->SetTextRenderColor(FColor(235, 230, 210));
				Text->RegisterComponent();

				// 밝은 금속 아이템 위에서도 읽히게, 글자 바로 뒤(-X)에 네 대각선으로 검은 복사본을 깔아 외곽선을 만든다
				for (const FVector2D Offset : {FVector2D(1, 1), FVector2D(1, -1), FVector2D(-1, 1), FVector2D(-1, -1)})
				{
					UTextRenderComponent* Outline = NewObject<UTextRenderComponent>(this);
					Outline->SetupAttachment(Text);
					Outline->SetRelativeLocation(FVector(-0.03f, Offset.X * 0.13f, Offset.Y * 0.13f));
					Outline->SetHorizontalAlignment(EHTA_Right);
					Outline->SetVerticalAlignment(EVRTA_TextBottom);
					Outline->SetWorldSize(2.4f);
					Outline->SetTextRenderColor(FColor(8, 6, 4));
					Outline->RegisterComponent();
				}
			}
			Text->SetText(FText::AsNumber(Item.Count));
			for (USceneComponent* Child : Text->GetAttachChildren())
			{
				if (UTextRenderComponent* Outline = Cast<UTextRenderComponent>(Child))
				{
					Outline->SetText(FText::AsNumber(Item.Count));
				}
			}
		}
	}

	for (auto It = ItemComponents.CreateIterator(); It; ++It)
	{
		if (!Alive.Contains(It.Key()))
		{
			if (It.Value())
			{
				It.Value()->DestroyComponent();
			}
			It.RemoveCurrent();
		}
	}
	for (auto It = PlacedItems.CreateIterator(); It; ++It)
	{
		if (!Alive.Contains(*It))
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = CountTexts.CreateIterator(); It; ++It)
	{
		if (!Alive.Contains(It.Key()))
		{
			if (It.Value())
			{
				// 외곽선 복사본도 같이 지운다 (지우는 동안 목록이 바뀌므로 복사해서 돈다)
				const TArray<TObjectPtr<USceneComponent>> Outlines = It.Value()->GetAttachChildren();
				for (USceneComponent* Outline : Outlines)
				{
					Outline->DestroyComponent();
				}
				It.Value()->DestroyComponent();
			}
			It.RemoveCurrent();
		}
	}

	// 기록이 바뀌면서 집고 있던 아이템이 사라졌으면 손을 비운다
	if (Cursor && Cursor->IsHolding() && !Alive.Contains(Cursor->GetHeldId()))
	{
		Cursor->Cancel();
	}
	RefreshPreview();
}

void ACaseViewActor::RefreshPreview()
{
	if (!Grid || !Cursor)
	{
		return;
	}
	const FIntPoint Size = GetCurrentGridSize();
	const int32 HeldId = Cursor->GetHeldId();

	// 아이템 목표 자리: 집은 아이템만 커서를 따라 떠 있고 나머지는 격자 상태 그대로
	FInventoryItem HoveredItem;
	const int32 HoveredId = (!Cursor->IsHolding() && Grid->GetItem(Grid->GetItemAt(Cursor->GetHoverCell()), HoveredItem)) ? HoveredItem.InstanceId : 0;
	TargetTransforms.Reset();
	for (const FInventoryItem& Item : Grid->GetItems())
	{
		UStaticMeshComponent* Component = ItemComponents.FindRef(Item.InstanceId);
		if (!Component || !Component->GetStaticMesh() || !Item.Definition)
		{
			continue;
		}
		const bool bHeld = Item.InstanceId == HeldId;
		FIntPoint Origin = bHeld ? Cursor->GetTargetOrigin() : Item.Origin;
		const int32 Rotation = bHeld ? Cursor->GetHeldRotation() : Item.Rotation;
		if (bHeld)
		{
			// 격자 밖을 가리켜도 아이템은 케이스 안에 머문다. 놓을 수 있는지는 실제 목표 칸(빨간 판)으로 판단한다
			const FIntPoint Footprint = ItemShape::GetFootprint(Item.Definition->Size, Rotation);
			Origin.X = FMath::Clamp(Origin.X, 0, FMath::Max(0, Size.X - Footprint.X));
			Origin.Y = FMath::Clamp(Origin.Y, 0, FMath::Max(0, Size.Y - Footprint.Y));
		}
		const float Lift = bHeld ? HeldLift : (Item.InstanceId == HoveredId ? HoverLift : 0.0f);
		const FTransform Target = ComputeItemTransform(Component->GetStaticMesh()->GetBoundingBox(), *Item.Definition, Origin, Rotation, CellSize, GetBaseTopZ() + Lift);
		TargetTransforms.Add(Item.InstanceId, Target);
		// 처음 나타난 아이템(시작, 버리기 되돌리기)은 날아오지 않고 바로 제자리에 놓는다
		bool bAlreadyPlaced = false;
		PlacedItems.Add(Item.InstanceId, &bAlreadyPlaced);
		if (!ShouldAnimate() || !bAlreadyPlaced)
		{
			ApplyItemTransform(Item.InstanceId, Component, Target);
		}
	}

	// 칸 강조 판
	int32 Used = 0;
	auto ShowPlate = [&](FIntPoint Cell, const FLinearColor& Color)
	{
		if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Size.X || Cell.Y >= Size.Y || !FallbackMesh)
		{
			return;
		}
		if (Plates.Num() <= Used)
		{
			Plates.Add(MakeRuntimeMesh(FallbackMesh));
		}
		UStaticMeshComponent* Plate = Plates[Used++];
		const float Inner = CellSize - GridLineWidth * 2.0f;
		PlaceBox(Plate, FVector((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize, GridLineHeight + PlateHeight * 0.5f), FVector(Inner, Inner, PlateHeight));
		SetColor(Plate, Color);
		Plate->SetVisibility(true);
	};
	auto ShowArea = [&](FIntPoint Origin, FIntPoint Footprint, const FLinearColor& Color)
	{
		for (int32 Y = 0; Y < Footprint.Y; ++Y)
		{
			for (int32 X = 0; X < Footprint.X; ++X)
			{
				ShowPlate(Origin + FIntPoint(X, Y), Color);
			}
		}
	};

	if (Cursor->IsHolding())
	{
		ShowArea(Cursor->GetTargetOrigin(), Cursor->GetHeldFootprint(), Cursor->CanDropHere() ? ValidColor : InvalidColor);
	}
	else
	{
		FInventoryItem Hovered;
		if (Grid->GetItem(Grid->GetItemAt(Cursor->GetHoverCell()), Hovered) && Hovered.Definition)
		{
			ShowArea(Hovered.Origin, ItemShape::GetFootprint(Hovered.Definition->Size, Hovered.Rotation), HoverColor);
		}
		else
		{
			ShowPlate(Cursor->GetHoverCell(), HoverColor * 0.4f);
		}
	}
	for (int32 i = Used; i < Plates.Num(); ++i)
	{
		Plates[i]->SetVisibility(false);
	}
}

bool ACaseViewActor::ShouldAnimate() const
{
	return bAnimate && HasActorBegunPlay();
}

void ACaseViewActor::ApplyItemTransform(int32 InstanceId, UStaticMeshComponent* Component, const FTransform& Transform)
{
	Component->SetRelativeTransform(Transform);
	if (UTextRenderComponent* Text = CountTexts.FindRef(InstanceId))
	{
		// 아이템 오른쪽 아래 모서리 위에 눕혀서, 글자 위쪽이 화면 위쪽(-Y)을 향하게
		const FBox Box = Component->GetStaticMesh()->GetBoundingBox().TransformBy(Transform);
		const FVector Corner(Box.Max.X - 0.2f, Box.Max.Y - 0.2f, Box.Max.Z + 0.05f);
		Text->SetRelativeLocationAndRotation(Corner, FRotationMatrix::MakeFromXZ(FVector::UpVector, -FVector::YAxisVector).Rotator());
	}
}

void ACaseViewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceAnimations(DeltaSeconds);
}

void ACaseViewActor::AdvanceAnimations(float DeltaSeconds)
{
	// 지수 감쇠: 프레임 속도와 상관없이 같은 시간에 같은 만큼 다가간다
	const float MoveAlpha = 1.0f - FMath::Exp(-MoveSharpness * DeltaSeconds);
	const float RotateAlpha = 1.0f - FMath::Exp(-RotateSharpness * DeltaSeconds);
	FailShakeTime = FMath::Max(0.0f, FailShakeTime - DeltaSeconds);

	for (const TPair<int32, FTransform>& Pair : TargetTransforms)
	{
		UStaticMeshComponent* Component = ItemComponents.FindRef(Pair.Key);
		if (!Component || !Component->GetStaticMesh())
		{
			continue;
		}
		FTransform Target = Pair.Value;
		if (Pair.Key == FailShakeId && FailShakeTime > 0.0f && FailShakeDuration > 0.0f)
		{
			// 못 놓는 자리: 좌우로 빠르게 흔들고 점점 잦아든다
			const float Fade = FailShakeTime / FailShakeDuration;
			Target.AddToTranslation(FVector(FMath::Sin(FailShakeTime * 70.0f) * 0.8f * Fade, 0.0f, 0.0f));
		}
		if (float* Pulse = LandPulses.Find(Pair.Key))
		{
			// 내려놓음: 살짝 눌렸다가 원래 크기로
			*Pulse = FMath::Max(0.0f, *Pulse - DeltaSeconds);
			const float Phase = LandPulseDuration > 0.0f ? *Pulse / LandPulseDuration : 0.0f;
			Target.SetScale3D(Target.GetScale3D() * (1.0f - 0.07f * FMath::Sin(Phase * PI)));
		}

		const FTransform Current = Component->GetRelativeTransform();
		FTransform Next;
		Next.SetLocation(FMath::Lerp(Current.GetLocation(), Target.GetLocation(), MoveAlpha));
		Next.SetRotation(FQuat::Slerp(Current.GetRotation(), Target.GetRotation(), RotateAlpha).GetNormalized());
		Next.SetScale3D(FMath::Lerp(Current.GetScale3D(), Target.GetScale3D(), MoveAlpha));
		// 충분히 가까우면 정확히 맞춰 둔다 (떨림·오차 누적 방지)
		if (Next.GetLocation().Equals(Target.GetLocation(), 0.01f) && Next.GetRotation().Equals(Target.GetRotation(), 1e-4f)
			&& Next.GetScale3D().Equals(Target.GetScale3D(), 1e-4f))
		{
			Next = Target;
		}
		ApplyItemTransform(Pair.Key, Component, Next);
	}
	for (auto It = LandPulses.CreateIterator(); It; ++It)
	{
		if (It.Value() <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}

	if (Camera && bCameraPlaced)
	{
		const FTransform Current = Camera->GetRelativeTransform();
		Camera->SetRelativeLocationAndRotation(FMath::Lerp(Current.GetLocation(), CameraTarget.GetLocation(), MoveAlpha * 0.5f),
			FQuat::Slerp(Current.GetRotation(), CameraTarget.GetRotation(), MoveAlpha * 0.5f));
	}
}

bool ACaseViewActor::GetItemTargetTransform(int32 InstanceId, FTransform& OutTransform) const
{
	if (const FTransform* Found = TargetTransforms.Find(InstanceId))
	{
		OutTransform = *Found;
		return true;
	}
	return false;
}

void ACaseViewActor::FinishAnimations()
{
	FailShakeTime = 0.0f;
	LandPulses.Reset();
	for (const TPair<int32, FTransform>& Pair : TargetTransforms)
	{
		if (UStaticMeshComponent* Component = ItemComponents.FindRef(Pair.Key))
		{
			ApplyItemTransform(Pair.Key, Component, Pair.Value);
		}
	}
	if (Camera)
	{
		Camera->SetRelativeTransform(CameraTarget);
	}
}

bool ACaseViewActor::IsAnimating() const
{
	if (FailShakeTime > 0.0f || LandPulses.Num() > 0)
	{
		return true;
	}
	for (const TPair<int32, FTransform>& Pair : TargetTransforms)
	{
		const UStaticMeshComponent* Component = ItemComponents.FindRef(Pair.Key);
		if (Component && !Component->GetRelativeTransform().Equals(Pair.Value, 0.01f))
		{
			return true;
		}
	}
	return Camera && bCameraPlaced && !Camera->GetRelativeTransform().Equals(CameraTarget, 0.01f);
}

void ACaseViewActor::SetStatus(const FString& Text)
{
	StatusText = Text;
}

bool ACaseViewActor::UpdateCursorFromRay(FVector RayOrigin, FVector RayDirection)
{
	if (!Cursor)
	{
		return false;
	}
	const FTransform& ActorTransform = GetActorTransform();
	const FVector LocalOrigin = ActorTransform.InverseTransformPosition(RayOrigin);
	const FVector LocalDir = ActorTransform.InverseTransformVector(RayDirection);
	// 집었을 때도 바닥 평면 기준으로 찾는다. 떠 있는 높이로 바꾸면 비스듬한 카메라에서 집는 순간 한 칸 밀린다
	const float PlaneZ = GetBaseTopZ();
	if (FMath::Abs(LocalDir.Z) < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	const float Distance = (PlaneZ - LocalOrigin.Z) / LocalDir.Z;
	if (Distance < 0.0f)
	{
		return false;
	}
	const FVector Hit = LocalOrigin + LocalDir * Distance;
	const FIntPoint Cell(FMath::FloorToInt(Hit.X / CellSize), FMath::FloorToInt(Hit.Y / CellSize));
	if (Cell != Cursor->GetHoverCell())
	{
		Cursor->SetHoverCell(Cell);
		RefreshPreview();
	}
	return true;
}

bool ACaseViewActor::PrimaryAction()
{
	if (!Cursor || !Grid)
	{
		return false;
	}
	bool bResult = false;
	if (Cursor->IsHolding())
	{
		const int32 Id = Cursor->GetHeldId();
		bResult = Cursor->Drop();
		SetStatus(bResult ? TEXT("Placed") : TEXT("Can't place here"));
		if (bResult)
		{
			LandPulses.Add(Id, LandPulseDuration);
		}
		else
		{
			FailShakeId = Id;
			FailShakeTime = FailShakeDuration;
		}
	}
	else
	{
		bResult = Cursor->PickUp();
		FInventoryItem Item;
		if (bResult && Grid->GetItem(Cursor->GetHeldId(), Item) && Item.Definition)
		{
			SetStatus(FString::Printf(TEXT("Holding %s"), *Item.Definition->DisplayName.ToString()));
		}
	}
	RefreshPreview();
	return bResult;
}

bool ACaseViewActor::RotateHeld()
{
	const bool bResult = Cursor && Cursor->Rotate();
	RefreshPreview();
	return bResult;
}

void ACaseViewActor::CancelHeld()
{
	if (Cursor && Cursor->IsHolding())
	{
		Cursor->Cancel();
		SetStatus(TEXT("Cancelled"));
		RefreshPreview();
	}
}

bool ACaseViewActor::DiscardHeld()
{
	if (!Cursor || !Cursor->IsHolding())
	{
		return false;
	}
	const bool bResult = Cursor->DiscardHeld();
	SetStatus(bResult ? TEXT("Discarded (Z to undo)") : TEXT("Discard failed"));
	RefreshPreview();
	return bResult;
}

bool ACaseViewActor::UndoLast()
{
	if (!Grid)
	{
		return false;
	}
	CancelHeld();
	const bool bResult = Grid->Undo();
	SetStatus(bResult ? TEXT("Undone") : TEXT("Nothing to undo"));
	RefreshPreview();
	return bResult;
}

bool ACaseViewActor::Arrange(EArrangeMode Mode)
{
	if (!Grid)
	{
		return false;
	}
	CancelHeld();
	const bool bResult = Grid->AutoArrange(Mode);
	const TCHAR* ModeName = Mode == EArrangeMode::SpaceFirst ? TEXT("space first") : TEXT("category first");
	SetStatus(bResult ? FString::Printf(TEXT("Auto-arranged (%s)"), ModeName) : TEXT("Auto-arrange failed: not enough room"));
	return bResult;
}

bool ACaseViewActor::ExpandCase()
{
	if (!Grid)
	{
		return false;
	}
	const int32 NewWidth = FMath::Min(Grid->GetWidth() + 1, MaxGridSize.X);
	const int32 NewHeight = FMath::Min(Grid->GetHeight() + 1, MaxGridSize.Y);
	if (NewWidth == Grid->GetWidth() && NewHeight == Grid->GetHeight())
	{
		SetStatus(TEXT("Case is already at max size"));
		return false;
	}
	CancelHeld();
	const bool bResult = Grid->Resize(NewWidth, NewHeight);
	SetStatus(FString::Printf(TEXT("Case expanded to %d x %d"), Grid->GetWidth(), Grid->GetHeight()));
	return bResult;
}

bool ACaseViewActor::ParseMeshCandidate(const FString& Candidate, FString& OutObjectPath, FRotator& OutRotation)
{
	FString Name = Candidate.TrimStartAndEnd();
	FString RotationText;
	OutRotation = FRotator::ZeroRotator;
	if (Name.Split(TEXT("@"), &Name, &RotationText))
	{
		TArray<FString> Parts;
		RotationText.ParseIntoArray(Parts, TEXT(","));
		if (Parts.Num() != 3 || !Parts[0].IsNumeric() || !Parts[1].IsNumeric() || !Parts[2].IsNumeric())
		{
			return false;
		}
		OutRotation = FRotator(FCString::Atof(*Parts[0]), FCString::Atof(*Parts[1]), FCString::Atof(*Parts[2]));
	}
	if (Name.IsEmpty())
	{
		return false;
	}
	OutObjectPath = Name.StartsWith(TEXT("/"))
		? FString::Printf(TEXT("%s.%s"), *Name, *FPaths::GetBaseFilename(Name))
		: FString::Printf(TEXT("/Game/Meshes/%s.%s"), *Name, *Name);
	return true;
}

TArray<FStartingItem> ACaseViewActor::MakeDemoItems(UObject* Outer)
{
	UStaticMesh* Cube = LoadEngineMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cylinder = LoadEngineMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Sphere = LoadEngineMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	TArray<FStartingItem> Result;
	// MeshName: /Game/Meshes 아래 독자 디자인 메시. 찾지 못하면(임포트 전, 실패) Fallback 도형을 칸에 꽉 차게 늘려 색을 입힌다
	auto Add = [&](const TCHAR* Id, const TCHAR* Name, FIntPoint Size, EItemCategory Category, int32 MaxStack, int32 Count,
		const TCHAR* MeshName, float Fit, UStaticMesh* Fallback, float FallbackHeight, const FLinearColor& FallbackColor,
		const FRotator& FallbackRotation = FRotator::ZeroRotator)
	{
		UItemDefinition* Def = NewObject<UItemDefinition>(Outer ? Outer : GetTransientPackage());
		Def->ItemId = Id;
		Def->DisplayName = FText::FromString(Name);
		Def->Size = Size;
		Def->Category = Category;
		Def->MaxStack = MaxStack;
		Def->FitRatio = Fit;
		// MeshName은 '|'로 여러 후보를 줄 수 있고 앞쪽이 우선. '/'로 시작하면 전체 경로, 아니면 /Game/Meshes 아래 이름
		UStaticMesh* Mesh = nullptr;
		TArray<FString> Candidates;
		FString(MeshName).ParseIntoArray(Candidates, TEXT("|"));
		FRotator MeshRotation = FRotator::ZeroRotator;
		for (const FString& Candidate : Candidates)
		{
			FString Path;
			if (ParseMeshCandidate(Candidate, Path, MeshRotation) && FPackageName::DoesPackageExist(Path))
			{
				Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
			}
			if (Mesh)
			{
				break;
			}
		}
		if (Mesh)
		{
			Def->Mesh = Mesh;
			Def->MeshRotation = MeshRotation;
		}
		else
		{
			Def->Mesh = Fallback;
			Def->MeshRotation = FallbackRotation;
			Def->bStretchToCells = true;
			Def->StretchHeight = FallbackHeight;
			Def->bOverrideColor = true;
			Def->Color = FallbackColor;
		}
		FStartingItem& Start = Result.AddDefaulted_GetRef();
		Start.Definition = Def;
		Start.Count = Count;
	};

	// 원기둥은 축이 Z이므로 Pitch 90으로 눕혀 긴 쪽이 가로(X)를 향하게 한다
	const FRotator LayDown(90.0f, 0.0f, 0.0f);
	Add(TEXT("Shotgun"), TEXT("Shotgun"), FIntPoint(7, 2), EItemCategory::Weapon, 1, 1, TEXT("/Game/Fab/Props/SM_Fab_Shotgun|SM_Item_Shotgun"), 0.96f, Cube, 2.2f, FLinearColor(0.09f, 0.045f, 0.02f));
	Add(TEXT("Handgun"), TEXT("Handgun"), FIntPoint(3, 2), EItemCategory::Weapon, 1, 1, TEXT("/Game/Fab/M1911/SM_Fab_M1911|SM_Item_Handgun"), 0.92f, Cube, 2.0f, FLinearColor(0.02f, 0.02f, 0.025f));
	Add(TEXT("Knife"), TEXT("Combat Knife"), FIntPoint(3, 1), EItemCategory::Weapon, 1, 1, TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/M9_Knife/SM_M9_Knife@90,0,0|SM_Item_Knife"), 0.92f, Cube, 0.8f, FLinearColor(0.45f, 0.45f, 0.47f));
	Add(TEXT("HandgunAmmo"), TEXT("Handgun Ammo"), FIntPoint(1, 2), EItemCategory::Ammo, 50, 60, TEXT("/Game/Fab/Props/SM_Fab_AmmoCrate@0,90,0|SM_Item_AmmoSmall"), 0.86f, Cube, 1.8f, FLinearColor(0.35f, 0.25f, 0.04f));
	Add(TEXT("ShotgunShells"), TEXT("Shotgun Shells"), FIntPoint(2, 2), EItemCategory::Ammo, 12, 8, TEXT("/Game/Fab/Props/SM_Fab_Shells|SM_Item_AmmoShells"), 0.86f, Cube, 2.0f, FLinearColor(0.35f, 0.03f, 0.02f));
	Add(TEXT("FirstAidSpray"), TEXT("First Aid Spray"), FIntPoint(2, 1), EItemCategory::Recovery, 1, 1, TEXT("/Game/Fab/Props/SM_Fab_SprayCan|SM_Item_Spray"), 0.9f, Cylinder, 3.0f, FLinearColor(0.6f, 0.6f, 0.55f), LayDown);
	Add(TEXT("GreenHerb"), TEXT("Green Herb"), FIntPoint(2, 1), EItemCategory::Recovery, 1, 2, TEXT("/Game/Fab/Herbs/SM_Fab_HerbGreen|SM_Item_HerbGreen"), 0.92f, Sphere, 3.0f, FLinearColor(0.04f, 0.25f, 0.03f));
	Add(TEXT("RedHerb"), TEXT("Red Herb"), FIntPoint(2, 1), EItemCategory::Recovery, 1, 1, TEXT("/Game/Fab/Herbs/SM_Fab_HerbRed|SM_Item_HerbRed"), 0.92f, Sphere, 3.0f, FLinearColor(0.3f, 0.03f, 0.02f));
	Add(TEXT("HandGrenade"), TEXT("Hand Grenade"), FIntPoint(2, 1), EItemCategory::Throwable, 1, 1, TEXT("/Game/FPS_Weapon_Bundle/Weapons/Meshes/G67_Grenade/SM_G67@90,0,0|SM_Item_Grenade"), 0.9f, Sphere, 3.5f, FLinearColor(0.1f, 0.12f, 0.06f));
	Add(TEXT("Gunpowder"), TEXT("Gunpowder"), FIntPoint(1, 1), EItemCategory::Material, 10, 5, TEXT("/Game/Fab/Props/SM_Fab_Tin|SM_Item_Gunpowder"), 0.86f, Cube, 1.2f, FLinearColor(0.2f, 0.2f, 0.2f));
	return Result;
}

FTransform ACaseViewActor::ComputeItemTransform(const FBox& MeshLocalBounds, const UItemDefinition& Definition,
	FIntPoint Origin, int32 ItemRotation, float InCellSize, float BaseTopZ)
{
	const int32 Rotation = ItemShape::NormalizeRotation(ItemRotation);
	const FQuat MeshRotation = Definition.MeshRotation.Quaternion();

	// 방향 보정을 먼저 적용한 상태의 바운드. 회전 0일 때 이 바운드의 X가 칸 가로에 대응한다
	const FBox Corrected = MeshLocalBounds.TransformBy(FTransform(MeshRotation));
	const FVector CorrectedSize = Corrected.GetSize();

	FVector Scale(Definition.MeshScale);
	if (Definition.bFitToCells)
	{
		const float ScaleX = Definition.Size.X * InCellSize * Definition.FitRatio / FMath::Max(CorrectedSize.X, KINDA_SMALL_NUMBER);
		const float ScaleY = Definition.Size.Y * InCellSize * Definition.FitRatio / FMath::Max(CorrectedSize.Y, KINDA_SMALL_NUMBER);
		if (Definition.bStretchToCells)
		{
			const float ScaleZ = Definition.StretchHeight / FMath::Max(CorrectedSize.Z, KINDA_SMALL_NUMBER);
			Scale = FVector(ScaleX, ScaleY, ScaleZ);
		}
		else
		{
			Scale = FVector(FMath::Min(ScaleX, ScaleY));
		}
	}

	// Scale은 보정 회전 뒤의 축 기준이다. 컴포넌트 배율은 모델 자체 축 기준이므로 되돌려 놓는다.
	// (보정 회전이 90도 단위일 때 정확하다. 균일 배율이면 회전과 상관없이 같다)
	const FVector MeshAxisScale = MeshRotation.UnrotateVector(Scale).GetAbs();

	// 아이템 회전은 바닥 평면에서 90도 단위로 돈다
	const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(90.0f * Rotation));
	const FIntPoint Footprint = ItemShape::GetFootprint(Definition.Size, Rotation);
	const FVector2D Target((Origin.X + Footprint.X * 0.5f) * InCellSize, (Origin.Y + Footprint.Y * 0.5f) * InCellSize);

	// 바운드 가운데가 칸 영역 가운데에, 바운드 바닥이 케이스 바닥에 오도록 피벗 차이를 상쇄한다
	const FVector ScaledCenter = Corrected.GetCenter() * Scale;
	const FVector RotatedCenter = Yaw.RotateVector(FVector(ScaledCenter.X, ScaledCenter.Y, 0.0f));
	const FVector Location(Target.X - RotatedCenter.X, Target.Y - RotatedCenter.Y, BaseTopZ - Corrected.Min.Z * Scale.Z);

	return FTransform(Yaw * MeshRotation, Location, MeshAxisScale);
}

bool ACaseViewActor::WorldToCell(FVector WorldLocation, FIntPoint& OutCell) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	OutCell = FIntPoint(FMath::FloorToInt(Local.X / CellSize), FMath::FloorToInt(Local.Y / CellSize));
	const FIntPoint Size = Grid ? FIntPoint(Grid->GetWidth(), Grid->GetHeight()) : GridSize;
	return OutCell.X >= 0 && OutCell.Y >= 0 && OutCell.X < Size.X && OutCell.Y < Size.Y;
}

FVector ACaseViewActor::CellAreaCenterToWorld(FIntPoint Origin, FIntPoint Footprint) const
{
	const FVector Local((Origin.X + Footprint.X * 0.5f) * CellSize, (Origin.Y + Footprint.Y * 0.5f) * CellSize, GetBaseTopZ());
	return GetActorTransform().TransformPosition(Local);
}
