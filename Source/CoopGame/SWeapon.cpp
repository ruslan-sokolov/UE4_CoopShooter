// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"

#include "DrawDebugHelpers.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#include "GameFramework/PlayerController.h"

#include "PhysicalMaterials/PhysicalMaterial.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "SCrosshairWidget.h"
#include "CoopHUD.h"

#include "Net/UnrealNetwork.h"

#include "CoopGame.h"

#include "GameFramework/PlayerState.h"


FAutoConsoleVariableRef CVARDebugWeaponDrawing_Shot(
	TEXT("COOP.DebugWeapons.Shot"),
	DebugWeaponDrawing_Shot,
	TEXT("Draw Debug Lines For Weapon Shot Placement"),
	ECVF_Cheat);

FAutoConsoleVariableRef CVARDebugWeaponDrawing_Recoil(
	TEXT("COOP.DebugWeapons.Recoil"),
	DebugWeaponDrawing_Recoil,
	TEXT("Draw Debug Text For Weapon Recoil"),
	ECVF_Cheat);

FAutoConsoleVariableRef CVARDebugWeaponDrawing_RecoilCompensate(
	TEXT("COOP.DebugWeapons.RecoilCompensate"),
	DebugWeaponDrawing_RecoilCompensate,
	TEXT("Draw Debug Text For Weapon Recoil Compenstation"),
	ECVF_Cheat);

FAutoConsoleVariableRef CVARDebugWeaponDrawing_Spread(
	TEXT("COOP.DebugWeapons.Spread"),
	DebugWeaponDrawing_Spread,
	TEXT("Draw Debug Text For Weapon Spread"),
	ECVF_Cheat);


ASWeapon::ASWeapon()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	
	// Semi auto shot delay lambda
	SemiAutoFireTimerDel.BindLambda([this] {SemiAutoFireTimerBind(false); });

	// Initialize Hud Spread Values
	CurrentSpreadAngle = SpreadBaseAngle;

	DamageType = UDamageType::StaticClass();

	// Reload Anim Speed
	if (ReloadSpeed < 0.0f)
		ReloadSpeed = ReloadAnimMontage->SequenceLength;

	// Replication
	SetReplicates(true);
	SetReplicateMovement(true);

	NetUpdateFrequency = 60.0f;
	MinNetUpdateFrequency = 30.0f;
}


void ASWeapon::BeginDestroy()
{
	Super::BeginDestroy();

	// InterruptReload();

	if (CharOwner) 
	{
		CharOwner->CarriedWeaponSpeedModifier = 1.0f;
	
		// Weapon Crosshair Resolve
		APlayerController* OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController());

		if (CrosshairOverride && OwnerPlayerController)
		{
			ACoopHUD* HUD = Cast<ACoopHUD>(OwnerPlayerController->GetHUD());

			if (HUD)
				HUD->RestoreDefaultCrosshair();
		}
	}
}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();
}


void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// DBG ammo todo: debug variable check
	FVector Loc = MeshComp->GetSocketLocation(MuzzleSocketName);
	Loc += GetActorForwardVector() * -30;
	DrawDebugString(GetWorld(), Loc, *FString::Printf(TEXT("%d / %d"), AmmoCurrent, AmmoMax), (AActor*)0, FColor::White, DeltaTime);
	//
}


// Character Attach BLOCK  ////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::ClientAttachToASCharacter_Implementation()
{
	if (!CharOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::ClientAttachToASCharacter can't run, CharOwner is null ptr!"));
	}

	// Weapon Crosshair Resolve
	APlayerController* OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController());

	if (CrosshairOverride && OwnerPlayerController) 
	{
		ACoopHUD* HUD = Cast<ACoopHUD>(OwnerPlayerController->GetHUD());

		if (HUD)
			HUD->SetCrosshair(CrosshairOverride);
	}
	//

	EnableWeaponLogicTick(true);

}  // attach to character client implementation


void ASWeapon::ServerAttachToASCharacter_Implementation(ASCharacter* Character)
{
	if (!Character || Character->GetState() == ECharacterState::Dead) {
		UE_LOG(LogTemp, Warning, TEXT("Attempt to attach weapon to invalid character"))
			return;
	}

	if (Character->Weapon) // remove prev weapon
	{
		Character->Weapon->InterruptReload();
		Character->Weapon->Destroy();
	}

	Character->Weapon = this;
	Character->CarriedWeaponSpeedModifier = CharacterSpeedModifier;

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Character->WeaponSocketName);
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	CharOwner = Character;    // avoid Casting
	SetInstigator(Character);
	SetOwner(Character);

	if (CharOwner->IsLocallyControlled())  // if it's server character, exec client attach
	{
		ClientAttachToASCharacter();
	} 

	ServerEnableWeaponLogicTick(true);

} // attach to character server implementation


void ASWeapon::OnRep_CharOwner()
{
	if (CharOwner->IsLocallyControlled())  // if it's server character, exec client attach
	{
		ClientAttachToASCharacter();
	} 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// RECOIL BLOCK  //////////////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::AddRecoil()
{
	// check
	if (!CharOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::AddRecoil() CharOwner is null ptr!"));
		return;
	}

	APlayerController* OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController());

	if (!OwnerPlayerController) // if character is not player return
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::AddRecoil() OwnerPlayerController is null ptr! "));
		return;
	}
	//

	float TotalModifier = RecoilModifiers.GetTotalModifier();

	OwnerPlayerController->AddPitchInput(RecoilParams.GetPitchVal(TotalModifier));
	OwnerPlayerController->AddYawInput(RecoilParams.GetYawVal(TotalModifier));

	// DBG Recoil
	if (DebugWeaponDrawing_Spread > 0 && GEngine)
		GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds() * 10, FColor::Green, *FString::Printf(
			TEXT("[RECOIL] Vel %f Pos %f Rate %f Aim %f, Total %f"),
			RecoilModifiers.GetVelModifier(), RecoilModifiers.GetPosModifier(), 
			RecoilModifiers.GetFireRateModifier(), RecoilModifiers.GetAimingModifier(), TotalModifier));
	//
}


void ASWeapon::CompensateRecoil(float DeltaSec)
{
	// check
	if (!CharOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::CompensateRecoil() CharOwner is null ptr!"));
		return;
	}

	APlayerController* OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController());

	if (!OwnerPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::CompensateRecoil() OwnerPlayerController is null ptr!"));
		return;
	}
	//

	if (TimeSinceLastShot >= RecoilParams.RecoilFadeTime && RecoilParams.RecoilFadeProgress != 0.0f) {

		RecoilParams.ClearRecoilAcommulation();
	}
	else if (TimeSinceLastShot >= RecoilParams.RecoilFadeDelay || RecoilParams.RecoilFadeProgress > 0.0f) {

		float PitchCompensation = -RecoilParams.RecoilAccumulationPitch * DeltaSec * RecoilParams.RecoilFadeTime;
		RecoilParams.RecoilAccumulationPitch += PitchCompensation;

		float YawCompensation = -RecoilParams.RecoilAccumulationYaw * DeltaSec * RecoilParams.RecoilFadeTime;
		RecoilParams.RecoilAccumulationYaw += YawCompensation;
		
		OwnerPlayerController->AddPitchInput(PitchCompensation);
		OwnerPlayerController->AddYawInput(YawCompensation);

		RecoilParams.RecoilFadeProgress += DeltaSec;
		
		// DBG RecoilCompensation
		if (DebugWeaponDrawing_RecoilCompensate > 0 && GEngine)
			GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds() * 10, FColor::Blue, 
				*FString::Printf(TEXT("Compensate Recoil Left (X: %f, y: %f)"), 
				RecoilParams.RecoilAccumulationPitch, RecoilParams.RecoilAccumulationYaw));
		//
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// SPREAD BLOCK  //////////////////////////////////////////////////////////////////////////////////////////////////////

FRotator ASWeapon::CalcSpread()
{
	float AllowedSpreadRange = CurrentSpreadAngle;
	
	float RandPitch = (FMath::FRand() * 2.0f - 1.0f) * AllowedSpreadRange;

	AllowedSpreadRange = sqrtf(AllowedSpreadRange * AllowedSpreadRange - RandPitch * RandPitch);

	float RandYaw = (FMath::FRand() * 2.0f - 1.0f) * AllowedSpreadRange;

	return FRotator(RandPitch, RandYaw, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// FIRE BLOCK  ////////////////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::WeaponLogicTickServer()
{
	// check
	if (!CharOwner)
		return;
	//

	float DeltaTime = WEAPON_TICK;

	TimeSinceLastShot += DeltaTime;

	float VelocityModifierNormalized = CharOwner->GetVelocity().Size() / CharOwner->BaseSpeed;
	CharacterAimPose Pos = CharStateToAimPose(CharOwner->GetState());

	SpreadModifiers.SetPosModifier(Pos);
	RecoilModifiers.SetPosModifier(Pos);

	SpreadModifiers.SetVelModifier(VelocityModifierNormalized);
	RecoilModifiers.SetVelModifier(VelocityModifierNormalized);

	SpreadModifiers.AllModifiersChangeOnTick(DeltaTime);
	RecoilModifiers.AllModifiersChangeOnTick(DeltaTime);

	// Set Current Spread Value for Shot Placement and Crosshair HUD dynamic effect
	CurrentSpreadAngle = SpreadBaseAngle * SpreadModifiers.GetTotalModifier();

	// DBG Spread
	if (DebugWeaponDrawing_Spread && GEngine)
		GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 10, FColor::Cyan, *FString::Printf(
			TEXT("[Spread] Vel %f Pos %f Rate %f Aim %f, Total %f, SpreadAngle %f"),
			SpreadModifiers.CurrentVelocityModifier, SpreadModifiers.CurrentPosModifier,
			SpreadModifiers.CurrentFireRateModifier, SpreadModifiers.CurrentAimingModifier,
			SpreadModifiers.CurrentTotalModifier, CurrentSpreadAngle));

}  // tick weapon logic server


void ASWeapon::WeaponLogicTickClient()
{
	if (!CharOwner) // check
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::LogicTickClient() can't exec, CharOwner is nullptr!"));
		return;
	}

	// Recoil fade
	CompensateRecoil(WEAPON_TICK);

}  // tick weapon logic client

void ASWeapon::EnableWeaponLogicTick(bool Enable)
{
	if (Enable) 
	{
		if (!CharOwner) {
			UE_LOG(LogTemp, Warning, TEXT("Can't Enable Client Weapon Tick When weapon hasn't attached to character"));
			return;
		}

		if (CharOwner->IsLocallyControlled())
		{
			GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTickClient);
			GetWorldTimerManager().SetTimer(TimerHandle_WeaponTickClient, this, &ASWeapon::WeaponLogicTickClient, WEAPON_TICK, true);
		} 
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ASWeapon::EnableWeaponLogicTick() WeaponTickClient can't run, CharOwner not LocalyControlled"));
		}
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTickClient);
	}
}  // client weapon logic tick activation, better activate when attached to character


void ASWeapon::ServerEnableWeaponLogicTick_Implementation(bool Enable)
{
	if (Enable)
	{
		if (!CharOwner) {
			UE_LOG(LogTemp, Warning, TEXT("Can't Enable Server Weapon Tick When weapon hasn't attached to character"));
			return;
		}

		GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTickServer);
		GetWorldTimerManager().SetTimer(TimerHandle_WeaponTickServer, this, &ASWeapon::WeaponLogicTickServer, WEAPON_TICK, true);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTickServer);
	}
}  // server weapon logic tick activation, better activate when attached to character


void ASWeapon::SemiAutoFireTimerBind(bool ShotDelayed)
{
	bShotIsDelayed = ShotDelayed;
	bShouldFire = true;
	Fire();
	bShouldFire = false;
}  // semi-auto shot delayed queue


void ASWeapon::FireLogic()
{
	AActor* MyOwner = GetOwner();
	
	if (!MyOwner)  // check
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::FireLogic() can't exec, Owner is null ptr!"));
	}
	
	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FVector ShotDirection = EyeRotation.Vector();

	LastHit = FHitResult();

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	FVector ShotEnd = EyeLocation + ((EyeRotation + CalcSpread()).Vector() * Range);
	
	if (GetWorld()->LineTraceSingleByChannel(LastHit, EyeLocation, ShotEnd, COLLISION_WEAPON, QueryParams)) 
	{
		// Blocking hit! Process damage
		AActor* HitActor = LastHit.GetActor();

		// surface determination
		EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(LastHit.PhysMaterial.Get());

		// apply damage handle
		float CurrentDamage = Damage;
		if (SurfaceType == SURFACE_FLESHVULNERABLE)
			CurrentDamage *= HeadshotDamageMultiplier;

		UGameplayStatics::ApplyPointDamage(HitActor, CurrentDamage, ShotDirection, LastHit, MyOwner->GetInstigatorController(), this, DamageType);
		//

		// Replication shot fx
		HitScanTrace.ImpactPoint = LastHit.ImpactPoint;
		HitScanTrace.HitSurfaceType = SurfaceType;
		HitScanTrace.bHitSuccess = true;
		//
	}
	else
	{
		// Hit not blocked!
		// Replication shot fx
		HitScanTrace.ImpactPoint = ShotEnd;
		HitScanTrace.HitSurfaceType = SurfaceType_Default;
		HitScanTrace.bHitSuccess = false;
	}  // line trace

	// Shooting Modifiers On Shoot Modifier Resolve
	SpreadModifiers.SetFireRateModifier(TimeSinceLastShot);
	RecoilModifiers.SetFireRateModifier(TimeSinceLastShot);

	// // DEBUG
	if (DebugWeaponDrawing_Shot > 0) 
	{
		float DrawSize = 3.0f;

		if (LastHit.Distance >= 10000.0f)
			DrawSize *= 5;
		else if (LastHit.Distance >= 7500.0f)
			DrawSize *= 4;
		else if (LastHit.Distance >= 5000.0f)
			DrawSize *= 3;
		else if (LastHit.Distance >= 2500.0f)
			DrawSize *= 2;

		DrawDebugSphere(GetWorld(), LastHit.ImpactPoint, DrawSize, 12, FColor::Cyan, true, 1.0f);
	}
}  // actual single shot logic (fire line trace)


void ASWeapon::PlayFireEffects()
{
	if (MuzzleEffect)
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);

	if (TraceEffect) {

		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLoc);

		if (PSC)
			PSC->SetVectorParameter(TargetTraceEffect, HitScanTrace.ImpactPoint);
	}

	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, MeshComp->GetSocketLocation(MuzzleSocketName));

}  // fire FX


void ASWeapon::PlayImpactEffects()
{
	UParticleSystem* SelectedEffect;

	switch (HitScanTrace.HitSurfaceType)
	{
	case SURFACE_FLESHDEFAULT:
		SelectedEffect = ImpactEffectFlesh;
		break;
	case SURFACE_FLESHVULNERABLE:
		SelectedEffect = ImpactEffectFlesh;
		break;
	default:
		SelectedEffect = ImpactEffectDefault;
		break;
	}

	if (HitScanTrace.bHitSuccess || bImpactEffectIgnoreShotTraceBlocked)
	{
		if (SelectedEffect)
		{
			FVector ImpactPoint = HitScanTrace.ImpactPoint;
			FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

			FVector ShotDirection = ImpactPoint - MuzzleLoc;
			ShotDirection.Normalize();

			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());
		}
	}
}  // impact surface FX


void ASWeapon::PlayCameraShakeEffect()
{
	if (!CharOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::PlayCameraShakeEffect() CharOwner is null ptr!"));
		return;
	}

	APlayerController* OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController());

	if (!OwnerPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::PlayCameraShakeEffect() OwnerPlayerController is null ptr!"));
		return;
	}

	if (bCameraShaking && CameraShakeEffect) {
		OwnerPlayerController->ClientPlayCameraShake(CameraShakeEffect, CameraShakeScale);
	}
}


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects();
	PlayImpactEffects();
	
	if (CharOwner && CharOwner->IsLocallyControlled())
	{
		AddRecoil();
		PlayCameraShakeEffect();
	}


}  // replication shot


void ASWeapon::Fire()
{
	ServerFire();
}  // character (player) on fire button logic handler


void ASWeapon::ServerFire_Implementation()
{
	if (bShotIsDelayed || bIsReloading || AmmoCurrent == 0)
	{
		return;
	}

	if (!bShouldFire && !bShotIsDelayed) {
		TimerHandle_FireCoolDown.Invalidate();
		return;
	}

	if (TimeSinceLastShot < FireRate) {

		if (bShotCanBeDelayed && !bShotIsDelayed) {
			bShotIsDelayed = true;

			GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
			GetWorldTimerManager().SetTimer(TimerHandle_FireCoolDown, SemiAutoFireTimerDel, FireRate - TimeSinceLastShot, false);

			return;

		} // semi auto fire delay smooth

		else {

			return;

		} // no semi auto fire delay smooth


	} // semiauto and tap handle

	FireLogic();
	PlayFireEffects();
	PlayImpactEffects();
	PlayCameraShakeEffect();
	AddRecoil();

	if (FireMode == WeaponFireMode::Fullauto) {
		GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
		GetWorldTimerManager().SetTimer(TimerHandle_FireCoolDown, this, &ASWeapon::Fire, FireRate);
	} // full auto handle

	ShotCount++;
	AmmoCurrent--;

	TimeSinceLastShot = WEAPON_TICK;
	bShotIsDelayed = false;

} // server character (player) on fire button logic handler


bool ASWeapon::ServerFire_Validate()
{
	return true;
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// RELOADING BLOCK  ///////////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::PlayReloadAnim()  
{
	if (CharOwner)
	{
		UAnimInstance* CharacterAnim = CharOwner->GetMesh()->GetAnimInstance();

		if (CharacterAnim && ReloadAnimMontage)
			CharacterAnim->Montage_Play(ReloadAnimMontage, ReloadAnimMontage->SequenceLength / ReloadSpeed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::PlayReloadAnim NOT FOUND CharOwner !!"));
	}

}  // play reload anim


void ASWeapon::StopReloadAnim()
{
	if (CharOwner)
	{
		UAnimInstance* CharacterAnim = CharOwner->GetMesh()->GetAnimInstance();

		if (CharacterAnim && ReloadAnimMontage) 
			CharacterAnim->Montage_Stop(0.15f, ReloadAnimMontage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeapon::PlayReloadAnim NOT FOUND CharOwner !!"));
	}

}  // stop reload anim

void ASWeapon::InterruptReload()
{
	if (GetLocalRole() != ROLE_Authority)
		StopReloadAnim();

	ServerInterruptReload();

}  // interrupt reload


void ASWeapon::ServerInterruptReload_Implementation()
{
	StopReloadAnim(); // stop anim if server also is client

	GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
	bIsReloading = false;

}  // server interrupt reload


void ASWeapon::FinishReload()
{
	ServerFinishReload();

}  // finish reload


void ASWeapon::ServerFinishReload_Implementation()
{
	AmmoCurrent = AmmoMax;
	bIsReloading = false;

}  // server finish reload


void ASWeapon::StartReload()
{	
	if (bIsReloading || AmmoCurrent == AmmoMax)  // client check (same as server check)
		return;

	if (GetLocalRole() != ROLE_Authority)
		PlayReloadAnim();

	ServerStartReload();

}  // start reload


void ASWeapon::ServerStartReload_Implementation()
{
	if (bIsReloading || AmmoCurrent == AmmoMax)  // server check (same as client)
		return;
		
	PlayReloadAnim();  // play anim if server also is client

	bIsReloading = true;

	GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
	GetWorldTimerManager().SetTimer(TimerHandle_FireCoolDown, this, &ASWeapon::FinishReload, ReloadSpeed);

}  // server start reload


void ASWeapon::OnRep_Reloading()
{
	if (bIsReloading)
	{
		PlayReloadAnim();
	}
	else
	{
		StopReloadAnim();
	}
}  // replication reload

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// NETWORKING REPLICATION  ////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASWeapon, CharOwner);

	DOREPLIFETIME_CONDITION(ASWeapon, bIsReloading, COND_SkipOwner);
	DOREPLIFETIME(ASWeapon, AmmoCurrent);  // temp for cosmetic
	DOREPLIFETIME(ASWeapon, AmmoMax);  // temp for cosmetic
	// DOREPLIFETIME(ASWeapon, TimerHandle_FireCoolDown);

	// DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME(ASWeapon, HitScanTrace);
	// DOREPLIFETIME_CONDITION_NOTIFY(ASWeapon, HitScanTrace, COND_Custom, REPNOTIFY_Always);

	// weapon tick (and not only that)
	DOREPLIFETIME_CONDITION(ASWeapon, SpreadModifiers, COND_OwnerOnly);  // temp for cosmetic weapon spread indicator
	DOREPLIFETIME_CONDITION(ASWeapon, RecoilModifiers, COND_OwnerOnly);  // temp for recoil
	// DOREPLIFETIME_CONDITION(ASWeapon, RecoilParams, COND_AutonomousOnly);  // no need, used locally on client
	DOREPLIFETIME_CONDITION(ASWeapon, CurrentSpreadAngle, COND_OwnerOnly);  // for dynamic hud

	// DOREPLIFETIME_CONDITION(ASWeapon, TimeSinceLastShot, COND_AutonomousOnly);  // need to recoil compensation
	DOREPLIFETIME_CONDITION(ASWeapon, TimeSinceLastShot, COND_OwnerOnly);  // need to recoil compensation

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
