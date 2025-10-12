#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "ItemDragOperation.generated.h"

class UItemBase;
class UItemWidget;
class UInventoryComponent;

UCLASS()
class FISHING_API UItemDragOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UItemBase* Item = nullptr;

	UPROPERTY()
	UItemWidget* GhostItemWidget = nullptr;

	UPROPERTY()
	UInventoryComponent* SourceInventoryComponent = nullptr;

	FIntPoint OriginalTopLeftTile = FIntPoint::ZeroValue;
};
