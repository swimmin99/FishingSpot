#include "Variant_Fishing/Actor/ItemActor.h"
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

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetSimulatePhysics(true);
}

void AItemActor::InitFromItem(const UItemBase* Item)
{
	if (!Item)
	{
		return;
	}

	ItemDef = Item->ItemDef;

	ApplyFromDataAsset();

	InitPhysicsSetting();
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

void AItemActor::InitPhysicsSetting()
{
	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;
	bNetUseOwnerRelevancy = false;
	NetDormancy = DORM_Awake;
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
