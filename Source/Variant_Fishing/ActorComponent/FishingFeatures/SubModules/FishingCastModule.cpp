#include "FishingCastModule.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/Actor/FishSpawnPool.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"

void UFishingCastModule::Initialize(UFishingComponent* InOwner, AFishingCharacter* InCharacter)
{
	OwnerComponent = InOwner;
	OwnerCharacter = InCharacter;
	
	UE_LOG(LogFishingComponent, Log, TEXT("CastModule initialized"));
}

bool UFishingCastModule::CheckCastingCollision(FVector& OutTargetLocation)
{
	if (!OwnerComponent)
	{
		return false;
	}
	
	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("CheckCastingCollision: GetWorld() returned null!"));
		return false;
	}
	
	if (!OwnerCharacter)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("CheckCastingCollision: Owner is null!"));
		return false;
	}

	const FVector CharLoc = OwnerCharacter->GetActorLocation();
	const FVector Forward = OwnerCharacter->GetActorForwardVector();

	const FVector StartLoc = CharLoc + Forward * CastCheckForwardOffset;
	const FVector EndLoc = StartLoc + FVector(0.f, 0.f, -CastCheckDownOffset);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	bool bHit = World->LineTraceSingleByChannel(
		Hit,
		StartLoc,
		EndLoc,
		ECC_Visibility,
		QueryParams
	);

	if (!bHit)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, FVector::ZeroVector, false, TEXT("No Hit"));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: No surface hit"));
		return false;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, TEXT("No Actor"));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Hit location has no actor"));
		return false;
	}

	bool bIsFishingWater = HitActor->ActorHasTag(FishingWaterTag);
	AFishSpawnPool* HitPool = Cast<AFishSpawnPool>(HitActor);
	
	if (!bIsFishingWater && !HitPool)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, 
				FString::Printf(TEXT("Not Water (Actor: %s)"), *HitActor->GetName()));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Hit actor '%s' is not FishingWater"), 
			*HitActor->GetName());
		return false;
	}

	const float CastHeight = FVector::Dist(StartLoc, Hit.Location);
	if (CastHeight > CastMaxHeight)
	{
		if (bShowDebugCasting)
		{
			DrawCastingDebug(StartLoc, EndLoc, Hit.Location, false, 
				FString::Printf(TEXT("Too High: %.1fcm > %.1fcm"), CastHeight, CastMaxHeight));
		}
		UE_LOG(LogFishingComponent, Warning, TEXT("Cast failed: Height %.1f exceeds max %.1f"), 
			CastHeight, CastMaxHeight);
		return false;
	}

	OutTargetLocation = Hit.Location;

	if (bShowDebugCasting)
	{
		DrawCastingDebug(StartLoc, EndLoc, Hit.Location, true, 
			FString::Printf(TEXT("Success! Height: %.1fcm"), CastHeight));
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Cast successful: Height=%.1f, Target=%s"), 
		CastHeight, *OutTargetLocation.ToString());

	return true;
}

void UFishingCastModule::DrawCastingDebug(const FVector& StartLoc, const FVector& EndLoc, 
                                          const FVector& HitLoc, bool bValid, const FString& Message)
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

	constexpr float Duration = 3.0f;
	const FColor LineColor = bValid ? FColor::Green : FColor::Red;

	// Line from character to start location
	if (OwnerCharacter)
	{
		DrawDebugLine(World, OwnerCharacter->GetActorLocation(), StartLoc, 
			FColor::Cyan, false, Duration, 0, 2.0f);
		DrawDebugSphere(World, StartLoc, 10.0f, 8, FColor::Cyan, false, Duration, 0, 1.0f);
	}

	// Trace line
	DrawDebugLine(World, StartLoc, EndLoc, LineColor, false, Duration, 0, 3.0f);

	// Hit location indicator
	if (!HitLoc.IsZero())
	{
		DrawDebugSphere(World, HitLoc, 15.0f, 12, LineColor, false, Duration, 0, 2.0f);
		
		// Height indicator line (vertical)
		const float Height = FVector::Dist(StartLoc, HitLoc);
		DrawDebugLine(World, StartLoc, HitLoc, FColor::Yellow, false, Duration, 0, 2.0f);
		
		// Height text
		const FVector MidPoint = (StartLoc + HitLoc) * 0.5f;
		DrawDebugString(World, MidPoint + FVector(30, 0, 0),
			FString::Printf(TEXT("%.1f cm"), Height),
			nullptr, FColor::White, Duration, true, 1.2f);

		if (bValid)
		{
			// Success - emphasize target location
			DrawDebugSphere(World, HitLoc, 25.0f, 12, FColor::Green, false, Duration, 0, 1.0f);
			DrawDebugString(World, HitLoc + FVector(0, 0, 50),
				TEXT("✓ VALID TARGET"),
				nullptr, FColor::Green, Duration, true, 1.5f);
		}
	}

	// Message display
	if (!Message.IsEmpty())
	{
		FVector MessageLoc = HitLoc.IsZero() ? EndLoc : HitLoc;
		DrawDebugString(World, MessageLoc + FVector(0, 0, 80),
			Message,
			nullptr, bValid ? FColor::Green : FColor::Red, Duration, true, 1.2f);
	}
}