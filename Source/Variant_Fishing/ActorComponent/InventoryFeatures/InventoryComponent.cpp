
// ============================================
// InventoryComponent.cpp (REFACTORED)
// ============================================
#include "InventoryComponent.h"
#include "SubModules/InventoryGridManager.h"
#include "SubModules/InventoryStorage.h"
#include "SubModules/InventoryPlacementValidator.h"
#include "SubModules/InventoryItemHandler.h"
#include "SubModules/InventoryUIManager.h"
#include "SubModules/InventoryNetworkSync.h"
#include "././Fishing.h"
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
    
    // Initialize all modules
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
    
    UE_LOG(LogInventory, Log, TEXT("BeginPlay: InventoryComponent ready\n%s"), *DumpGrid());
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogInventory, Log, TEXT("EndPlay: Reason=%d"), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::InitializeModules()
{
    // Create all module instances
    GridManager = NewObject<UInventoryGridManager>(this, TEXT("GridManager"));
    Storage = NewObject<UInventoryStorage>(this, TEXT("Storage"));
    Validator = NewObject<UInventoryPlacementValidator>(this, TEXT("Validator"));
    ItemHandler = NewObject<UInventoryItemHandler>(this, TEXT("ItemHandler"));
    UIManager = NewObject<UInventoryUIManager>(this, TEXT("UIManager"));
    NetworkSync = NewObject<UInventoryNetworkSync>(this, TEXT("NetworkSync"));
    
    // Initialize modules with dependencies
    if (GridManager)
    {
        GridManager->Initialize(Columns, Rows, TileSize);
    }
    
    if (Storage && GridManager)
    {
        Storage->Initialize(GridManager);
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
    if (!ItemToAdd)
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddItem: ItemToAdd is null"));
        return false;
    }
    
    if (!ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("TryAddItem: ItemHandler is null!"));
        return false;
    }
    
    int32 TopLeftIndex = -1;
    const bool bSuccess = ItemHandler->TryAddItem(ItemToAdd, TopLeftIndex);
    
    if (bSuccess && GetOwnerRole() == ROLE_Authority)
    {
        NotifyItemsChanged();
        Client_SyncItem(ItemToAdd->ItemDef, ItemToAdd->bIsRotated, ItemToAdd->ItemGuid, TopLeftIndex);
        
        UE_LOG(LogInventory, Log, TEXT("TryAddItem: Added and synced to clients"));
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
    if (!ItemToRemove)
    {
        UE_LOG(LogInventory, Warning, TEXT("RemoveItem: ItemToRemove is null"));
        return false;
    }
    
    if (!ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("RemoveItem: ItemHandler is null!"));
        return false;
    }
    
    const FGuid RemovedGuid = ItemToRemove->ItemGuid;
    const bool bSuccess = ItemHandler->RemoveItem(ItemToRemove);
    
    if (bSuccess && GetOwnerRole() == ROLE_Authority)
    {
        NotifyItemsChanged();
        Client_RemoveItem(RemovedGuid);
        
        UE_LOG(LogInventory, Log, TEXT("RemoveItem: Removed and synced to clients"));
    }
    
    return bSuccess;
}

void UInventoryComponent::AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
    if (!ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("AddItemAt: ItemHandler is null!"));
        return;
    }
    
    ItemHandler->AddItemAt(ItemToAdd, TopLeftIndex);
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
    if (!Item)
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_MoveItem: Item is null"));
        return;
    }
    
    if (!Validator || !ItemHandler || !GridManager)
    {
        UE_LOG(LogInventory, Error, TEXT("Server_MoveItem: Modules not initialized!"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Moving %s to (%d,%d)"),
           *Item->GetName(), NewTopLeftTile.X, NewTopLeftTile.Y);
    
    // Validate placement
    if (!Validator->CanPlaceItemAt(Item, NewTopLeftTile, Item))
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_MoveItem: Cannot place at (%d,%d)"),
               NewTopLeftTile.X, NewTopLeftTile.Y);
        
        // Find original position and reject
        int32 OriginalIndex = -1;
        if (NetworkSync && NetworkSync->FindItemIndex(Item, OriginalIndex))
        {
            Client_RejectItemPlacement(Item->ItemGuid, OriginalIndex);
        }
        return;
    }
    
    // Execute move
    if (ItemHandler->MoveItem(Item, NewTopLeftTile))
    {
        const int32 NewIndex = GridManager->TileToIndex(NewTopLeftTile);
        
        NotifyItemsChanged();
        Client_SyncItem(Item->ItemDef, Item->bIsRotated, Item->ItemGuid, NewIndex);
        
        UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Successfully moved to index %d"), NewIndex);
    }
}

void UInventoryComponent::Server_DropItemToWorld_Implementation(UItemBase* ItemToDrop)
{
    if (!ItemToDrop || !CharacterReference)
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Invalid item or character"));
        return;
    }
    
    if (!ItemHandler || !NetworkSync)
    {
        UE_LOG(LogInventory, Error, TEXT("Server_DropItemToWorld: Modules not initialized!"));
        return;
    }
    
    // Verify item exists
    int32 ItemIndex = -1;
    if (!NetworkSync->FindItemIndex(ItemToDrop, ItemIndex))
    {
        UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Item not found in inventory"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Dropping %s"), *ItemToDrop->GetName());
    
    // Calculate spawn location
    const FVector SpawnLoc = CharacterReference->GetActorLocation() +
        CharacterReference->GetActorForwardVector() * 200.f;
    const FRotator SpawnRot = CharacterReference->GetActorRotation();
    const FTransform SpawnTransform(SpawnRot, SpawnLoc);
    
    // Spawn actor
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
        UE_LOG(LogInventory, Error, TEXT("Server_DropItemToWorld: Failed to spawn ItemActor"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Spawned at %s"), *SpawnLoc.ToString());
    
    // Remove from inventory
    const FGuid RemovedGuid = ItemToDrop->ItemGuid;
    if (ItemHandler->RemoveItem(ItemToDrop))
    {
        NotifyItemsChanged();
        Client_RemoveItem(RemovedGuid);
        
        UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Complete"));
    }
    else
    {
        // Rollback: destroy spawned actor
        UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Failed to remove, rolling back"));
        SpawnedActor->Destroy();
    }
}

void UInventoryComponent::RemoveItemForServer_Implementation(UItemBase* ItemToRemove)
{
    if (!ItemToRemove)
    {
        UE_LOG(LogInventory, Warning, TEXT("RemoveItemForServer: ItemToRemove is null"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Removing %s"), *ItemToRemove->GetName());
    
    if (RemoveItem(ItemToRemove))
    {
        UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Successfully removed"));
    }
    else
    {
        UE_LOG(LogInventory, Warning, TEXT("RemoveItemForServer: Item not found"));
    }
}

void UInventoryComponent::TryAddForServer_Implementation(UItemBase* ItemToAdd)
{
    if (!TryAddItem(ItemToAdd))
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddForServer: Failed to add item"));
    }
}

void UInventoryComponent::AddItemAtForServer_Implementation(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
    if (!ItemToAdd)
    {
        UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: ItemToAdd is null"));
        return;
    }
    
    if (!GridManager || !Validator || !ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("AddItemAtForServer: Modules not initialized!"));
        return;
    }
    
    if (!GridManager->IsIndexValid(TopLeftIndex))
    {
        UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Invalid TopLeftIndex=%d"), TopLeftIndex);
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Adding %s at index %d"),
           *ItemToAdd->GetName(), TopLeftIndex);
    
    const FIntPoint TopLeftTile = GridManager->IndexToTile(TopLeftIndex);
    
    // Validate placement
    if (!Validator->CanPlaceItemAt(ItemToAdd, TopLeftTile, nullptr))
    {
        UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Cannot place at index %d"), TopLeftIndex);
        return;
    }
    
    // Place item
    if (ItemHandler->AddItemAt(ItemToAdd, TopLeftIndex))
    {
        NotifyItemsChanged();
        Client_SyncItem(ItemToAdd->ItemDef, ItemToAdd->bIsRotated, ItemToAdd->ItemGuid, TopLeftIndex);
        
        UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Successfully added"));
    }
}

// ============================================
// Network RPCs - Client
// ============================================

void UInventoryComponent::Client_SyncItem_Implementation(UItemDataAsset* ItemDef, bool bIsRotated,
                                                        FGuid ItemGuid, int32 TopLeftIndex)
{
    if (!ItemDef)
    {
        UE_LOG(LogInventory, Warning, TEXT("Client_SyncItem: ItemDef is null"));
        return;
    }
    
    if (!GridManager || !Storage || !ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_SyncItem: Modules not initialized!"));
        return;
    }
    
    if (!GridManager->IsIndexValid(TopLeftIndex))
    {
        UE_LOG(LogInventory, Warning, TEXT("Client_SyncItem: Invalid index %d"), TopLeftIndex);
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Client_SyncItem: Syncing item %s at index %d"),
           *ItemDef->DisplayName.ToString(), TopLeftIndex);
    
    // Create new item instance
    UItemBase* NewItem = NewObject<UItemBase>(this, UItemBase::StaticClass());
    if (!NewItem)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_SyncItem: Failed to create UItemBase"));
        return;
    }
    
    NewItem->Initialize(ItemDef);
    NewItem->bIsRotated = bIsRotated;
    NewItem->ItemGuid = ItemGuid;
    
    // Clear any existing instances with this GUID
    const int32 TotalTiles = GridManager->GetTotalTiles();
    for (int32 i = 0; i < TotalTiles; i++)
    {
        UItemBase* ExistingItem = Storage->GetItemAtIndex(i);
        if (ExistingItem && ExistingItem->ItemGuid == ItemGuid)
        {
            Storage->ClearItemAtIndex(i);
        }
    }
    
    // Place item
    ItemHandler->PlaceItemInGrid(NewItem, TopLeftIndex);
    
    // Refresh UI
    RefreshAllItems();
    RefreshGridLayout();
}

void UInventoryComponent::Client_RemoveItem_Implementation(FGuid ItemGuid)
{
    if (!GridManager || !Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_RemoveItem: Modules not initialized!"));
        return;
    }
    
    UE_LOG(LogInventory, Log, TEXT("Client_RemoveItem: Removing item with GUID"));
    
    bool bFound = false;
    const int32 TotalTiles = GridManager->GetTotalTiles();
    
    for (int32 i = 0; i < TotalTiles; i++)
    {
        UItemBase* Item = Storage->GetItemAtIndex(i);
        if (Item && Item->ItemGuid == ItemGuid)
        {
            Storage->ClearItemAtIndex(i);
            bFound = true;
        }
    }
    
    if (!bFound)
    {
        UE_LOG(LogInventory, Warning, TEXT("Client_RemoveItem: Item not found"));
    }
    
    // Refresh UI
    RefreshAllItems();
    RefreshGridLayout();
}

void UInventoryComponent::Client_RejectItemPlacement_Implementation(FGuid ItemGuid, int32 OriginalIndex)
{
    if (!GridManager || !Storage || !ItemHandler)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_RejectItemPlacement: Modules not initialized!"));
        return;
    }
    
    UE_LOG(LogInventory, Warning, TEXT("Client_RejectItemPlacement: Server rejected placement"));
    
    if (!GridManager->IsIndexValid(OriginalIndex))
    {
        UE_LOG(LogInventory, Error, TEXT("Client_RejectItemPlacement: Invalid OriginalIndex=%d"), 
               OriginalIndex);
        return;
    }
    
    // Find the item
    UItemBase* ItemToRestore = nullptr;
    const int32 TotalTiles = GridManager->GetTotalTiles();
    
    for (int32 i = 0; i < TotalTiles; i++)
    {
        UItemBase* Item = Storage->GetItemAtIndex(i);
        if (Item && Item->ItemGuid == ItemGuid)
        {
            ItemToRestore = Item;
            break;
        }
    }
    
    if (!ItemToRestore)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_RejectItemPlacement: Item not found"));
        return;
    }
    
    // Clear item from current position
    Storage->ClearAllOccurrences(ItemToRestore);
    
    // Restore to original position
    ItemHandler->PlaceItemInGrid(ItemToRestore, OriginalIndex);
    
    // Refresh UI
    RefreshAllItems();
    RefreshGridLayout();
    
    UE_LOG(LogInventory, Log, TEXT("Client_RejectItemPlacement: Restored to index %d"), OriginalIndex);
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
    if (!ItemClass)
    {
        UE_LOG(LogInventory, Warning, TEXT("Client_SyncItemClass: ItemClass is null"));
        return;
    }
    
    if (!ItemHandler || !GridManager)
    {
        UE_LOG(LogInventory, Error, TEXT("Client_SyncItemClass: Modules not initialized!"));
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
        UE_LOG(LogInventory, Log, TEXT("Client_SyncItemClass: Spawned item %s at index %d"),
               *Item->GetNameFromItem(), TopLeftIndex);
        
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
// Replication
// ============================================

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UInventoryComponent, Columns);
    DOREPLIFETIME(UInventoryComponent, Rows);
    DOREPLIFETIME(UInventoryComponent, TileSize);
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
/*
#include "InventoryComponent.h"
#include "FishingCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogInventory, Log, All);

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

static FString TileToString(const FIntPoint& P)
{
	return FString::Printf(TEXT("(%d,%d)"), P.X, P.Y);
}

FString UInventoryComponent::DumpGrid() const
{
	FString S;
	S += FString::Printf(TEXT("Grid %dx%d, Items.Num=%d\n"), Columns, Rows, Items.Num());

	for (int32 y = 0; y < Rows; ++y)
	{
		for (int32 x = 0; x < Columns; ++x)
		{
			const int32 idx = TileToIndex(FIntPoint(x, y));
			const bool valid = Items.IsValidIndex(idx);
			UItemBase* it = valid ? Items[idx] : nullptr;
			S += it ? TEXT("[X]") : TEXT("[.]");
		}
		S += TEXT("\n");
	}
	return S;
}

void UInventoryComponent::Server_MoveItem_Implementation(UItemBase* Item, FIntPoint NewTopLeftTile)
{
	if (!Item)
	{
		UE_LOG(LogInventory, Warning, TEXT("Server_MoveItem: Item is null"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Moving %s to (%d,%d) in same inventory"),
	       *Item->GetName(), NewTopLeftTile.X, NewTopLeftTile.Y);

	if (!IsRoomAvailableAt(Item, NewTopLeftTile, Item))
	{
		UE_LOG(LogInventory, Warning, TEXT("Server_MoveItem: Cannot place at (%d,%d)"),
		       NewTopLeftTile.X, NewTopLeftTile.Y);

		for (int32 i = 0; i < Items.Num(); ++i)
		{
			if (Items[i] == Item)
			{
				Client_SyncItem(Item->ItemDef, Item->bIsRotated, Item->ItemGuid, i);
				return;
			}
		}

		UE_LOG(LogInventory, Error, TEXT("Server_MoveItem: Item not found in inventory!"));
		return;
	}

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] == Item)
		{
			Items[i] = nullptr;
		}
	}

	const int32 NewIndex = TileToIndex(NewTopLeftTile);
	AddItemAt(Item, NewIndex);

	NotifyItemsChanged();

	Client_SyncItem(Item->ItemDef, Item->bIsRotated, Item->ItemGuid, NewIndex);

	UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Successfully moved to index %d"), NewIndex);
}

void UInventoryComponent::ResizeItemsToGrid(bool bZeroInit)
{
	const int32 Expected = Columns * Rows;
	if (Expected <= 0) { return; }

	if (bZeroInit)
	{
		Items.SetNumZeroed(Expected);
	}
	else
	{
		Items.SetNum(Expected);
	}
}

void UInventoryComponent::Server_DropItemToWorld_Implementation(UItemBase* ItemToDrop)
{
	if (!ItemToDrop)
	{
		UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: ItemToDrop is null"));
		return;
	}

	if (!CharacterReference)
	{
		UE_LOG(LogInventory, Warning, TEXT("Not a player inventory, cannot drop to world"));
		return;
	}

	bool bFoundInInventory = false;
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] == ItemToDrop)
		{
			bFoundInInventory = true;
			break;
		}
	}

	if (!bFoundInInventory)
	{
		UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Item %s not found in inventory"),
		       *ItemToDrop->GetName());
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Dropping %s from %s's inventory"),
	       *ItemToDrop->GetName(),
	       *CharacterReference->GetName());

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
		UE_LOG(LogInventory, Error, TEXT("Server_DropItemToWorld: Failed to spawn ItemActor for %s"),
		       *ItemToDrop->GetName());

		return;
	}

	UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Successfully spawned %s at %s"),
	       *SpawnedActor->GetNameFromItem(),
	       *SpawnLoc.ToString());

	FGuid RemovedGuid = ItemToDrop->ItemGuid;
	bool bRemoved = RemoveItem(ItemToDrop);

	if (!bRemoved)
	{
		UE_LOG(LogInventory, Warning, TEXT("Server_DropItemToWorld: Failed to remove item from inventory"));

		SpawnedActor->Destroy();
		return;
	}

	NotifyItemsChanged();

	Client_RemoveItem(RemovedGuid);

	UE_LOG(LogInventory, Log, TEXT("Server_DropItemToWorld: Complete. Item dropped and removed from inventory"));
}

void UInventoryComponent::NotifyItemsChanged()
{
	RefreshAllItems();
	RefreshGridLayout();
}

void UInventoryComponent::ClearInventoryWidget()
{
	InventoryWidget = nullptr;
}

void UInventoryComponent::RemoveItemForServer_Implementation(UItemBase* ItemToRemove)
{
	if (!ItemToRemove)
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItemForServer: ItemToRemove is null"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Removing %s from %s"),
	       *ItemToRemove->GetName(),
	       *GetOwner()->GetName());

	bool bRemoved = RemoveItem(ItemToRemove);

	if (bRemoved)
	{
		NotifyItemsChanged();

		UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Successfully removed from server"));
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItemForServer: Item not found in inventory"));
	}
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	Activate(true);
	SetComponentTickEnabled(true);
	PrimaryComponentTick.SetTickFunctionEnable(true);

	const int32 Expected = Columns * Rows;
	if (Items.Num() != Expected)
	{
		UE_LOG(LogInventory, Warning, TEXT("BeginPlay: Items resized from %d to %d (Columns=%d, Rows=%d)"),
		       Items.Num(), Expected, Columns, Rows);
		Items.SetNumZeroed(Expected);
	}
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogInventory, Log, TEXT("EndPlay: Reason=%d"), static_cast<int32>(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::Initalize(AActor* Owner)
{
	CharacterReference = Cast<AFishingCharacter>(Owner);

	UE_LOG(LogInventory, Log, TEXT("BeginPlay: this=%p, Owner=%s, CharacterReference=%s\n%s"),
	       this,
	       Owner ? *Owner->GetName() : TEXT("None"),
	       CharacterReference ? *CharacterReference->GetName() : TEXT("None"),
	       *DumpGrid());
}

void UInventoryComponent::RegisterInventoryWidget(UInventoryWidget* InInventoryWidget)
{
	InventoryWidget = InInventoryWidget;
	RefreshGridLayout();
}

void UInventoryComponent::UpdateDescription(UItemBase* Item)
{
	if (!InventoryWidget)
	{
		UE_LOG(LogInventory, Warning, TEXT("OnDragCancelled: UpdateDescription  InventoryWidget is nullptr"));
		return;
	}
	InventoryWidget->UpdateDescription(Item);
}

void UInventoryComponent::ClearDescription()
{
	if (!InventoryWidget)
	{
		return;
	}
	InventoryWidget->ClearDescription();
}

void UInventoryComponent::SetFocusGridWidget()
{
	if (!InventoryWidget)
	{
		return;
	}
	InventoryWidget->SetFocusGridWidget();
}

void UInventoryComponent::Client_RejectItemPlacement_Implementation(FGuid ItemGuid, int32 OriginalIndex)
{
	UE_LOG(LogInventory, Warning, TEXT("Client_RejectItemPlacement: Server rejected placement, rolling back"));

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i]->ItemGuid == ItemGuid)
		{
			Items[i] = nullptr;

			if (Items.IsValidIndex(OriginalIndex))
			{
				Items[OriginalIndex] = Items[i];
				UE_LOG(LogInventory, Log, TEXT("Client_RejectItemPlacement: Restored item to index %d"),
				       OriginalIndex);
			}
			break;
		}
	}

	RefreshAllItems();
	RefreshGridLayout();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Columns);
	DOREPLIFETIME(UInventoryComponent, Rows);
	DOREPLIFETIME(UInventoryComponent, TileSize);
}

bool UInventoryComponent::IsTileValid(FIntPoint IntPoint)
{
	const bool ok = (IntPoint.X >= 0 && IntPoint.X < Columns &&
		IntPoint.Y >= 0 && IntPoint.Y < Rows);
#if !(UE_BUILD_SHIPPING)
	if (!ok)
	{
		UE_LOG(LogInventory, Verbose, TEXT("IsTileValid: false for %s (Columns=%d, Rows=%d)"),
		       *TileToString(IntPoint), Columns, Rows);
	}
#endif
	return ok;
}

bool UInventoryComponent::IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, UItemBase* IgnoreItem)
{
	if (!ItemToPlace)
	{
		UE_LOG(LogInventory, Warning, TEXT("IsRoomAvailableAt: ItemToPlace is null"));
		return false;
	}

	const FIntPoint CurrentDims = ItemToPlace->GetCurrentDimensions();

	if (TopLeftTile.X < 0 || TopLeftTile.Y < 0 ||
		TopLeftTile.X + CurrentDims.X > Columns ||
		TopLeftTile.Y + CurrentDims.Y > Rows)
	{
		UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailableAt: Out of bounds. TopLeft=%s, Dims=%s, Grid=%dx%d"),
		       *TileToString(TopLeftTile), *TileToString(CurrentDims), Columns, Rows);
		return false;
	}

	for (int32 y = 0; y < CurrentDims.Y; ++y)
	{
		for (int32 x = 0; x < CurrentDims.X; ++x)
		{
			const FIntPoint CheckTile(TopLeftTile.X + x, TopLeftTile.Y + y);
			const int32 idx = TileToIndex(CheckTile);

			if (!Items.IsValidIndex(idx))
			{
				UE_LOG(LogInventory, Warning, TEXT("IsRoomAvailableAt: Invalid index %d for tile %s"),
				       idx, *TileToString(CheckTile));
				return false;
			}

			UItemBase* Occupant = Items[idx];
			if (Occupant && Occupant != IgnoreItem)
			{
				UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailableAt: Occupied by %s at %s"),
				       *Occupant->GetName(), *TileToString(CheckTile));
				return false;
			}
		}
	}

	return true;
}

bool UInventoryComponent::IsRoomAvailable(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogInventory, Warning, TEXT("IsRoomAvailable: ItemToAdd=nullptr"));
		return false;
	}

	if (!Items.IsValidIndex(TopLeftIndex))
	{
		UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailable: TopLeftIndex invalid %d"), TopLeftIndex);
		return false;
	}

	const FIntPoint CurrentDims = ItemToAdd->GetCurrentDimensions();
	const FIntPoint TopLeft = IndexToTile(TopLeftIndex);

	for (int32 y = 0; y < CurrentDims.Y; ++y)
	{
		for (int32 x = 0; x < CurrentDims.X; ++x)
		{
			const FIntPoint CheckTile(TopLeft.X + x, TopLeft.Y + y);

			if (!IsTileValid(CheckTile))
			{
				UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailable: Out of bounds at %s"),
				       *TileToString(CheckTile));
				return false;
			}

			const int32 Idx = TileToIndex(CheckTile);

			if (!GetResultAtIndex(Idx))
			{
				UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailable: Invalid backing index %d for %s"),
				       Idx, *TileToString(CheckTile));
				return false;
			}

			if (GetItemAtIndex(Idx))
			{
				UE_LOG(LogInventory, Verbose, TEXT("IsRoomAvailable: Occupied at %s (Index=%d)"),
				       *TileToString(CheckTile), Idx);
				return false;
			}
		}
	}

	return true;
}

FIntPoint UInventoryComponent::IndexToTile(int32 Index) const
{
	if (Columns <= 0)
	{
		UE_LOG(LogInventory, Error, TEXT("IndexToTile: Columns<=0 (%d)"), Columns);
		return FIntPoint::ZeroValue;
	}
	return FIntPoint(Index % Columns, Index / Columns);
}

void UInventoryComponent::AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItemAt: ItemToAdd is null"));
		return;
	}

	if (!Items.IsValidIndex(TopLeftIndex))
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItemAt: Invalid TopLeftIndex=%d"), TopLeftIndex);
		return;
	}

	const FIntPoint CurrentDims = ItemToAdd->GetCurrentDimensions();
	const FIntPoint TopLeft = IndexToTile(TopLeftIndex);

	UE_LOG(LogInventory, Log, TEXT("AddItemAt: Item=%s, TopLeft=%s, Dims=%s, IsRotated=%s"),
	       *ItemToAdd->GetName(),
	       *TileToString(TopLeft),
	       *TileToString(CurrentDims),
	       ItemToAdd->GetIsRotated() ? TEXT("Yes") : TEXT("No"));

	for (int32 y = 0; y < CurrentDims.Y; ++y)
	{
		for (int32 x = 0; x < CurrentDims.X; ++x)
		{
			const FIntPoint Tile(TopLeft.X + x, TopLeft.Y + y);
			const int32 Idx = TileToIndex(Tile);

			if (Items.IsValidIndex(Idx))
			{
				Items[Idx] = ItemToAdd;
			}
			else
			{
				UE_LOG(LogInventory, Warning, TEXT("AddItemAt: Skip invalid index %d (Tile=%s)"),
				       Idx, *TileToString(Tile));
			}
		}
	}

	UE_LOG(LogInventory, Log, TEXT("AddItemAt: Item placed"));
}

bool UInventoryComponent::GetResultAtIndex(int32 Index)
{
	return Items.IsValidIndex(Index);
}

void UInventoryComponent::RefreshGridLayout()
{
	if (InventoryWidget)
	{
		InventoryWidget->RefreshGrid();
	}
	else
		UE_LOG(LogInventory, Warning, TEXT("RefreshGridLayout: Failed due to invalid InventoryWidget"));
}

UItemBase* UInventoryComponent::GetItemAtIndex(int32 Index)
{
	return Items.IsValidIndex(Index) ? Items[Index] : nullptr;
}

TMap<UItemBase*, FIntPoint> UInventoryComponent::GetAllItems()
{
	AllItems.Reset();

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (UItemBase* It = Items[i])
		{
			if (!AllItems.Contains(It))
			{
				AllItems.Add(It, IndexToTile(i));
			}
		}
	}

	UE_LOG(LogInventory, Log, TEXT("GetAllItems: Found %d unique items"), AllItems.Num());

	return AllItems;
}

bool UInventoryComponent::TryAddItem(UItemBase* ItemToAdd)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogInventory, Warning, TEXT("TryAddItem: ItemToAdd=nullptr"));
		return false;
	}

	const FIntPoint CurrentDims = ItemToAdd->GetCurrentDimensions();
	UE_LOG(LogInventory, Log, TEXT("TryAddItem: Item=%s, CurrentDim=%s, IsRotated=%s"),
	       *ItemToAdd->GetName(),
	       *TileToString(CurrentDims),
	       ItemToAdd->GetIsRotated() ? TEXT("Yes") : TEXT("No"));

	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (IsRoomAvailable(ItemToAdd, i))
		{
			UE_LOG(LogInventory, Log, TEXT("TryAddItem: Found room at TopLeftIndex=%d (Tile=%s)"),
			       i, *TileToString(IndexToTile(i)));

			AddItemAt(ItemToAdd, i);

			if (GetOwnerRole() == ROLE_Authority)
			{
				NotifyItemsChanged();

				Client_SyncItem(ItemToAdd->ItemDef, ItemToAdd->bIsRotated, ItemToAdd->ItemGuid, i);
			}

			UE_LOG(LogInventory, Log, TEXT("TryAddItem: Added. Grid:\n%s"), *DumpGrid());
			return true;
		}
	}

	UE_LOG(LogInventory, Warning, TEXT("TryAddItem: No room for Item=%s"), *ItemToAdd->GetName());
	return false;
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
		UE_LOG(LogInventory, Warning, TEXT("TryAddItemFroActor: Failed to create UItemBase from AItemActor"));
		return false;
	}

	return TryAddItem(NewItem);
}

bool UInventoryComponent::RemoveItem(UItemBase* ItemToRemove)
{
	if (!ItemToRemove)
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItem: ItemToRemove is null"));
		return false;
	}

	bool bRemoved = false;
	FGuid RemovedGuid = ItemToRemove->ItemGuid;

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] == ItemToRemove)
		{
			Items[i] = nullptr;
			bRemoved = true;
		}
	}

	if (bRemoved)
	{
		UE_LOG(LogInventory, Log, TEXT("RemoveItem: Removed %s"), *ItemToRemove->GetName());

		if (GetOwnerRole() == ROLE_Authority)
		{
			NotifyItemsChanged();
			Client_RemoveItem(RemovedGuid);
		}
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("RemoveItem: Item %s not found in inventory"),
		       *ItemToRemove->GetName());
	}

	return bRemoved;
}

void UInventoryComponent::Client_SyncItem_Implementation(UItemDataAsset* ItemDef, bool bIsRotated, FGuid ItemGuid,
                                                         int32 TopLeftIndex)
{
	if (!ItemDef)
	{
		UE_LOG(LogInventory, Warning, TEXT("Client_SyncItem: ItemDef is null"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("Client_SyncItem: Syncing item %s at index %d"),
	       *ItemDef->DisplayName.ToString(), TopLeftIndex);

	UItemBase* NewItem = NewObject<UItemBase>(this, UItemBase::StaticClass());
	if (!NewItem)
	{
		UE_LOG(LogInventory, Error, TEXT("Client_SyncItem: Failed to create UItemBase"));
		return;
	}

	NewItem->Initialize(ItemDef);
	NewItem->bIsRotated = bIsRotated;
	NewItem->ItemGuid = ItemGuid;

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i]->ItemGuid == ItemGuid)
		{
			Items[i] = nullptr;
		}
	}

	AddItemAt(NewItem, TopLeftIndex);

	RefreshAllItems();
	RefreshGridLayout();
}

void UInventoryComponent::Client_RemoveItem_Implementation(FGuid ItemGuid)
{
	UE_LOG(LogInventory, Log, TEXT("Client_RemoveItem: Removing item with GUID"));

	bool bFound = false;
	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] && Items[i]->ItemGuid == ItemGuid)
		{
			Items[i] = nullptr;
			bFound = true;
		}
	}

	if (!bFound)
	{
		UE_LOG(LogInventory, Warning, TEXT("Client_RemoveItem: Item not found with given GUID"));
	}

	RefreshAllItems();
	RefreshGridLayout();
}

void UInventoryComponent::RefreshAllItems()
{
	AllItems.Empty();

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i])
		{
			if (!AllItems.Contains(Items[i]))
			{
				AllItems.Add(Items[i], IndexToTile(i));
			}
		}
	}

	UE_LOG(LogInventory, Log, TEXT("RefreshAllItems: Found %d unique items"), AllItems.Num());
}

void UInventoryComponent::Client_SyncItemClass_Implementation(TSubclassOf<AItemActor> ActorClass, int32 TopLeftIndex)
{
	if (!ActorClass)
	{
		UE_LOG(LogInventory, Warning, TEXT("Client_SyncItemClass: ItemClass is null"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AItemActor* Item = GetWorld()->SpawnActor<AItemActor>(
		ActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Item)
	{
		UE_LOG(LogInventory, Log, TEXT("Client_SyncItemClass: Spawned item %s at index %d"),
		       *Item->GetNameFromItem(), TopLeftIndex);

		AddItemAt(Item->MakeItemInstance(this, CharacterReference), TopLeftIndex);

		RefreshAllItems();
		RefreshGridLayout();

		Item->SetActorHiddenInGame(true);
		Item->SetActorEnableCollision(false);
	}
}

void UInventoryComponent::RefreshLayoutForClient_Implementation()
{
	UE_LOG(LogInventory, Log, TEXT("RefreshLayoutForClient_Implementation: Remote client refreshing"));

	RefreshAllItems();
	RefreshGridLayout();
}

void UInventoryComponent::TryAddForServer_Implementation(UItemBase* ItemToAdd)
{
	if (!TryAddItem(ItemToAdd))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn FishItem from ItemClass!"));
	}
}

void UInventoryComponent::AddItemAtForServer_Implementation(UItemBase* ItemToAdd, int32 TopLeftIndex)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: ItemToAdd is null"));
		return;
	}

	if (!Items.IsValidIndex(TopLeftIndex))
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Invalid TopLeftIndex=%d"), TopLeftIndex);
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Adding %s to %s at index %d"),
	       *ItemToAdd->GetName(),
	       *GetOwner()->GetName(),
	       TopLeftIndex);

	const FIntPoint TopLeft = IndexToTile(TopLeftIndex);
	if (!IsRoomAvailableAt(ItemToAdd, TopLeft, nullptr))
	{
		UE_LOG(LogInventory, Warning, TEXT("AddItemAtForServer: Cannot place at index %d"), TopLeftIndex);

		return;
	}

	AddItemAt(ItemToAdd, TopLeftIndex);

	NotifyItemsChanged();

	Client_SyncItem(ItemToAdd->ItemDef, ItemToAdd->bIsRotated, ItemToAdd->ItemGuid, TopLeftIndex);

	UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Successfully added to server"));
}
*/