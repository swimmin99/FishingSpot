
#include "LeaderboardWidget.h"
#include "FishingGameState.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "LeaderboardEntryWidget.h"
#include "Components/SlateWrapperTypes.h" 
#include "Variant_Fishing/Widget/Inventory/CategoryFilterButton.h"
#include "Variant_Fishing/Database/DatabaseManager.h"


DEFINE_LOG_CATEGORY_STATIC(LogLeaderboard, Log, All);

static const TCHAR* SortTypeToString(ELeaderboardSortType T)
{
	switch (T)
	{
	case ELeaderboardSortType::BiggestLength:  return TEXT("BiggestLength");
	case ELeaderboardSortType::HeaviestWeight: return TEXT("HeaviestWeight");
	case ELeaderboardSortType::MostCaught:     return TEXT("MostCaught");
	default:                                    return TEXT("Rank(Default)");
	}
}


void ULeaderboardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogLeaderboard, Log, TEXT("NativeConstruct: Begin"));

	DatabaseManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDatabaseManager>() : nullptr;
	if (!DatabaseManager)
	{
		UE_LOG(LogLeaderboard, Error, TEXT("NativeConstruct: DatabaseManager is null"));
	}
	else
	{
		UE_LOG(LogLeaderboard, Log, TEXT("NativeConstruct: DatabaseManager acquired"));
	}

	
	CreateSortButtons();

	
	RefreshLeaderboard();

	UE_LOG(LogLeaderboard, Log, TEXT("NativeConstruct: End"));
}

void ULeaderboardWidget::ToggleMenu()
{
	const UEnum* VisEnum = StaticEnum<ESlateVisibility>();
	const FString VisStr = VisEnum
		? VisEnum->GetNameStringByValue(static_cast<int64>(GetVisibility()))
		: FString::Printf(TEXT("<Unknown>(%d)"), static_cast<int32>(GetVisibility()));

	UE_LOG(LogLeaderboard, Log, TEXT("ToggleMenu: Current Visibility=%s"), *VisStr);

	if (GetVisibility() == ESlateVisibility::Visible)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogLeaderboard, Log, TEXT("ToggleMenu: -> Collapsed"));
	}
	else
	{
		SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogLeaderboard, Log, TEXT("ToggleMenu: -> Visible, RefreshLeaderboard"));
		RefreshLeaderboard();
	}
}

bool ULeaderboardWidget::IsOpen()
{
	const bool bOpen = (GetVisibility() == ESlateVisibility::Visible);
	UE_LOG(LogLeaderboard, Verbose, TEXT("IsOpen -> %s"), bOpen ? TEXT("true") : TEXT("false"));
	return bOpen;
}

void ULeaderboardWidget::CreateSortButtons()
{
	UE_LOG(LogLeaderboard, Log, TEXT("CreateSortButtons: Begin"));

	if (!SortButtonContainer)
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("CreateSortButtons: SortButtonContainer is null"));
		return;
	}
	if (!SortButtonWidgetClass)
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("CreateSortButtons: SortButtonWidgetClass is null"));
		return;
	}

	SortButtonContainer->ClearChildren();
	SortButtons.Empty();

	struct FSortButtonInfo
	{
		FString ID;
		FText DisplayText;
		ELeaderboardSortType SortType;
	};

	TArray<FSortButtonInfo> ButtonInfos = {
		{ TEXT("Biggest"),    FText::FromString(TEXT("🏆 Biggest")),     ELeaderboardSortType::BiggestLength },
		{ TEXT("Heaviest"),   FText::FromString(TEXT("⚖️ Heaviest")),   ELeaderboardSortType::HeaviestWeight },
		{ TEXT("MostCaught"), FText::FromString(TEXT("🎣 Most Caught")), ELeaderboardSortType::MostCaught }
	};

	int32 Created = 0;
	for (const FSortButtonInfo& Info : ButtonInfos)
	{
		UCategoryFilterButton* SortButton = CreateWidget<UCategoryFilterButton>(GetWorld(), SortButtonWidgetClass);
		if (!SortButton)
		{
			UE_LOG(LogLeaderboard, Warning, TEXT("CreateSortButtons: Failed to create button '%s'"), *Info.ID);
			continue;
		}

		const bool bIsSelected = (Info.SortType == CurrentSortType);
		SortButton->InitializeButton(Info.ID, Info.DisplayText, bIsSelected);
		SortButton->OnFilterSelected.AddDynamic(this, &ULeaderboardWidget::OnSortButtonSelected);

		SortButtonContainer->AddChild(SortButton);
		SortButtons.Add(SortButton);
		++Created;

		UE_LOG(LogLeaderboard, Log, TEXT("CreateSortButtons: Added '%s' (Selected=%s, Sort=%s)"),
			*Info.ID, bIsSelected ? TEXT("true") : TEXT("false"), SortTypeToString(Info.SortType));
	}

	UE_LOG(LogLeaderboard, Log, TEXT("CreateSortButtons: End (Created=%d)"), Created);
}

void ULeaderboardWidget::OnSortButtonSelected(FString ButtonID)
{
	UE_LOG(LogLeaderboard, Log, TEXT("OnSortButtonSelected: ButtonID=%s (PrevSort=%s)"),
		*ButtonID, SortTypeToString(CurrentSortType));

	
	if (ButtonID == TEXT("Biggest"))
	{
		CurrentSortType = ELeaderboardSortType::BiggestLength;
	}
	else if (ButtonID == TEXT("Heaviest"))
	{
		CurrentSortType = ELeaderboardSortType::HeaviestWeight;
	}
	else if (ButtonID == TEXT("MostCaught"))
	{
		CurrentSortType = ELeaderboardSortType::MostCaught;
	}
	else
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("OnSortButtonSelected: Unknown ButtonID=%s (keeping PrevSort)"), *ButtonID);
	}

	UE_LOG(LogLeaderboard, Log, TEXT("OnSortButtonSelected: NewSort=%s"), SortTypeToString(CurrentSortType));

	
	for (UCategoryFilterButton* Button : SortButtons)
	{
		if (Button)
		{
			const bool bSelect = (Button->GetButtonID() == ButtonID);
			Button->SetSelected(bSelect);
			UE_LOG(LogLeaderboard, Verbose, TEXT("OnSortButtonSelected: Button '%s' Selected=%s"),
				*Button->GetButtonID(), bSelect ? TEXT("true") : TEXT("false"));
		}
	}

	
	PopulateEntryList();
}

void ULeaderboardWidget::RefreshLeaderboard()
{
	UE_LOG(LogLeaderboard, Log, TEXT("RefreshLeaderboard: Begin"));

	if (!DatabaseManager)
	{
		UE_LOG(LogLeaderboard, Error, TEXT("RefreshLeaderboard: DatabaseManager is null!"));
		return;
	}

	
	AFishingGameState* FishingGS = GetFishingGameState();
	if (!FishingGS)
	{
		UE_LOG(LogLeaderboard, Error, TEXT("RefreshLeaderboard: FishingGameState not found!"));
		return;
	}

	const int32 HostPlayerID = FishingGS->GetHostPlayerID();
	if (HostPlayerID == -1)
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("RefreshLeaderboard: No HostPlayerID set!"));
		return;
	}

	UE_LOG(LogLeaderboard, Log, TEXT("RefreshLeaderboard: Using HostPlayerID=%d"), HostPlayerID);

	
	CachedEntries = DatabaseManager->GetSessionLeaderboard(HostPlayerID);
	UE_LOG(LogLeaderboard, Log, TEXT("RefreshLeaderboard: Loaded %d entries from DB"), CachedEntries.Num());

	
	const int32 LocalPlayerID = DatabaseManager->GetActivePlayerID();
	for (FLeaderboardEntry& Entry : CachedEntries)
	{
		Entry.bIsLocalPlayer = (Entry.PlayerID == LocalPlayerID);
	}

	UE_LOG(LogLeaderboard, Verbose, TEXT("RefreshLeaderboard: LocalPlayerID=%d (Marked flags where applicable)"), LocalPlayerID);

	PopulateEntryList();
	UpdateStatistics();

	UE_LOG(LogLeaderboard, Log, TEXT("RefreshLeaderboard: End"));
}

AFishingGameState* ULeaderboardWidget::GetFishingGameState() const
{
	if (UWorld* World = GetWorld())
	{
		AFishingGameState* GS = Cast<AFishingGameState>(World->GetGameState());
		UE_LOG(LogLeaderboard, Verbose, TEXT("GetFishingGameState: %s"), GS ? *GS->GetName() : TEXT("null"));
		return GS;
	}
	UE_LOG(LogLeaderboard, Warning, TEXT("GetFishingGameState: World is null"));
	return nullptr;
}

void ULeaderboardWidget::PopulateEntryList()
{
	UE_LOG(LogLeaderboard, Log, TEXT("PopulateEntryList: Begin (Cached=%d, CurrentSort=%s, MaxShow=%d)"),
		CachedEntries.Num(), SortTypeToString(CurrentSortType), MaxEntriesToShow);

	if (!EntryList)
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("PopulateEntryList: EntryList is null"));
		return;
	}
	if (!EntryWidgetClass)
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("PopulateEntryList: EntryWidgetClass is null"));
		return;
	}

	EntryList->ClearChildren();

	
	CachedEntries.Sort([this](const FLeaderboardEntry& A, const FLeaderboardEntry& B)
	{
		switch (CurrentSortType)
		{
		case ELeaderboardSortType::BiggestLength:  return A.Length > B.Length;
		case ELeaderboardSortType::HeaviestWeight: return A.Weight > B.Weight;
		case ELeaderboardSortType::MostCaught:     return A.CaughtCount > B.CaughtCount;
		default:                                    return A.Rank < B.Rank;
		}
	});
	UE_LOG(LogLeaderboard, Verbose, TEXT("PopulateEntryList: Sorting done (%s)"), SortTypeToString(CurrentSortType));

	
	for (int32 i = 0; i < CachedEntries.Num(); i++)
	{
		CachedEntries[i].Rank = i + 1;
	}
	UE_LOG(LogLeaderboard, Verbose, TEXT("PopulateEntryList: Ranks re-assigned (TopRank=%d)"), CachedEntries.Num() > 0 ? CachedEntries[0].Rank : 0);

	
	const int32 Count = FMath::Min(CachedEntries.Num(), MaxEntriesToShow);
	int32 Spawned = 0;
	for (int32 i = 0; i < Count; i++)
	{
		ULeaderboardEntryWidget* EntryWidget = CreateWidget<ULeaderboardEntryWidget>(this, EntryWidgetClass);
		if (EntryWidget)
		{
			EntryWidget->SetEntryData(CachedEntries[i]);
			EntryList->AddChild(EntryWidget);
			++Spawned;

			if (i < 3) 
			{
				UE_LOG(LogLeaderboard, Verbose, TEXT("PopulateEntryList: #%d PID=%d Name=%s Len=%.2f W=%.2f C=%d Local=%s"),
					CachedEntries[i].Rank,
					CachedEntries[i].PlayerID,
					*CachedEntries[i].PlayerName,
					CachedEntries[i].Length,
					CachedEntries[i].Weight,
					CachedEntries[i].CaughtCount,
					CachedEntries[i].bIsLocalPlayer ? TEXT("true") : TEXT("false"));
			}
		}
		else
		{
			UE_LOG(LogLeaderboard, Warning, TEXT("PopulateEntryList: Failed to create EntryWidget at i=%d"), i);
		}
	}

	UE_LOG(LogLeaderboard, Log, TEXT("PopulateEntryList: Displayed %d / %d entries"), Spawned, Count);
}

void ULeaderboardWidget::UpdateStatistics()
{
	UE_LOG(LogLeaderboard, Verbose, TEXT("UpdateStatistics: Begin (Cached=%d)"), CachedEntries.Num());

	if (TotalEntriesText)
	{
		TotalEntriesText->SetText(FText::FromString(FString::Printf(TEXT("Total: %d records"), CachedEntries.Num())));
	}
	else
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("UpdateStatistics: TotalEntriesText is null"));
	}

	TSet<int32> UniquePlayers;
	for (const FLeaderboardEntry& Entry : CachedEntries)
	{
		UniquePlayers.Add(Entry.PlayerID);
	}

	if (TotalPlayersText)
	{
		TotalPlayersText->SetText(FText::FromString(FString::Printf(TEXT("%d players"), UniquePlayers.Num())));
	}
	else
	{
		UE_LOG(LogLeaderboard, Warning, TEXT("UpdateStatistics: TotalPlayersText is null"));
	}

	UE_LOG(LogLeaderboard, Log, TEXT("UpdateStatistics: Players=%d, Records=%d"),
		UniquePlayers.Num(), CachedEntries.Num());
}
