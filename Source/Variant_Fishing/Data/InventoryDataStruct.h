#pragma once

#include "CoreMinimal.h"
#include "InventoryDataStruct.generated.h"

USTRUCT()
struct FLines
{
	GENERATED_USTRUCT_BODY()
	;

	FLines()
	{
	};
	TArray<FVector2d> XLines;
	TArray<FVector2d> YLines;
};

USTRUCT()
struct FMousePositionInTile
{
	GENERATED_USTRUCT_BODY()
	;

	FMousePositionInTile()
	{
	};
	bool Right;
	bool Down;
};
