#include "FishingInventoryModule.h"
#include "FishingCharacter.h"
#include "Variant_Fishing/Actor/Fish.h"
#include "Variant_Fishing/Actor/ItemActor.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Variant_Fishing/ActorComponent/FishingFeatures/FishingComponent.h"
#include "Variant_Fishing/Database/DatabaseManager.h"

void UFishingInventoryModule::Initialize(UFishingComponent* InOwner,
                                         AFishingCharacter* InCharacter,
                                         USkeletalMeshComponent* InCharacterMesh)
{
	OwnerComponent = InOwner;
	OwnerCharacter = InCharacter;
	CharacterMesh = InCharacterMesh;
	
	UE_LOG(LogFishingComponent, Log, TEXT("InventoryModule initialized"));
}

void UFishingInventoryModule::CreateFishItemFromCatch(AFish* CaughtFish, int32 PlayerID)
{
    
    if (!OwnerComponent || !OwnerComponent->GetOwner() || !OwnerComponent->GetOwner()->HasAuthority())
    {
        UE_LOG(LogFishingComponent, Error, TEXT("CreateFishItemFromCatch: Must be called on server!"));
        return;
    }

    if (!CaughtFish)
    {
        UE_LOG(LogFishingComponent, Error, TEXT("CreateFishItemFromCatch: CaughtFish is null!"));
        return;
    }

    UFishData* FishData = CaughtFish->GetFishData();
    if (!FishData)
    {
        UE_LOG(LogFishingComponent, Error, TEXT("CaughtFish has no FishData!"));
        return;
    }

    UWorld* World = OwnerComponent->GetWorld();
    if (!World)
    {
        UE_LOG(LogFishingComponent, Error, TEXT("CreateFishItemFromCatch: GetWorld() returned null!"));
        return;
    }

    
    if (CurrentDisplayFishItem)
    {
        CurrentDisplayFishItem->Destroy();
        CurrentDisplayFishItem = nullptr;
    }

    
    
    
    
    FString LocationName = TEXT("Unknown Waters");
    if (OwnerComponent->GetOwner())
    {
        FVector Location = OwnerComponent->GetOwner()->GetActorLocation();
        LocationName = FString::Printf(TEXT("Location (%.0f, %.0f, %.0f)"), 
            Location.X, Location.Y, Location.Z);
    }
	
    
    FFishSpecificData FishStats = FishData->GenerateRandomFishStats(LocationName);
    
    UE_LOG(LogFishingComponent, Log, TEXT("🐟 Generated Fish: %s - Length: %.1fcm (%.0f%%), Weight: %.2fkg (%.0f%%)"),
           *FishStats.FishDataName,
           FishStats.ActualLength, FishStats.GetLengthPercentile() * 100.0f,
           FishStats.ActualWeight, FishStats.GetWeightPercentile() * 100.0f);
	
    
    UItemBase* ItemFish = FishData->CreateItemFromFishData(OwnerComponent, FishStats);
    if (!ItemFish)
    {
        UE_LOG(LogFishingComponent, Error, 
            TEXT("Failed to create item from fish data! - Check ItemActorSubClass Cache"));
        return;
    }

    
    if (UGameInstance* GI = World->GetGameInstance())
    {
        UDatabaseManager* DatabaseManager = GI->GetSubsystem<UDatabaseManager>();
        if (DatabaseManager)
        {
            DatabaseManager->RecordCaughtFish(
                PlayerID,
                TEXT("TEMP"),
                FishStats.FishDataName,
                FishStats.ActualLength,
                FishStats.ActualWeight
            );
        }
    }
	
    
    
    
    
    const FTransform SpawnTM(FRotator::ZeroRotator, FVector::ZeroVector);

    
    CurrentDisplayFishItem = ItemFish->SpawnItemActor(
        World,
        SpawnTM,
        ItemActorSubClass,
        OwnerComponent->GetOwner()
    );

    if (CurrentDisplayFishItem)
    {
        CurrentDisplayFishItem->SetHidden(true);
        
        UE_LOG(LogFishingComponent, Log, TEXT("Fish item spawned: %s"),
               *CurrentDisplayFishItem->GetNameFromItem());

        
        const FVector FinalScale = FishData->MeshScale.IsNearlyZero() ? FVector(10.f) : FishData->MeshScale;
        CurrentDisplayFishItem->SetActorScale3D(FinalScale);

        
        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CurrentDisplayFishItem->GetRootComponent()))
        {
            Prim->SetSimulatePhysics(false);
            Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }

        
        if (CharacterMesh && CharacterMesh->DoesSocketExist(ShowOffSocket))
        {
            CurrentDisplayFishItem->AttachToComponent(
                CharacterMesh,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                ShowOffSocket);

            UE_LOG(LogFishingComponent, Log, TEXT("Attached to socket: %s"), *ShowOffSocket.ToString());
        }

        
        if (USceneComponent* Root = CurrentDisplayFishItem->GetRootComponent())
        {
            Root->SetRelativeLocation(FVector::ZeroVector);
            Root->SetRelativeRotation(FRotator::ZeroRotator);
        }

        CurrentDisplayFishItem->SetHidden(true);
        
        
        if (OwnerComponent)
        {
            OwnerComponent->CurrentDisplayFishItem = CurrentDisplayFishItem;
        }
        
        UE_LOG(LogFishingComponent, Log, TEXT("✅ Fish item setup complete: %s"), 
               *ItemFish->GetDebugString());
    }
    else
    {
        UE_LOG(LogFishingComponent, Error, TEXT("Failed to spawn fish item actor!"));
    }

    
    CaughtFish->OnCaught();
}

void UFishingInventoryModule::DestroyCurrentFishItem()
{
	if (CurrentDisplayFishItem)
	{
		CurrentDisplayFishItem->Destroy();
		CurrentDisplayFishItem = nullptr;
		
		if (OwnerComponent)
		{
			OwnerComponent->CurrentDisplayFishItem = nullptr;
		}
	}
}

void UFishingInventoryModule::AttachAndRevealShowOffItem(FName Socket)
{
	if (!CurrentDisplayFishItem)
	{
		return;
	}

	if (CharacterMesh && Socket != NAME_None)
	{
		CurrentDisplayFishItem->AttachToComponent(
			CharacterMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			Socket);

		if (USceneComponent* Root = CurrentDisplayFishItem->GetRootComponent())
		{
			Root->SetRelativeLocation(FVector::ZeroVector);
			Root->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}

	CurrentDisplayFishItem->SetHidden(false);
	
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CurrentDisplayFishItem->GetRootComponent()))
	{
		Prim->SetVisibility(true, true);
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	if (UStaticMeshComponent* ItemMesh = CurrentDisplayFishItem->Mesh)
	{
		ItemMesh->SetVisibility(true, true);
	}
	
	UE_LOG(LogFishingComponent, Log, TEXT("Show-off item revealed"));
}

void UFishingInventoryModule::HideShowOffItem()
{
	if (CurrentDisplayFishItem)
	{
		CurrentDisplayFishItem->SetHidden(true);
	}
}

void UFishingInventoryModule::AddFishToInventory()
{
	if (!OwnerCharacter || !CurrentDisplayFishItem)
	{
		return;
	}

	if (!OwnerCharacter->CoreInventoryComponent)
	{
		UE_LOG(LogFishingComponent, Error, TEXT("OwnerCharacter has no CoreInventoryComponent!"));
		DropFishOnGround();
		return;
	}

	OwnerCharacter->CoreInventoryComponent->TryAddItemFroActor(CurrentDisplayFishItem);
}

void UFishingInventoryModule::DropFishOnGround()
{
	if (!OwnerComponent || !OwnerComponent->GetOwner() || !CurrentDisplayFishItem)
	{
		return;
	}

	if (!OwnerComponent->GetOwner()->HasAuthority())
	{
		return;
	}

	AActor* OwnerActor = OwnerComponent->GetOwner();
	AItemActor* Item = CurrentDisplayFishItem;
	CurrentDisplayFishItem = nullptr;
	
	if (OwnerComponent)
	{
		OwnerComponent->CurrentDisplayFishItem = nullptr;
	}

	if (!OwnerActor || !Item)
	{
		return;
	}

	UWorld* World = OwnerComponent->GetWorld();
	if (!World)
	{
		return;
	}

	Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	
	const FVector OwnerLoc = OwnerActor->GetActorLocation();
	const FVector OwnerFwd = OwnerActor->GetActorForwardVector();
	const FVector TraceStart = OwnerLoc - OwnerFwd * 25.f + FVector(0.f, 0.f, 50.f);
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 500.f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DropTrace), false);
	Params.AddIgnoredActor(OwnerActor);
	Params.AddIgnoredActor(Item);

	FVector DropLoc = TraceStart;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) && Hit.bBlockingHit)
	{
		DropLoc = Hit.ImpactPoint + FVector(0.f, 0.f, 2.f);
	}

	const FRotator DropRot(0.f, OwnerActor->GetActorRotation().Yaw, 0.f);

	
	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Item->GetRootComponent()))
	{
		Prim->SetSimulatePhysics(false);
		Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Prim->SetEnableGravity(true);
		Prim->SetVisibility(true, true);

		Item->SetReplicateMovement(true);
		Prim->SetIsReplicated(true);

		Item->SetActorLocationAndRotation(DropLoc, DropRot, false, nullptr, ETeleportType::TeleportPhysics);

		Prim->SetSimulatePhysics(true);
		Prim->WakeAllRigidBodies();
		
		
		const FVector Nudge = (-OwnerFwd * 50.f) + FVector(0.f, 0.f, 100.f);
		Prim->AddImpulse(Nudge, NAME_None, true);

		Item->ForceNetUpdate();
	}
	else
	{
		Item->SetActorLocationAndRotation(DropLoc, DropRot, false, nullptr, ETeleportType::TeleportPhysics);
		Item->ForceNetUpdate();
	}

	UE_LOG(LogFishingComponent, Log, TEXT("Fish dropped on ground at %s"), *DropLoc.ToString());
}