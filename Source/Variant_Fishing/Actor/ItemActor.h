#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Fishing/ActorComponent/InventoryFeatures/InventoryComponent.h"
#include "Variant_Fishing/Interface/Interactable.h"
#include "ItemActor.generated.h"

class UItemDataAsset;
class UItemBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class FISHING_API AItemActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Mesh;

	FString GetNameFromItem() const;
	virtual FString GetName_Implementation() const override;
	virtual EInteractionType GetType_Implementation() const override;

	AItemActor();
	void InitFromItem(const UItemBase* Item);
	void ApplyFromDataAsset();
	void InitPhysicsSetting();
	UItemDataAsset* GetItemData() { return ItemDef; }
	UItemBase* MakeItemInstance(UInventoryComponent* InventoryComponent, AActor* OwnerRef);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Base|Inventory")
	UItemDataAsset* ItemDef = nullptr;

protected:
	virtual void BeginPlay() override;
};
