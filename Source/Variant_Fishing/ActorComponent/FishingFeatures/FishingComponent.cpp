#include "FishingComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "FishingCharacter.h"
#include "FishingPlayerState.h"
#include "NiagaraComponent.h"
#include "SubModules/FishingAnimationModule.h"
#include "SubModules/FishingBiteModule.h"
#include "SubModules/FishingBobberModule.h"
#include "SubModules/FishingCastModule.h"
#include "SubModules/FishingInventoryModule.h"
#include "Variant_Fishing/Actor/Fish.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Variant_Fishing/Database/DatabaseManager.h"

DEFINE_LOG_CATEGORY(LogFishingComponent);

UFishingComponent::UFishingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UFishingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFishingComponent, bBobberActive);
	DOREPLIFETIME(UFishingComponent, BobberTargetLocation);
	DOREPLIFETIME(UFishingComponent, FishState);
	DOREPLIFETIME(UFishingComponent, bIsFishing);
	DOREPLIFETIME(UFishingComponent, bInBiteWindow);
	DOREPLIFETIME(UFishingComponent, CurrentDisplayFishItem);
	DOREPLIFETIME(UFishingComponent, CurrentFishingRodSocket);
}

void UFishingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	
	BobberModule = NewObject<UFishingBobberModule>(this);
	AnimationModule = NewObject<UFishingAnimationModule>(this);
	BiteModule = NewObject<UFishingBiteModule>(this);
	CastModule = NewObject<UFishingCastModule>(this);
	InventoryModule = NewObject<UFishingInventoryModule>(this);
	StateModule = NewObject<UFishingStateModule>(this);

	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		DatabaseManager = GI->GetSubsystem<UDatabaseManager>();
	}
	
	UE_LOG(LogFishingComponent, Log, TEXT("FishingComponent BeginPlay - All modules created"));
}

void UFishingComponent::Initialize(USkeletalMeshComponent* InCharacterMesh,
                                   UStaticMeshComponent* InFishingRod,
                                   UStaticMeshComponent* InBobber,
                                   UNiagaraComponent* InSplashEffect,
                                   AFishingCharacter* InOwnerCharacter)
{
	CharacterMesh = InCharacterMesh;
	FishingRod = InFishingRod;
	Bobber = InBobber;
	SplashEffect = InSplashEffect;
	OwnerCharacter = InOwnerCharacter;
	
	
	if (BobberModule)
	{
		BobberModule->Initialize(this, Bobber, SplashEffect);
		
		
		BobberModule->SetFakeBiteDipAmount(FakeBiteDipAmount);
		BobberModule->SetFakeBiteAnimSpeed(FakeBiteAnimSpeed);
		BobberModule->SetRealBiteAnimSpeed(RealBiteAnimSpeed);
		BobberModule->SetRealBiteSinkAmount(RealBiteSinkAmount);
		
		UE_LOG(LogFishingComponent, Log, TEXT("BobberModule initialized with config"));
	}
	
	
	if (AnimationModule)
	{
		AnimationModule->Initialize(this, CharacterMesh, FishingRod);
		AnimationModule->SetMontage(FishingMontage);
		AnimationModule->SetIdleSocket(IdleFishingSocket);
		AnimationModule->SetActiveSocket(ActiveFishingSocket);
		
		
		CurrentFishingRodSocket = IdleFishingSocket;
		AnimationModule->AttachToIdleSocket();
		
		UE_LOG(LogFishingComponent, Log, TEXT("AnimationModule initialized with montage and sockets"));
	}
	
	
	if (BiteModule)
	{
		BiteModule->Initialize(this);
		
		UE_LOG(LogFishingComponent, Log, TEXT("BiteModule initialized"));
	}
	
	
	if (CastModule)
	{
		CastModule->Initialize(this, OwnerCharacter);
		
		
		CastModule->SetForwardOffset(CastCheckForwardOffset);
		CastModule->SetDownOffset(CastCheckDownOffset);
		CastModule->SetMaxHeight(CastMaxHeight);
		CastModule->SetFishingWaterTag(FishingWaterTag);
		CastModule->SetShowDebug(bShowDebugCasting);
		
		UE_LOG(LogFishingComponent, Log, TEXT("CastModule initialized with config"));
	}
	
	
	if (InventoryModule)
	{
		InventoryModule->Initialize(this, OwnerCharacter, CharacterMesh);
		InventoryModule->SetItemActorClass(ItemActorSubClass);
		InventoryModule->SetShowOffSocket(ShowOffSocket);
		
		UE_LOG(LogFishingComponent, Log, TEXT("InventoryModule initialized"));
	}
	
	
	if (StateModule)
	{
		StateModule->Initialize(this, OwnerCharacter);
		
		UE_LOG(LogFishingComponent, Log, TEXT("StateModule initialized"));
	}
	
	UE_LOG(LogFishingComponent, Log, TEXT("=== FishingComponent fully initialized with all modules ==="));
}

void UFishingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	
	if (BobberModule && bBobberActive)
	{
		BobberModule->UpdateMovement(DeltaTime, FishState);
	}

	
	if (BiteModule && FishState == EFishingState::BiteFight)
	{
		BiteModule->UpdateBiteFight(DeltaTime);
	}
}



void UFishingComponent::ProcessFishingSuccess()
{
	
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("ProcessFishingSuccess: Not server authority!"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("ProcessFishingSuccess: GetWorld() returned null!"));
		return;
	}

	Server_SetState(EFishingState::PullOut);
    
	if (BiteModule)
	{
		BiteModule->SetInBiteWindow(false);
		bInBiteWindow = false;
		BiteModule->ClearTimers();
	}
    
	if (BobberModule)
	{
		BobberModule->Hide();
	}

	if (InventoryModule && BiteModule)
	{
		APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (OwnerPawn)
		{
			AController* OwnerController = OwnerPawn->GetController();
			if (OwnerController)
			{
				AFishingPlayerState* MyPlayerState = Cast<AFishingPlayerState>(OwnerController->GetPlayerState<APlayerState>());

				if (MyPlayerState)
				{
					AFish* CaughtFish = BiteModule->GetCurrentBitingFish();
					if (CaughtFish)
					{
						
						InventoryModule->CreateFishItemFromCatch(CaughtFish, MyPlayerState->DatabasePlayerID);
            
						BiteModule->ClearCurrentBitingFish();
					}
				}
			}
		}
	}
}



void UFishingComponent::OnPrimaryInput()
{
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("OnPrimaryInput! IsFishing=%d State=%s"),
	       bIsFishing, UFishingStateModule::StateToString(FishState));

	Server_RequestPrimary();
}

bool UFishingComponent::IsFishing() const
{
	return StateModule ? StateModule->IsFishing() : bIsFishing;
}

bool UFishingComponent::IsInBiteWindow() const
{
	return BiteModule ? BiteModule->IsInBiteWindow() : bInBiteWindow;
}

EFishingState UFishingComponent::GetFishState() const
{
	return StateModule ? StateModule->GetState() : FishState;
}

AFish* UFishingComponent::GetCurrentBitingFish() const
{
	return BiteModule ? BiteModule->GetCurrentBitingFish() : nullptr;
}

void UFishingComponent::OnFishBite(AFish* Fish)
{
	if (BiteModule)
	{
		BiteModule->OnFishBite(Fish);
	}
}

void UFishingComponent::OnFishFakeBite(AFish* Fish, float FakeBiteTime)
{
	if (BiteModule)
	{
		BiteModule->OnFishFakeBite(Fish, FakeBiteTime);
	}
}



void UFishingComponent::OnAnimNotify_CastEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	if (StateModule)
	{
		StateModule->EnterFishing();
	}
}

void UFishingComponent::OnAnimNotify_PullOutEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (CurrentDisplayFishItem)
	{
		Server_SetState(EFishingState::ShowOff);
		OwnerCharacter->SwitchToShowOffCamera(true);
		Multicast_ShowFishItem();
	}
	else
	{
		Server_SetState(EFishingState::Exit);
	}
}

void UFishingComponent::OnAnimNotify_TakingOffEnd()
{
	if (StateModule)
	{
		StateModule->ExitFishing();
	} else
	{
		UE_LOG(LogFishingComponent, Log, TEXT("OnAnimNotify_TakingOffEnd StateModule is not set"));
	}
}

void UFishingComponent::OnAnimNotify_ShowOffEnd()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	OwnerCharacter->SwitchToShowOffCamera(false);
	
	if (InventoryModule)
	{
		InventoryModule->AddFishToInventory();
	}
	
	Server_SetState(EFishingState::Exit);
}



void UFishingComponent::Server_RequestPrimary_Implementation()
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (!StateModule)
	{
		return;
	}

	if (!bIsFishing)
	{
		Server_SetState(EFishingState::Casting);
		StateModule->LockMovement(true);
		return;
	}

	switch (FishState)
	{
	case EFishingState::Idle:
		if (BobberModule)
		{
			BobberModule->Hide();
		}
		Server_SetState(EFishingState::Exit);
		break;

	case EFishingState::BiteWindow:
		if (BiteModule)
		{
			BiteModule->HandleBiteWindow();
		}
		break;

	default:
		break;
	}
}

void UFishingComponent::Server_SetState_Implementation(EFishingState NewState)
{
	const EFishingState OldState = FishState;
	FishState = NewState;
	
	if (StateModule)
	{
		StateModule->SetState(NewState);
	}

	OnStateChanged.Broadcast(OldState, NewState);

	switch (NewState)
	{
	case EFishingState::Casting:
		Multicast_PlayFishingAnimation(Sec_Cast, true);
		break;

	case EFishingState::Idle:
		Multicast_PlayFishingAnimation(Sec_Idle, true);
		if (BobberModule)
		{
			BobberModule->SetState(EBobberState::Idle);
			BobberModule->ResetAnimation();
		}
		break;

	case EFishingState::BiteWindow:
		Multicast_PlayFishingAnimation(Sec_Bite, true);
		break;

	case EFishingState::BiteFight:
		Multicast_PlayFishingAnimation(Sec_FishFight, true);
		if (BiteModule)
		{
			BiteModule->InitBiteFight();
		}
		break;

	case EFishingState::PullOut:
		Multicast_PlayFishingAnimation(Sec_PullOut, true);
		break;

	case EFishingState::ShowOff:
		if (CurrentDisplayFishItem)
		{
			CurrentDisplayFishItem->SetHidden(false);
		}
		Multicast_PlayFishingAnimation(Sec_ShowOff, false);
		break;

	case EFishingState::Exit:
		Multicast_PlayFishingAnimation(Sec_TakingOff, false);
		break;

	case EFishingState::None:
		if (AnimationModule)
		{
			AnimationModule->AttachToIdleSocket();
			AnimationModule->StopMontage();
		}
		break;
	}

	OnRep_FishState();
}



void UFishingComponent::Multicast_PlayFishingAnimation_Implementation(FName SectionName, bool bPositionRotHand)
{
	UE_LOG(LogFishingComponent, Log, TEXT("Multicast Play Anim Called: %s, HandPos=%d"),
	       *SectionName.ToString(), bPositionRotHand);
	if (AnimationModule)
	{
		AnimationModule->PlayWithHandPosition(SectionName, bPositionRotHand);
	}
}

void UFishingComponent::Multicast_ShowFishItem_Implementation()
{
	if (InventoryModule && CurrentDisplayFishItem)
	{
		InventoryModule->AttachAndRevealShowOffItem(ShowOffSocket);
	}
}



void UFishingComponent::OnRep_CurrentSocket()
{
	if (AnimationModule)
	{
		AnimationModule->UpdateFishingRodSocket(CurrentFishingRodSocket);
	}
}

void UFishingComponent::OnRep_CurrentDisplayFishItem()
{
	if (CurrentDisplayFishItem && InventoryModule)
	{
		InventoryModule->AttachAndRevealShowOffItem(ShowOffSocket);
	}
}

void UFishingComponent::OnRep_FishState()
{
	UE_LOG(LogFishingComponent, Log, TEXT("State -> %s"), 
	       UFishingStateModule::StateToString(FishState));
	
	if (FishState == EFishingState::ShowOff && CurrentDisplayFishItem && InventoryModule)
	{
		InventoryModule->AttachAndRevealShowOffItem(ShowOffSocket);
	}
	
	if (OwnerCharacter)
	{
		OwnerCharacter->OnFishingStateChanged(EFishingState::None, FishState);
	}
}

void UFishingComponent::OnRep_BobberActive()
{
	if (BobberModule)
	{
		if (bBobberActive)
		{
			BobberModule->Show(BobberTargetLocation);
		}
		else
		{
			BobberModule->Hide();
		}
	}
}

void UFishingComponent::OnRep_BobberTarget()
{
	if (BobberModule && bBobberActive)
	{
		BobberModule->SetTargetLocation(BobberTargetLocation);
	}
}