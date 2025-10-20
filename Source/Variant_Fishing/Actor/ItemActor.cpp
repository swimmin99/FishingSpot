



#include "Variant_Fishing/Actor/ItemActor.h"
#include "Net/UnrealNetwork.h"
#include "Variant_Fishing/Data/ItemBase.h"
#include "Variant_Fishing/Data/FishData.h"
#include "Fishing.h"

AItemActor::AItemActor()
{
    PrimaryActorTick.bCanEverTick = true;

    
    bReplicates = true;
    SetReplicateMovement(true);
    bAlwaysRelevant = true;
    bNetUseOwnerRelevancy = false;
    NetDormancy = DORM_Awake;

    
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    Mesh->SetGenerateOverlapEvents(true);
    
    
    Mesh->SetSimulatePhysics(true);
    Mesh->SetEnableGravity(true);
    
    
    Mesh->SetIsReplicated(true);
}

void AItemActor::BeginPlay()
{
    Super::BeginPlay();
    
    
}

void AItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AItemActor, ItemDataProvider);
}




void AItemActor::InitializeFromItem(UItemBase* Item)
{
    if (!Item)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeFromItem: Item is null"));
        return;
    }

    
    ItemDataProvider = Item->ItemDataProvider;
    
    
    ItemReference = Item;
    
    
    ApplyFromDataProvider();
    
    
    UE_LOG(LogTemp, Log, TEXT("InitializeFromItem: Initialized %s"), *GetNameFromItem());
}

void AItemActor::ApplyFromDataProvider()
{
    if (!ItemDataProvider)
    {
        UE_LOG(LogTemp, Log, TEXT("ApplyFromDataProvider: Failed due to lack of ItemDataProvider"));
        return;
    }

    UStaticMesh* WorldMesh = IItemDataProvider::Execute_GetWorldMesh(ItemDataProvider.GetObject());
    if (WorldMesh && Mesh)
    {
        Mesh->SetStaticMesh(WorldMesh);
        UE_LOG(LogTemp, Log, TEXT("ApplyFromDataProvider: Applied mesh for %s"), 
               *GetNameFromItem());
    }
}




UItemBase* AItemActor::MakeItemInstance(UInventoryComponent* InventoryComponent, AActor* OwnerRef)
{
    
    if (ItemReference.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("MakeItemInstance: Returning existing ItemBase with preserved data"));
        return ItemReference.Get();
    }
    
    
    if (!ItemDataProvider)
    {
        UE_LOG(LogTemp, Error, TEXT("MakeItemInstance: ItemDataProvider is null"));
        return nullptr;
    }
    
    UItemBase* NewItem = NewObject<UItemBase>(OwnerRef, UItemBase::StaticClass());
    if (!NewItem)
    {
        UE_LOG(LogTemp, Error, TEXT("MakeItemInstance: Failed to create UItemBase instance"));
        return nullptr;
    }

    
    NewItem->ItemDataProvider = ItemDataProvider;
    NewItem->ItemGuid = FGuid::NewGuid();

    UE_LOG(LogTemp, Log, TEXT("MakeItemInstance: Created new UItemBase for %s (GUID: %s)"),
           *GetNameFromItem(),
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




FString AItemActor::GetNameFromItem() const
{
    if (!ItemDataProvider)
    {
        return TEXT("NoItem");
    }
    return IItemDataProvider::Execute_GetDisplayName(ItemDataProvider.GetObject()).ToString();
}

FString AItemActor::GetInteractableName_Implementation() const
{
    return GetNameFromItem();
}

EInteractionType AItemActor::GetType_Implementation() const
{
    return EInteractionType::Item;
}