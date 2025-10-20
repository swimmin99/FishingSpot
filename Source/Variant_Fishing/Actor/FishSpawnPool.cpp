#include "FishSpawnPool.h"
#include "Fish.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"
#include "FishingCharacter.h"

#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Fishing.h"
#include "Kismet/GameplayStatics.h"


AFishSpawnPool::AFishSpawnPool()
{
	PrimaryActorTick.bCanEverTick = true;

	
	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	RootComponent = SpawnBox;
}

void AFishSpawnPool::BeginPlay()
{
	Super::BeginPlay();

	if (!Tags.Contains(TEXT("FishingWater")))
	{
		Tags.Add(TEXT("FishingWater"));
		UE_LOG(LogFishSpawnPool, Log, TEXT("Added 'FishingWater' tag to FishSpawnPool"));
	}

	
	if(!SpawnBox)
	{
		UE_LOG(LogFishSpawnPool, Error, TEXT("SpawnBox is null! Cannot initialize FishSpawnPool"));
		return;
	}
	
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SpawnBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	if (!HasAuthority())
	{
		return;
	}

	if (bEnablePooling)
	{
		PrewarmPool();
	}

	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AFishSpawnPool::TrySpawnFish,
		SpawnCooldown,
		true
	);

	UE_LOG(LogFishSpawnPool, Log,
		   TEXT("FishSpawnPool initialized - MaxFish: %d, BorderOffset: %.1f, MinDistance: %.1f"),
		   MaxFish, BorderOffset, MinDistanceBetweenFish);
}

void AFishSpawnPool::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	if (SpawnedFish.Num() == 0)
	{
		EmptyTime += DeltaTime;
		if (EmptyTime >= MaxEmptyTime)
		{
			UE_LOG(LogFishSpawnPool, Warning, TEXT("Pool empty for %.1fs - forcing spawn!"), EmptyTime);
			TrySpawnFish();
			EmptyTime = 0.f;
		}
	}
	else
	{
		EmptyTime = 0.f;
	}

	ManagementTickTimer += DeltaTime;
	if (ManagementTickTimer >= ManagementTickInterval)
	{
		ManagementTickTimer = 0.f;
		ManageFish(DeltaTime);
	}

	if (bShowDebugBox && SpawnBox)
	{
		FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
		FVector BoxCenter = GetActorLocation();
		FColor BoxColor = SpawnedFish.Num() > 0 ? FColor::Green : FColor::Red;

		DrawDebugBox(GetWorld(), BoxCenter, BoxExtent, BoxColor, false, -1.f, 0, 2.0f);

		if (bShowDebugWanderArea)
		{
			FVector SafeExtent = GetSafeAreaExtent();
			DrawDebugBox(GetWorld(), BoxCenter, SafeExtent, FColor::Cyan, false, -1.f, 0, 1.5f);
		}

		if (bShowDebugInfo)
		{
			FString DebugText = FString::Printf(TEXT("Active: %d/%d | Pooled: %d | Assigned: %d"),
			                                    SpawnedFish.Num(), MaxFish, InactiveFishPool.Num(),
			                                    BobberAssignments.Num());
			DrawDebugString(GetWorld(), BoxCenter + FVector(0, 0, BoxExtent.Z + 50.f),
			                DebugText, nullptr, FColor::White, 0.f, true);
		}
	}
}

void AFishSpawnPool::PrewarmPool()
{
	if (!bEnablePooling || PrewarmPoolSize <= 0)
	{
		return;
	}

	UE_LOG(LogFishSpawnPool, Log, TEXT("Prewarming object pool with %d fish..."), PrewarmPoolSize);

	FVector SafeLocation = GetActorLocation() + FVector(0.f, 0.f, -10000.f);

	for (int32 i = 0; i < PrewarmPoolSize; ++i)
	{
		UFishData* RandomFishData = GetRandomFishData();
		if (!RandomFishData)
		{
			continue;
		}

		AFish* NewFish = CreateNewFish(RandomFishData, SafeLocation);
		if (NewFish)
		{
			NewFish->Deactivate();
			InactiveFishPool.Add(NewFish);
		}
	}

	UE_LOG(LogFishSpawnPool, Log, TEXT("Pool prewarmed: %d fish ready"), InactiveFishPool.Num());
}

AFish* AFishSpawnPool::GetFromPool()
{
	if (!bEnablePooling || InactiveFishPool.Num() == 0)
	{
		return nullptr;
	}

	AFish* Fish = InactiveFishPool.Pop();
	UE_LOG(LogFishSpawnPool, Verbose, TEXT("Reusing fish from pool (Remaining: %d)"), InactiveFishPool.Num());
	return Fish;
}

void AFishSpawnPool::ReturnToPool(AFish* Fish)
{
	if (!Fish || !bEnablePooling)
	{
		return;
	}

	Fish->Deactivate();
	InactiveFishPool.Add(Fish);

	UE_LOG(LogFishSpawnPool, Log, TEXT("Fish returned to pool (Pool size: %d)"), InactiveFishPool.Num());
}

void AFishSpawnPool::ManageFish(float DeltaTime)
{
	TryAssignBobberToFish();

	for (int i = 0; i < SpawnedFish.Num(); ++i)
	{
		auto& Fish = SpawnedFish[i];
		if (!Fish || !Fish->IsActive())
		{
			continue;
		}

		switch (Fish->GetBehaviorState())
		{
		case EFishBehaviorState::Wandering:
			UpdateWanderingFish(Fish);
			break;

		case EFishBehaviorState::MovingToBobber:
			UpdateMovingToBobberFish(Fish);
			break;

		case EFishBehaviorState::Biting:
			UpdateBitingFish(Fish);
			break;
		}
	}
}

void AFishSpawnPool::UpdateWanderingFish(AFish* Fish)
{
	if (Fish->NeedsNewWanderTarget())
	{
		FVector NewTarget = GenerateWanderTarget(Fish);
		Fish->SetWanderTarget(NewTarget);

		UE_LOG(LogFishSpawnPool, Verbose, TEXT("Assigned new wander target to fish"));
	}
}

void AFishSpawnPool::UpdateMovingToBobberFish(AFish* Fish)
{
	FFishBobberAssignment* Assignment = FindAssignmentByFish(Fish);
	if (!Assignment || !Assignment->Bobber || !Assignment->Bobber->IsVisible())
	{
		UnassignFish(Fish);
		Fish->OnBobberLost();
		UE_LOG(LogFishSpawnPool, Log, TEXT("Fish lost bobber - returning to wandering"));
		return;
	}

	FVector BobberLoc = Assignment->Bobber->GetComponentLocation();

	if (Fish->IsWithinBiteRange(BobberLoc))
	{
		Fish->SetStateFakeBiting();
	}
	else
	{
		if (Fish->GetMovementState() == EFishMovementState::Idle)
		{
			Fish->StartMovingToBobber(Assignment->Bobber);
		}
	}
}

void AFishSpawnPool::UpdateBitingFish(AFish* Fish)
{
}

FVector AFishSpawnPool::GenerateWanderTarget(AFish* ForFish)
{
	FVector BestTarget = ForFish->GetActorLocation();
	float BestScore = -FLT_MAX;

	FVector SafeCenter = GetSafeAreaCenter();
	FVector SafeExtent = GetSafeAreaExtent();
	FVector FishLoc = ForFish->GetActorLocation();
	FVector FishForward = ForFish->GetActorForwardVector();

	constexpr int32 NumCandidates = 12;
	for (int32 i = 0; i < NumCandidates; ++i)
	{
		FVector Candidate = SafeCenter + FVector(
			FMath::FRandRange(-SafeExtent.X, SafeExtent.X),
			FMath::FRandRange(-SafeExtent.Y, SafeExtent.Y),
			0.f
		);
		Candidate.Z = FishLoc.Z;

		if (!IsGoodWanderTarget(Candidate, ForFish))
		{
			continue;
		}

		float Score = 0.f;

		float MinDistanceToOthers = FLT_MAX;
		for (AFish* OtherFish : SpawnedFish)
		{
			if (OtherFish == ForFish || !OtherFish->IsActive())
			{
				continue;
			}

			float Dist = FVector::Dist2D(Candidate, OtherFish->GetActorLocation());
			MinDistanceToOthers = FMath::Min(MinDistanceToOthers, Dist);
		}
		Score += MinDistanceToOthers * 0.5f;

		float DistFromCenter = FVector::Dist2D(Candidate, SafeCenter);
		float MaxDistFromCenter = FMath::Min(SafeExtent.X, SafeExtent.Y);
		float CentralityScore = (1.0f - (DistFromCenter / MaxDistFromCenter)) * 100.f;
		Score += CentralityScore;

		FVector ToCandidate = Candidate - FishLoc;
		ToCandidate.Normalize();
		float DotProduct = FVector::DotProduct(FishForward, ToCandidate);
		if (DotProduct > 0.3f)
		{
			Score += ForwardSpacePreference * DotProduct;
		}

		float MinDistToBorder = FLT_MAX;
		MinDistToBorder = FMath::Min(MinDistToBorder, SafeExtent.X - FMath::Abs(Candidate.X - SafeCenter.X));
		MinDistToBorder = FMath::Min(MinDistToBorder, SafeExtent.Y - FMath::Abs(Candidate.Y - SafeCenter.Y));
		Score += MinDistToBorder * 0.3f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool AFishSpawnPool::IsGoodWanderTarget(const FVector& Target, AFish* ForFish) const
{
	FVector SafeCenter = GetSafeAreaCenter();
	FVector SafeExtent = GetSafeAreaExtent();
	FVector LocalPos = Target - SafeCenter;

	if (FMath::Abs(LocalPos.X) > SafeExtent.X ||
		FMath::Abs(LocalPos.Y) > SafeExtent.Y)
	{
		return false;
	}

	for (AFish* OtherFish : SpawnedFish)
	{
		if (OtherFish == ForFish || !OtherFish->IsActive())
		{
			continue;
		}

		float Dist = FVector::Dist2D(Target, OtherFish->GetActorLocation());
		if (Dist < MinDistanceBetweenFish)
		{
			return false;
		}
	}

	return true;
}

FVector AFishSpawnPool::GetSafeAreaCenter() const
{
	return GetActorLocation();
}

FVector AFishSpawnPool::GetSafeAreaExtent() const
{
	if (!SpawnBox)
	{
		return FVector::ZeroVector;
	}

	FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	return FVector(
		BoxExtent.X - BorderOffset,
		BoxExtent.Y - BorderOffset,
		BoxExtent.Z
	);
}

void AFishSpawnPool::OnFishStartVanishing(AFish* Fish)
{
	if (!Fish)
	{
		return;
	}

	UnassignFish(Fish);
	SpawnedFish.Remove(Fish);

	UE_LOG(LogFishSpawnPool, Verbose, TEXT("Fish start vanishing -> removed from SpawnedFish (Active: %d)"),
	       SpawnedFish.Num());
}

void AFishSpawnPool::OnFishVanished(AFish* Fish)
{
	if (!Fish)
	{
		return;
	}

	if (bEnablePooling)
	{
		ReturnToPool(Fish);
	}
	else
	{
		Fish->Destroy();
	}
	UE_LOG(LogFishSpawnPool, Log, TEXT("Fish vanished -> returned/destroyed (Active: %d, Pooled: %d)"),
	       SpawnedFish.Num(), InactiveFishPool.Num());
}

void AFishSpawnPool::TryAssignBobberToFish()
{
	TArray<UStaticMeshComponent*> Bobbers = FindAllVisibleBobbers();

	for (AFish* Fish : SpawnedFish)
	{
		if (!Fish || !Fish->IsActive())
		{
			continue;
		}
		if (Fish->GetBehaviorState() != EFishBehaviorState::Wandering)
		{
			continue;
		}

		if (FindAssignmentByFish(Fish))
		{
			continue;
		}

		UStaticMeshComponent* DetectedBobber = Fish->DetectBobberInView();
		if (!DetectedBobber)
		{
			continue;
		}

		if (IsBobberAlreadyAssigned(DetectedBobber))
		{
			continue;
		}

		AssignBobberToFish(Fish, DetectedBobber);
		Fish->StartMovingToBobber(DetectedBobber);

		UE_LOG(LogFishSpawnPool, Log, TEXT("Assigned bobber to fish"));
	}
}

bool AFishSpawnPool::IsBobberAlreadyAssigned(UStaticMeshComponent* Bobber) const
{
	for (const FFishBobberAssignment& Assignment : BobberAssignments)
	{
		if (Assignment.Bobber == Bobber)
		{
			return true;
		}
	}
	return false;
}

void AFishSpawnPool::AssignBobberToFish(AFish* Fish, UStaticMeshComponent* Bobber)
{
	FFishBobberAssignment NewAssignment;
	NewAssignment.Fish = Fish;
	NewAssignment.Bobber = Bobber;
	NewAssignment.AssignedTime = GetWorld()->GetTimeSeconds();

	BobberAssignments.Add(NewAssignment);
}

void AFishSpawnPool::UnassignFish(AFish* Fish)
{
	for (int32 i = BobberAssignments.Num() - 1; i >= 0; --i)
	{
		if (BobberAssignments[i].Fish == Fish)
		{
			BobberAssignments.RemoveAt(i);
			break;
		}
	}
}

FFishBobberAssignment* AFishSpawnPool::FindAssignmentByFish(AFish* Fish)
{
	for (FFishBobberAssignment& Assignment : BobberAssignments)
	{
		if (Assignment.Fish == Fish)
		{
			return &Assignment;
		}
	}
	return nullptr;
}

FFishBobberAssignment* AFishSpawnPool::FindAssignmentByBobber(UStaticMeshComponent* Bobber)
{
	for (FFishBobberAssignment& Assignment : BobberAssignments)
	{
		if (Assignment.Bobber == Bobber)
		{
			return &Assignment;
		}
	}
	return nullptr;
}

void AFishSpawnPool::TrySpawnFish()
{
	if (!HasAuthority())
	{
		return;
	}

	if (SpawnedFish.Num() >= MaxFish)
	{
		return;
	}

	float RandomValue = FMath::FRand();
	if (RandomValue > SpawnChance)
	{
		return;
	}

	UFishData* SelectedFishData = GetRandomFishData();
	if (!SelectedFishData)
	{
		UE_LOG(LogFishSpawnPool, Warning, TEXT("No valid FishData to spawn!"));
		return;
	}

	AFish* NewFish = SpawnFish(SelectedFishData);
	if (NewFish)
	{
		UE_LOG(LogFishSpawnPool, Log, TEXT("Spawned fish: %s (Active: %d/%d)"),
		       *SelectedFishData->FishID.ToString(), SpawnedFish.Num(), MaxFish);
	}
}

AFish* AFishSpawnPool::SpawnFish(UFishData* FishData)
{
	if (!FishData || !GetWorld())
	{
		return nullptr;
	}

	FVector SpawnLocation = GetRandomSpawnLocation();
	AFish* Fish = nullptr;

	if (bEnablePooling)
	{
		Fish = GetFromPool();
	}

	if (!Fish)
	{
		Fish = CreateNewFish(FishData, SpawnLocation);
		if (!Fish)
		{
			return nullptr;
		}
	}

	Fish->Activate(FishData, SpawnLocation);
	SpawnedFish.Add(Fish);

	return Fish;
}

AFish* AFishSpawnPool::CreateNewFish(UFishData* FishData, const FVector& SpawnLocation)
{
	if (!FishData || !GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFish* NewFish = GetWorld()->SpawnActor<AFish>(AFish::StaticClass(), SpawnLocation, FRotator::ZeroRotator,
	                                               SpawnParams);

	if (NewFish)
	{
		NewFish->Initialize(FishData, this, SpawnLocation);
		UE_LOG(LogFishSpawnPool, Log, TEXT("Created new fish actor"));
	}

	return NewFish;
}

FVector AFishSpawnPool::GetRandomSpawnLocation()
{
	FVector SafeCenter = GetSafeAreaCenter();
	FVector SafeExtent = GetSafeAreaExtent();

	float RandomX = FMath::FRandRange(-SafeExtent.X, SafeExtent.X);
	float RandomY = FMath::FRandRange(-SafeExtent.Y, SafeExtent.Y);

	return FVector(SafeCenter.X + RandomX, SafeCenter.Y + RandomY, SafeCenter.Z);
}

UFishData* AFishSpawnPool::GetRandomFishData()
{
	if (FishDataList.Num() == 0)
	{
		return nullptr;
	}
	int32 RandomIndex = FMath::RandRange(0, FishDataList.Num() - 1);
	return FishDataList[RandomIndex];
}

void AFishSpawnPool::RemoveFish(AFish* Fish, bool bCaught)
{
	if (!Fish)
	{
		return;
	}

	UnassignFish(Fish);
	SpawnedFish.Remove(Fish);

	if (bCaught)
	{
		UE_LOG(LogFishSpawnPool, Log, TEXT("Fish caught (Active: %d, Pooled: %d)"),
		       SpawnedFish.Num(), InactiveFishPool.Num());
	}
	else
	{
		UE_LOG(LogFishSpawnPool, Log, TEXT("Fish escaped (Active: %d, Pooled: %d)"),
		       SpawnedFish.Num(), InactiveFishPool.Num());
	}

	if (bEnablePooling)
	{
		ReturnToPool(Fish);
	}
	else
	{
		Fish->Destroy();
	}
}

bool AFishSpawnPool::IsLocationInBounds(const FVector& Location) const
{
	if (!SpawnBox)
	{
		return false;
	}

	FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	FVector BoxCenter = GetActorLocation();

	FVector LocalPos = Location - BoxCenter;
	return FMath::Abs(LocalPos.X) <= BoxExtent.X &&
		FMath::Abs(LocalPos.Y) <= BoxExtent.Y &&
		FMath::Abs(LocalPos.Z) <= BoxExtent.Z;
}

FVector AFishSpawnPool::ClampLocationToBounds(const FVector& Location) const
{
	if (!SpawnBox)
	{
		return Location;
	}

	FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	FVector BoxCenter = GetActorLocation();

	FVector LocalPos = Location - BoxCenter;
	LocalPos.X = FMath::Clamp(LocalPos.X, -BoxExtent.X, BoxExtent.X);
	LocalPos.Y = FMath::Clamp(LocalPos.Y, -BoxExtent.Y, BoxExtent.Y);
	LocalPos.Z = FMath::Clamp(LocalPos.Z, -BoxExtent.Z, BoxExtent.Z);

	return BoxCenter + LocalPos;
}

TArray<UStaticMeshComponent*> AFishSpawnPool::FindAllVisibleBobbers()
{
	TArray<UStaticMeshComponent*> Result;

	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFishingCharacter::StaticClass(), Characters);

	for (AActor* Actor : Characters)
	{
		AFishingCharacter* FishChar = Cast<AFishingCharacter>(Actor);
		if (!FishChar || !FishChar->CoreFishingComponent)
		{
			continue;
		}
		if (!FishChar->CoreFishingComponent->IsFishing())
		{
			continue;
		}

		UStaticMeshComponent* Bobber = FishChar->CoreFishingComponent->GetBobber();
		if (Bobber && Bobber->IsVisible())
		{
			Result.Add(Bobber);
		}
	}

	return Result;
}