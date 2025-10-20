

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LeaderboardEntryWidget.generated.h"

UENUM(BlueprintType)
enum class ELeaderboardSortType : uint8
{
	BiggestLength    UMETA(DisplayName = "Biggest (Length)"),
	HeaviestWeight   UMETA(DisplayName = "Heaviest (Weight)"),
	MostCaught       UMETA(DisplayName = "Most Caught"),
	RecentCatch      UMETA(DisplayName = "Recent Catch")
};


USTRUCT(BlueprintType)
struct FLeaderboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 PlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString FishName;



	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	float Length = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	float Weight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	int32 CaughtCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	FString CaughtDate;

	
	UPROPERTY(BlueprintReadOnly, Category = "Leaderboard")
	bool bIsLocalPlayer = false;
};

class UTextBlock;
class UImage;
class UBorder;

UCLASS()
class FISHING_API ULeaderboardEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category = "Leaderboard")
	void SetEntryData(const FLeaderboardEntry& Entry);

protected:
	virtual void NativeConstruct() override;

	
	
	

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RankText;

	UPROPERTY(meta = (BindWidget))
	UImage* MedalIcon;  

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	UImage* FishIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* FishNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LengthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeightText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DateText;

	UPROPERTY(meta = (BindWidget))
	UBorder* BackgroundBorder;  

	
	
	

	UPROPERTY(EditDefaultsOnly, Category = "Leaderboard|Style")
	FLinearColor HighlightColor = FLinearColor(1.0f, 0.84f, 0.0f, 0.3f);  

	UPROPERTY(EditDefaultsOnly, Category = "Leaderboard|Style")
	FLinearColor NormalColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.5f);

private:
	void UpdateMedalIcon(int32 Rank);
	void UpdateHighlight(bool bIsLocalPlayer);
};