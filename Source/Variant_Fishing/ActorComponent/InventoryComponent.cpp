#include "Variant_Fishing/ActorComponent/InventoryComponent.h"

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
