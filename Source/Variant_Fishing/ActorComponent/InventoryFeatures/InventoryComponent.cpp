


#include "InventoryComponent.h"

#include "Fishing.h"
#include "SubModules/InventoryGridManager.h"
#include "SubModules/InventoryStorage.h"
#include "SubModules/InventoryPlacementValidator.h"
#include "SubModules/InventoryItemHandler.h"
#include "SubModules/InventoryUIManager.h"
#include "FishingCharacter.h"
#include "FishingPlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "../../Interface/ItemDataProvider.h"
#include "Variant_Fishing/Database/DatabaseManager.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(true);
}

void UInventoryComponent::TurnReplicationOff()
{
    IsReplicationOff = true;
    SetIsReplicated(false);
}


void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    
    
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
    
    if (GridManager)
    {
        GridManager->Initialize(Columns, Rows);
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
    
    UE_LOG(LogInventory, Log, TEXT("InitializeModules: All modules initialized"));
}

void UInventoryComponent::Initalize(AActor* Owner)
{
    CharacterReference = Cast<AFishingCharacter>(Owner);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UInventoryComponent, Columns);
    DOREPLIFETIME(UInventoryComponent, Rows);
    
    
    DOREPLIFETIME(UInventoryComponent, ItemSyncData);
}



bool UInventoryComponent::IsItemCategoryAllowed(UItemBase* Item) const
{

    if (!Item)
    {
        return false;
    }
    EItemCategory ItemCategory = IItemDataProvider::Execute_GetCategory(Item->ItemDataProvider.GetObject());
    

    
    if (bAllowAllCategories || AllowedCategories.Num() == 0)
    {
        return true;
    }

    
    return AllowedCategories.Contains(ItemCategory);
}

bool UInventoryComponent::IsCategoryAllowed(EItemCategory Category) const
{
    if (bAllowAllCategories || AllowedCategories.Num() == 0)
    {
        return true;
    }

    return AllowedCategories.Contains(Category);
}

TArray<EItemCategory> UInventoryComponent::GetAllowedCategoriesArray() const
{
    TArray<EItemCategory> Result;

    
    if (bAllowAllCategories || AllowedCategories.Num() == 0)
    {
        Result.Add(EItemCategory::All);
        Result.Add(EItemCategory::Fish);
        Result.Add(EItemCategory::Equipment);
        Result.Add(EItemCategory::Consumable);
        
        
        Result.Add(EItemCategory::Misc);
    }
    else
    {
        
        Result.Add(EItemCategory::All);
		
        
        Result.Append(AllowedCategories.Array());
    }

    return Result;
}




void UInventoryComponent::OnRep_ItemSyncData()
{
    UE_LOG(LogInventory, Log, TEXT("OnRep_ItemSyncData: Received %d items (Role=%d)"),
           ItemSyncData.Num(), static_cast<int32>(GetOwnerRole()));
    
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("OnRep_ItemSyncData: Storage is null!"));
        return;
    }
    
    
    Storage->ApplySyncData(ItemSyncData);
    
    
    RefreshAllItems();
    RefreshGridLayout();
    
    UE_LOG(LogInventory, Log, TEXT("OnRep_ItemSyncData: Sync complete"));
}


void UInventoryComponent::SyncToClients()
{
    if (IsReplicationOff)
        return;
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
    
    ItemSyncData = Storage->GenerateSyncData();
    UE_LOG(LogInventory, Log, TEXT("SyncToClients: Updated ItemSyncData with %d entries"), 
           ItemSyncData.Num());
    
    RefreshAllItems();
    RefreshGridLayout();
}






void UInventoryComponent::RegisterInventoryWidget(UInventoryWidget* InInventoryWidget)
{
    if (!UIManager)
    {
        UE_LOG(LogInventory, Error, TEXT("RegisterInventoryWidget: UIManager is null!"));
        return;
    }
    
    UIManager->RegisterWidget(InInventoryWidget);
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





bool UInventoryComponent::IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, 
                                            UItemBase* IgnoreItem)
{
    if (!Validator)
    {
        UE_LOG(LogInventory, Error, TEXT("IsRoomAvailableAt: Validator is null!"));
        return false;
    }

    
    if (!IsItemCategoryAllowed(ItemToPlace))
    {
        UE_LOG(LogInventory, Warning, TEXT("IsRoomAvailableAt: Item category not allowed in this inventory"));
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

    if (!IsItemCategoryAllowed(ItemToAdd))
    {
        return false;
    }

    
    return Validator->CanPlaceItemAt(ItemToAdd, TopLeftIndex, nullptr);
}





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
        
        if (GetOwnerRole() == ROLE_Authority)
        {
            SyncToClients();
        }
        
        NotifyItemsChanged();

        if (CharacterReference)
        {
            AFishingPlayerController* PC = CharacterReference->GetController<AFishingPlayerController>();
            if (PC)
            {
                PC->ShowItemAcquiredBanner(ItemToAdd);
                UE_LOG(LogInventory, Log, TEXT("TryAddItem: Success (Role=%d)"), 
                       static_cast<int32>(GetOwnerRole()));
            }
        }
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
        return;
    }
    
    if (ItemHandler->MoveItem(Item, NewTopLeftTile))
    {
        
        SyncToClients();
        
        UE_LOG(LogInventory, Log, TEXT("Server_MoveItem: Success, synced to clients"));
    }
}


bool UInventoryComponent::FindItemTopLeftIndex(UItemBase* Item, int32& OutIndex) const
{
    if (!Item || !Storage || !GridManager)
    {
        return false;
    }
    
    
    const TMap<UItemBase*, FIntPoint>& UniqueItems = Storage->GetCachedUniqueItems();
    
    if (const FIntPoint* TopLeftTile = UniqueItems.Find(Item))
    {
        OutIndex = GridManager->TileToIndex(*TopLeftTile);
        return true;
    }
    
    return false;
}


bool UInventoryComponent::FindItemIndex(UItemBase* Item, int32& OutIndex) const
{
    if (!Item || !Storage || !GridManager)
    {
        return false;
    }
    
    const TArray<UItemBase*>& Items = Storage->GetItemsArray();
    const int32 TotalTiles = GridManager->GetTotalTiles();
    
    for (int32 i = 0; i < TotalTiles; i++)
    {
        if (Items.IsValidIndex(i) && Items[i] == Item)
        {
            OutIndex = i;
            return true;
        }
    }
    
    return false;
}


void UInventoryComponent::Server_DropItemToWorld_Implementation(UItemBase* ItemToDrop)
{
    if (!ItemToDrop || !CharacterReference || !ItemHandler)
    {
        return;
    }
    
    int32 ItemIndex = -1;
    if (!FindItemIndex(ItemToDrop, ItemIndex))
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
        
        UE_LOG(LogInventory, Log, TEXT("RemoveItemForServer: Success"));
    }
}

void UInventoryComponent::TryAddForServer_Implementation(UItemBase* ItemToAdd)
{
    if (!TryAddItem(ItemToAdd))
    {
        UE_LOG(LogInventory, Warning, TEXT("TryAddForServer: Failed"));
    }
    
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
        
        SyncToClients();
        
        UE_LOG(LogInventory, Log, TEXT("AddItemAtForServer: Success, synced"));
    }
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





void UInventoryComponent::ResizeItemsToGrid(bool bZeroInit)
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("ResizeItemsToGrid: Storage is null!"));
        return;
    }
    
    Storage->ResizeStorage(bZeroInit);
}

bool UInventoryComponent::SaveInventoryToDatabase(int32 PlayerID)
{
    
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogInventory, Warning, TEXT("SaveInventoryToDatabase: Not authority! (Role=%d)"), 
               static_cast<int32>(GetOwnerRole()));
        return false;
    }

    if (PlayerID < 0)
    {
        UE_LOG(LogInventory, Error, TEXT("SaveInventoryToDatabase: Invalid PlayerID=%d"), PlayerID);
        return false;
    }

    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("SaveInventoryToDatabase: Storage is null!"));
        return false;
    }

    
    UDatabaseManager* DatabaseManager = GetWorld()->GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DatabaseManager)
    {
        UE_LOG(LogInventory, Error, TEXT("SaveInventoryToDatabase: DatabaseManager not found!"));
        return false;
    }

    
    TMap<UItemBase*, FIntPoint> ItemsWithPositions = Storage->GetAllUniqueItems();

    UE_LOG(LogInventory, Log, TEXT("SaveInventoryToDatabase: Saving %d items for PlayerID=%d"),
           ItemsWithPositions.Num(), PlayerID);

    
    bool bSuccess = DatabaseManager->SaveInventory(PlayerID, ItemsWithPositions);

    if (bSuccess)
    {
        UE_LOG(LogInventory, Log, TEXT("âœ… SaveInventoryToDatabase: Success! Saved %d items"),
               ItemsWithPositions.Num());
    }
    else
    {
        UE_LOG(LogInventory, Error, TEXT("âŒ SaveInventoryToDatabase: Failed!"));
    }

    return bSuccess;
}

bool UInventoryComponent::LoadInventoryFromDatabase(int32 PlayerID)
{
    
    if (GetOwnerRole() != ROLE_Authority)
    {
        UE_LOG(LogInventory, Warning, TEXT("LoadInventoryFromDatabase: Not authority! (Role=%d)"),
               static_cast<int32>(GetOwnerRole()));
        return false;
    }

    if (PlayerID < 0)
    {
        UE_LOG(LogInventory, Error, TEXT("LoadInventoryFromDatabase: Invalid PlayerID=%d"), PlayerID);
        return false;
    }

    if (!Storage || !ItemHandler || !GridManager)
    {
        UE_LOG(LogInventory, Error, TEXT("LoadInventoryFromDatabase: Required modules not initialized!"));
        return false;
    }

    
    UDatabaseManager* DatabaseManager = GetWorld()->GetGameInstance()->GetSubsystem<UDatabaseManager>();
    if (!DatabaseManager)
    {
        UE_LOG(LogInventory, Error, TEXT("LoadInventoryFromDatabase: DatabaseManager not found!"));
        return false;
    }

    UE_LOG(LogInventory, Log, TEXT("LoadInventoryFromDatabase: Loading inventory for PlayerID=%d"), PlayerID);

    
    Storage->ClearAll();

    
    TMap<UItemBase*, FIntPoint> LoadedItems = DatabaseManager->LoadInventory(PlayerID, this);

    if (LoadedItems.Num() == 0)
    {
        UE_LOG(LogInventory, Warning, TEXT("LoadInventoryFromDatabase: No items found for PlayerID=%d"), PlayerID);
        
        
        SyncToClients();
        NotifyItemsChanged();
        return true;
    }

    UE_LOG(LogInventory, Log, TEXT("LoadInventoryFromDatabase: Loaded %d items from database"), 
           LoadedItems.Num());

    
    int32 PlacedCount = 0;
    for (const auto& Pair : LoadedItems)
    {
        UItemBase* Item = Pair.Key;
        const FIntPoint& GridPos = Pair.Value;

        if (!Item)
        {
            UE_LOG(LogInventory, Warning, TEXT("LoadInventoryFromDatabase: Null item in loaded data"));
            continue;
        }

        
        int32 TopLeftIndex = GridManager->TileToIndex(GridPos);

        if (!GridManager->IsIndexValid(TopLeftIndex))
        {
            UE_LOG(LogInventory, Warning, TEXT("LoadInventoryFromDatabase: Invalid position (%d,%d) for %s"),
                   GridPos.X, GridPos.Y, *Item->DisplayName());
            continue;
        }

        
        ItemHandler->PlaceItemInGrid(Item, TopLeftIndex);
        PlacedCount++;

        
        if (Item->IsFish())
        {
            float Length, Weight;
            FString FishName;
            if (Item->GetFishInfo(Length, Weight, FishName))
            {
                UE_LOG(LogInventory, Log, TEXT("  âœ… Placed Fish: %s - %.1fcm, %.2fkg at (%d,%d)"),
                       *FishName, Length, Weight, GridPos.X, GridPos.Y);
            }
        }
        else
        {
            UE_LOG(LogInventory, Log, TEXT("  âœ… Placed: %s at (%d,%d)"),
                   *Item->DisplayName(), GridPos.X, GridPos.Y);
        }
    }

    UE_LOG(LogInventory, Log, TEXT("LoadInventoryFromDatabase: Successfully placed %d/%d items"),
           PlacedCount, LoadedItems.Num());

    
    Storage->RefreshUniqueItemsCache();

    
    SyncToClients();

    
    NotifyItemsChanged();

    UE_LOG(LogInventory, Log, TEXT("âœ… LoadInventoryFromDatabase: Complete! (PlayerID=%d, Items=%d)"),
           PlayerID, PlacedCount);

    return true;
}

void UInventoryComponent::ClearAllItems()
{
    if (!Storage)
    {
        UE_LOG(LogInventory, Error, TEXT("ClearAllItems: Storage is null!"));
        return;
    }

    UE_LOG(LogInventory, Log, TEXT("ClearAllItems: Clearing all items from inventory"));

    
    Storage->ClearAll();

    
    if (GetOwnerRole() == ROLE_Authority)
    {
        SyncToClients();
    }

    
    NotifyItemsChanged();

    UE_LOG(LogInventory, Log, TEXT("âœ… ClearAllItems: Complete"));
}

bool UInventoryComponent::GetResultAtIndex(int32 Index)
{
    if (!GridManager)
    {
        return false;
    }
    
    return GridManager->IsIndexValid(Index);
}
