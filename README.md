# RE4Inventory

바이오하자드 RE:4의 아타셰 케이스 인벤토리를 분석해 Unreal Engine 5.8 C++로 다시 구현한 대학 과제 프로젝트입니다.
이 저장소에는 **시스템 코드와 문서만** 있습니다. 원작 게임의 모델, 텍스처, 애니메이션은 포함하지 않으며 배포하지 않습니다.

> 이 작품은 비영리 2차 창작(팬 작품)입니다. CAPCOM과 관련이 없고 승인받지 않았으며, [캡콤 2차 창작 가이드라인](https://www.capcom-games.com/ja-jp/fan-content-guidelines/)을 따릅니다. 원작 에셋·로고는 쓰지 않습니다.
> 시연 영상의 아이템 모델은 Fab 라이선스 에셋이라 이 저장소에 없습니다. 출처와 조건은 `ASSETS.md`에 있습니다.

## 구조

| 경로 | 내용 |
|---|---|
| `Source/RE4Inventory/Inventory/ItemDefinition.h` | 아이템 정의 데이터 에셋 (크기, 종류, 최대 묶음, 메시와 메시 보정값) |
| `Source/RE4Inventory/Inventory/ItemShape.h` | 회전 계산 (`(x, y) → (H-1-y, x)`) |
| `Source/RE4Inventory/Inventory/CaseGrid.*` | 격자 배치 검사, 획득(묶음 채우기 → first-fit), 확장, 자동 정리 계산 |
| `Source/RE4Inventory/Inventory/InventoryCommands.*` | 커맨드 패턴: 이동·버리기·자동 정리와 되돌리기 기록 |
| `Source/RE4Inventory/Inventory/CaseCursor.*` | 커서 조작 상태: 집기, 90도 회전(잡은 칸 유지), 놓을 수 있는지, 놓기·취소·버리기 |
| `Source/RE4Inventory/View/CaseViewActor.*` | 3D 케이스 화면. 케이스 외형·격자선·칸 강조판(초록 가능/빨강 불가/노랑 커서)·묶음 개수를 그리고, 격자 상태에 맞춰 아이템 메시를 배치 |
| `Source/RE4Inventory/Game/*` | 입력 컨트롤러, 조작 안내 HUD, 게임 모드 |
| `Content/Maps/L_AttacheCase.umap` | 기본 실행 맵 (케이스 액터 하나) |
| `Source/RE4Inventory/Tests/*.cpp` | Automation 테스트 18개 (월드에 케이스를 띄워 격자↔3D 위치 대조 포함) |
| `ASSETS.md` | 에셋 출처와 이용 조건, 조사했지만 쓰지 않은 로컬 파일 |

## 실행과 조작

에디터에서 `L_AttacheCase`를 열고 플레이합니다. `StartingItems`가 비어 있으면 시연용 아이템 10종이 들어갑니다. Fab 에셋이 프로젝트에 있으면 그 모델을, 없으면 이 저장소의 독자 모델(`Content/Meshes/SM_Item_*`)을 씁니다.

| 입력 | 동작 |
|---|---|
| 좌클릭 | 집기 / 놓기 (놓을 수 없는 자리면 계속 들고 있음) |
| 우클릭, R | 집은 아이템 90도 회전 |
| C | 집기 취소 (원래 자리) |
| X | 집은 아이템 버리기 |
| Z | 되돌리기 |
| 1 / 2 | 자동 정리 (공간 우선 / 종류 우선) |
| E | 케이스 가로·세로 1칸 확장 (최대 13x9) |

## 원본 에셋 없이 실행하기

`UItemDefinition`의 `Mesh`가 비어 있으면 기본 큐브로 표시됩니다. 코드와 테스트는 어떤 모델에도 의존하지 않습니다.
기본 도형으로 임시 아이템을 만들 때는 `bStretchToCells`(칸 크기에 꽉 차게 늘림)와 `bOverrideColor`/`Color`를 씁니다.

## 로컬 모델 연결하기 (로컬 전용)

1. 모델은 `Content/LocalOnly/` 아래에 임포트합니다. 이 폴더는 `.gitignore`로 커밋에서 제외됩니다.
2. 아이템마다 `UItemDefinition` 데이터 에셋을 만들고 `Mesh`에 모델을 지정합니다.
3. 모델의 원래 크기·피벗과 상관없이 `bFitToCells`가 켜져 있으면 차지하는 칸 안에 자동으로 맞춰집니다.
   - `MeshRotation`: 회전 0일 때 모델의 긴 쪽이 칸의 가로(X)를 향하고 바닥에 눕도록 보정
   - `FitRatio`: 칸 영역 대비 크기 (기본 0.9)
   - `bFitToCells`를 끄면 `MeshScale` 배율을 그대로 씀
4. 아이템 메시는 충돌을 쓰지 않습니다. 마우스 선택은 마우스 광선과 케이스 바닥 평면의 교점으로 칸을 찾습니다(집은 뒤에도 바닥 평면 기준).

## 빌드와 테스트

```bash
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" RE4InventoryEditor Win64 Development "-Project=<경로>/RE4Inventory.uproject" -WaitMutex
"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<경로>/RE4Inventory.uproject" -ExecCmds="Automation RunTests RE4Inventory;Quit" -unattended -nullrhi
```

에디터가 켜져 있으면 라이브 코딩 때문에 명령줄 빌드가 막히므로 에디터를 끄고 빌드합니다.
