# 에셋 출처와 이용 조건

이 저장소에 들어가는 에셋과, 조사했지만 쓰지 않은 로컬 파일을 기록합니다.

## 사용 중 (저장소에 포함 가능)

| 에셋 | 출처 | 이용 조건 | 쓰는 곳 |
|---|---|---|---|
| `/Engine/BasicShapes/Cube`, `Cylinder`, `Sphere` | Unreal Engine 5.8 기본 엔진 콘텐츠 (Epic Games) | UE EULA. 언리얼 프로젝트 안에서 사용 가능, 엔진 콘텐츠 자체를 따로 재배포하지 않음 (저장소에는 경로만 있음) | 케이스 외형, 격자선, 칸 강조판, 시연용 아이템 9종 |
| `/Engine/BasicShapes/BasicShapeMaterial` | 위와 같음 | 위와 같음 | `Color` 파라미터로 색만 바꿔 씀 (머티리얼 인스턴스는 실행 중에 만듦) |
| `Content/Maps/L_AttacheCase.umap` | 엔진 템플릿 맵 `/Engine/Maps/Templates/Template_Default`를 복제하고 `CaseViewActor` 하나를 놓음 | UE EULA (엔진 템플릿 기반 프로젝트 콘텐츠) | 기본 실행 맵 |
| `Content/Meshes/SM_Item_*`(10종), `SM_Case_*`(5종) 과 같은 폴더의 슬롯 머티리얼 | 이 프로젝트에서 직접 만듦: `Tools/blender/make_items.py`가 Blender 5.2에서 절차 생성 → `ArtSource/Meshes/*.fbx` → `Tools/import_meshes.json`으로 임포트 | 프로젝트 자체 저작물 | 시연 아이템, 케이스 잠금쇠·손잡이·경첩·모서리 보호대·둥근 판 |
| `Content/Materials/M_CaseSurface` | 이 프로젝트에서 MCP 스크립트로 만듦 (Color, Roughness, Metallic, GrainAmount, GrainScale 파라미터, 월드 위치 노이즈 결) | 프로젝트 자체 저작물 | 모든 부품·아이템의 PBR 표면. 값은 `ACaseViewActor::SurfaceLooks` 표 |
| `Content/MaterialsLeather/` (Leather Material Pack) | Fab, JulioSillet, https://www.fab.com/listings/70847d34-920e-43f6-a4d7-65da0b5155b8 (2026-11-07 받음) | Fab Standard License, 개인·기업 0원 | 케이스 겉면 `MI_Leather_Worn_02` |
| `Content/MetalMat/` (Metal Material Pack PBR) | Fab, Stone Material, https://www.fab.com/listings/ec4b0dc8-37f4-41b9-913b-8cc84b4664bf (2026-11-07, UE 5.3판) | CC-BY: 크레딧에 "Metal Material Pack PBR by Stone Material" 표기 필요 | 잠금쇠·보호대 `MI_MetallMat_Inst9`, 강철 `Inst6` |
| `Content/FabricMat/` (Fabric Material Pack5) | Fab, Stone Material, https://www.fab.com/listings/b30a59a7-963d-4939-a3aa-b5fc7fdaeab1 (2026-11-07, UE 5.4판) | CC-BY: "Fabric Material Pack5 by Stone Material" 표기 필요 | 아직 안 씀 (어두운 폼에 맞는 천이 없음, 1.5GB) |
| `Content/HC_CannedFoodSampler/` | Fab, Horizon Crew, https://www.fab.com/listings/4a1f2a19-10e0-425d-8010-f6e3a41a6444 (2026-11-07) | Fab Standard License, 개인·기업 0원 | 아직 안 씀 (몸통과 뚜껑이 따로라 아이템 1메시 구조에 안 맞음) |
| `Content/Fab/M1911/` | Fab, Xeradev "M1911 (FREE)", https://www.fab.com/listings/77247d26-33a3-4dba-a840-7a7d8c92bb9c (2026-11-07). 원본 zip은 `C:/RE4Inventory_LocalAssets/fab/M1911`, `Tools/blender/prep_fab_m1911.py`로 눕히고 ORM을 나눠 `Tools/import_fab.json`으로 임포트 | Fab Standard License, 개인·기업 0원 | 시연 권총 (없으면 `SM_Item_Handgun`) |
| `Content/FPS_Weapon_Bundle/` | Fab, Deadghost Interactive "FPS Weapon Bundle", https://www.fab.com/listings/8aeb9c48-b404-4dcd-9e56-1d0ecedba7f5 (2026-11-08, UE 4.27판) | UE Marketplace License, 0원 | 시연 칼 `SM_M9_Knife`, 수류탄 `SM_G67` (없으면 독자 모델). 산탄총은 이 묶음에 없음 |
| `Content/Fab/Herbs/` | Fab, Quixel Megascans "Peppermint", https://www.fab.com/listings/f3cc150a-c728-4d4e-b6a8-d641b9f382b9 (2026-11-08, FBX Medium). 원본 zip은 `C:/RE4Inventory_LocalAssets/fab/Peppermint`, `Tools/blender/prep_fab_herb.py`로 변형 B·C를 묶고 높이를 눌러 BaseColor를 밝힘. 붉은 약초는 잎 색만 돌린 텍스처 | Fab Standard License, 개인·기업 0원 (수정 허용) | 시연 녹색·붉은 약초 (없으면 독자 모델) |
| `Content/Fab/Props/SM_Fab_Shotgun` | Fab, the 9th Earl "Model 12 Pump Action Shotgun Free Standard", https://www.fab.com/listings/c0761ef9-a19c-415b-a5c0-efe2ad238c4f (2026-11-08). 탄창 속 탄피(shell1~6)와 낱개 탄(shellshot*) 메시는 뺌. 나무 부분 색은 `Tools/mute_shotgun_wood.py`로 채도·밝기를 낮춤(`4k_Shotgun_colour_muted.png`). 원본은 `C:/RE4Inventory_LocalAssets/fab/<에셋>/src`, `Tools/blender/prep_fab_props.py`로 손질 후 `Tools/import_fab_props.json`으로 `/Game/Fab/Props`에 임포트 | Fab Standard License, 개인·기업 0원 | 시연 산탄총 |
| `Content/Fab/Props/SM_Fab_Shells` | 위 Model 12 에셋에 들어 있는 낱개 탄(`shellshot_2`) 하나를 2열 x 6줄로 복제해 묶음으로 만듦(`prep_fab_props.py`의 `grid`). 갈라진 몸통 끝은 길이 86%에서 잘라 막음(`trim`). 색은 나무 보정 전 원본 colour 텍스처에 탄 영역 UV 패딩만 넣은 것(`Tools/pad_shell_texture.py` → `4k_Shotgun_colour_shells.png`) | Fab Standard License (Model 12와 같음) | 시연 산탄총 탄 |
| `Content/Fab/Props/SM_Fab_AmmoCrate` | Fab, Ya106 "Box of cartridges (4k)", https://www.fab.com/listings/d2574944-951e-49ca-a953-ef302f667d5f (2026-11-08). 뚜껑 연 상자+탄만 사용 | CC-BY: "Box of cartridges (4k) by Ya106" 표기 필요 | 시연 권총 탄약 (1x2, Yaw 90) |
| `Content/Fab/Props/SM_Fab_SprayCan` | Fab, Houdini "Spray Paint Bottles", https://www.fab.com/listings/0bf007b5-9d74-4aaf-8cb4-71d4cd997c42 (2026-11-08). 깨끗한 캔 하나만 사용. 페인트 라벨은 `Tools/make_spray_label.py`로 직접 그린 구급 라벨(녹색 띠·녹색 십자)로 덮음(`spray_paint_bottles_diff_4k_medical.png`) | CC-BY: "Spray Paint Bottles by Houdini" 표기 필요. 라벨 텍스처를 수정했다고 같이 적을 것 | 시연 회복 스프레이 |
| `Content/Fab/Props/SM_Fab_Tin` | Fab, plaggy "CC0 - Tin Can Box", https://www.fab.com/listings/3288e4cc-a1d6-44ae-b441-33caa74c39aa (2026-11-08). 뚜껑을 닫아 얹고 높이 40%로 누름. Base/Top.png는 노멀 맵이라 색은 Albedo 사용 | Fab에는 CC-BY로 표시(제목은 CC0). 크레딧 "Tin Can Box by plaggy" 표기 | 시연 화약 |

## 원작(바이오하자드 RE:4) 에셋

- 쓰지 않음. 원작 설치 폴더(Steam)는 있으나 pak을 열거나 모델·텍스처를 추출·임포트하지 않았습니다. 원작 화면을 그 게임의 그림으로 그대로 복제하는 일이 되기 때문입니다.
- `C:/RE4Inventory_LocalAssets/tools`에는 RE 엔진 Blender 애드온 코드(GPL-3.0)와 경로·이름 목록만 받아 두었고, 게임 데이터는 없습니다.
- 시연용 아이템 이름(Shotgun, Handgun 등)과 칸 크기는 게임 분석용 일반 명칭과 수치이며 원작 데이터를 복사한 것이 아닙니다.
- 나중에 원작 모델을 개인 학습용으로 로컬에서 연결하려면 `Content/LocalOnly/` 아래에만 임포트합니다. 이 폴더는 `.gitignore`로 커밋에서 빠집니다.

### 외부 모델 구분 기준 (2026-11-08)

근거: 캡콤 2차 창작 가이드라인 (2025-11-04, https://www.capcom-games.com/ja-jp/fan-content-guidelines/). 비영리 개인의 팬 게임·입체 작품·영상은 이용자의 창작성이 더해진 경우 2차 창작으로 허용합니다. 캡콤 소재를 가공·전용하거나 그대로 쓰는 것, 데드카피, 로고 사용, 공식으로 오인될 표기(©CAPCOM 포함)는 금지합니다.

| 구분 | 예 | 판단 |
|---|---|---|
| 원작에서 추출한 모델·텍스처 | pak 추출, 추출본을 올린 Sketchfab 모델 | 가이드라인상 2차 창작이 아님(소재를 그대로 쓰거나 가공). 쓰지 않음 |
| 원작을 참고해 직접 만든 팬 모델 | Rucaido "Resident Evil 4 survival props" | 가이드라인 범위에 들 수 있음. 제작자의 이용 허락이나 라이선스가 확인될 때만 검토. 현재 이 모델은 다운로드 비활성, 라이선스 표기 없음 → 쓰지 않음 |
| 정식 라이선스 모델 | Fab Standard/CC-BY/CC0 | 각 라이선스 조건대로 사용 (위 표) |

공개할 때는 "이 작품은 2차 창작입니다" 같은 표기를 넣고 ©CAPCOM과 타이틀 로고는 쓰지 않습니다.

## 조사했지만 쓰지 않은 로컬 파일

| 파일 | 형식 | 임포트 가능 여부 | 쓰지 않은 이유 |
|---|---|---|---|
| `Downloads/BLEND/sample_ItemBox.blend` | Blender | UE는 `.blend`를 직접 못 읽음. Blender(5.1/5.2 설치됨)에서 FBX/glTF로 내보내면 가능 | 출처·이용 조건 기록 없음 |
| `Downloads/FBX/SM_Heart_001.fbx` | FBX | 바로 임포트 가능 | 출처 불명, 인벤토리와 무관 |
| `Downloads/이미지/strawberry_2.fbx` | FBX | 바로 임포트 가능 | 출처 불명, 인벤토리와 무관 |
| Starter Content 팩 | — | 이 엔진 설치에 `FeaturePacks/StarterContent.upack`이 없음 | 설치되어 있지 않음 |

새 외부 에셋을 쓰면 이 표에 출처 URL, 라이선스, 받은 날짜를 추가합니다.
