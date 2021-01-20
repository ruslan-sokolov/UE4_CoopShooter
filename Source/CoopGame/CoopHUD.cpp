// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopHUD.h"

#include "SCrosshairWidget.h"


ACoopHUD::ACoopHUD()
{

}


void ACoopHUD::DrawHUD()
{
	AHUD::DrawHUD();

	if (DefaultCrosshairClass) 
		SetCrosshair(DefaultCrosshairClass);
	else 
		UE_LOG(LogTemp, Warning, TEXT("Please Setup DefaultCrosshairClass for current HUD class."));

}


void ACoopHUD::SetCrosshair(TSubclassOf<USCrosshairWidget> NewCrosshair)
{
	if (CurrentCrosshair)
		CurrentCrosshair->RemoveFromParent();

	CurrentCrosshair = CreateWidget<USCrosshairWidget>(GetWorld(), NewCrosshair);
	CurrentCrosshair->AddToViewport();

}


void ACoopHUD::RestoreDefaultCrosshair()
{
	if (DefaultCrosshairClass && !CurrentCrosshair->IsA(DefaultCrosshairClass))
		SetCrosshair(DefaultCrosshairClass);
}