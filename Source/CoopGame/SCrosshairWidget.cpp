// Fill out your copyright notice in the Description page of Project Settings.


#include "SCrosshairWidget.h"

#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Blueprint/WidgetTree.h"

#include "SCharacter.h"
#include "SWeapon.h"

void USCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	SetImagesTranslationOffset();

}

void USCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Character = GetOwningPlayerPawn<ASCharacter>();

	if (Character && Character->Weapon) {

		ASWeapon* Weapon = Character->Weapon;

		SpreadAngleClampMin = Weapon->SpreadBaseAngle * Weapon->SpreadModifiers.TotalModifierMin;
		SpreadAngleClampMax = Weapon->SpreadBaseAngle * Weapon->SpreadModifiers.TotalModifierMax;

	}
}

float USCrosshairWidget::GetCurrentTranslationOffset()
{

	if (Character && Character->Weapon) 
	{
		return (Character->Weapon->CurrentSpreadAngle - SpreadAngleClampMin) / 
			(SpreadAngleClampMax - SpreadAngleClampMin) * CrosshairTranslationOffsetMax;
	}
	else 
	{
		return 0.0f;
	}
}

void USCrosshairWidget::SetImagesTranslationOffset()
{

	float Offset = GetCurrentTranslationOffset();

	LeftCrosshairImage->SetRenderTranslation(FVector2D(-Offset, 0.0f));
	RightCrosshairImage->SetRenderTranslation(FVector2D(Offset, 0.0f));
	TopCrosshairImage->SetRenderTranslation(FVector2D(0.0f, -Offset));
	BottomCrosshairImage->SetRenderTranslation(FVector2D(0.0f, Offset));

}
