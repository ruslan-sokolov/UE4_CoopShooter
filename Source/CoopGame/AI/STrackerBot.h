// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

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
	
	// [Tracker bot Move resolving logic] ////////////////////////////////////////////////////////////////////////////////

	FVector GetNextPathPoint();
	
	// Next point in navigation path
	FVector NextPathPoint;

	/** Force Applied To Mesh Comp Every MoveTick */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
		float MovementForce;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
		bool bUseVelocityChange;
	
	/** Allowed Distance Inaccuracy to consider NextPathPoint reached */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
		float RequiredDistanceToTarget;

	/** Delay when MoveToNextPathPoint will be called */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot")
		float MoveControlTick;

	FTimerHandle TimerHandle_MoveControl;

	void UpdateNextPathPoint_WhenReach();

	void OnTimer_MoveControl();

	void OnTick_MoveToNextPathPoint();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot Stuck resolving logic] ///////////////////////////////////////////////////////////////////////////////

	/** When TracketBot stuck somewhere it will be launched to NextPathPoint with LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracket Bot")
		bool bUseLaunchWhenStuck;

	/** X vector val here is towards direction launch impulse to NextPathPoint */
	UPROPERTY(EditDefaultsOnly, Category = "Tracket Bot", meta = (EditCondition = "bUseLaunchWhenStuck"))
		FVector LaunchOnStuckImpulse;

	/** When TrackerBot stays in StuckDistanceDelta range that amount of time - it will recieve LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot", meta = (EditCondition = "bUseLaunchWhenStuck"))
		float TimeToConsiderStuck;

	float TimeToConsiderStuckAccomulation;

	/** If TrackerBot stayes in this range for TimeToConsiderStuck - it will be recieve LaunchOnStuckImpulse*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot", meta = (EditCondition = "bUseLaunchWhenStuck"))
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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
