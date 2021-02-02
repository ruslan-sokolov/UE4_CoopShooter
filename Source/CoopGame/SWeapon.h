// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CoopGame.h"
#include "SCharacter.h"

#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageClass;
class UParticleSystem;
class USoundBase;
class UCameraShake;
class UUserWidget;
class UAnimMontage;
class UAnimInstance;
class APlayerController;

static int32 DebugWeaponDrawing_Shot;
static int32 DebugWeaponDrawing_Recoil;
static int32 DebugWeaponDrawing_Spread;
static int32 DebugWeaponDrawing_RecoilCompensate;


UENUM(BlueprintType)
enum class WeaponFireMode : uint8
{

	Semiauto,
	Fullauto,

};

UENUM(BlueprintType)
enum class CharacterAimPose : uint8
{

	Standing,
	Crouch,
	Prone, // can't be used yet
	InAir,

	Vehicle, // never used
	Cover, // never used
};

FORCEINLINE CharacterAimPose CharStateToAimPose(ECharacterState CharState)
{
	if (CharState == ECharacterState::Run) { return CharacterAimPose::Standing; }
	else if (CharState == ECharacterState::Sprint) { return CharacterAimPose::Standing; }
	else if (CharState == ECharacterState::Idle) { return CharacterAimPose::Standing; }
	else if (CharState == ECharacterState::Crouch) { return CharacterAimPose::Crouch; }
	else if (CharState == ECharacterState::Jumping) { return CharacterAimPose::InAir; }
	else if (CharState == ECharacterState::Falling) { return CharacterAimPose::InAir; }
	else { return CharacterAimPose::Standing; }
};

USTRUCT(BlueprintType)
struct FCharacterShootModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Modifiers")
		float StandingModifier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Modifiers")
		float CrouchModifier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Modifiers")
		float ProneModifier = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Modifiers")
		float InAirModifier = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Pose Modifiers")
		TMap<CharacterAimPose, float> CharPoseModifiersMap = {
	
		{CharacterAimPose::Standing, StandingModifier},
		{CharacterAimPose::Crouch, CrouchModifier},
		{CharacterAimPose::Prone, ProneModifier},
		{CharacterAimPose::InAir, InAirModifier}
	};

	/** Current Accomulated FireRateModifier. */
	UPROPERTY(BlueprintReadOnly, Category = "Pose Modifiers")
		float CurrentPosModifier;


	/** Modifier When Character Speed Is Near Zero (Lower Modifier Linear Interpolation Border) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Modifiers", meta = (ClampMin = 0.0f, ClampMax = 20.0f))
		float VelocityModiferMin = 1.0f;

	/** Modifier When Character Speed (Velocity size) is max (equal to base run speed) (Top Modifier Linear Inperpolation Border) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Modifiers", meta = (ClampMin = 0.001f, ClampMax = 20.0f))
		float VelocityModiferMax = 2.0f;

	/** Modifier When Character Speed (Velocity Size) is Equal to ero.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Modifiers", meta = (ClampMin = 0.001f, ClampMax = 20.0f))
		float VelocityModifierZero = 0.4f;

	/** Time to smoothly set modifier When Modifier Changed from bigger to lesser value. When 1.0 mean decrement modifier to 1.0f in one sec. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Velocity Modifiers", meta = (ClampMin = 0.001f, ClampMax = 20.0f))
		float VelocityModifierTime = 1;
	
	float VelocityModifierTo;

	/** Time to smoothly set modifier When Modifier Changed from bigger to lesser value. When 1.0 mean decrement modifier to 1.0f in one sec time.*/
	UPROPERTY(BlueprintReadOnly, Category = "Velocity Modifiers")
		float CurrentVelocityModifier;


	/** Modifier When Chracter Aim Using Weapon Scope/IronSight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Modifiers", meta = (ClampMin = 0.0f, ClampMax = 5.0f))
		float AimingModifierMin = 0.5f;

	/** Modifier When Chracter Aim Without Using Weapon Scope/IronSight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Modifiers", meta = (ClampMin = 0.001f, ClampMax = 5.0f))
		float AimingModifierMax = 1.2f;

	/** Time When AimingModifier Is Applied When Chracter Start Aiminig*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming Modifiers", meta = (ClampMin = 0.001f, ClampMax = 5.0f))
		float AimingModifierMinApplyTime = 0.5f;

	/** True if modifier applied else false */
	UPROPERTY(BlueprintReadOnly, Category = "Aiming Modifiers")
		bool bAimingModifierApplied;

	/** Current Smooth Tick Applied Aim Sight Modifier */
	UPROPERTY(BlueprintReadOnly, Category = "Aiming Modifiers")
		float CurrentAimingModifier;
	

	/** TimeBetweenShot To Add FireRatePenaltyPerShotMin to Current Accomulated Penalty Value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.01f, ClampMax = 5.0f))
		float FireRateTimeMinPenalty = 0.3f;

	/** TimeBetweenShot To Get FireRatePenaltyPerShotMax to Current Accomulated Penalty Value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.01f, ClampMax = 3.0f))
		float FireRateTimeMaxPenalty = 0.12f;

	/** Max FireRate Penalty can be added to current FireRateModifer (Max When TimeBetweenShots < FireRateTimeMaxPenalty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.01f, ClampMax = 5.0f))
		float FireRatePenaltyPerShotMax = 0.2f;

	/** Min FireRate Penalty can be added to current FireRateModifer (Min When TimeBetweenShots > FireRateTimeMinPenalty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = -5.0f, ClampMax = 5.0f))
		float FireRatePenaltyPerShotMin = -0.05f;

	/** FireRateModifier total maximum value can reach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.01f, ClampMax = 20.0f))
		float FireRateModifierMax = 3.0f;

	/** FireRateModifier total minimun value can reach */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.0f, ClampMax = 20.0f))
		float FireRateModifierMin = 0.5f;

	/** FireRateModifier  How much decrement FireRatePenaltyAccomulated in one sec. smoothed with tick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireRate Modifiers", meta = (ClampMin = 0.1f, ClampMax = 20.0f))
		float FireRatePenaltyTimeRecovery = 0.5f;

	/** Current Accomulated FireRateModifier. */
	UPROPERTY(BlueprintReadOnly, Category = "FireRate Modifiers")
		float CurrentFireRateModifier;


	/** Max Value Can Effect Base Weapon Spread Angle / Base Recoil Val. Greater Total Modifier Will be clamped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Total Modifier", meta = (ClampMin = 1.0f, ClampMax = 10.0f))
		float TotalModifierMax = 6.0f;

	/** Min Value Can Effect Base Weapon Spread Angle / Base Recoil Val. Lesser Total Modifier Will be clamped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Total Modifiers", meta = (ClampMin = 0.1f, ClampMax = 1.0f))
		float TotalModifierMin = 0.2f;

	/** Current Total Modifier Cache. Unsave to use. Setted in last GetTotalModifier() call. Use GetTotalModifier() for fresh valid val. */
	UPROPERTY(BlueprintReadOnly, Category = "Total Modifiers")
		float CurrentTotalModifier;

	/** Utility to return Character Pos Modifier. */
	FORCEINLINE float GetPosModifier() const {return CurrentPosModifier;}

	/** Utility to set Current Pos Modifier */
	FORCEINLINE float SetPosModifier(CharacterAimPose Pos) { CurrentPosModifier = CharPoseModifiersMap[Pos]; return CurrentPosModifier;  }


	/** Utility to return current Character Velocity Modifier. */
	FORCEINLINE float GetVelModifier() const { return CurrentVelocityModifier; }

	/** Utility to set current Character Velocity Modifier. */
	FORCEINLINE float SetVelModifier(float VelocityModifierNormalized)
	{
		VelocityModifierTo = VelocityModifierNormalized == 0.0f ? VelocityModifierZero :
			VelocityModifierNormalized * (VelocityModiferMax - VelocityModiferMin) + VelocityModiferMin;

		return CurrentVelocityModifier;

	}

	/**  Utility to smoothly change velocity modifier to lower in tick */
	FORCEINLINE void VelocityModifierDecrementTick(float DeltaTime)
	{
		if (CurrentVelocityModifier > VelocityModifierTo)
			CurrentVelocityModifier -= VelocityModifierTime * DeltaTime;
		else
			CurrentVelocityModifier = VelocityModifierTo;

	}


	/** Utility to Drop FireRate Modifier (when stop aiming or weapon changed or sprinting) */
	FORCEINLINE void DropFireRateModifier() { CurrentFireRateModifier = FireRateModifierMin; }

	/** Utility to clamp FireRate Current Modifier */
	FORCEINLINE void ClampFireRateModifier() 
	{
		if (CurrentFireRateModifier > FireRateModifierMax)
			CurrentFireRateModifier = FireRateModifierMax;
		else if (CurrentFireRateModifier < FireRateModifierMin)
			CurrentFireRateModifier = FireRateModifierMin;
	}

	/** Utility to Set FireRate Modifier. */
	FORCEINLINE float SetFireRateModifier(float TimeBetweenShot)
	{
		if (TimeBetweenShot <= FireRateTimeMaxPenalty) // add max penalty
		{
			CurrentFireRateModifier += FireRatePenaltyPerShotMax;
		}
		else if (TimeBetweenShot >= FireRateTimeMinPenalty) // add min penalty
		{
			CurrentFireRateModifier += FireRatePenaltyPerShotMin;
		}
		else // add penalty in range (FireRatePenaltyPerShotMin, FireRatePenaltyPerShotMax)
		{
			float NormalizedModifier = (TimeBetweenShot - FireRateTimeMinPenalty) / (FireRateTimeMaxPenalty - FireRateTimeMinPenalty);
			CurrentFireRateModifier +=  NormalizedModifier * (FireRatePenaltyPerShotMax - FireRatePenaltyPerShotMin) + FireRatePenaltyPerShotMin; 
		}
		
		ClampFireRateModifier();

		 return CurrentFireRateModifier;
	}

	/** Utility to Get Current FireRate Modifier. */
	FORCEINLINE float GetFireRateModifier() const { return CurrentFireRateModifier; }

	/** Utility to smoothly drop FireRatePenalty with each tick*/
	FORCEINLINE void FireRatePenaltyDecrementTick(float DeltaTime)
	{
		if (CurrentFireRateModifier == FireRateModifierMin)
			return;

		CurrentFireRateModifier -= FireRatePenaltyTimeRecovery * DeltaTime;

		ClampFireRateModifier();
	}


	/** Utility to apply Aiming Modifier with delay */
	FORCEINLINE void SetAimingModifier(bool bCharIsAiming)
	{
		if (bCharIsAiming) {
			bAimingModifierApplied = true;
		}
		else {
			bAimingModifierApplied = false;
			CurrentAimingModifier = AimingModifierMax;
		}
	}

	/** Utility to get Current Aiming Modifier */
	FORCEINLINE float GetAimingModifier() const { return CurrentAimingModifier; }
	
	/** Utility to smoothly change Aiming Modifier if Applied with each Tick */
	FORCEINLINE void AimingModifierChangeTick(float DeltaTime)
	{
		if (bAimingModifierApplied && CurrentAimingModifier > AimingModifierMin)
		{
			CurrentAimingModifier -= AimingModifierMin * DeltaTime / AimingModifierMinApplyTime;

			if (CurrentAimingModifier < AimingModifierMin)
				CurrentAimingModifier = AimingModifierMin;

		}
	}


	/** Utility to calculate all Tick depended modifiers */
	FORCEINLINE void AllModifiersChangeOnTick(float DeltaTime)
	{
		FireRatePenaltyDecrementTick(DeltaTime);
		AimingModifierChangeTick(DeltaTime);
		VelocityModifierDecrementTick(DeltaTime);
	}

	/** Utility to get Total Modifier. */
	FORCEINLINE float GetTotalModifier()
	{
		CurrentTotalModifier = GetPosModifier() * GetVelModifier() * GetFireRateModifier() * GetAimingModifier();

		if (CurrentTotalModifier > TotalModifierMax)
			CurrentTotalModifier = TotalModifierMax;
		else if (CurrentTotalModifier < TotalModifierMin)
			CurrentTotalModifier = TotalModifierMin;
	
		return CurrentTotalModifier;
	}


	FCharacterShootModifiers() { CurrentAimingModifier = AimingModifierMax; CurrentFireRateModifier = FireRateModifierMin; CurrentVelocityModifier = VelocityModiferMax;  }

	explicit FCharacterShootModifiers(float Standing, float Crouch, float Prone, float InAir, float VelocityMin, float VelocityMax, float FireRatePenaltyTimeMin, float FireRatePenaltyTimeMax, float FireRatePenalty) : 
		StandingModifier(Standing), CrouchModifier(Crouch), ProneModifier(Prone), InAirModifier(InAir), VelocityModiferMin(VelocityMin), VelocityModiferMax(VelocityMax), 
		FireRateTimeMinPenalty(FireRatePenaltyTimeMin), FireRateTimeMaxPenalty(FireRatePenaltyTimeMax), FireRatePenaltyPerShotMax(FireRatePenalty) 
	{ FCharacterShootModifiers(); }

};

USTRUCT(BlueprintType)
struct FRecoilInput
{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Clamp")
		float PitchMin = -0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Clamp")
		float PitchMax = -0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Clamp")
		float YawMin = -0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Clamp")
		float YawMax = 0.15f;

	/** Speed crosshair compensate recoil back*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Fade", meta = (ClampMin = 0.01f, ClampMax = 2.0f))
		float RecoilFadeTime = 0.75f;

	/** Time Delay in Sec when shooting is stopped and recoil compensation will start */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil: Fade", meta = (ClampMin = 0.01f, ClampMax = 2.0f))
		float RecoilFadeDelay = 0.2f;

	float RecoilAccumulationYaw; // use for smooth recoil auto compensate
	float RecoilAccumulationPitch;  // use for smooth recoil auto compensate

	float RecoilFadeProgress; // actual time when recoil compenstation played

	/* Utility to return Base Recoil Pitch Value In Range [PitchMin, PitchMax] */
	FORCEINLINE float GetPitchVal(float Modifier = 1.0f) { float Val = (PitchMin + (PitchMax - PitchMin) * FMath::FRand()) * Modifier;  RecoilAccumulationPitch += Val; return Val; }

	/* Utility to return Base Recoil Yaw Value In Range [YawMin, YawMax] */
	FORCEINLINE float GetYawVal(float Modifier = 1.0f) { float Val = (YawMin + (YawMax - YawMin) * FMath::FRand()) * Modifier; RecoilAccumulationYaw += Val; return Val; }

	/* Utility to Drop Recoil Accumulation Values */
	FORCEINLINE void ClearRecoilAcommulation() { RecoilAccumulationYaw = 0.0f; RecoilAccumulationPitch = 0.0f; RecoilFadeProgress = 0.0f; }

	FRecoilInput() {}
	explicit FRecoilInput(float PMin, float PMax, float YMin, float YMax) : PitchMin(PMin), PitchMax(PMax), YawMin(YMin), YawMax(YawMax) {}

};


// Contains information of a single hitscan weapon
USTRUCT()
struct FHitScanTrace
{

	GENERATED_BODY()
	
	UPROPERTY()
		FVector_NetQuantize ImpactPoint;

	UPROPERTY()
		TEnumAsByte<EPhysicalSurface> HitSurfaceType;

};


UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

	/**  Temporarly Change Player Camera FOV when aiming. 0 - don't change fov */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Aim")
		float ZoomFOV = 60.0f;

	/** Temporarly Change Player Camera FOV Speed when aiming. 0 - don't change fov Speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Aim", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
		float ZoomInterpSpeed = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Aim")
		float Range = 10000.0f;

	/** Spread In Degrees */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Aim")
		float SpreadBaseAngle = 5.0f;

	/** Actual current calculated Spread Angle for shot placement and Crosshair UMG Dynamic effect. */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon: Aim")
		float CurrentSpreadAngle;

	/**  Recoil Parameters */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Aim")
		FRecoilInput RecoilParams;

	/**  Base Weapon Damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Fire")
		float Damage = 20.0f;

	/**  HeadShot Damage Multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Fire")
		float HeadshotDamageMultiplier = 16.0f;

	/**  Weapon Fire rate in sec. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Fire")
		float FireRate = 0.12;

	/** Weapon Fire Mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Fire")
		WeaponFireMode FireMode = WeaponFireMode::Semiauto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Aim")
		bool bCameraShaking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Aim", meta = (EditCondition = "bCameraShaking"))
		float CameraShakeScale = 1.0f;

	/** Mag Ammo Capacity */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon: Ammo")
		int32 AmmoMax = 30;

	/** Current Ammo in Mag */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon: Ammo")
		int32 AmmoCurrent = 30;

	/** Monage Reload Speed Override, if 0 use Anim speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Ammo")
		float ReloadSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Ammo")
		UAnimMontage* ReloadAnimMontage;
	
	/** Change Weapon Owner Speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Owner: Speed")
		float CharacterSpeedModifier = 0.95f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon: Fire")
		TSubclassOf<UDamageType> DamageType;

	/** IF true then when we press shot when cooldown still not complete - shot will be placed as soon as cd finished */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Weapon: Fire")
		bool bShotCanBeDelayed = true;

	/** Semiauto mode smooth fire rate. */
	bool bShotIsDelayed;

	float TimeSinceLastShot;

	UPROPERTY(Replicated)
		FTimerHandle TimerHandle_FireCoolDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		FName MuzzleSocketName = "Muzzle";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		FName TargetTraceEffect = "Target";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		UParticleSystem* MuzzleEffect;

	/** Default Particle effect on weapon hit surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		UParticleSystem* ImpactEffectDefault;

	/** Particle effect on weapon hit surface and it's flesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		UParticleSystem* ImpactEffectFlesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		UParticleSystem* TraceEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		USoundBase* FireSound;

	/** TODO Sound To play when fire pressed and no ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX")
		USoundBase* NoAmmoSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: WeaponFX", meta = (EditCondition = "bCameraShaking"))
		TSubclassOf<UCameraShake> CameraShakeEffect;

	UFUNCTION()
		void SemiAutoFireTimerBind(bool ShotDelayed);

	FTimerDelegate SemiAutoFireTimerDel;

	/** Fire Func resolving fire effects and base shoot logic (damage, spread, firerate, firemode, etc) common with all weapons. Better to override this and not Fire() */
	virtual void FireLogic();

	// RELOAD BLOCK

	/** RELOAD TimerHandler */
	FTimerHandle TimerHandle_Reload;

	/** RELOAD function called to complete reloading logic when anim montage is end playing */
	void FinishReload();

	UFUNCTION(Server, Reliable)
		void ServerFinishReload();

	void PlayReloadAnim();

	void StopReloadAnim();

	// RELOAD BLOCK END

	/* SPREAD Calc Random Base Spread Angle For Shot */
	 FRotator CalcSpread();

	 /* RECOIL Add Recoil After Shot */
	 void AddRecoil();

	 /* RECOIL Compensate Recoil */
	 void CompensateRecoil(float DeltaSec);

	 // DEPR
	 ///** RELOAD Character Ownrer Anim Reference for reloading */
	 //UAnimInstance* CharacterAnim;

	 /** CAMERA SHAKE player controller ref */
	 APlayerController* OwnerPlayerController;

	 /* SPREAD Character ref to get velocity for spread cal etc */
	 UPROPERTY(Replicated)
		ASCharacter* CharOwner;

	 void WeaponLogicTick();
	 
	 UFUNCTION(Server, Unreliable)
		 void ServerWeaponLogicTick();

public:
	
	/** Player Pose Recoil Multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Aim")
		FCharacterShootModifiers SpreadModifiers;

	/** Player Pose Spread Multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: Aim")
		FCharacterShootModifiers RecoilModifiers;

	/** Current Crosshair To Override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon: UMG")
		TSubclassOf<class USCrosshairWidget> CrosshairOverride;

	UFUNCTION(BlueprintCallable, Category = "Weapon: Fire")
		virtual void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon: Ammo")
		virtual void StartReload();

	UFUNCTION(Server, Reliable)
		virtual void ServerStartReload();

	UFUNCTION(BlueprintCallable, Category = "Weapon: Ammo")
		virtual void InterruptReload();

	UFUNCTION(Server, Reliable)
		virtual void ServerInterruptReload();

	/** True if weapon reloading in current time False if not also used for anim reload replication */
	UPROPERTY(ReplicatedUsing = OnRep_Reloading, BlueprintReadOnly, Category = "Weapon: Ammo")
		bool bIsReloading;

	UFUNCTION()
		void OnRep_Reloading();

	/** IF key pressed true. else false */
	bool bShouldFire;

	void PlayFireEffects();
	void PlayImpactEffects();
	
	uint32 ShotCount;

	FHitResult LastHit;

	virtual void Tick(float DeltaTime) override;

	/** Attach Weapon To Character */
	UFUNCTION(BlueprintCallable, Category = "Weapon: Use")
		void AttachToASCharacter(ASCharacter* Character);

	UFUNCTION(Server, Reliable)
		void ServerAttachToASCharacter(ASCharacter* Character);

	virtual void BeginDestroy() override;

	UPROPERTY(ReplicatedUsing = OnRep_HitScanTrace)
		FHitScanTrace HitScanTrace;
	
	UFUNCTION()
		void OnRep_HitScanTrace();

};
