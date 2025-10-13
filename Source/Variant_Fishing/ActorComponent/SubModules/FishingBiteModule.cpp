#include "FishingBiteModule.h"
#include "FishingBobberModule.h"
#include "Variant_Fishing/Actor/Fish.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "FishingCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Variant_Fishing/ActorComponent/FishingComponent.h"

void UFishingBiteModule::Initialize(UFishingComponent* InOwner)
{
	OwnerComponent = InOwner;
	
	UE_LOG(LogFishingComponent, Log, TEXT("BiteModule initialized"));
}

void UFishingBiteModule::OnFishBite(AFish* Fish)
{
	if (!OwnerComponent || !OwnerComponent->OwnerCharacter)
	{
		return;
	}
	
	if (!OwnerComponent->OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	if (!Fish)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OnFishBite: GetWorld() returned null!"));
		return;
	}

	if (CurrentBitingFish)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Already have a biting fish - ignoring new bite"));
		return;
	}

	if (!OwnerComponent->bIsFishing || OwnerComponent->FishState != EFishingState::Idle)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Not in fishing idle state - ignoring bite"));
		return;
	}

	CurrentBitingFish = Fish;
	bInBiteWindow = true;
	
	// Update parent component's replicated state
	OwnerComponent->bInBiteWindow = true;
	
	// Trigger bobber real bite animation
	if (OwnerComponent->BobberModule)
	{
		OwnerComponent->BobberModule->PlayRealBite();
	}
	
	// Change state to BiteWindow
	OwnerComponent->Server_SetState(EFishingState::BiteWindow);

	if (UFishData* FishData = Fish->GetFishData())
	{
		UE_LOG(LogFishingComponent, Log, TEXT("Fish %s is REALLY biting! Will start window after bobber sinks"),
		       *FishData->FishID.ToString());
	}
}

void UFishingBiteModule::OnFishFakeBite(AFish* Fish, float FakeBiteTime)
{
	if (!OwnerComponent || !OwnerComponent->OwnerCharacter)
	{
		return;
	}
	
	if (!OwnerComponent->OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	if (!Fish)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OnFakeFishBite: GetWorld() returned null!"));
		return;
	}

	if (!OwnerComponent->bIsFishing || OwnerComponent->FishState != EFishingState::Idle)
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("Not in fishing idle state - ignoring fake bite"));
		return;
	}

	// Play fake bite animation on bobber
	if (OwnerComponent->BobberModule)
	{
		OwnerComponent->BobberModule->PlayFakeBite();
	}
	
	UE_LOG(LogFishingComponent, Log, TEXT("Fish fake bite - bobber dipping"));
}

void UFishingBiteModule::HandleBiteWindow()
{
	if (!OwnerComponent || !OwnerComponent->GetOwner() || !OwnerComponent->GetWorld())
	{
		return;
	}

	OwnerComponent->Server_SetState(EFishingState::BiteFight);
}

void UFishingBiteModule::InitBiteFight()
{
	if (!OwnerComponent || !OwnerComponent->GetOwner() || !OwnerComponent->GetWorld() || !CurrentBitingFish)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);
	bBiteFighting = true;
	BiteFightTimer = 0.f;

	BobberOrbitCenter = OwnerComponent->BobberTargetLocation;
	BobberOrbitAngle = 0.f;

	float FightDur = 3.f;
	if (UFishData* FishData = CurrentBitingFish->GetFishData())
	{
		FightDur = FishData->BiteFightingTime;
		UE_LOG(LogFishingComponent, Log, TEXT("BiteFight started (%.2fs) with %s"),
		       FightDur, *FishData->FishID.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishData null. Using default BiteFight=3.0s"));
	}

	World->GetTimerManager().SetTimer(Th_BiteFightDur, this, &UFishingBiteModule::FinishBiteFight, FightDur, false);
}

void UFishingBiteModule::UpdateBiteFight(float DeltaSeconds)
{
	if (!OwnerComponent || !OwnerComponent->OwnerCharacter || !OwnerComponent->OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	if (!CurrentBitingFish)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		return;
	}

	// Update orbit angle
	BobberOrbitAngle += BobberOrbitAngularSpeed * DeltaSeconds;

	// Update bobber position in orbit
	if (OwnerComponent->BobberModule)
	{
		OwnerComponent->BobberModule->UpdateOrbitMovement(DeltaSeconds, BobberOrbitAngle);
	}

	// Get fish length
	const float FishLength = CurrentBitingFish->GetFishLength();
	
	// Calculate angle delta based on fish length (arc length = radius × angle)
	const float AngleDelta = FishLength / BobberOrbitRadius;
	
	// Head position (current angle)
	const float HeadAngle = BobberOrbitAngle;
	const float HeadX = FMath::Cos(HeadAngle) * BobberOrbitRadius;
	const float HeadY = FMath::Sin(HeadAngle) * BobberOrbitRadius;
	FVector HeadTargetPos = BobberOrbitCenter;
	HeadTargetPos.X += HeadX;
	HeadTargetPos.Y += HeadY;
	HeadTargetPos.Z += -15.f * 0.5f; // RealBiteSinkAmount
	
	// Tail position (angle minus delta - following behind)
	const float TailAngle = BobberOrbitAngle - AngleDelta;
	const float TailX = FMath::Cos(TailAngle) * BobberOrbitRadius;
	const float TailY = FMath::Sin(TailAngle) * BobberOrbitRadius;
	FVector TailTargetPos = BobberOrbitCenter;
	TailTargetPos.X += TailX;
	TailTargetPos.Y += TailY;
	TailTargetPos.Z += -15.f * 0.5f; // RealBiteSinkAmount
	
	// Move fish to head position smoothly
	const FVector CurrentLoc = CurrentBitingFish->GetActorLocation();
	const FVector NewLoc = FMath::VInterpTo(CurrentLoc, HeadTargetPos, DeltaSeconds, 8.0f);
	CurrentBitingFish->SetActorLocation(NewLoc, false);
	
	// Rotate fish to swim direction (from tail to head)
	FVector SwimDirection = HeadTargetPos - TailTargetPos;
	SwimDirection.Z = 0.f; // Horizontal direction only
	
	if (SwimDirection.SizeSquared() > 0.01f)
	{
		SwimDirection.Normalize();
		const FRotator TargetRot = SwimDirection.Rotation();
		const FRotator NewRot = FMath::RInterpTo(
			CurrentBitingFish->GetActorRotation(),
			TargetRot,
			DeltaSeconds,
			8.0f
		);
		CurrentBitingFish->SetActorRotation(NewRot);
	}
	
	// Debug visualization
	if (OwnerComponent->bShowDebugCasting)
	{
		// Target head/tail positions
		DrawDebugSphere(World, HeadTargetPos, 10.f, 12, FColor::Orange, false, -1.f, 0, 2.f);
		DrawDebugSphere(World, TailTargetPos, 10.f, 12, FColor::Magenta, false, -1.f, 0, 2.f);
		DrawDebugLine(World, TailTargetPos, HeadTargetPos, FColor::Yellow, false, -1.f, 0, 3.f);
		
		// Orbit circle
		DrawDebugCircle(World, BobberOrbitCenter, BobberOrbitRadius, 32, FColor::Green, false, -1.f, 0, 1.f,
		                FVector(0, 1, 0), FVector(1, 0, 0), false);
		
		// Fish direction arrow
		DrawDebugDirectionalArrow(World, CurrentBitingFish->GetActorLocation(),
		                          CurrentBitingFish->GetActorLocation() + CurrentBitingFish->GetActorForwardVector() * 50.f,
		                          20.f, FColor::White, false, -1.f, 0, 3.f);
	}
}

void UFishingBiteModule::FinishBiteFight()
{
	if (!OwnerComponent || !OwnerComponent->GetOwner() || !OwnerComponent->GetWorld())
	{
		return;
	}
	
	bBiteFighting = false;

	if (OwnerComponent)
	{
		OwnerComponent->ProcessFishingSuccess();
		// This will be handled by FishingComponent::ProcessFishingSuccess
		// which creates the fish item and transitions to PullOut state
	}
}

void UFishingBiteModule::StartBiteWindowTimer()
{
	if (!OwnerComponent || !OwnerComponent->OwnerCharacter || !OwnerComponent->OwnerCharacter->HasAuthority())
	{
		return;
	}
	
	if (!CurrentBitingFish)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("StartBiteWindowTimer: GetWorld() returned null!"));
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);

	float BiteWindowDuration = 3.f;
	if (UFishData* FishData = CurrentBitingFish->GetFishData())
	{
		BiteWindowDuration = FishData->BiteWindowTime;
		UE_LOG(LogFishingComponent, Log, TEXT("BiteWindow timer started (%.2fs) for %s"),
		       BiteWindowDuration, *FishData->FishID.ToString());
	}
	else
	{
		UE_LOG(LogFishingComponent, Warning, TEXT("FishData null. Using default BiteWindow=3.0s"));
	}

	World->GetTimerManager().SetTimer(
		Th_BiteDur,
		this,
		&UFishingBiteModule::OnBiteTimeout,
		BiteWindowDuration,
		false);

	UE_LOG(LogFishingComponent, Log, TEXT("BiteWindow timer started successfully"));
}

void UFishingBiteModule::OnBiteTimeout()
{
	if (!OwnerComponent || !OwnerComponent->bIsFishing)
	{
		return;
	}

	UE_LOG(LogFishingComponent, Warning, TEXT("Bite window timeout - fish got away!"));

	bInBiteWindow = false;
	OwnerComponent->bInBiteWindow = false;

	OwnerComponent->Server_SetState(EFishingState::Exit);
	
	if (CurrentBitingFish)
	{
		CurrentBitingFish->OnEscaped();
		CurrentBitingFish = nullptr;
	}
}

void UFishingBiteModule::ClearTimers()
{
	if (!OwnerComponent)
	{
		return;
	}
	
	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(Th_BiteDur);
	World->GetTimerManager().ClearTimer(Th_BiteFightDur);
}