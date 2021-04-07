// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHealingComponent.generated.h"


class USHealthComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActorHealStart, AActor*, HealActor, float, HealthBefore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorHealEnd, AActor*, HealActor, float, HealthAfter, bool, Success);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOPGAME_API USHealingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USHealingComponent();

protected:

	UPROPERTY(EditDefaultsOnly, Category = "HealingComponent", meta = (ClampMin = 0.1f, ClampMax = 120.0f))
		float HealTime;

	UPROPERTY(EditDefaultsOnly, Category = "HealingComponent", meta = (ClampMin = 0.1f, ClampMax = 5.0f))
		float HealTick;

	float HealthOnHealStart;

	FTimerHandle TimerHandle_Heal;

	USHealthComponent* OwnerHealthComp;

	UFUNCTION()
		void Heal();

	void EndHealing(bool bSkip);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "HealingComponent")
		bool bHealingInProgress;

public:

	UFUNCTION(BlueprintCallable, Category = "HealingComponent")
		void StartHealing();
	
	UFUNCTION(BlueprintCallable, Category = "HealingComponent")
		void InterruptHealing();

	UPROPERTY(BlueprintAssignable, Category = "HealingComponent")
		FOnActorHealStart OnActorHealStart;

	UPROPERTY(BlueprintAssignable, Category = "HealingComponent")
		FOnActorHealEnd OnActorHealEnd;

};

