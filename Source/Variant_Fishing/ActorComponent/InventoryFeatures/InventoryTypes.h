#pragma once

#include "CoreMinimal.h"
#include "InventoryTypes.generated.h"

class UItemDataAsset;

USTRUCT(BlueprintType)
struct FItemSyncData
{
	GENERATED_BODY()

	// 아이템 정의
	UPROPERTY()
	UItemDataAsset* ItemDef = nullptr;

	// 고유 ID
	UPROPERTY()
	FGuid ItemGuid;

	// 회전 여부
	UPROPERTY()
	bool bIsRotated = false;

	// 그리드 상 첫 등장 위치 (TopLeft)
	UPROPERTY()
	int32 TopLeftIndex = -1;

	// 기본 생성자
	FItemSyncData()
		: ItemDef(nullptr)
		, ItemGuid()
		, bIsRotated(false)
		, TopLeftIndex(-1)
	{
	}

	// 파라미터 생성자
	FItemSyncData(UItemDataAsset* InItemDef, FGuid InGuid, bool InRotated, int32 InIndex)
		: ItemDef(InItemDef)
		, ItemGuid(InGuid)
		, bIsRotated(InRotated)
		, TopLeftIndex(InIndex)
	{
	}

	// 유효성 검사
	bool IsValid() const
	{
		return ItemDef != nullptr && ItemGuid.IsValid() && TopLeftIndex >= 0;
	}

	// 비교 연산자 (GUID 기준)
	bool operator==(const FItemSyncData& Other) const
	{
		return ItemGuid == Other.ItemGuid;
	}
};