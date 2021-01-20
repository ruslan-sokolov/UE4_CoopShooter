// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CoopHUD.generated.h"

class USCrosshairWidget;

/**
 * 
 */

UCLASS()
class COOPGAME_API ACoopHUD : public AHUD
{
	GENERATED_BODY()

	ACoopHUD();

protected:

	/** Default character crosshair **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
		TSubclassOf<USCrosshairWidget> DefaultCrosshairClass;

	USCrosshairWidget* CurrentCrosshair;

public:
	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

	/** Set New Crosshair **/
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
		void SetCrosshair(TSubclassOf<USCrosshairWidget> NewCrosshair);

	/** Restore Default Crosshair **/
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
		void RestoreDefaultCrosshair();

};
