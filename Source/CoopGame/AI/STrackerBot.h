// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class USHealthComponent;
class URadialForceComponent;
class UAudioComponent;

static int32 DebugTrackerBot;

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

	ACharacter* ChasedCharacter;

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

	void ChooseTargetCharacter();

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

	UFUNCTION(NetMulticast, Unreliable)
		void PlayLaunchFX();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot explosion logic] /////////////////////////////////////////////////////////////////////////////////////

	UFUNCTION()
		void OnHealthChange_HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
			const class UDamageType* InstigatedDamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
		void OnHealthChangedClient();

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

	void PlayExplosionEffects();

	//UFUNCTION()
	//	void Destroy_OnPSCExplosionFinish(UParticleSystemComponent* PSC);

	UFUNCTION()
		void OnRep_Exploded();

	UPROPERTY(ReplicatedUsing=OnRep_Exploded)
		bool bExploded;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot sound effects] ///////////////////////////////////////////////////////////////////////////////////////

	UPROPERTY(EditDefaultsOnly, Category = "Components")
		UAudioComponent* BounceSound;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
		UAudioComponent* DelayedDetonationSound;

	/** When on MeshComp simulation hit impulse is lower then this value, sound will not be played */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Sound")
		float BounceSound_HitImpulseTrashhold;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		USoundBase* ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Launch")
		USoundBase* LaunchOnStuckSound;

	UFUNCTION()
		void OnMeshCompHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
			UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot near character jump and explode] /////////////////////////////////////////////////////////////////////

	/** If in  DistanceToJumpAndExplode, TrackerBot will jump to character and explode*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion")
		bool bJumpAndExplodeNearChasedCharacter;

	/** If Distance from TrackerBot to ChasedCharacter is lower than this val, Tracker bot will jump and explode */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bJumpAndExplodeNearChasedCharacter"))
		float DistanceToJumpAndExplode;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bJumpAndExplodeNearChasedCharacter"))
		FName ChasedCharacterBoneJumpTo;

	void CheckExplodeActivation_OnTimer_MoveControl();

	void RunDelayedDetonation();

	void PlayDelayedDetonationEffect();

	UFUNCTION()
		void OnRep_DelayedDetonationActivated();

	UPROPERTY(ReplicatedUsing=OnRep_DelayedDetonationActivated)
		bool bDelayedDetonationActivated;

	/** Extra Delay before explosion after jump max height Z reached */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bJumpAndExplodeNearChasedCharacter"))
		float JumpHeightReachExplosionDelay;

	/** Extra Z Height added to ChasedCharacterBoneLocation to Jump Before Detonation */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bJumpAndExplodeNearChasedCharacter"))
		float JumpExtraHeight;

	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bJumpAndExplodeNearChasedCharacter"))
		bool bAddAngularImpulseOnJumpExplosion;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Explosion", meta = (EditCondition = "bAddAngularImpulseOnJumpExplosion"))
		FVector AngularVelocityOnJumpExplosion;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// [Tracker bot damage boost] ////////////////////////////////////////////////////////////////////////////////////////
	
	/** Allow to increasing damage when other tracker bots are near by */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Damage Boost")
		bool bAllowDamageBoost;

	/** Time in seconds to update damage boost */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Damage Boost", meta = (EditCondition = "bAllowDamageBoost"))
		float DamageBoostCheckTime;

	/** Nearby bot check radius*/
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Damage Boost", meta = (EditCondition = "bAllowDamageBoost"))
		float NearbyBotCheckRadius;

	/** Number of nearby bot to max damage boost */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Damage Boost", meta = (EditCondition = "bAllowDamageBoost"))
		int32 NearbyBotMaxDamageBoost;

	UFUNCTION()
		void OnRep_DamageBoostUpdated();

	UPROPERTY(ReplicatedUsing=OnRep_DamageBoostUpdated)
		float CurrentDamageBoostNormalized;
	
	float CurrentDamageBoost;

	/** Max Damage boost mult, max val when nearby bot num equal NearbyBotMaxDamageBoost */
	UPROPERTY(EditDefaultsOnly, Category = "Tracker Bot: Damage Boost", meta = (EditCondition = "bAllowDamageBoost"))
		float MaxDamageBoost;

	FTimerHandle TimerHandle_CheckDamageBoost;

	void CheckDamageBoost();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
