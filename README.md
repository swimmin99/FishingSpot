# 🎣 FishingSpot
> Advanced Unreal Engine 5.6 Multiplayer Fishing Simulation Project by Inyoung

FishingSpot은 **Unreal Engine 5.6** 기반의 **멀티플레이 낚시 시뮬레이션 게임**입니다.  
리슨 서버 기반의 네트워크 구조, 고급 인벤토리 시스템, 물리 기반 낚시 연출,  
그리고 모듈형 데이터 자산 시스템으로 설계된 포트폴리오 프로젝트입니다.

---

## 🧩 Features

### 🎮 Core Gameplay
- **Server-Authoritative Fishing System**
  - 물리 기반 `Bobber`와 `Fish AI`의 **진짜/가짜 입질 시스템**
  - `UFishingComponent` 기반 Bite Window 및 Bite Prediction 처리
  - `QueryWaterInfo`를 통한 수면 판정 및 캐스팅 충돌 처리

- **Dynamic Fish Spawn System**
  - `AFishSpawnPool`에서 **Niagara 기반 물결/효과와 함께 동적 생성**
  - 오브젝트 풀링(Object Pooling)으로 성능 최적화

- **Catch & Display System**
  - 플레이어가 낚은 물고기를 실시간으로 **들어올리고 보여주는 Show-off Sequence**
  - Skeletal → Static Mesh 변환으로 최적화된 표현

---

## 🧰 Systems

### 🧱 Inventory System
- `UInventoryComponent` & `UInventoryGridWidget`
  - **Grid 기반 Drag & Drop / Rotation 지원**
  - `ItemDataAsset`과 `ItemBase`를 통한 **데이터 주도형 설계**
  - 클라이언트-서버 간 `Replication` 및 `RPC` 처리 완비

### 💰 Shop System
- **상점 캐릭터(ShopCharacter)**와 상호작용 시 인벤토리 자동 연결
- `MenuManagerComponent`를 통한 **위젯 통합 제어**
- 구매/판매 트랜잭션 구조 및 HUD 연동

### 🌊 Environment
- Unreal **Water Plugin** 기반의 수면 시뮬레이션
- `EnterFishing_Server()`에서 **콜리전 기반 수면 감지 및 낚시 시작 위치 결정**
- 나이아가라 기반 Water Plugin 사용 및 나이아가라 파티클 시스템 컴포넌트로 Water Splash 구현

## 🧠 Architecture Overview
FishingSpot/
├─ Source/
│ ├─ Fishing/
│ │ ├─ Actor/
│ │ ├─ ActorComponent/
│ │ ├─ Data/
│ │ ├─ Widget/
│ │ ├─ System/
│ │ └─ Network/
├─ Content/
│ ├─ UI/
│ ├─ Fish/
│ ├─ Materials/
│ └─ Blueprints/
└─ Config/



---
