#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "InventoryComponent.generated.h"

class UInventoryWidget;
class UInventoryDescriptionWidget;
class AItemActor;
class AFishingCharacter;
class UInventoryGridWidget;
class UItemBase;
class UItemDataAsset;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FISHING_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Initalize(AActor* Owner);
	virtual void RegisterInventoryWidget(UInventoryWidget* InInventoryWidget);
	virtual void UpdateDescription(UItemBase* Item);
	virtual void ClearDescription();
	virtual void SetFocusGridWidget();

	UFUNCTION(Client, Reliable)
	void Client_RejectItemPlacement(FGuid ItemGuid, int32 OriginalIndex);
	void RefreshGridLayout();

	UItemBase* GetItemAtIndex(int32 Index);
	TMap<UItemBase*, FIntPoint> GetAllItems();

	bool IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, UItemBase* IgnoreItem);
	bool IsRoomAvailable(UItemBase* ItemToAdd, int32 TopLeftIndex);
	bool IsTileValid(FIntPoint IntPoint);

	FIntPoint IndexToTile(int32 Index) const;
	int32 TileToIndex(FIntPoint Tile) const { return Tile.Y * Columns + Tile.X; }
	FString DumpGrid() const;

	UFUNCTION(Client, Reliable)
	void RefreshLayoutForClient();

	UFUNCTION(Server, Reliable)
	void Server_MoveItem(UItemBase* Item, FIntPoint NewTopLeftTile);

	UFUNCTION(Server, Reliable)
	void RemoveItemForServer(UItemBase* ItemToRemove);

	UFUNCTION(Server, Reliable)
	void TryAddForServer(UItemBase* ItemToAdd);

	UFUNCTION(Server, Reliable)
	void AddItemAtForServer(UItemBase* ItemToAdd, int32 TopLeftIndex);

	void ResizeItemsToGrid(bool bZeroInit);

	UFUNCTION(Server, Reliable)
	void Server_DropItemToWorld(UItemBase* ItemToDrop);

	UFUNCTION()
	void NotifyItemsChanged();

	UFUNCTION()
	void ClearInventoryWidget();

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> ItemWidgetClass;

	UPROPERTY()
	UUserWidget* ItemWidget;

	UFUNCTION(BlueprintCallable)
	bool TryAddItem(UItemBase* ItemToAdd);

	UFUNCTION(BlueprintCallable)
	bool TryAddItemFroActor(AItemActor* ItemToAdd);

	UFUNCTION(BlueprintCallable)
	bool RemoveItem(UItemBase* ItemToRemove);

	UPROPERTY(EditAnywhere, Replicated)
	int32 Columns = 10;

	UPROPERTY(EditAnywhere, Replicated)
	int32 Rows = 10;

	UPROPERTY(EditAnywhere, Replicated)
	float TileSize = 32.f;

	void AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex);
	void RefreshAllItems();

	UFUNCTION(Client, Reliable)
	void Client_SyncItemClass(TSubclassOf<AItemActor> ItemClass, int32 TopLeftIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	TSubclassOf<AItemActor> ItemActorSubClass;

protected:
	UPROPERTY()
	AFishingCharacter* CharacterReference = nullptr;

	UPROPERTY()
	UInventoryWidget* InventoryWidget;

	TArray<UItemBase*> Items;

	TMap<UItemBase*, FIntPoint> AllItems;

	UFUNCTION(Client, Reliable)
	void Client_SyncItem(UItemDataAsset* ItemDef, bool bIsRotated, FGuid ItemGuid, int32 TopLeftIndex);

	UFUNCTION(Client, Reliable)
	void Client_RemoveItem(FGuid ItemGuid);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool GetResultAtIndex(int32 Index);
};
