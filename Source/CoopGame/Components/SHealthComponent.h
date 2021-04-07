// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SHealthComponent.generated.h"

// OnHealthChanged event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, USHealthComponent*, HealthComp, float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

/**
	Delegate on health change network clients only. 	// @Todo: add some param
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthChangedClientSignature);


UCLASS( ClassGroup=(COOP), meta=(BlueprintSpawnableComponent) )
class COOPGAME_API USHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USHealthComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HealthComponent")
		bool bHasTeam;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HealthComponent", meta = (EditCondition = "bHasTeam"))
		uint8 TeamNum;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	bool bIsDead;

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "HealthComponent")
		float Health;

	UFUNCTION()
		void OnRep_Health();

	UFUNCTION()
		void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, 
			class AController* InstigatedBy, AActor* DamageCauser);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent")
		float DefaultHealth;

	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnHealthChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Events")
		FOnHealthChangedClientSignature OnHealthChangedClient;

	FORCEINLINE float GetHealth() const { return Health; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "HealthComponent")
		void Heal(float HealAmount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HealthComponent")
		static bool IsFriendly(AActor* ActorLeft, AActor* ActorRight);
};
