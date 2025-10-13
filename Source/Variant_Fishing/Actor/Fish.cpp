#include "Fish.h"
#include "Variant_Fishing/Data/FishData.h"
#include "FishSpawnPool.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"
#include "FishingCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogFish);

static void SetMeshPivotToEnd(UStaticMeshComponent* MeshComp, EAxis::Type Axis, bool bPositiveSide)
{
	if (!MeshComp)
	{
		return;
	}
	const UStaticMesh* SM = MeshComp->GetStaticMesh();
	if (!SM)
	{
		return;
	}

	const FBox LocalBox = SM->GetBoundingBox();
	FVector Center = LocalBox.GetCenter();
	FVector Extent = LocalBox.GetExtent();
	const FVector S = MeshComp->GetRelativeScale3D();

	UE_LOG(LogFish, Log, TEXT("SetMeshPivotToEnd - Original LocalBox: Center=%s, Extent=%s"),
	       *LocalBox.GetCenter().ToString(), *LocalBox.GetExtent().ToString());

	Center *= S;
	Extent *= S.GetAbs();

	FVector Min = Center - Extent;
	FVector Max = Center + Extent;

	UE_LOG(LogFish, Log, TEXT("SetMeshPivotToEnd - After Scale: Min=%s, Max=%s, Scale=%s"),
	       *Min.ToString(), *Max.ToString(), *S.ToString());

	FVector FaceCenter = (Min + Max) * 0.5f;

	auto Pick = [&](EAxis::Type A, bool bPos)-> float
	{
		switch (A)
		{
		case EAxis::X: return bPos ? Max.X : Min.X;
		case EAxis::Y: return bPos ? Max.Y : Min.Y;
		case EAxis::Z: return bPos ? Max.Z : Min.Z;
		default: return 0.f;
		}
	};

	switch (Axis)
	{
	case EAxis::X: FaceCenter.X = Pick(EAxis::X, bPositiveSide);
		break;
	case EAxis::Y: FaceCenter.Y = Pick(EAxis::Y, bPositiveSide);
		break;
	case EAxis::Z: FaceCenter.Z = Pick(EAxis::Z, bPositiveSide);
		break;
	default: break;
	}

	UE_LOG(LogFish, Log, TEXT("SetMeshPivotToEnd - FaceCenter=%s (Axis=%d, PositiveSide=%d)"),
	       *FaceCenter.ToString(), (int32)Axis, bPositiveSide);
	UE_LOG(LogFish, Log, TEXT("SetMeshPivotToEnd - Setting RelativeLocation to %s"), *(-FaceCenter).ToString());

	MeshComp->SetRelativeLocation(-FaceCenter);
}

AFish::AFish()
{
	PrimaryActorTick.bCanEverTick = true;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	bReplicates = true;

	SetNetUpdateFrequency(15.f);
	SetMinNetUpdateFrequency(5.f);

	FishMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FishMesh"));
	FishMesh->SetupAttachment(Pivot);
	FishMesh->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	FishMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FishMesh->SetMobility(EComponentMobility::Movable);

	// 머리 마커 (Pivot 위치 = 머리)
	HeadMarker = CreateDefaultSubobject<USceneComponent>(TEXT("HeadMarker"));
	HeadMarker->SetupAttachment(Pivot);
	HeadMarker->SetRelativeLocation(FVector::ZeroVector);

	// 꼬리 마커 (나중에 메시 크기에 따라 위치 조정)
	TailMarker = CreateDefaultSubobject<USceneComponent>(TEXT("TailMarker"));
	TailMarker->SetupAttachment(Pivot);

	BehaviorState = EFishBehaviorState::Wandering;
	MovementState = EFishMovementState::Idle;
	TargetBobber = nullptr;
	bIsActive = false;
	bNeedsNewTarget = false;
	IdleTimer = 0.f;
	IdleDuration = 0.f;
	MovementTimer = 0.f;
	TotalMovementTime = 0.f;
	FakeBitePhase = EFakeBitePhase::None;
	FakeBiteTimer = 0.f;
}

void AFish::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFish, FishData);
	DOREPLIFETIME(AFish, BehaviorState);
	DOREPLIFETIME(AFish, bIsActive);
}

void AFish::BeginPlay()
{
	Super::BeginPlay();
	SetReplicateMovement(true);
}

void AFish::Initialize(UFishData* InFishData, AFishSpawnPool* InSpawnPool, const FVector& SpawnLocation)
{
	FishData = InFishData;
	SpawnPool = InSpawnPool;
	InitialSpawnLocation = SpawnLocation;

	if (FishData)
	{
		if (FishData->FishMesh)
		{
			FishMesh->SetStaticMesh(FishData->FishMesh);
		}
		FishMesh->SetWorldScale3D(FishData->MeshScale);

		SetMeshPivotToEnd(FishMesh, EAxis::X, true);
		UpdateTailMarkerPosition();

		MaxFakeBiteCount = FMath::RandRange(FishData->MinFakeBites, FishData->MaxFakeBites);
		CurrentFakeBiteCount = MaxFakeBiteCount;
	}

	SetActorLocation(SpawnLocation);
	ResetInternalState();

	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	UE_LOG(LogFish, Log, TEXT("Fish initialized: %s"), FishData ? *FishData->FishID.ToString() : TEXT("Unknown"));
}

void AFish::Activate(UFishData* InFishData, const FVector& SpawnLocation)
{
	FishData = InFishData;
	InitialSpawnLocation = SpawnLocation;

	if (FishData)
	{
		if (FishData->FishMesh)
		{
			FishMesh->SetStaticMesh(FishData->FishMesh);
		}
		FishMesh->SetWorldScale3D(FishData->MeshScale);

		SetMeshPivotToEnd(FishMesh, EAxis::X, true);
		UpdateTailMarkerPosition();
	}

	SetActorLocation(SpawnLocation);
	SetActorRotation(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f));
	ResetInternalState();

	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	UE_LOG(LogFish, Log, TEXT("Fish reactivated: %s"), FishData ? *FishData->FishID.ToString() : TEXT("Unknown"));
}

void AFish::Deactivate()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;
	ResetInternalState();

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	SetActorLocation(FVector(0.f, 0.f, -10000.f));

	UE_LOG(LogFish, Log, TEXT("Fish deactivated"));
}

void AFish::ResetInternalState()
{
	BehaviorState = EFishBehaviorState::Wandering;
	MovementState = EFishMovementState::Idle;
	TargetBobber = nullptr;
	bNeedsNewTarget = false;
	IdleTimer = 0.f;
	IdleDuration = FMath::FRandRange(MinIdleTime, MaxIdleTime);
	CurrentTarget = GetActorLocation();
	MovementStartLocation = GetActorLocation();
	MovementTimer = 0.f;
	TotalMovementTime = 0.f;
	TotalMovementDistance = 0.f;
	FakeBitePhase = EFakeBitePhase::None;
	FakeBiteTimer = 0.f;
}

void AFish::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	if (HasAuthority() &&
		((BehaviorState == EFishBehaviorState::FakeBiting
				|| BehaviorState == EFishBehaviorState::Biting
				|| BehaviorState == EFishBehaviorState::MovingToBobber)
			&& (TargetBobber == nullptr)))
	{
		SetStateVanishing();
	}

	if (BehaviorState == EFishBehaviorState::Vanishing)
	{
		TickVanishing(DeltaTime);
	}
	else if (BehaviorState == EFishBehaviorState::FakeBiting)
	{
		TickFakeBite(DeltaTime);
	}
	else
	{
		TickMovement(DeltaTime);
	}

	if (BehaviorState == EFishBehaviorState::FakeBiting)
	{
		TickFakeBite(DeltaTime);
	}
	else
	{
		if (BehaviorState == EFishBehaviorState::MovingToBobber && TargetBobber && TargetBobber->IsVisible())
		{
			FVector BobberLoc = TargetBobber->GetComponentLocation();
			CurrentTarget = BobberLoc;

			if (MovementState == EFishMovementState::Rotating)
			{
				FVector Direction = BobberLoc - GetActorLocation();
				Direction.Z = 0.f;
				Direction.Normalize();
				TargetRotation = Direction.Rotation();
			}

			if (MovementState == EFishMovementState::Moving)
			{
				float NewDistance = FVector::Dist2D(GetActorLocation(), CurrentTarget);
				float BaseSpeed = GetCurrentMoveSpeed();
				if (BaseSpeed > 0.f)
				{
					TotalMovementTime = NewDistance / BaseSpeed;
				}
			}
		}

		TickMovement(DeltaTime);
	}

	if (bShowDebugMovement)
	{
		DrawMovementDebug();
	}
}

void AFish::TickMovement(float DeltaTime)
{
	switch (MovementState)
	{
	case EFishMovementState::Idle:
		TickIdle(DeltaTime);
		break;
	case EFishMovementState::Rotating:
		TickRotating(DeltaTime);
		break;
	case EFishMovementState::Moving:
		TickMoving(DeltaTime);
		break;
	}
}

void AFish::TickIdle(float DeltaTime)
{
	IdleTimer += DeltaTime;

	if (IdleTimer >= IdleDuration)
	{
		if (BehaviorState == EFishBehaviorState::Wandering)
		{
			bNeedsNewTarget = true;
			UE_LOG(LogFish, Verbose, TEXT("Idle finished, requesting new target"));
		}
	}
}

void AFish::TickRotating(float DeltaTime)
{
	FRotator CurrentRot = GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime, RotationSpeed);
	SetActorRotation(NewRot);

	const float AngleDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(NewRot.Yaw, TargetRotation.Yaw));
	if (AngleDiff < 5.0f)
	{
		SetActorRotation(TargetRotation);
		StartMovingToTarget();
	}
}

void AFish::TickMoving(float DeltaTime)
{
	FVector CurrentLoc = GetActorLocation();
	FVector Direction = CurrentTarget - CurrentLoc;
	Direction.Z = 0.f;

	const float Distance = Direction.Size2D();
	if (Distance < ArrivalThreshold)
	{
		OnReachedTarget();
		return;
	}

	MovementTimer += DeltaTime;

	const float SpeedMultiplier = CalculateSpeedMultiplier();
	Direction.Normalize();
	const float MoveSpeed = GetCurrentMoveSpeed() * SpeedMultiplier;

	FVector NewLocation = CurrentLoc + Direction * MoveSpeed * DeltaTime;
	NewLocation.Z = InitialSpawnLocation.Z;

	SetActorLocation(NewLocation);
}

void AFish::TickFakeBite(float DeltaTime)
{
	switch (FakeBitePhase)
	{
	case EFakeBitePhase::Freeze:
		{
			FakeBiteTimer += DeltaTime;
			if (FakeBiteTimer >= FakeBiteTime)
			{
				StartFakeBiteBackOff();
			}
			break;
		}

	case EFakeBitePhase::BackOff:
		{
			TickMoving(DeltaTime);
			break;
		}

	case EFakeBitePhase::Return:
		{
			TickMoving(DeltaTime);
			break;
		}

	default:
		break;
	}
}

float AFish::CalculateSpeedMultiplier() const
{
	if (TotalMovementTime <= 0.f)
	{
		return 1.0f;
	}

	const float Progress = FMath::Clamp(MovementTimer / TotalMovementTime, 0.f, 1.f);
	float SpeedMultiplier = 1.0f;

	const float AccelPhaseEnd = AccelerationTime / TotalMovementTime;
	const float DecelPhaseStart = 1.0f - (DecelerationTime / TotalMovementTime);

	if (AccelPhaseEnd >= DecelPhaseStart)
	{
		if (Progress < 0.5f)
		{
			const float LocalProgress = Progress * 2.0f;
			SpeedMultiplier = FMath::InterpEaseIn(MinSpeedMultiplier, 1.0f, LocalProgress, 2.0f);
		}
		else
		{
			const float LocalProgress = (Progress - 0.5f) * 2.0f;
			SpeedMultiplier = FMath::InterpEaseOut(1.0f, MinSpeedMultiplier, LocalProgress, 2.0f);
		}
	}
	else
	{
		if (Progress < AccelPhaseEnd)
		{
			const float LocalProgress = Progress / AccelPhaseEnd;
			SpeedMultiplier = FMath::InterpEaseIn(MinSpeedMultiplier, 1.0f, LocalProgress, 2.0f);
		}
		else if (Progress > DecelPhaseStart)
		{
			const float LocalProgress = (Progress - DecelPhaseStart) / (1.0f - DecelPhaseStart);
			SpeedMultiplier = FMath::InterpEaseOut(1.0f, MinSpeedMultiplier, LocalProgress, 2.0f);
		}
		else
		{
			const float WaveProgress = (Progress - AccelPhaseEnd) / (DecelPhaseStart - AccelPhaseEnd);
			const float Wave = FMath::Sin(WaveProgress * PI * 2.0f) * SpeedVariation;
			SpeedMultiplier = 1.0f + Wave;
		}
	}

	return FMath::Clamp(SpeedMultiplier, MinSpeedMultiplier, 1.0f + SpeedVariation);
}

void AFish::StartRotatingToTarget(const FVector& TargetLocation)
{
	MovementState = EFishMovementState::Rotating;
	CurrentTarget = TargetLocation;

	FVector Direction = TargetLocation - GetActorLocation();
	Direction.Z = 0.f;
	Direction.Normalize();

	TargetRotation = Direction.Rotation();
	UE_LOG(LogFish, Verbose, TEXT("Fish starting rotation to target"));
}

void AFish::StartMovingToTarget()
{
	MovementState = EFishMovementState::Moving;
	MovementStartLocation = GetActorLocation();
	MovementTimer = 0.f;

	TotalMovementDistance = FVector::Dist2D(MovementStartLocation, CurrentTarget);
	const float BaseSpeed = GetCurrentMoveSpeed();
	TotalMovementTime = (BaseSpeed > 0.f) ? (TotalMovementDistance / BaseSpeed) : 1.0f;

	UE_LOG(LogFish, Verbose, TEXT("Fish started moving to target (Distance: %.1f, Time: %.1fs)"),
	       TotalMovementDistance, TotalMovementTime);
}

void AFish::OnReachedTarget()
{
	if (BehaviorState == EFishBehaviorState::FakeBiting)
	{
		StartMovingToBobber(TargetBobber);
		return;
	}

	MovementState = EFishMovementState::Idle;
	IdleTimer = 0.f;
	IdleDuration = FMath::FRandRange(MinIdleTime, MaxIdleTime);
	bNeedsNewTarget = false;
	MovementTimer = 0.f;
	TotalMovementTime = 0.f;

	UE_LOG(LogFish, Verbose, TEXT("Fish reached target, idling for %.1fs"), IdleDuration);
}

void AFish::RequestNewWanderTarget()
{
}

float AFish::GetCurrentMoveSpeed() const
{
	if (!FishData)
	{
		return 100.f;
	}

	float Speed = FishData->MoveSpeed;
	if (BehaviorState == EFishBehaviorState::MovingToBobber || BehaviorState == EFishBehaviorState::FakeBiting)
	{
		Speed *= BobberSpeedMultiplier;
	}
	return Speed;
}

void AFish::SetWanderTarget(const FVector& TargetLocation)
{
	bNeedsNewTarget = false;
	StartRotatingToTarget(TargetLocation);
}

UStaticMeshComponent* AFish::DetectBobberInView()
{
	if (!FishData || !bIsActive || BehaviorState != EFishBehaviorState::Wandering)
	{
		return nullptr;
	}

	UStaticMeshComponent* DetectedBobber = nullptr;

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
		if (!Bobber || !Bobber->IsVisible())
		{
			continue;
		}

		const FVector BobberLoc = Bobber->GetComponentLocation();
		const float Distance = FVector::Dist2D(GetActorLocation(), BobberLoc);
		if (Distance > FishData->BobberDetectionRange)
		{
			continue;
		}

		FVector ToTarget = BobberLoc - GetActorLocation();
		ToTarget.Normalize();

		const float DotProduct = FVector::DotProduct(GetActorForwardVector(), ToTarget);
		if (DotProduct > FishData->DetectionViewAngle)
		{
			DetectedBobber = Bobber;
			break;
		}
	}

	if (bShowDebugDetection)
	{
		DrawDetectionDebug(DetectedBobber);
	}

	return DetectedBobber;
}

void AFish::StartMovingToBobber(UStaticMeshComponent* InTargetBobber)
{
	if (!bIsActive)
	{
		return;
	}

	TargetBobber = InTargetBobber;
	BehaviorState = EFishBehaviorState::MovingToBobber;

	const FVector BobberLoc = TargetBobber->GetComponentLocation();
	StartRotatingToTarget(BobberLoc);

	UE_LOG(LogFish, Log, TEXT("Fish started moving to bobber (Speed: %.1fx)"), BobberSpeedMultiplier);
}

bool AFish::IsWithinBiteRange(const FVector& BobberLocation) const
{
	if (!bIsActive)
	{
		return false;
	}
	const float Distance = FVector::Dist2D(GetActorLocation(), BobberLocation);
	return Distance < 20.f;
}

void AFish::SetStateWandering()
{
	if (!bIsActive)
	{
		return;
	}

	BehaviorState = EFishBehaviorState::Wandering;
	TargetBobber = nullptr;

	MovementState = EFishMovementState::Idle;
	IdleTimer = 0.f;
	IdleDuration = FMath::FRandRange(MinIdleTime * 0.5f, MaxIdleTime * 0.5f);
	bNeedsNewTarget = false;

	FakeBitePhase = EFakeBitePhase::None;
	FakeBiteTimer = 0.f;
}

void AFish::SetStateBiting()
{
	if (!bIsActive)
	{
		return;
	}

	BehaviorState = EFishBehaviorState::Biting;
	MovementState = EFishMovementState::Idle;
	FakeBitePhase = EFakeBitePhase::None;
	FakeBiteTimer = 0.f;

	UE_LOG(LogFish, Log, TEXT("Fish started REAL BITING!"));

	if (TargetBobber)
	{
		AFishingCharacter* FishChar = Cast<AFishingCharacter>(TargetBobber->GetOwner());
		if (FishChar && FishChar->CoreFishingComponent)
		{
			FishChar->CoreFishingComponent->OnFishBite(this);
		}
	}
}

void AFish::SetStateFakeBiting()
{
	if (!bIsActive)
	{
		return;
	}

	CurrentFakeBiteCount--;

	UE_LOG(LogFish, Log, TEXT("Fish started FAKE BITING! (Remaining: %d)"), CurrentFakeBiteCount);

	if (CurrentFakeBiteCount >= 0)
	{
		BehaviorState = EFishBehaviorState::FakeBiting;
		StartFakeBiteFreeze();

		if (TargetBobber)
		{
			AFishingCharacter* FishChar = Cast<AFishingCharacter>(TargetBobber->GetOwner());
			if (FishChar && FishChar->CoreFishingComponent)
			{
				FishChar->CoreFishingComponent->OnFishFakeBite(this, FakeBiteTime);
			}
		}
	}
	else
	{
		CurrentFakeBiteCount = MaxFakeBiteCount;
		SetStateBiting();
	}
}

void AFish::TickVanishing(float DeltaTime)
{
	VanishingTimer += DeltaTime;

	const FVector BackDir = (GetActorForwardVector()).GetSafeNormal2D();
	FVector NewLoc = GetActorLocation() + BackDir * VanishSpeed * DeltaTime;
	NewLoc.Z = InitialSpawnLocation.Z;
	SetActorLocation(NewLoc, false);

	if (VanishingTimer >= VanishingTime)
	{
		if (SpawnPool)
		{
			SpawnPool->OnFishVanished(this);
		}
		else
		{
			Deactivate();
		}
	}
}

void AFish::SetStateVanishing()
{
	if (!bIsActive)
	{
		return;
	}

	BehaviorState = EFishBehaviorState::Vanishing;
	MovementState = EFishMovementState::Moving;
	TargetBobber = nullptr;

	const FVector BackDir = (-GetActorForwardVector()).GetSafeNormal2D();
	const FRotator BackRot = BackDir.Rotation();
	SetActorRotation(BackRot);

	VanishingTimer = 0.f;

	if (SpawnPool && !bRemovedFromPoolOnVanishing)
	{
		SpawnPool->OnFishStartVanishing(this);
		bRemovedFromPoolOnVanishing = true;
	}

	FishMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UE_LOG(LogFish, Log, TEXT("Fish is vanishing (time=%.2fs, speed=%.0f)"), VanishingTime, VanishSpeed);
}

void AFish::StartFakeBiteFreeze()
{
	FakeBitePhase = EFakeBitePhase::Freeze;
	FakeBiteTimer = 0.f;
	MovementState = EFishMovementState::Idle;

	UE_LOG(LogFish, Verbose, TEXT("FakeBite: Freeze phase started"));
}

void AFish::StartFakeBiteBackOff()
{
	FakeBitePhase = EFakeBitePhase::BackOff;

	const FVector BackOffDirection = -GetActorForwardVector();
	FVector BackOffTarget = GetActorLocation() + BackOffDirection * BackOffDistance;
	BackOffTarget.Z = InitialSpawnLocation.Z;

	if (SpawnPool)
	{
		BackOffTarget = SpawnPool->ClampLocationToBounds(BackOffTarget);
	}

	MovementState = EFishMovementState::Moving;
	CurrentTarget = BackOffTarget;
	MovementStartLocation = GetActorLocation();
	MovementTimer = 0.f;
	TotalMovementDistance = FVector::Dist2D(MovementStartLocation, CurrentTarget);
	TotalMovementTime = 0.3f;

	UE_LOG(LogFish, Verbose, TEXT("FakeBite: BackOff phase started"));
}

void AFish::OnBobberLost()
{
	UE_LOG(LogFish, Log, TEXT("Fish lost bobber - returning to wandering"));
	SetStateVanishing();
}

void AFish::OnCaught()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogFish, Log, TEXT("Fish CAUGHT!"));

	if (SpawnPool)
	{
		SpawnPool->RemoveFish(this, true);
	}
}

void AFish::OnEscaped()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogFish, Log, TEXT("Fish ESCAPED!"));

	if (SpawnPool)
	{
		SetStateVanishing();
	}
}

void AFish::PlayBackOff()
{
	if (!bIsActive || BehaviorState != EFishBehaviorState::Biting)
	{
		return;
	}

	const FVector BackOffDirection = -GetActorForwardVector();
	FVector BackOffTarget = GetActorLocation() + BackOffDirection * BackOffDistance;
	BackOffTarget.Z = InitialSpawnLocation.Z;

	if (SpawnPool)
	{
		BackOffTarget = SpawnPool->ClampLocationToBounds(BackOffTarget);
	}

	MovementState = EFishMovementState::Moving;
	CurrentTarget = BackOffTarget;
	MovementStartLocation = GetActorLocation();
	MovementTimer = 0.f;
	TotalMovementDistance = FVector::Dist2D(MovementStartLocation, CurrentTarget);
	TotalMovementTime = 0.3f;

	UE_LOG(LogFish, Verbose, TEXT("Fish backing off during bite struggle"));
}

void AFish::DrawDetectionDebug(UStaticMeshComponent* DetectedBobber)
{
	if (!GetWorld() || !FishData)
	{
		return;
	}

	const FVector FishLoc = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const float DetectionRange = FishData->BobberDetectionRange;
	const float ViewAngleDot = FishData->DetectionViewAngle;

	const FColor RangeColor = DetectedBobber ? FColor::Green : FColor::Red;

	DrawDebugCircle(GetWorld(), FishLoc, DetectionRange, 32, RangeColor, false, -1.f, 0, 2.0f,
	                FVector(0, 1, 0), FVector(1, 0, 0), false);

	const float ViewAngleRadians = FMath::Acos(ViewAngleDot);
	constexpr int32 NumSegments = 16;
	FVector LastPoint = FishLoc;

	for (int32 i = 0; i <= NumSegments; ++i)
	{
		const float Angle = -ViewAngleRadians + (2.0f * ViewAngleRadians * i / NumSegments);
		const FVector Direction = Forward.RotateAngleAxis(FMath::RadiansToDegrees(Angle), FVector::UpVector);
		const FVector EndPoint = FishLoc + Direction * DetectionRange;

		DrawDebugLine(GetWorld(), FishLoc, EndPoint, RangeColor, false, -1.f, 0, 1.0f);

		if (i > 0)
		{
			DrawDebugLine(GetWorld(), LastPoint, EndPoint, RangeColor, false, -1.f, 0, 1.5f);
		}
		LastPoint = EndPoint;
	}

	if (DetectedBobber)
	{
		const FVector BobberLoc = DetectedBobber->GetComponentLocation();
		DrawDebugLine(GetWorld(), FishLoc, BobberLoc, FColor::Cyan, false, -1.f, 0, 3.0f);
		DrawDebugSphere(GetWorld(), BobberLoc, 15.0f, 12, FColor::Cyan, false, -1.f, 0, 2.0f);

		const float Distance = FVector::Dist2D(FishLoc, BobberLoc);
		DrawDebugString(GetWorld(), (FishLoc + BobberLoc) * 0.5f + FVector(0, 0, 30.f),
		                FString::Printf(TEXT("%.1fm"), Distance / 100.0f),
		                nullptr, FColor::White, -1.f, true);
	}
}

void AFish::DrawMovementDebug()
{
	if (!GetWorld())
	{
		return;
	}

	const FVector FishLoc = GetActorLocation();
	FColor StateColor;
	FString StateText;

	if (BehaviorState == EFishBehaviorState::FakeBiting)
	{
		switch (FakeBitePhase)
		{
		case EFakeBitePhase::Freeze:
			StateColor = FColor::Purple;
			StateText = FString::Printf(TEXT("FakeBite: Freeze (%.2fs/%.2fs)"), FakeBiteTimer, FakeBiteTime);
			break;
		case EFakeBitePhase::BackOff:
			StateColor = FColor::Orange;
			StateText = TEXT("FakeBite: BackOff");
			break;
		case EFakeBitePhase::Return:
			StateColor = FColor::Cyan;
			StateText = TEXT("FakeBite: Return");
			break;
		default:
			StateColor = FColor::White;
			StateText = TEXT("FakeBite: Unknown");
			break;
		}

		DrawDebugString(GetWorld(), FishLoc + FVector(0, 0, 50.f),
		                StateText, nullptr, StateColor, .01f, true, 1.2f);

		DrawDebugString(GetWorld(), FishLoc + FVector(0, 0, 70.f),
		                FString::Printf(TEXT("Remaining: %d"), CurrentFakeBiteCount),
		                nullptr, FColor::Yellow, .01f, true, 1.0f);
	}
	else
	{
		switch (MovementState)
		{
		case EFishMovementState::Idle:
			StateColor = FColor::Yellow;
			StateText = FString::Printf(TEXT("Idle (%.1fs/%.1fs)"), IdleTimer, IdleDuration);
			break;

		case EFishMovementState::Rotating:
			StateColor = FColor::Orange;
			StateText = TEXT("Rotating");
			DrawDebugDirectionalArrow(GetWorld(), FishLoc,
			                          FishLoc + TargetRotation.Vector() * 50.f,
			                          20.f, FColor::Orange, false, -1.f, 0, 2.0f);
			break;

		case EFishMovementState::Moving:
			{
				StateColor = FColor::Green;

				const float SpeedMultiplier = CalculateSpeedMultiplier();
				const float Progress = TotalMovementTime > 0.f ? (MovementTimer / TotalMovementTime) * 100.f : 0.f;

				FString SpeedPhase = TEXT("Cruise");
				const float AccelPhaseEnd = AccelerationTime / TotalMovementTime;
				const float DecelPhaseStart = 1.0f - (DecelerationTime / TotalMovementTime);

				if (MovementTimer / TotalMovementTime < AccelPhaseEnd)
				{
					SpeedPhase = TEXT("Accel");
					StateColor = FColor::Cyan;
				}
				else if (MovementTimer / TotalMovementTime > DecelPhaseStart)
				{
					SpeedPhase = TEXT("Decel");
					StateColor = FColor::Magenta;
				}

				const float FinalSpeed = GetCurrentMoveSpeed() * SpeedMultiplier;
				StateText = FString::Printf(TEXT("Moving [%s] %.0f cm/s (%.0f%%)"),
				                            *SpeedPhase, FinalSpeed, Progress);

				DrawDebugLine(GetWorld(), FishLoc, CurrentTarget, StateColor, false, -1.f, 0, 2.0f);
				DrawDebugSphere(GetWorld(), CurrentTarget, 10.f, 8, StateColor, false, -1.f, 0, 1.5f);

				if (TotalMovementTime > 0.f)
				{
					const FVector Dir = (CurrentTarget - FishLoc).GetSafeNormal();

					if (AccelPhaseEnd > 0.f && AccelPhaseEnd < 1.f)
					{
						const FVector AccelEnd = MovementStartLocation + Dir * (TotalMovementDistance * AccelPhaseEnd);
						DrawDebugSphere(GetWorld(), AccelEnd, 6.f, 8, FColor::Cyan, false, -1.f, 0, 1.0f);
					}
					if (DecelPhaseStart > 0.f && DecelPhaseStart < 1.f)
					{
						const FVector DecelStart = MovementStartLocation + Dir * (TotalMovementDistance *
							DecelPhaseStart);
						DrawDebugSphere(GetWorld(), DecelStart, 6.f, 8, FColor::Magenta, false, -1.f, 0, 1.0f);
					}
				}
				break;
			}
		}

		if (BehaviorState == EFishBehaviorState::MovingToBobber)
		{
			StateText += TEXT(" [→Bobber]");
		}
		else if (BehaviorState == EFishBehaviorState::Biting)
		{
			StateText = TEXT("BITING!");
			StateColor = FColor::Red;
		}

		DrawDebugString(GetWorld(), FishLoc + FVector(0, 0, 50.f),
		                StateText, nullptr, StateColor, .01f, true, 1.2f);
	}

	// Actor Forward 방향 화살표
	DrawDebugDirectionalArrow(GetWorld(), FishLoc,
	                          FishLoc + GetActorForwardVector() * 30.f,
	                          15.f, FColor::Blue, false, .01f, 0, 1.5f);

	// 머리와 꼬리 마커 디버그 표시
	if (HeadMarker)
	{
		FVector HeadPos = HeadMarker->GetComponentLocation();
		DrawDebugSphere(GetWorld(), HeadPos, 12.f, 12, FColor::Red, false, .01f, 0, 3.0f);
		DrawDebugString(GetWorld(), HeadPos + FVector(0, 0, 20.f),
		                TEXT("HEAD"), nullptr, FColor::Red, .01f, true, 1.2f);
	}
	
	if (TailMarker)
	{
		FVector TailPos = TailMarker->GetComponentLocation();
		DrawDebugSphere(GetWorld(), TailPos, 12.f, 12, FColor::Purple, false, .01f, 0, 3.0f);
		DrawDebugString(GetWorld(), TailPos + FVector(0, 0, 20.f),
		                TEXT("TAIL"), nullptr, FColor::Purple, .01f, true, 1.2f);
		
		// 머리-꼬리 연결선
		if (HeadMarker)
		{
			DrawDebugLine(GetWorld(), HeadMarker->GetComponentLocation(), TailPos,
			              FColor::Cyan, false, .01f, 0, 2.0f);
		}
	}

	if (bNeedsNewTarget && MovementState == EFishMovementState::Idle)
	{
		DrawDebugString(GetWorld(), FishLoc + FVector(0, 0, 90.f),
		                TEXT("Needs Target!"), nullptr, FColor::Red, .01f, true, 1.0f);
	}
}

FVector AFish::GetHeadWorldLocation() const
{
	if (HeadMarker)
	{
		return HeadMarker->GetComponentLocation();
	}
	return GetActorLocation();
}

FVector AFish::GetTailWorldLocation() const
{
	if (TailMarker)
	{
		return TailMarker->GetComponentLocation();
	}
	
	// Fallback: 계산으로 구하기
	if (!FishMesh)
	{
		return GetActorLocation();
	}

	const UStaticMesh* SM = FishMesh->GetStaticMesh();
	if (!SM)
	{
		return GetActorLocation();
	}

	const FBox LocalBox = SM->GetBoundingBox();
	const FVector MeshScale = FishMesh->GetRelativeScale3D();
	const float FishLength = LocalBox.GetSize().X * MeshScale.X;
	
	return GetActorLocation() - GetActorForwardVector() * FishLength;
}

float AFish::GetFishLength() const
{
	if (!FishMesh)
	{
		return 100.f;
	}

	const UStaticMesh* SM = FishMesh->GetStaticMesh();
	if (!SM)
	{
		return 100.f;
	}

	const FBox LocalBox = SM->GetBoundingBox();
	const FVector MeshScale = FishMesh->GetRelativeScale3D();
	
	return LocalBox.GetSize().X * MeshScale.X;
}

void AFish::UpdateTailMarkerPosition()
{
	if (!TailMarker || !FishMesh)
	{
		return;
	}

	const UStaticMesh* SM = FishMesh->GetStaticMesh();
	if (!SM)
	{
		return;
	}

	// 메시의 바운딩 박스에서 길이 계산
	const FBox LocalBox = SM->GetBoundingBox();
	const FVector MeshScale = FishMesh->GetRelativeScale3D();
	const float FishLength = LocalBox.GetSize().X * MeshScale.X;

	// TailMarker는 Pivot(머리)에서 뒤로 FishLength만큼
	// Actor의 Forward 방향이 이동 방향이므로, 꼬리는 -Forward
	TailMarker->SetRelativeLocation(FVector(-FishLength, 0.f, 0.f));

	UE_LOG(LogFish, Verbose, TEXT("TailMarker updated: FishLength=%.1f, TailPos=%s"),
	       FishLength, *TailMarker->GetRelativeLocation().ToString());
}

void AFish::OnRep_FishData()
{
	if (FishData)
	{
		if (FishData->FishMesh)
		{
			FishMesh->SetStaticMesh(FishData->FishMesh);
		}
		FishMesh->SetWorldScale3D(FishData->MeshScale);
		UpdateTailMarkerPosition();
	}
}

void AFish::OnRep_IsActive()
{
	const bool bActive = bIsActive;
	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);
}