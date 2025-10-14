// ============================================
// InventoryGridManager.h
// ============================================
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InventoryGridManager.generated.h"


/**
 * Manages grid layout, dimensions, and coordinate conversions
 */
UCLASS()
class FISHING_API UInventoryGridManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(int32 InColumns, int32 InRows, float InTileSize);
    void UpdateDimensions(int32 InColumns, int32 InRows);
    
    FIntPoint IndexToTile(int32 Index) const;
    int32 TileToIndex(FIntPoint Tile) const;
    bool IsTileValid(FIntPoint Tile) const;
    bool IsIndexValid(int32 Index) const;
    
    int32 GetTotalTiles() const { return Columns * Rows; }
    int32 GetColumns() const { return Columns; }
    int32 GetRows() const { return Rows; }
    float GetTileSize() const { return TileSize; }
    
    FString DumpGridLayout() const;
    FString TileToString(const FIntPoint& Tile) const;

private:
    int32 Columns = 10;
    int32 Rows = 10;
    float TileSize = 32.f;
};
