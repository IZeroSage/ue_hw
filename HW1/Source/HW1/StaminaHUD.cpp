#include "StaminaHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "MechanicsCharacter.h"

namespace ResourceHUD
{
	void Bar(AHUD* HUD, float X, float Y, float Width, float Ratio, float Scale,
		FColor CenterColor)
	{
		auto Rect = [&](FLinearColor Color, float DX, float DY, float W, float H)
		{
			HUD->DrawRect(Color, X + DX * Scale, Y + DY * Scale, W * Scale, H * Scale);
		};
		Rect({0.f, 0.f, 0.f, 0.42f}, 0, 1.5f, Width + 2.f, 14.f);
		Rect({0.09f, 0.09f, 0.075f}, 0, 0, Width, 13.f);
		Rect({0.38f, 0.36f, 0.28f}, 0, 0, Width, 1.f);
		Rect({0.23f, 0.22f, 0.18f}, 0, 12.f, Width, 1.f);
		Rect({0.30f, 0.29f, 0.23f}, Width - 1.f, 1.f, 1.f, 11.f);
		Rect({0.045f, 0.055f, 0.052f}, 2.f, 2.f, Width - 5.f, 9.f);
		Rect({0.11f, 0.12f, 0.105f}, 2.f, 2.f, Width - 5.f, 1.f);
		const float Fill = (Width - 5.f) * FMath::Clamp(Ratio, 0.f, 1.f);
		if (Fill <= 0.f) return;
		// Invert Canvas's display-gamma transfer, preserving sampled screen RGB values.
		// The sRGB constructor lifts dark tones slightly in this Canvas rendering path.
		const float Gamma = GEngine ? GEngine->GetDisplayGamma() : 2.2f;
		auto CanvasColor = [Gamma](FColor Color)
		{
			return FLinearColor(FMath::Pow(Color.R / 255.f, Gamma),
				FMath::Pow(Color.G / 255.f, Gamma), FMath::Pow(Color.B / 255.f, Gamma));
		};
		const FLinearColor Center = CanvasColor(CenterColor);
		const FColor EdgeColor(
			FMath::Min(255, int32(CenterColor.R) + 32),
			FMath::Min(255, int32(CenterColor.G) + 32),
			FMath::Min(255, int32(CenterColor.B) + 32));
		const FLinearColor Edge = CanvasColor(EdgeColor);
		for (int32 Row = 0; Row < 8; ++Row)
		{
			// The two central rows keep the exact requested base color.
			const float EdgeWeight = FMath::Max(0.f, (FMath::Abs(Row - 3.5f) - 0.5f) / 3.f);
			Rect(FMath::Lerp(Center, Edge, EdgeWeight), 2.f, 3.f + Row, Fill, 1.f);
		}
	}
}

AStaminaHUD::AStaminaHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Texture(TEXT("/Game/UI/T_SunMedallion.T_SunMedallion"));
	MedallionTexture = Texture.Object;
}

void AStaminaHUD::DrawHUD()
{
	Super::DrawHUD();
	const AMechanicsCharacter* Character = Cast<AMechanicsCharacter>(GetOwningPawn());
	if (!Canvas || !Character || !GEngine) return;

	const float Scale = FMath::Max(0.25f, FMath::Min(Canvas->SizeX / 1280.f, Canvas->SizeY / 720.f));
	const FVector2D Center(74.f * Scale, 64.f * Scale);
	const float BarX = 99.f * Scale, BarY = 41.f * Scale, StaminaWidth = 238.f;
	const float Ratio = Character->Stamina / FMath::Max(1.f, Character->MaxStamina);
	ResourceHUD::Bar(this, BarX, BarY, 296.f, DecorativeHealth, Scale, HealthCenterColor);
	ResourceHUD::Bar(this, BarX, BarY + 16.f * Scale, 202.f, DecorativeMana, Scale, ManaCenterColor);
	ResourceHUD::Bar(this, BarX, BarY + 32.f * Scale, StaminaWidth, Ratio, Scale, StaminaCenterColor);
	// Alpha-blended UI texture overlaps the left bar caps; no background or pedestal.
	if (MedallionTexture)
		DrawTexture(MedallionTexture, Center.X - 32.f * Scale, Center.Y - 32.f * Scale,
			64.f * Scale, 64.f * Scale, 0.f, 0.f, 1.f, 1.f);

	const FString Value = FString::Printf(TEXT("%.0f / %.0f"), Character->Stamina, Character->MaxStamina);
	const float TextScale = 0.85f * Scale;
	float TextWidth, TextHeight;
	GetTextSize(Value, TextWidth, TextHeight, GEngine->GetSmallFont(), TextScale);
	const float TextX = BarX + StaminaWidth * Scale - TextWidth, TextY = BarY + 49.f * Scale;
	DrawText(Value, FLinearColor::Black, TextX + Scale, TextY + Scale, GEngine->GetSmallFont(), TextScale);
	DrawText(Value, {0.78f, 0.76f, 0.65f}, TextX, TextY, GEngine->GetSmallFont(), TextScale);
	if (Character->IsSprintLocked())
	{
		DrawText(TEXT("RELEASE SHIFT"), FLinearColor::Black, BarX + Scale, TextY + Scale, GEngine->GetSmallFont(), TextScale);
		DrawText(TEXT("RELEASE SHIFT"), {0.70f, 0.59f, 0.42f}, BarX, TextY, GEngine->GetSmallFont(), TextScale);
	}
}
