// ============================================
// InventoryComponent.h (수정)
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "Variant_Fishing/Widget/Inventory/InventoryWidget.h"
#include "InventoryComponent.generated.h"

class UInventoryWidget;
class UInventoryDescriptionWidget;
class AItemActor;
class AFishingCharacter;
class UInventoryGridWidget;
class UItemBase;
class UItemDataAsset;

// Module classes
class UInventoryGridManager;
class UInventoryStorage;
class UInventoryPlacementValidator;
class UInventoryItemHandler;
class UInventoryUIManager;
class UInventoryNetworkSync;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FISHING_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();
    
    // Lifecycle
    virtual void BeginPlay() override;
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
    virtual void Initalize(AActor* Owner);
    
    // Widget Management
    virtual void RegisterInventoryWidget(UInventoryWidget* InInventoryWidget);
    virtual void UpdateDescription(UItemBase* Item);
    virtual void ClearDescription();
    virtual void SetFocusGridWidget();
    void ClearInventoryWidget();
    
    // Grid Operations
    void RefreshGridLayout();
    
    // Grid Info
    FIntPoint IndexToTile(int32 Index) const;
    int32 TileToIndex(FIntPoint Tile) const;
    bool IsTileValid(FIntPoint IntPoint);
    FString DumpGrid() const;
    
    // Storage Access
    UItemBase* GetItemAtIndex(int32 Index);
    TMap<UItemBase*, FIntPoint> GetAllItems();
    void RefreshAllItems();
    
    // Validation
    bool IsRoomAvailableAt(UItemBase* ItemToPlace, FIntPoint TopLeftTile, UItemBase* IgnoreItem);
    bool IsRoomAvailable(UItemBase* ItemToAdd, int32 TopLeftIndex);
    
    // Item Operations
    UFUNCTION(BlueprintCallable)
    bool TryAddItem(UItemBase* ItemToAdd);
    
    UFUNCTION(BlueprintCallable)
    bool TryAddItemFroActor(AItemActor* ItemToAdd);
    
    UFUNCTION(BlueprintCallable)
    bool RemoveItem(UItemBase* ItemToRemove);
    
    void AddItemAt(UItemBase* ItemToAdd, int32 TopLeftIndex);
    void NotifyItemsChanged();
    
    // Network RPCs
    UFUNCTION(Client, Reliable)
    void Client_RejectItemPlacement(FGuid ItemGuid, int32 OriginalIndex);
    
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
    
    UFUNCTION(Client, Reliable)
    void Client_SyncItem(UItemDataAsset* ItemDef, bool bIsRotated, FGuid ItemGuid, int32 TopLeftIndex);
    
    UFUNCTION(Client, Reliable)
    void Client_RemoveItem(FGuid ItemGuid);
    
    UFUNCTION(Client, Reliable)
    void Client_SyncItemClass(TSubclassOf<AItemActor> ItemClass, int32 TopLeftIndex);
    
    // ★★★ 레플리케이션 콜백 ★★★
    UFUNCTION()
    void OnRep_ItemSyncData();
    
    // Public Properties
    UPROPERTY(EditAnywhere, Replicated)
    int32 Columns = 10;
    
    UPROPERTY(EditAnywhere, Replicated)
    int32 Rows = 10;
    
    UPROPERTY(EditAnywhere, Replicated)
    float TileSize = 32.f;
    
    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UUserWidget> ItemWidgetClass;
    
    UPROPERTY()
    UUserWidget* ItemWidget;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    TSubclassOf<AItemActor> ItemActorSubClass;

    void ResizeItemsToGrid(bool bZeroInit);

    
protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    UPROPERTY()
    AFishingCharacter* CharacterReference = nullptr;
    
    // Module Instances
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
    
    UPROPERTY()
    UInventoryNetworkSync* NetworkSync;
    
    // ★★★ 레플리케이션되는 동기화 데이터 ★★★
    UPROPERTY(ReplicatedUsing=OnRep_ItemSyncData)
    TArray<FItemSyncData> ItemSyncData;


private:
    void InitializeModules();
    bool GetResultAtIndex(int32 Index);
    
    // ★★★ 서버 전용: 클라이언트에 동기화 ★★★
    void SyncToClients();
};