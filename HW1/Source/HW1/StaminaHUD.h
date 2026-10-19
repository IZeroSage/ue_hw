#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StaminaHUD.generated.h"

class UTexture2D;

/** Souls-inspired bars. Health and mana are decoration; stamina uses gameplay data. */
UCLASS()
class HW1_API AStaminaHUD : public AHUD
{
	GENERATED_BODY()
public:
	AStaminaHUD();
	virtual void DrawHUD() override;

	UPROPERTY(EditDefaultsOnly, Category="HUD|Decoration", meta=(ClampMin="0", ClampMax="1"))
	float DecorativeHealth = 0.82f;

	UPROPERTY(EditDefaultsOnly, Category="HUD|Decoration", meta=(ClampMin="0", ClampMax="1"))
	float DecorativeMana = 0.68f;

	/** Center colors are stored as sRGB, matching sampled screen RGB values. */
	UPROPERTY(EditDefaultsOnly, Category="HUD|Colors")
	FColor HealthCenterColor = FColor(80, 39, 40);

	UPROPERTY(EditDefaultsOnly, Category="HUD|Colors")
	FColor ManaCenterColor = FColor(40, 40, 89);

	UPROPERTY(EditDefaultsOnly, Category="HUD|Colors")
	FColor StaminaCenterColor = FColor(65, 92, 63);

	UPROPERTY(EditDefaultsOnly, Category="HUD|Decoration")
	TObjectPtr<UTexture2D> MedallionTexture;
};
