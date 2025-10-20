
#include "TooltipSubsystem.h"
#include "Variant_Fishing/Widget/ToolTipWidget.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Fishing.h"

DEFINE_LOG_CATEGORY_STATIC(LogTooltipSubsystem, Log, All);

void UTooltipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTooltipSubsystem, Log, TEXT("TooltipSubsystem initialized for World: %s"), 
		*GetWorld()->GetName());

	if (!TooltipWidgetClass)
	{
		FSoftClassPath WidgetClassPath(TEXT("/Game/FishingGame/UI/Inventory/InGame/BPW_ToolTip.BPW_ToolTip_C"));
		TooltipWidgetClass = WidgetClassPath.TryLoadClass<UToolTipWidget>();
		
		if (TooltipWidgetClass)
		{
			UE_LOG(LogTooltipSubsystem, Log, TEXT("Loaded default tooltip widget class"));
		}
		else
		{
			UE_LOG(LogTooltipSubsystem, Warning, 
				TEXT("Failed to load default tooltip widget class. Set it manually or in Project Settings."));
		}
	}
}

void UTooltipSubsystem::Deinitialize()
{
	UE_LOG(LogTooltipSubsystem, Log, TEXT("TooltipSubsystem deinitializing"));

	
	if (TooltipWidget)
	{
		if (TooltipWidget->IsInViewport())
		{
			TooltipWidget->RemoveFromParent();
		}
		TooltipWidget = nullptr;
	}
	Super::Deinitialize();
}

void UTooltipSubsystem::ShowTooltip(FToolTipData Data, FVector2D MousePos)
{
	
	
	if (!IsTooltipValid())
	{
		LazyInitializeTooltip();
	}

	if (!IsTooltipValid())
	{
		UE_LOG(LogTooltipSubsystem, Error, TEXT("ShowItemTooltip: Failed to create tooltip widget"));
		return;
	}

	TooltipWidget->SetItemToolTipData(Data);
}

void UTooltipSubsystem::UpdateTooltipPosition(FVector2D MousePos)
{
	if (!IsTooltipValid())
	{
		return;
	}

	
	if (IsTooltipVisible())
	{
		TooltipWidget->UpdateToolTipPosition(MousePos);
	}
}

void UTooltipSubsystem::HideTooltip()
{
	if (!IsTooltipValid())
	{
		return;
	}

	TooltipWidget->ClearItemToolTipData();
	
	UE_LOG(LogTooltipSubsystem, Verbose, TEXT("HideTooltip: Tooltip hidden"));
}

bool UTooltipSubsystem::IsTooltipVisible() const
{
	if (!IsTooltipValid())
	{
		return false;
	}

	return TooltipWidget->GetVisibility() == ESlateVisibility::HitTestInvisible;
}

UTooltipSubsystem* UTooltipSubsystem::Get(const UWorld* World)
{
	if (!World)
	{
		UE_LOG(LogTooltipSubsystem, Warning, TEXT("Get: World is null"));
		return nullptr;
	}

	return World->GetSubsystem<UTooltipSubsystem>();
}

void UTooltipSubsystem::SetTooltipWidgetClass(TSubclassOf<UToolTipWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		UE_LOG(LogTooltipSubsystem, Warning, TEXT("SetTooltipWidgetClass: WidgetClass is null"));
		return;
	}

	TooltipWidgetClass = WidgetClass;
	
	
	if (TooltipWidget)
	{
		if (TooltipWidget->IsInViewport())
		{
			TooltipWidget->RemoveFromParent();
		}
		TooltipWidget = nullptr;
	}

	UE_LOG(LogTooltipSubsystem, Log, TEXT("SetTooltipWidgetClass: Widget class updated"));
}

void UTooltipSubsystem::LazyInitializeTooltip()
{
	if (TooltipWidget)
	{
		return; 
	}

	if (!TooltipWidgetClass)
	{
		UE_LOG(LogTooltipSubsystem, Error, 
			TEXT("LazyInitializeTooltip: TooltipWidgetClass is not set. Call SetTooltipWidgetClass first."));
		return;
	}

	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTooltipSubsystem, Warning, 
			TEXT("LazyInitializeTooltip: PlayerController not found. Will retry later."));
		return;
	}

	
	TooltipWidget = CreateWidget<UToolTipWidget>(PC, TooltipWidgetClass);
	if (!TooltipWidget)
	{
		UE_LOG(LogTooltipSubsystem, Error, TEXT("LazyInitializeTooltip: Failed to create tooltip widget"));
		return;
	}

	
	TooltipWidget->AddToViewport(999);  	TooltipWidget->SetVisibility(ESlateVisibility::Collapsed);

	UE_LOG(LogTooltipSubsystem, Log, TEXT("LazyInitializeTooltip: Tooltip widget created and added to viewport"));
}

bool UTooltipSubsystem::IsTooltipValid() const
{
	return TooltipWidget != nullptr;
}