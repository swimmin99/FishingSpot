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


<img width="1089" height="545" alt="스크린샷 2025-10-20 193847" src="https://github.com/user-attachments/assets/c7c9e776-4645-41e2-a5d2-adf98c9d791d" />

### Fish Spawn Pool
Niagara 기반 물결 효과와 함께 물고기를 동적으로 생성합니다.
- Object Pooling으로 성능 최적화
- 생성/파괴 대신 활성화/비활성화로 처리
- 각 Fish는 독립적인 AI 상태를 가지고 Idle/Approach/Bite 상태 전환

<img width="1114" height="544" alt="스크린샷 2025-10-20 193933" src="https://github.com/user-attachments/assets/75cc3513-f205-4853-ae61-074202d1af72" />

## 인벤토리 시스템
Rotation과 공간 배치를 지원하는 Grid 시스템으로 발전시켰습니다.

**구조**
- `InventoryComponent`: 서버 권한, 아이템 검증
- `InventoryGridWidget`: 클라이언트 UI, Drag & Drop
- `ItemBase`: 아이템 데이터와 회전 상태
- MenuManagerComponent가 여러 위젯을 통합 관리
- 구매/판매 트랜잭션은 서버에서 검증
- 플레이어 인벤토리와 자동 연동

## 로컬 DB 기반 저장 시스템
<img width="1280" height="686" alt="스크린샷 2025-10-20 194009" src="https://github.com/user-attachments/assets/f49cc097-684f-4ec0-8573-fa15686269f7" />

- `SQLite 기반 DB`: 진행 사항 데이터를 SQLite에 저장했습니다.
- Unreal의 Build.cs 모듈에 SQLite 종속성을 추가하고 관련 API를 활용하여 정보를 보관하고 쿼리하는 방식으로 구현하였습니다.

Players (PlayerID PK)
  ├──< InventoryItems (PlayerID FK)
  └──< FishRecords   (PlayerID FK)

게스트: Players.HostPlayerID = 호스트의 PlayerID, IsHost=0
호스트: Players.IsHost=1, HostPlayerID=NULL

<img width="1270" height="700" alt="스크린샷 2025-10-20 193957" src="https://github.com/user-attachments/assets/05924734-db83-4c20-8df5-8dc93cd32d0d" />


## 프로젝트 구조

```
FishingSpot/
├─ Source/Fishing/
│  ├─ Actor/           # Bobber, Fish, ItemBase
│  ├─ ActorComponent/  # Fishing, Inventory, MenuManager
│  │  └─ SubModules/   # Fishing 서브모듈들
│  ├─ Data/            # DataAsset, Structs
│  ├─ Database/        # DBManager
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
- Replication & RPC
- SQLite

## 개발 과정에서 배운 점

1. **모듈화의 중요성**: 하나의 거대한 Component보다 작은 SubModule로 나누면 유지보수가 훨씬 쉽다
2. **Server Authority**: 멀티플레이에서 치팅 방지를 위해 중요한 로직은 반드시 서버에서 처리해야 한다
3. **DB로 세이브 구현 가능**: 로컬 게임이더라도 DB를 사용하면 비교적 Data 트랜잭션 모니터링, 쿼리 등의 장점을 가진다.
