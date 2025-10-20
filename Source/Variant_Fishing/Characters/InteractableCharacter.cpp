
#include "InteractableCharacter.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Net/UnrealNetwork.h"

AInteractableCharacter::AInteractableCharacter()
{
    bReplicates = true;

    
    InventoryPortraitCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("InventoryPortraitCamera"));
    InventoryPortraitCamera->SetupAttachment(RootComponent);
    InventoryPortraitCamera->SetRelativeLocation(FVector(220.0f, 0.f, 0.f));
    InventoryPortraitCamera->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    
    InventoryPortraitCamera->bCaptureEveryFrame = false;
    InventoryPortraitCamera->bCaptureOnMovement = false;
    InventoryPortraitCamera->CaptureSource = SCS_SceneColorHDR;
    InventoryPortraitCamera->ShowFlags.SetTranslucency(true);
    
    
    InventoryPortraitCamera->FOVAngle = 45.0f;
}

void AInteractableCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        SetupPortraitCamera();
    }
}

void AInteractableCharacter::SetupPortraitCamera()

{

    if (!InventoryPortraitCamera)

    {

        return;

    }



    USkeletalMeshComponent* MeshComp = GetMesh();

    if (!MeshComp)

    {

        UE_LOG(LogTemp, Warning, TEXT("SetupPortraitCamera: Mesh is null for %s"), *GetName());

        return;

    }

    
    InventoryPortraitCamera->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

    InventoryPortraitCamera->ShowOnlyComponents.Empty();

    InventoryPortraitCamera->ShowOnlyComponent(MeshComp);

    InventoryPortraitCamera->ProjectionType = ECameraProjectionMode::Orthographic;

    InventoryPortraitCamera->OrthoWidth = 100.0f;

    InventoryPortraitCamera->ShowFlags.SetAtmosphere(false);
    InventoryPortraitCamera->ShowFlags.SetFog(false);
    InventoryPortraitCamera->ShowFlags.SetVolumetricFog(false);
    InventoryPortraitCamera->ShowFlags.SetSkyLighting(false);
    InventoryPortraitCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
    InventoryPortraitCamera->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    InventoryPortraitCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
    InventoryPortraitCamera->PostProcessSettings.AutoExposureBias = 1.0f;
    InventoryPortraitCamera->ShowFlags.SetLighting(false);

    UE_LOG(LogTemp, Log, TEXT("✅ SetupPortraitCamera completed for %s"), *GetName());

}

void AInteractableCharacter::SetPortraitRenderTarget(UTextureRenderTarget2D* RenderTarget)
{
    if (InventoryPortraitCamera)
    {
        InventoryPortraitCamera->TextureTarget = RenderTarget;
        UE_LOG(LogTemp, Log, TEXT("Portrait RenderTarget %s for %s"), 
            RenderTarget ? TEXT("assigned") : TEXT("released"), *GetName());
    }
}

void AInteractableCharacter::SetPortraitCaptureEnabled(bool bEnabled)
{
    if (InventoryPortraitCamera)
    {
        InventoryPortraitCamera->bCaptureEveryFrame = bEnabled;
        UE_LOG(LogTemp, Verbose, TEXT("Portrait capture %s for %s"), 
            bEnabled ? TEXT("enabled") : TEXT("disabled"), *GetName());
    }
}

void AInteractableCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AInteractableCharacter, Gold);
}

void AInteractableCharacter::OnRep_Gold()
{
    OnGoldChanged.Broadcast(Gold);
}

void AInteractableCharacter::ApplyGoldDelta(int32 Delta)
{
    if (!HasAuthority()) return;
    const int32 NewGold = FMath::Max(0, Gold + Delta);
    if (NewGold == Gold) return;
    Gold = NewGold;
    OnGoldChanged.Broadcast(Gold);
}

void AInteractableCharacter::AddGold(int32 Amount)
{
    if (HasAuthority())
    {
        ApplyGoldDelta(Amount);
    }
    else
    {
        Server_AddGold(Amount);
    }
}

void AInteractableCharacter::SetGold(int32 NewGold)
{
    if (HasAuthority())
    {
        const int32 Clamped = FMath::Max(0, NewGold);
        if (Clamped == Gold) return;
        Gold = Clamped;
        OnGoldChanged.Broadcast(Gold);
    }
    else
    {
        Server_SetGold(NewGold);
    }
}

void AInteractableCharacter::Server_AddGold_Implementation(int32 Amount)
{
    ApplyGoldDelta(Amount);
}

void AInteractableCharacter::Server_SetGold_Implementation(int32 NewGold)
{
    SetGold(NewGold);
}