# FishingSpot
![1](https://github.com/user-attachments/assets/e1d0d531-a249-43d8-a837-f8afd23dce1b)

낚시를 주제로 한 사이드 프로젝트!
Unreal Engine 5.6 기반 멀티플레이 낚시 시뮬레이션 게임입니다.  
리슨 서버 구조에서 물리 기반 낚시 시스템과 Grid 인벤토리를 구현했습니다.


## 주요 구현

### 낚시 시스템
<details>
  <summary style="display: inline-block; padding: 8px 16px; font-weight: 600; color: #24292e; background-color: #f6f8fa; border: 1px solid #d1d5da; border-radius: 6px; cursor: pointer;">
    🎬 YouTube 영상 보기
  </summary>
  <p>

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/Xc8zvyYvkSs/0.jpg)](https://www.youtube.com/watch?v=Xc8zvyYvkSs)

  </p>
</details>

Server-Authoritative 방식으로 낚시 로직을 구현했습니다.
- Bobber가 물에 닿으면 Fish AI가 감지하고 입질 판정
- 진짜/가짜 입질을 구분하는 Bite Window 시스템
- 입질 타이밍 예측(Bite Prediction)으로 클라이언트 반응성 개선
- Water Plugin의 수면 정보를 쿼리해서 캐스팅 충돌 처리

- `FishingBobberModule`: Bobber 애니메이션, 움직임
- `FishingBiteModule`: 입질 판정, Bite Window
- `FishingCastModule`: 캐스팅 물리, 수면 감지

### Fish Spawn Pool
Niagara 기반 물결 효과와 함께 물고기를 동적으로 생성합니다.
- Object Pooling으로 성능 최적화
- 생성/파괴 대신 활성화/비활성화로 처리
- 각 Fish는 독립적인 AI 상태를 가지고 Idle/Approach/Bite 상태 전환

### 잡은 물고기 연출
플레이어가 물고기를 낚으면 실시간으로 들어올리는 Show-off Sequence가 실행됩니다.
- Skeletal Mesh를 Static Mesh로 변환해서 최적화
- 카메라 앞에서 물고기를 보여주는 연출
- 이후 인벤토리에 자동 저장

## 인벤토리 시스템
Rotation과 공간 배치를 지원하는 Grid 시스템으로 발전시켰습니다.

**구조**
- `InventoryComponent`: 서버 권한, 아이템 검증
- `InventoryGridWidget`: 클라이언트 UI, Drag & Drop
- `ItemBase`: 아이템 데이터와 회전 상태
- `ItemDataAsset`: 데이터 주도 설계

## 상점 시스템

ShopCharacter와 상호작용하면 상점 UI가 열립니다.
- MenuManagerComponent가 여러 위젯을 통합 관리
- 구매/판매 트랜잭션은 서버에서 검증
- 플레이어 인벤토리와 자동 연동

## Water System

Unreal Water Plugin 기반입니다.
- 수면 충돌 감지는 `QueryWaterInfo()`로 처리
- Bobber가 수면에 닿으면 Niagara Particle로 Splash 효과
- `EnterFishing_Server()`에서 낚시 가능 여부 판정

## 프로젝트 구조

```
FishingSpot/
├─ Source/Fishing/
│  ├─ Actor/           # Bobber, Fish, ItemBase
│  ├─ ActorComponent/  # Fishing, Inventory, MenuManager
│  │  └─ SubModules/   # Fishing 서브모듈들
│  ├─ Data/            # DataAsset, Structs
│  ├─ Widget/          # Inventory, Shop UI
│  └─ System/          # Network, GameMode
├─ Content/
│  ├─ UI/
│  ├─ Fish/
│  └─ Materials/
└─ Config/
```

## 기술 스택
- Unreal Engine 5.6
- C++ (Server Logic)
- Blueprint (UI, Visual Effects)
- Replication & RPC
- Niagara VFX
- Water Plugin

## 개발 과정에서 배운 점

1. **모듈화의 중요성**: 하나의 거대한 Component보다 작은 SubModule로 나누면 유지보수가 훨씬 쉬워집니다.
2. **Server Authority**: 멀티플레이에서 치팅 방지를 위해 중요한 로직은 반드시 서버에서 처리해야 합니다.
3. **Single Source of Truth**: 같은 데이터를 여러 곳에서 관리하면 반드시 동기화 문제가 생깁니다.
4. **프리뷰와 실제**: UI 프리뷰와 실제 게임 로직이 다를 경우 최종적으로 서버 검증이 필요합니다.
