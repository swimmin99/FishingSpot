
#include "InventoryPortrait.h"
#include "InteractableCharacter.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"

void UInventoryPortrait::InitializeWidget(AInteractableCharacter* InteractableCharacter, UMaterialInstanceDynamic* AlphaInvertMaterial)
{
	if (!InteractableCharacter || !PortraitImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryPortrait::InitializeWidget - Invalid parameters"));
		return;
	}

	ConnectedCharacter = InteractableCharacter;

			if (AlphaInvertMaterial)
			{
			
				FSlateBrush Brush;
				Brush.SetResourceObject(AlphaInvertMaterial);
				Brush.DrawAs = ESlateBrushDrawType::Image;
				Brush.TintColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
				
				PortraitImage->SetBrush(Brush);
				
				UE_LOG(LogTemp, Log, TEXT("✅ Portrait with inverted alpha set for %s"), 
					*InteractableCharacter->GetName());
			}


}