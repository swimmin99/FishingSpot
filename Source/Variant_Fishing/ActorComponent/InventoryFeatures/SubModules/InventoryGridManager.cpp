#include "InventoryGridManager.h"






#include "InventoryGridManager.h"

#include "Fishing.h"

void UInventoryGridManager::Initialize(int32 InColumns, int32 InRows)
{
    Columns = FMath::Max(1, InColumns);
    Rows = FMath::Max(1, InRows);
    
    UE_LOG(LogInventoryGrid, Log, TEXT("Initialize: Grid %dx%d, TileSize=%.1f"), 
           Columns, Rows, TileSize);
}

void UInventoryGridManager::UpdateDimensions(int32 InColumns, int32 InRows)
{
    Columns = FMath::Max(1, InColumns);
    Rows = FMath::Max(1, InRows);
    
    UE_LOG(LogInventoryGrid, Log, TEXT("UpdateDimensions: Grid now %dx%d"), 
           Columns, Rows);
}


FIntPoint UInventoryGridManager::IndexToTile(int32 Index) const
{
    if (Columns <= 0)
    {
        UE_LOG(LogInventoryGrid, Error, TEXT("IndexToTile: Invalid Columns=%d"), Columns);
        return FIntPoint::ZeroValue;
    }
    
    return FIntPoint(Index % Columns, Index / Columns);
}

int32 UInventoryGridManager::TileToIndex(FIntPoint Tile) const
{
    return Tile.Y * Columns + Tile.X;
}

bool UInventoryGridManager::IsTileValid(FIntPoint Tile) const
{
    const bool bValid = (Tile.X >= 0 && Tile.X < Columns && 
                         Tile.Y >= 0 && Tile.Y < Rows);
    
#if !(UE_BUILD_SHIPPING)
    if (!bValid)
    {
        UE_LOG(LogInventoryGrid, Verbose, TEXT("IsTileValid: false for %s (Grid=%dx%d)"),
               *TileToString(Tile), Columns, Rows);
    }
#endif
    
    return bValid;
}

bool UInventoryGridManager::IsIndexValid(int32 Index) const
{
    return Index >= 0 && Index < GetTotalTiles();
}

FString UInventoryGridManager::DumpGridLayout() const
{
    return FString::Printf(TEXT("Grid %dx%d (Total: %d tiles, TileSize=%.1f)"),
                          Columns, Rows, GetTotalTiles(), TileSize);
}

FString UInventoryGridManager::TileToString(const FIntPoint& Tile) const
{
    return FString::Printf(TEXT("(%d,%d)"), Tile.X, Tile.Y);
}