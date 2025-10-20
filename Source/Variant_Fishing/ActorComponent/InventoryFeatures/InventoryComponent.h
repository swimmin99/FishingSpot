


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "../../Interface/ItemDataProvider.h"
#include "Variant_Fishing/Widget/Inventory/ItemWidget.h"
#include "FishingCharacter.h"
#include "InventoryComponent.generated.h"

class UInventoryWidget;
class UInventoryDescriptionWidget;
class AItemActor;
class UInventoryGridWidget;
class UItemBase;


class UInventoryGridManager;
class UInventoryStorage;
class UInventoryPlacementValidator;
class UInventoryItemHandler;
class UInventoryUIManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FISHING_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void TurnReplicationOff();
    
    
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
    virtual void Initalize(AActor* Owner);

    
    virtual void RegisterInventoryWidget(UInventoryWidget* InInventoryWidget);
    virtual void SetFocusGridWidget();
    void ClearInventoryWidget();
    
    
    void RefreshGridLayout();
    
    
    FIntPoint IndexToTile(int32 Index) const;
    int32 TileToIndex(FIntPoint Tile) const;
    bool IsTileValid(FIntPoint IntPoint);
    FString DumpGrid() const;
    
    
    UItemBase* GetItemAtIndex(int32 Index);
    TMap<UItemBase*, FIntPoint> GetAllItems();
    void RefreshAllItems();
    
    
    bool IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, UItemBase* IgnoreItem);
    bool IsRoomAvailable(UItemBase* ItemToAdd, int32 TopLeftIndex);
    
    
    UFUNCTION(BlueprintCallable)
    bool TryAddItem(UItemBase* ItemToAdd);
    
    UFUNCTION(BlueprintCallable)
    bool TryAddItemFroActor(AItemActor* ItemToAdd);
    
    UFUNCTION(BlueprintCallable)
    bool RemoveItem(UItemBase* ItemToRemove);
    
    void AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex);
    void NotifyItemsChanged();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    FIntPoint GetItemTopLeftTile(UItemBase* Item)
    {
        if (!Item)
        {
            return FIntPoint::ZeroValue;
        }

        TMap<UItemBase*, FIntPoint> AllItems = GetAllItems();
        if (AllItems.Contains(Item))
        {
            return AllItems[Item];
        }

        return FIntPoint::ZeroValue;
    }
    

    
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
    
    UFUNCTION(Server, Reliable)
    void Server_DropItemToWorld(UItemBase* ItemToDrop);

    UFUNCTION()
    bool FindItemTopLeftIndex(UItemBase* Item, int32& OutIndex) const;

    UFUNCTION()
    bool FindItemIndex(UItemBase* Item, int32& OutIndex) const;


    UFUNCTION(Client, Reliable)
    void Client_SyncItemClass(TSubclassOf<AItemActor> ItemClass, int32 TopLeftIndex);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Category")
    TSet<EItemCategory> AllowedCategories;

    UFUNCTION(BlueprintPure, Category = "Inventory|Category")
    bool IsItemCategoryAllowed(UItemBase* Item) const;

    UFUNCTION(BlueprintPure, Category = "Inventory|Category")
    bool IsCategoryAllowed(EItemCategory Category) const;

    UFUNCTION(BlueprintPure, Category = "Inventory|Category")
    TArray<EItemCategory> GetAllowedCategoriesArray() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Category")
    bool bAllowAllCategories = true;


    
    
    UFUNCTION()
    void OnRep_ItemSyncData();
    
    
    UPROPERTY(EditAnywhere, Replicated)
    int32 Columns = 10;
    
    UPROPERTY(EditAnywhere, Replicated)
    int32 Rows = 10;
    
    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UUserWidget> ItemWidgetClass;
    
    UPROPERTY()
    UUserWidget* ItemWidget;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    TSubclassOf<AItemActor> ItemActorSubClass;

    void ResizeItemsToGrid(bool bZeroInit);

    

    
    UFUNCTION(BlueprintCallable, Category = "Inventory|Database")
    bool SaveInventoryToDatabase(int32 PlayerID);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Database")
    bool LoadInventoryFromDatabase(int32 PlayerID);

    UFUNCTION(BlueprintCallable, Category = "Inventory|Database")
    void ClearAllItems();


    
protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    UPROPERTY()
    AFishingCharacter* CharacterReference = nullptr;
    
    
    UPROPERTY()
    UInventoryGridManager* GridManager;
    
    UPROPERTY()
    UInventoryStorage* Storage;
    
    UPROPERTY()
    UInventoryPlacementValidator* Validator;
    
    UPROPERTY()
    UInventoryItemHandler* ItemHandler;
    
    UPROPERTY()
    UInventoryUIManager* UIManager;
    
    
    UPROPERTY(ReplicatedUsing=OnRep_ItemSyncData)
    TArray<FItemSyncData> ItemSyncData;


private:
    void InitializeModules();
    bool GetResultAtIndex(int32 Index);
    bool IsReplicationOff = false;
    
    void SyncToClients();
};