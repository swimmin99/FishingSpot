


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Interface/Interactable.h"
#include "Variant_Fishing/Interface/ItemDataProvider.h"
#include "ItemActor.generated.h"

class UItemBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class FISHING_API AItemActor : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    AItemActor();
    
    
    
    
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    
    
    
    
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", Replicated)
    TScriptInterface<IItemDataProvider> ItemDataProvider;
    
    
    UPROPERTY(BlueprintReadOnly, Category="Item")
    TWeakObjectPtr<UItemBase> ItemReference = nullptr;

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Item")
    void InitializeFromItem(UItemBase* Item);
    
    
	UFUNCTION(BlueprintCallable, Category = "Item")
    void ApplyFromDataProvider();
	
    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Item")
    UItemBase* MakeItemInstance(UInventoryComponent* InventoryComponent, AActor* OwnerRef);

    
    
    
    
    
    UFUNCTION(BlueprintCallable, Category = "Item")
    void PrepareForPickup();

    
    
    
    
    virtual FString GetInteractableName_Implementation() const override;
    virtual EInteractionType GetType_Implementation() const override;
    
    FString GetNameFromItem() const;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};