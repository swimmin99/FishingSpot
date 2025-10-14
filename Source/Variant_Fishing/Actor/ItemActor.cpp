#include "Variant_Fishing/Actor/ItemActor.h"

#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Data/ItemDataAsset.h"

FString AItemActor::GetNameFromItem() const
{
	return ItemDef ? ItemDef->DisplayName.ToString() : TEXT("NoItem");
}

FString AItemActor::GetName_Implementation() const
{
	return GetNameFromItem();
}

EInteractionType AItemActor::GetType_Implementation() const
{
	return EInteractionType::Item;
}

AItemActor::AItemActor()
{
	PrimaryActorTick.bCanEverTick = true;

	// ★★★ 생성자에서 레플리케이션 설정! ★★★
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;
	bNetUseOwnerRelevancy = false;
	NetDormancy = DORM_Awake;

	// Mesh 생성
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	// 충돌 설정
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetGenerateOverlapEvents(true);
    
	// ★★★ 물리 시뮬레이션 설정 ★★★
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
    
	// 물리 레플리케이션 설정
	Mesh->SetIsReplicated(true);
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	// ItemDef 레플리케이트
	DOREPLIFETIME(AItemActor, ItemDef);
}


void AItemActor::InitFromItem(const UItemBase* Item)
{
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitFromItem: Item is null"));
		return;
	}

	ItemDef = Item->ItemDef;
	ApplyFromDataAsset();
    
	UE_LOG(LogTemp, Log, TEXT("InitFromItem: Initialized %s"), *GetNameFromItem());
}
void AItemActor::ApplyFromDataAsset()
{
	if (!ItemDef)
	{
		return;
	}

	if (ItemDef->WorldMesh)
	{
		Mesh->SetStaticMesh(ItemDef->WorldMesh);
	}
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	if (ItemDef)
	{
		ApplyFromDataAsset();
	}
}

UItemBase* AItemActor::MakeItemInstance(UInventoryComponent* InventoryComponent, AActor* OwnerRef)
{
	if (!ItemDef)
	{
		UE_LOG(LogTemp, Error, TEXT("MakeItemInstance: ItemDef is null"));
		return nullptr;
	}
    
	UItemBase* NewItem = NewObject<UItemBase>(OwnerRef, UItemBase::StaticClass());
	if (!NewItem)
	{
		UE_LOG(LogTemp, Error, TEXT("MakeItemInstance: Failed to create UItemBase instance"));
		return nullptr;
	}

	if (!NewItem->Initialize(ItemDef))
	{
		UE_LOG(LogTemp, Error, TEXT("MakeItemInstance: Failed to initialize UItemBase"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("MakeItemInstance: Created UItemBase for %s (GUID: %s)"),
		   *ItemDef->DisplayName.ToString(),
		   *NewItem->ItemGuid.ToString());

	return NewItem;
}

void AItemActor::PrepareForPickup()
{
	if (Mesh)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetEnableGravity(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
    
	SetActorTickEnabled(false);
    
	UE_LOG(LogTemp, Log, TEXT("PrepareForPickup: %s prepared for destruction"), 
		   *GetNameFromItem());
}
