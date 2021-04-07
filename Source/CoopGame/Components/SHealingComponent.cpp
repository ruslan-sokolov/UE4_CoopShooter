// Fill out your copyright notice in the Description page of Project Settings.


#include "SHealingComponent.h"
#include "SHealthComponent.h"

USHealingComponent::USHealingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	HealTick = 0.2f;
	HealTime = 5.0f;


	AActor* Owner = GetOwner();

	if (Owner)
	{
		OwnerHealthComp = Cast<USHealthComponent>(Owner->FindComponentByClass(USHealthComponent::StaticClass()));
	}
	
	if (OwnerHealthComp == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SHealingComponent not found owner actor health component."));
	}

	bHealingInProgress = false;
}

void USHealingComponent::StartHealing()
{
	if (!OwnerHealthComp) return;

	bHealingInProgress = true;

	HealthOnHealStart = OwnerHealthComp->GetHealth();

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Heal, this, &USHealingComponent::Heal, HealTick, true);

	OnActorHealStart.Broadcast(GetOwner(), HealthOnHealStart);
}

void USHealingComponent::EndHealing(bool bSkip)
{
	if (!bHealingInProgress) return;

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Heal);
	
	OnActorHealEnd.Broadcast(GetOwner(), OwnerHealthComp->GetHealth(), bSkip);

	bHealingInProgress = false;
}

void USHealingComponent::InterruptHealing()
{
	if (!OwnerHealthComp) return;

	EndHealing(true);
}

void USHealingComponent::Heal()
{
	float HealAmount = (OwnerHealthComp->DefaultHealth - HealthOnHealStart) * (HealTick / HealTime);
	
	OwnerHealthComp->Heal(HealAmount);

	if (OwnerHealthComp->GetHealth() >= OwnerHealthComp->DefaultHealth)
	{
		EndHealing(false);
	}
}
