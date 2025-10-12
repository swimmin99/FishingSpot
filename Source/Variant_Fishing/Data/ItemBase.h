#pragma once
#include "CoreMinimal.h"
#include "ItemDataAsset.h"
#include "UObject/Object.h"
#include "ItemBase.generated.h"

class UItemDataAsset;
class AItemActor;

UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class FISHING_API UItemBase : public UObject
{
	GENERATED_BODY()

public:
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual bool IsNameStableForNetworking() const override { return true; }

	UFUNCTION(BlueprintCallable, Category="Item")
	bool Initialize(UItemDataAsset* InData);

	UFUNCTION(BlueprintCallable, Category="Item")
	FIntPoint GetCurrentDimensions() const
	{
		if (!ItemDef)
		{
			return FIntPoint::ZeroValue;
		}
		return bIsRotated ? FIntPoint(ItemDef->BaseDimensions.Y, ItemDef->BaseDimensions.X) : ItemDef->BaseDimensions;
	}

	UFUNCTION(BlueprintCallable, Category="Item")
	bool GetIsRotated() const { return bIsRotated; }

	UFUNCTION(BlueprintCallable, Category="Item")
	void RotateItem() { bIsRotated = !bIsRotated; }

	AItemActor* SpawnItemActor(UWorld* World, const FTransform& Transform, const TSubclassOf<AItemActor> ActorClass,
	                           AActor* Owner);

	FString DisplayName() const { return ItemDef ? ItemDef->DisplayName.ToString() : TEXT("Unknown Item"); }
	FString GetPriceString() const { return ItemDef ? FString::FromInt(ItemDef->Price) : TEXT("Unknown Item"); }

	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 GetPrice() const { return ItemDef ? ItemDef->Price : 0; }

	FString Description() const { return ItemDef ? ItemDef->Description.ToString() : TEXT("No Description"); }
	UMaterialInterface* GetIcon() { return ItemDef ? ItemDef->Icon : nullptr; }

	UPROPERTY(Replicated)
	FGuid ItemGuid;
	UPROPERTY(Replicated)
	UItemDataAsset* ItemDef = nullptr;
	UPROPERTY(Replicated)
	bool bIsRotated = false;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
