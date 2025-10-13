#include "FishingBobberModule.h"
#include "FishingBiteModule.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"

void UFishingBobberModule::Initialize(UFishingComponent* InOwner, 
                                      UStaticMeshComponent* InBobber, 
                                      UNiagaraComponent* InSplashEffect)
{
	OwnerComponent = InOwner;
	Bobber = InBobber;
	SplashEffect = InSplashEffect;
	
	ConfigureBobberCollision();
	
	if (SplashEffect)
	{
		SplashEffect->SetVisibility(false);
		SplashEffect->SetAutoActivate(false);
		SplashEffect->Deactivate();
		UE_LOG(LogFishingComponent, Log, TEXT("Niagara SplashEffect initialized"));
	}
}

void UFishingBobberModule::ConfigureBobberCollision()
{
	if (!Bobber)
	{
		return;
	}
	
	Bobber->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Bobber->SetCollisionObjectType(ECC_WorldDynamic);
	Bobber->SetCollisionResponseToAllChannels(ECR_Ignore);
	Bobber->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Bobber->SetNotifyRigidBodyCollision(true);
	Bobber->SetGenerateOverlapEvents(false);
	Bobber->BodyInstance.bNotifyRigidBodyCollision = true;
	Bobber->BodyInstance.bUseCCD = false;
	Bobber->SetSimulatePhysics(false);
	
	UE_LOG(LogFishingComponent, Log, TEXT("Bobber collision configured"));
}

void UFishingBobberModule::Show(const FVector& TargetLocation)
{
	if (!Bobber)
	{
		return;
	}

	BobberTargetLocation = TargetLocation;
	const FVector InitialLoc = TargetLocation;

	Bobber->SetWorldLocation(InitialLoc);
	Bobber->SetVisibility(true, true);

	if (SplashEffect)
	{
		SplashEffect->SetVisibility(true, true);
	}

	bBobberActive = true;
	BobberState = EBobberState::Idle;
	BobberAnimTimer = 0.f;
	bBobberAnimating = false;
	
	// Update parent component's replicated state
	if (OwnerComponent)
	{
		OwnerComponent->bBobberActive = true;
		OwnerComponent->BobberTargetLocation = TargetLocation;
	}

	PlaySplashEffect(0.5f);

	UE_LOG(LogFishingComponent, Log, TEXT("Bobber shown at: %s (Idle state)"), 
	       *InitialLoc.ToString());
}

void UFishingBobberModule::Hide()
{
	if (!Bobber)
	{
		return;
	}

	Bobber->SetVisibility(false, true);

	if (SplashEffect)
	{
		SplashEffect->Deactivate();
		SplashEffect->SetVisibility(false, true);
	}

	bBobberActive = false;
	BobberState = EBobberState::Hidden;
	BobberAnimTimer = 0.f;
	bBobberAnimating = false;
	
	// Update parent component's replicated state
	if (OwnerComponent)
	{
		OwnerComponent->bBobberActive = false;
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Bobber hidden"));
}

void UFishingBobberModule::PlayFakeBite()
{
	if (!bBobberActive)
	{
		return;
	}
	
	BobberState = EBobberState::FakeBite;
	BobberAnimTimer = 0.f;
	bBobberAnimating = true;
	
	UE_LOG(LogFishingComponent, Log, TEXT("Playing fake bite animation"));
}

void UFishingBobberModule::PlayRealBite()
{
	BobberState = EBobberState::RealBite;
	BobberAnimTimer = 0.f;
	bBobberAnimating = true;
	bWaitingForRealBiteAnimComplete = true;
	
	UE_LOG(LogFishingComponent, Log, TEXT("Bobber sinking for real bite"));
}

void UFishingBobberModule::StartOrbitMovement(const FVector& Center)
{
	// This would be called when bite fight starts
	// The actual orbit update happens in UpdateOrbitMovement
}

void UFishingBobberModule::UpdateMovement(float DeltaSeconds, EFishingState CurrentState)
{
	if (!Bobber || !bBobberActive)
	{
		return;
	}

	BobberAnimTimer += DeltaSeconds;

	// During bite fight, use orbit movement handled separately
	if (CurrentState == EFishingState::BiteFight)
	{
		return; // Orbit movement is handled in UpdateOrbitMovement
	}

	FVector NewLoc = BobberTargetLocation;
	float ZOffset = 0.f;

	switch (BobberState)
	{
	case EBobberState::Idle:
		ZOffset = 0.f;
		break;

	case EBobberState::FakeBite:
		if (bBobberAnimating)
		{
			const float Progress = BobberAnimTimer * FakeBiteAnimSpeed;
			if (Progress >= PI * 2.0f)
			{
				bBobberAnimating = false;
				BobberAnimTimer = 0.f;
				BobberState = EBobberState::Idle;
				ZOffset = 0.f;
			}
			else
			{
				ZOffset = FMath::Sin(Progress) * FakeBiteDipAmount;
			}
		}
		break;

	case EBobberState::RealBite:
		if (bBobberAnimating)
		{
			const float Progress = BobberAnimTimer * RealBiteAnimSpeed;

			if (Progress >= 1.0f)
			{
				bBobberAnimating = false;
				ZOffset = -RealBiteSinkAmount;

				// Notify that animation is complete
				if (bWaitingForRealBiteAnimComplete && OwnerComponent)
				{
					bWaitingForRealBiteAnimComplete = false;
					
					// Trigger bite window timer start through BiteModule
					if (OwnerComponent->BiteModule)
					{
						OwnerComponent->BiteModule->StartBiteWindowTimer();
					}

					UE_LOG(LogFishingComponent, Log, 
					       TEXT("RealBite animation complete - BiteWindow timer started"));
				}
			}
			else
			{
				// Ease out cubic for smooth deceleration
				const float EasedProgress = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
				ZOffset = -RealBiteSinkAmount * EasedProgress;
			}
		}
		else
		{
			ZOffset = -RealBiteSinkAmount;
		}
		break;

	case EBobberState::Hidden:
	default:
		return;
	}

	NewLoc.Z += ZOffset;
	Bobber->SetWorldLocation(NewLoc);
}

void UFishingBobberModule::UpdateOrbitMovement(float DeltaSeconds, float OrbitAngle)
{
	if (!Bobber || !bBobberActive)
	{
		return;
	}
	
	// This is called during bite fight from BiteModule
	// BiteModule calculates the angle, we just position the bobber
	
	constexpr float BobberOrbitRadius = 64.0f;
	
	if (!OwnerComponent || !OwnerComponent->BiteModule)
	{
		return;
	}
	
	// Get orbit center from BiteModule (where bobber was when fight started)
	const FVector& OrbitCenter = OwnerComponent->BobberTargetLocation;
	
	const float X = FMath::Cos(OrbitAngle) * BobberOrbitRadius;
	const float Y = FMath::Sin(OrbitAngle) * BobberOrbitRadius;

	FVector NewLoc = OrbitCenter;
	NewLoc.X += X;
	NewLoc.Y += Y;

	Bobber->SetWorldLocation(NewLoc);
}

void UFishingBobberModule::PlaySplashEffect(float Scale)
{
	if (!SplashEffect)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("PlaySplashEffect: SplashEffect is null!"));
		return;
	}

	SplashEffect->SetFloatParameter(FName("Scale"), Scale);
	SplashEffect->Deactivate();
	SplashEffect->Activate(true);

	UE_LOG(LogFishingComponent, Log, 
	       TEXT("PlaySplashEffect: Niagara effect activated with scale %.2f"), Scale);
}