#include "FishingStateModule.h"
#include "FishingCharacter.h"
#include "FishingBobberModule.h"
#include "FishingBiteModule.h"
#include "FishingCastModule.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Variant_Fishing/ActorComponent/FishingComponent.h"

void UFishingStateModule::Initialize(UFishingComponent* InOwner, AFishingCharacter* InCharacter)
{
	OwnerComponent = InOwner;
	OwnerCharacter = InCharacter;
	
	UE_LOG(LogFishingComponent, Log, TEXT("StateModule initialized"));
}

void UFishingStateModule::SetState(EFishingState NewState)
{
	const EFishingState OldState = FishState;
	FishState = NewState;
	
	// Update parent component's replicated state
	if (OwnerComponent)
	{
		OwnerComponent->FishState = NewState;
	}
	
	// Handle state entry logic
	OnStateEntered(NewState);
	
	UE_LOG(LogFishingComponent, Log, TEXT("State changed: %s -> %s"), 
	       StateToString(OldState), StateToString(NewState));
}

const TCHAR* UFishingStateModule::StateToString(EFishingState State)
{
	switch (State)
	{
	case EFishingState::None: return TEXT("None");
	case EFishingState::Casting: return TEXT("Casting");
	case EFishingState::Idle: return TEXT("Idle");
	case EFishingState::BiteWindow: return TEXT("BiteWindow");
	case EFishingState::BiteFight: return TEXT("BiteFight");
	case EFishingState::PullOut: return TEXT("PullOut");
	case EFishingState::ShowOff: return TEXT("ShowOff");
	case EFishingState::Losing: return TEXT("Losing");
	case EFishingState::Exit: return TEXT("Exit");
	default: return TEXT("Unknown");
	}
}

void UFishingStateModule::EnterFishing()
{
	if (!OwnerComponent || !OwnerCharacter)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("EnterFishing: Missing required components!"));
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("EnterFishing: GetWorld() returned null!"));
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("EnterFishing: Started!"));

	// Validate casting location
	FVector TargetLocation = FVector::ZeroVector;
	if (OwnerComponent->CastModule && !OwnerComponent->CastModule->CheckCastingCollision(TargetLocation))
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Casting blocked by collision!"));
		SetState(EFishingState::Exit);
		
		if (OwnerComponent->BobberModule)
		{
			OwnerComponent->BobberModule->Hide();
		}
		return;
	}

	bIsFishing = true;
	
	// Update parent component's replicated state
	if (OwnerComponent)
	{
		OwnerComponent->bIsFishing = true;
		OwnerComponent->BobberTargetLocation = TargetLocation;
		
		if (OwnerComponent->BiteModule)
		{
			OwnerComponent->BiteModule->SetInBiteWindow(false);
		}
	}

	// Show bobber at target location
	if (OwnerComponent->BobberModule)
	{
		OwnerComponent->BobberModule->Show(TargetLocation);
	}
	
	SetState(EFishingState::Idle);
	
	UE_LOG(LogFishingComponent, Log, TEXT("Entered fishing - waiting for fish at %s"), 
	       *TargetLocation.ToString());
}

void UFishingStateModule::ExitFishing()
{
	if (!OwnerComponent)
	{
		return;
	}
	
	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("ExitFishing: GetWorld() returned null!"));
		return;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("ExitFishing: Cleaning up fishing session"));

	// Clear all timers
	if (OwnerComponent->BiteModule)
	{
		OwnerComponent->BiteModule->ClearTimers();
		OwnerComponent->BiteModule->SetInBiteWindow(false);
		
		// Clear current biting fish
		AFish* CurrentFish = OwnerComponent->BiteModule->GetCurrentBitingFish();
		if (CurrentFish)
		{
			OwnerComponent->BiteModule->SetCurrentBitingFish(nullptr);
		}
	}

	bIsFishing = false;
	
	// Update parent component's replicated state
	if (OwnerComponent)
	{
		OwnerComponent->bIsFishing = false;
		OwnerComponent->bInBiteWindow = false;
	}

	SetState(EFishingState::None);

	// Unlock player movement
	LockMovement(false);
	
	// Hide bobber
	if (OwnerComponent->BobberModule)
	{
		OwnerComponent->BobberModule->Hide();
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Fishing session ended"));
}

void UFishingStateModule::LockMovement(bool bLock)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement())
	{
		if (bLock)
		{
			Move->StopMovementImmediately();
			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				PC->SetIgnoreMoveInput(true);
			}
		}
		else
		{
			Move->SetMovementMode(MOVE_Walking);
			if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
			{
				PC->SetIgnoreMoveInput(false);
			}
		}
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Movement locked: %d"), bLock);
}

void UFishingStateModule::OnStateEntered(EFishingState NewState)
{
	// State-specific entry logic can be handled here
	// Most animation and visual logic is handled in FishingComponent::Server_SetState
	// This is more for state-specific setup that doesn't involve animations
	
	switch (NewState)
	{
	case EFishingState::None:
		// Ensure movement is unlocked
		LockMovement(false);
		break;
		
	case EFishingState::Casting:
		// Movement should be locked during casting
		LockMovement(true);
		break;
		
	case EFishingState::Exit:
		ExitFishing();
		break;
		
	default:
		break;
	}
}