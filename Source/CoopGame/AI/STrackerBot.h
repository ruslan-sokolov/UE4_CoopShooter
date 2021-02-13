// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class USHealthComponent;
class URadialForceComponent;

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
		UStaticMeshComponent* MeshComp;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
		USHealthComponent* HealthComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
		URadialForceComponent* RadialForceComp;

	// [Tracker bot Move resolving logic] ////////////////////////////////////////////////////////////////////////////////

	FVector GetNextPathPoint();
	
	// Next point in navigation path
	FVector NextPathPoint;

	/** Force Applied To Mesh Comp Every MoveTick */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Move")
		float MovementForce;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Move")
		bool bUseVelocityChange;
	
	/** Allowed Distance Inaccuracy to consider NextPathPoint reached */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Move")
		float RequiredDistanceToTarget;

	/** Delay when MoveToNextPathPoint will be called */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Move")
		float MoveControlTick;

	FTimerHandle TimerHandle_MoveControl;

	void UpdateNextPathPoint_WhenReach();

	void OnTimer_MoveControl();

	void OnTick_MoveToNextPathPoint();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot Stuck resolving logic] ///////////////////////////////////////////////////////////////////////////////
	
	/** When TracketBot stuck somewhere it will be launched to NextPathPoint with LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Launch")
		bool bUseLaunchWhenStuck;

	/** X vector val here is towards direction launch impulse to NextPathPoint */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Launch", meta = (EditCondition = "bUseLaunchWhenStuck"))
		FVector LaunchOnStuckImpulse;

	/** When TrackerBot stays in StuckDistanceDelta range that amount of time - it will recieve LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Launch", meta = (EditCondition = "bUseLaunchWhenStuck"))
		float TimeToConsiderStuck;

	float TimeToConsiderStuckAccomulation;

	/** If TrackerBot stayes in this range for TimeToConsiderStuck - it will be recieve LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Launch", meta = (EditCondition = "bUseLaunchWhenStuck"))
		float StuckDistanceDelta;

	/* Last MoveTick Actor Location (used for initialization tracker bot possible stuck location) */
	FVector StuckCheck_PrevActorLocation;

	FVector PossibleStuckLocation;

	/** bool out val from IsStuckDesisionMake() */
	bool bPossiblyStuck;

	void HandleStuck_OnTimer_MoveControl();

	bool IsStuckDesisionMake();

	void LaunchOnStuck();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	UFUNCTION()
		void OnHealthChange_HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
			const class UDamageType* InstigatedDamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// Dynamic material to pulse on damage
	UMaterialInstanceDynamic* MatInst;

	void SelfDestruct();

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		bool bDoFullDamage;

	UPROPERTY(VisibleDefaultsOnly, Category = "Tracker Bot: Explosion")
		TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		bool bExplosionApplyRadialImpulse;

	bool bExploded;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
