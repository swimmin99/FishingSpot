
// ============================================
// InventoryComponent.cpp (REFACTORED)
// ============================================
#include "InventoryComponent.h"

#include "Fishing.h"
#include "SubModules/InventoryGridManager.h"
#include "SubModules/InventoryStorage.h"
#include "SubModules/InventoryPlacementValidator.h"
#include "SubModules/InventoryItemHandler.h"
#include "SubModules/InventoryUIManager.h"
#include "SubModules/InventoryNetworkSync.h"
#include "FishingCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(true);
}


void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // 모듈 초기화
    InitializeModules();
    
    Activate(true);
    SetComponentTickEnabled(true);
    PrimaryComponentTick.SetTickFunctionEnable(true);
    
    const int32 Expected = Columns * Rows;
    if (Storage && Storage->GetItemCount() != Expected)
    {
        UE_LOG(LogInventory, Warning, TEXT("BeginPlay: Storage resized from %d to %d"),
               Storage->GetItemCount(), Expected);
        Storage->ResizeStorage(true);
    }
    
    UE_LOG(LogInventory, Log, TEXT("BeginPlay: InventoryComponent ready (Role=%d)"), 
           static_cast<int32>(GetOwnerRole()));
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogInventory, Log, TEXT("EndPlay: Reason=%d"), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::InitializeModules()
{
    GridManager = NewObject<UInventoryGridManager>(this, TEXT("GridManager"));
    Storage = NewObject<UInventoryStorage>(this, TEXT("Storage"));
    Validator = NewObject<UInventoryPlacementValidator>(this, TEXT("Validator"));
    ItemHandler = NewObject<UInventoryItemHandler>(this, TEXT("ItemHandler"));
    UIManager = NewObject<UInventoryUIManager>(this, TEXT("UIManager"));
    NetworkSync = NewObject<UInventoryNetworkSync>(this, TEXT("NetworkSync"));
    
    if (GridManager)
    {
        GridManager->Initialize(Columns, Rows, TileSize);
    }
    
    if (Storage && GridManager)
    {
        Storage->Initialize(GridManager, this);
    }
    
    if (Validator && GridManager && Storage)
    {
        Validator->Initialize(GridManager, Storage);
    }
    
    if (ItemHandler && GridManager && Storage && Validator)
    {
        ItemHandler->Initialize(GridManager, Storage, Validator);
    }
    
    if (NetworkSync && Storage && GridManager)
    {
        NetworkSync->Initialize(this, Storage, GridManager);
    }
    
    UE_LOG(LogInventory, Log, TEXT("InitializeModules: All modules initialized"));
}

void UInventoryComponent::Initalize(AActor* Owner)
{
    CharacterReference = Cast<AFishingCharacter>(Owner);
    
    UE_LOG(LogInventory, Log, TEXT("Initalize: Owner=%s, CharacterReference=%s"),
           Owner ? *Owner->GetName() : TEXT("None"),
           CharacterReference ? *CharacterReference->GetName() : TEXT("None"));
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UInventoryComponent, Columns);
    DOREPLIFETIME(UInventoryComponent, Rows);
    DOREPLIFETIME(UInventoryComponent, TileSize);
    
    // ★★★ 동기화 데이터 레플리케이션 ★★★
    DOREPLIFETIME(UInventoryComponent, ItemSyncData);
}

// ★★★ 클라이언트에서 자동 호출되는 레플리케이션 콜백 ★★★
void UInventoryComponent::OnRep_ItemSyncData()
{
    UE_LOG(LogInventory, Log, TEXT("OnRep_ItemSyncData: Received %d items (Role=%d)"),
           ItemSyncData.Num(), static_cast<int32>(GetOwnerRole()));
    
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("OnRep_ItemSyncData: Storage is null!"));
        return;
    }
    
    // Storage에 동기화 데이터 적용
    Storage->ApplySyncData(ItemSyncData);
    
    // UI 갱신
    RefreshAllItems();
    RefreshGridLayout();
    
    UE_LOG(LogInventory, Log, TEXT("OnRep_ItemSyncData: Sync complete"));
}

// ★★★ 서버에서 호출: 클라이언트에 동기화 ★★★
void UInventoryComponent::SyncToClients()
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogInventory, Warning, TEXT("SyncToClients: Called on non-authority!"));
        return;
    }
    
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("SyncToClients: Storage is null!"));
        return;
    }
    
    // Storage로부터 동기화 데이터 생성
    ItemSyncData = Storage->GenerateSyncData();
    
    UE_LOG(LogInventory, Log, TEXT("SyncToClients: Updated ItemSyncData with %d entries"), 
           ItemSyncData.Num());
    
    // ItemSyncData가 Replicated이므로 자동으로 모든 클라이언트에 전송됨!
    // 클라이언트에서 OnRep_ItemSyncData()가 자동 호출됨!
}





// ============================================
// Widget Management (Delegates to UIManager)
// ============================================

void UInventoryComponent::RegisterInventoryWidget(UInventoryWidget* InInventoryWidget)
{
    if (!UIManager)
    {
        UE_LOG(LogInventory, Error, TEXT("RegisterInventoryWidget: UIManager is null!"));
        return;
    }
    
    UIManager->RegisterWidget(InInventoryWidget);
}

void UInventoryComponent::UpdateDescription(UItemBase* Item)
{
    if (!UIManager)
    {
        UE_LOG(LogInventory, Warning, TEXT("UpdateDescription: UIManager is null!"));
        return;
    }
    
    UIManager->UpdateDescription(Item);
}

void UInventoryComponent::ClearDescription()
{
    if (!UIManager)
    {
        return;
    }
    
    UIManager->ClearDescription();
}

void UInventoryComponent::SetFocusGridWidget()
{
    if (!UIManager)
    {
        return;
    }
    
    UIManager->SetFocusGridWidget();
}

void UInventoryComponent::ClearInventoryWidget()
{
    if (!UIManager)
    {
        return;
    }
    
    UIManager->UnregisterWidget();
}

void UInventoryComponent::RefreshGridLayout()
{
    if (!UIManager)
    {
        UE_LOG(LogInventory, Warning, TEXT("RefreshGridLayout: UIManager is null!"));
        return;
    }
    
    UIManager->RefreshGrid();
}

// ============================================
// Grid Operations (Delegates to GridManager)
// ============================================

FIntPoint UInventoryComponent::IndexToTile(int32 Index) const
{
    if (!GridManager)
    {
        UE_LOG(LogInventory, Error, TEXT("IndexToTile: GridManager is null!"));
        return FIntPoint::ZeroValue;
    }
    
    return GridManager->IndexToTile(Index);
}

int32 UInventoryComponent::TileToIndex(FIntPoint Tile) const
{
    if (!GridManager)
    {
        UE_LOG(LogInventory, Error, TEXT("TileToIndex: GridManager is null!"));
        return -1;
    }
    
    return GridManager->TileToIndex(Tile);
}

bool UInventoryComponent::IsTileValid(FIntPoint IntPoint)
{
    if (!GridManager)
    {
        return false;
    }
    
    return GridManager->IsTileValid(IntPoint);
}

FString UInventoryComponent::DumpGrid() const
{
    if (!Storage)
    {
        return TEXT("Storage not initialized");
    }
    
    return Storage->DumpStorageContents();
}

// ============================================
// Storage Access (Delegates to Storage)
// ============================================

UItemBase* UInventoryComponent::GetItemAtIndex(int32 Index)
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("GetItemAtIndex: Storage is null!"));
        return nullptr;
    }
    
    return Storage->GetItemAtIndex(Index);
}

TMap<UItemBase*, FIntPoint> UInventoryComponent::GetAllItems()
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("GetAllItems: Storage is null!"));
        return TMap<UItemBase*, FIntPoint>();
    }
    
    return Storage->GetAllUniqueItems();
}

void UInventoryComponent::RefreshAllItems()
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("RefreshAllItems: Storage is null!"));
        return;
    }
    
    Storage->RefreshUniqueItemsCache();
}

// ============================================
// Validation (Delegates to Validator)
// ============================================

bool UInventoryComponent::IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, 
                                            UItemBase* IgnoreItem)
{
    if (!Validator)
    {
        UE_LOG(LogInventory, Error, TEXT("IsRoomAvailableAt: Validator is null!"));
        return false;
    }
    
    return Validator->CanPlaceItemAt(ItemToPlace, TopLeftTile, IgnoreItem);
}

bool UInventoryComponent::IsRoomAvailable(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
    if (!Validator)
    {
        UE_LOG(LogInventory, Error, TEXT("IsRoomAvailable: Validator is null!"));
        return false;
    }
    
    return Validator->CanPlaceItemAt(ItemToAdd, TopLeftIndex, nullptr);
}

// ============================================
// Item Operations (Delegates to ItemHandler)
// ============================================

bool UInventoryComponent::TryAddItem(UItemBase* ItemToAdd)
{
    if (!ItemToAdd || !ItemHandler)
    {
        return false;
    }
    
    int32 TopLeftIndex = -1;
    const bool bSuccess = ItemHandler->TryAddItem(ItemToAdd, TopLeftIndex);
    
    if (bSuccess)
    {
        // ★★★ 서버에서만 동기화 ★★★
        if (GetOwnerRole() == ROLE_Authority)
        {
            SyncToClients();
        }
        
        NotifyItemsChanged();
        
        UE_LOG(LogInventory, Log, TEXT("TryAddItem: Success (Role=%d)"), 
               static_cast<int32>(GetOwnerRole()));
    }
    
    return bSuccess;
}

bool UInventoryComponent::TryAddItemFroActor(AItemActor* ItemToAdd)
{
    if (!ItemToAdd)
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddItemFroActor: ItemToAdd is null"));
        return false;
    }
    
    UItemBase* NewItem = ItemToAdd->MakeItemInstance(this, CharacterReference);
    if (!NewItem)
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddItemFroActor: Failed to create UItemBase"));
        return false;
    }
    
    return TryAddItem(NewItem);
}


bool UInventoryComponent::RemoveItem(UItemBase* ItemToRemove)
{
    if (!ItemToRemove || !ItemHandler)
    {
        return false;
    }
    
    const bool bSuccess = ItemHandler->RemoveItem(ItemToRemove);
    
    if (bSuccess)
    {
        // ★★★ 서버에서만 동기화 ★★★
        if (GetOwnerRole() == ROLE_Authority)
        {
            SyncToClients();
        }
        
        NotifyItemsChanged();
        
        UE_LOG(LogInventory, Log, TEXT("RemoveItem: Success (Role=%d)"), 
               static_cast<int32>(GetOwnerRole()));
    }
    
    return bSuccess;
}


void UInventoryComponent::AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
    if (!ItemHandler)
    {
        return;
    }
    
    if (ItemHandler->AddItemAt(ItemToAdd, TopLeftIndex))
    {
        // ★★★ 서버에서만 동기화 ★★★
        if (GetOwnerRole() == ROLE_Authority)
        {
            SyncToClients();
        }
        
        NotifyItemsChanged();
    }
}
void UInventoryComponent::NotifyItemsChanged()
{
    RefreshAllItems();
    RefreshGridLayout();
}

// ============================================
// Network RPCs - Server
// ============================================

void UInventoryComponent::Server_MoveItem_Implementation(UItemBase* Item, FIntPoint NewTopLeftTile)
{
    if (!Item || !Validator || !ItemHandler || !GridManager)
    {
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Moving %s to (%d,%d)"),
           *Item->GetName(), NewTopLeftTile.X, NewTopLeftTile.Y);
    
    if (!Validator->CanPlaceItemAt(Item, NewTopLeftTile, Item))
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_MoveItem: Cannot place"));
        
        // 실패 시 클라이언트에게 알림 (레플리케이션이 원래 상태 복원)
        int32 OriginalIndex = -1;
        if (NetworkSync && NetworkSync->FindItemIndex(Item, OriginalIndex))
        {
            Client_RejectItemPlacement(Item->ItemGuid, OriginalIndex);
        }
        return;
    }
    
    if (ItemHandler->MoveItem(Item, NewTopLeftTile))
    {
        // ★★★ 동기화 ★★★
        SyncToClients();
        
        UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Success, synced to clients"));
    }
}

void UInventoryComponent::Server_DropItemToWorld_Implementation(UItemBase* ItemToDrop)
{
    if (!ItemToDrop || !CharacterReference || !ItemHandler || !NetworkSync)
    {
        return;
    }
    
    int32 ItemIndex = -1;
    if (!NetworkSync->FindItemIndex(ItemToDrop, ItemIndex))
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Item not found"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Dropping %s"), *ItemToDrop->GetName());
    
    const FVector SpawnLoc = CharacterReference->GetActorLocation() +
        CharacterReference->GetActorForwardVector() * 200.f;
    const FRotator SpawnRot = CharacterReference->GetActorRotation();
    const FTransform SpawnTransform(SpawnRot, SpawnLoc);
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    SpawnParams.Owner = CharacterReference;
    
    AItemActor* SpawnedActor = ItemToDrop->SpawnItemActor(
        GetWorld(),
        SpawnTransform,
        ItemActorSubClass,
        CharacterReference
    );
    
    if (!SpawnedActor)
    {
        UE_LOG(LogInventory, Error, TEXT("Server_DropItemToWorld: Failed to spawn"));
        return;
    }
    
    if (ItemHandler->RemoveItem(ItemToDrop))
    {
        // ★★★ 동기화 ★★★
        SyncToClients();
        
        UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Complete, synced"));
    }
    else
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Failed to remove, rolling back"));
        SpawnedActor->Destroy();
    }
}

void UInventoryComponent::RemoveItemForServer_Implementation(UItemBase* ItemToRemove)
{
    if (!ItemToRemove)
    {
        return;
    }
    
    if (RemoveItem(ItemToRemove))
    {
        // RemoveItem에서 이미 SyncToClients() 호출됨
        UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Success"));
    }
}

void UInventoryComponent::TryAddForServer_Implementation(UItemBase* ItemToAdd)
{
    if (!TryAddItem(ItemToAdd))
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddForServer: Failed"));
    }
    // TryAddItem에서 이미 SyncToClients() 호출됨
}

void UInventoryComponent::AddItemAtForServer_Implementation(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
    if (!ItemToAdd || !GridManager || !Validator || !ItemHandler)
    {
        return;
    }
    
    if (!GridManager->IsIndexValid(TopLeftIndex))
    {
        UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Invalid index %d"), TopLeftIndex);
        return;
    }
    
    const FIntPoint TopLeftTile = GridManager->IndexToTile(TopLeftIndex);
    
    if (!Validator->CanPlaceItemAt(ItemToAdd, TopLeftTile, nullptr))
    {
        UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Cannot place"));
        return;
    }
    
    if (ItemHandler->AddItemAt(ItemToAdd, TopLeftIndex))
    {
        // ★★★ 동기화 ★★★
        SyncToClients();
        
        UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Success, synced"));
    }
}

// ============================================
// Network RPCs - Client
// ============================================

void UInventoryComponent::Client_SyncItem_Implementation(UItemDataAsset* ItemDef, bool bIsRotated,
                                                        FGuid ItemGuid, int32 TopLeftIndex)
{
    // OnRep_ItemSyncData가 자동으로 처리하므로 비어있어도 됨
    UE_LOG(LogInventory, Verbose, TEXT("Client_SyncItem: Handled by OnRep_ItemSyncData"));
}

void UInventoryComponent::Client_RemoveItem_Implementation(FGuid ItemGuid)
{
    // OnRep_ItemSyncData가 자동으로 처리
    UE_LOG(LogInventory, Verbose, TEXT("Client_RemoveItem: Handled by OnRep_ItemSyncData"));
}

void UInventoryComponent::Client_RejectItemPlacement_Implementation(FGuid ItemGuid, int32 OriginalIndex)
{
    // OnRep_ItemSyncData가 원래 상태로 복원
    UE_LOG(LogInventory, Log, TEXT("Client_RejectItemPlacement: Server rejected, will sync via OnRep"));
}

void UInventoryComponent::RefreshLayoutForClient_Implementation()
{
    UE_LOG(LogInventory, Log, TEXT("RefreshLayoutForClient: Remote client refreshing"));
    
    RefreshAllItems();
    RefreshGridLayout();
}

void UInventoryComponent::Client_SyncItemClass_Implementation(TSubclassOf<AItemActor> ItemClass, 
                                                              int32 TopLeftIndex)
{
    if (!ItemClass || !ItemHandler || !GridManager)
    {
        return;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetOwner();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    AItemActor* Item = GetWorld()->SpawnActor<AItemActor>(
        ItemClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );
    
    if (Item)
    {
        UItemBase* NewItem = Item->MakeItemInstance(this, CharacterReference);
        if (NewItem)
        {
            ItemHandler->PlaceItemInGrid(NewItem, TopLeftIndex);
            RefreshAllItems();
            RefreshGridLayout();
        }
        
        Item->SetActorHiddenInGame(true);
        Item->SetActorEnableCollision(false);
    }
}

// ============================================
// Helper Functions
// ============================================

void UInventoryComponent::ResizeItemsToGrid(bool bZeroInit)
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("ResizeItemsToGrid: Storage is null!"));
        return;
    }
    
    Storage->ResizeStorage(bZeroInit);
}

bool UInventoryComponent::GetResultAtIndex(int32 Index)
{
    if (!GridManager)
    {
        return false;
    }
    
    return GridManager->IsIndexValid(Index);
}

bool UInventoryPlacementValidator::CheckTileOccupancy(FIntPoint Tile, UItemBase* IgnoreItem) const
{
    if (!GridManager || !Storage)
    {
        return false;
    }
    
    if (!GridManager->IsTileValid(Tile))
    {
        return false;
    }
    
    const int32 Index = GridManager->TileToIndex(Tile);
    UItemBase* Occupant = Storage->GetItemAtIndex(Index);
    
    return (Occupant == nullptr || Occupant == IgnoreItem);
}

bool UInventoryPlacementValidator::CheckBoundsForItem(UItemBase* Item, FIntPoint TopLeftTile) const
{
    if (!Item || !GridManager)
    {
        return false;
    }
    
    const FIntPoint Dims = Item->GetCurrentDimensions();
    const int32 Cols = GridManager->GetColumns();
    const int32 Rows = GridManager->GetRows();
    
    if (TopLeftTile.X < 0 || TopLeftTile.Y < 0)
    {
        return false;
    }
    
    if (TopLeftTile.X + Dims.X > Cols || TopLeftTile.Y + Dims.Y > Rows)
    {
        return false;
    }
    
    return true;
}