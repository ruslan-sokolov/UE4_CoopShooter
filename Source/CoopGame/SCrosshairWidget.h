// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SCrosshairWidget.generated.h"

/**
 * 
 */

class UImage;
class ASCharacter;

UCLASS()
class COOPGAME_API USCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadWrite, Category = "Spread Params")
		float CrosshairTranslationOffsetMax = 30.0f;

	float SpreadAngleClampMin = 1.0f;
	float SpreadAngleClampMax = 6.0f;

	/** 4 Line Dynamic Crosshair Part Left Line Image*/
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UImage* LeftCrosshairImage;

	/** 4 Line Dynamic Crosshair Part Right Line Image*/
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UImage* RightCrosshairImage;

	/** 4 Line Dynamic Crosshair Part Top Line Image*/
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UImage* TopCrosshairImage;

	/** 4 Line Dynamic Crosshair Part Bottom Line Image*/
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
		UImage* BottomCrosshairImage;

	/** Character REF to get weapon spread */
	ASCharacter* Character;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void NativeConstruct() override;

	float GetCurrentTranslationOffset();

	void SetImagesTranslationOffset();

};
