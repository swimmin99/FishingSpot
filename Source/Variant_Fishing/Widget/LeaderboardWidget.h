
#pragma once

#include "CoreMinimal.h"
#include "LeaderboardEntryWidget.h"
#include "Blueprint/UserWidget.h"
#include "LeaderboardWidget.generated.h"

class UScrollBox;
class ULeaderboardEntryWidget;
class UDatabaseManager;
class UHorizontalBox;
class UCategoryFilterButton;
class AFishingGameState; 


UCLASS()
class FISHING_API ULeaderboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Leaderboard")
    void RefreshLeaderboard();
    
    AFishingGameState* GetFishingGameState() const;

    void ToggleMenu();
    bool IsOpen();

protected:
    
    
    

    UPROPERTY(meta = (BindWidget))
    UScrollBox* EntryList;

    
    UPROPERTY(meta = (BindWidget))
    UHorizontalBox* SortButtonContainer;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TotalEntriesText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TotalPlayersText;

    
    
    

    UPROPERTY(EditDefaultsOnly, Category = "Leaderboard")
    TSubclassOf<ULeaderboardEntryWidget> EntryWidgetClass;

    
    UPROPERTY(EditDefaultsOnly, Category = "Leaderboard")
    TSubclassOf<UCategoryFilterButton> SortButtonWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Leaderboard")
    int32 MaxEntriesToShow = 100;

    
    
    

    UPROPERTY()
    ELeaderboardSortType CurrentSortType = ELeaderboardSortType::BiggestLength;

private:
    void CreateSortButtons();

    UFUNCTION()
    void OnSortButtonSelected(FString ButtonID);

    void PopulateEntryList();
    void UpdateStatistics();

    UPROPERTY()
    UDatabaseManager* DatabaseManager;

    UPROPERTY()
    TArray<UCategoryFilterButton*> SortButtons;

    TArray<FLeaderboardEntry> CachedEntries;
};