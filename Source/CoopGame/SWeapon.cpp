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
	
	// SEMI AUTO PREBIND
	SemiAutoFireTimerDel.BindLambda([this] {SemiAutoFireTimerBind(false); });

	// HUD SPREAD VAL INITIALIZE
	CurrentSpreadAngle = SpreadBaseAngle;

}


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

	// RELOAD ANIM SPEED MODIFIER INITIALIZE
	if (ReloadSpeed > 0)
		ReloadMontageSpeedMultiplier = ReloadAnimMontage->SequenceLength / ReloadSpeed;
}


void ASWeapon::AttachToASCharacter(ASCharacter* Character)
{

	if (!Character || Character->CharacterState == ECharacterState::Dead) {
		UE_LOG(LogTemp, Warning, TEXT("Attempt to attach weapon to invalid character"))
		return;
	}

	if (Character->Weapon) // remove prev weapon
	{
		Character->Weapon->InterruptReload();
		Character->Weapon->Destroy();
	}

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Character->WeaponSocketName);
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	CharOwner = Character;    // Avoid Casting
	CharacterAnim = CharOwner->GetMesh()->GetAnimInstance();  	// Character Anim PTR For Reload Anim
	OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController()); 	// CAMERA SHAKE INITIALZIE
	SetInstigator(Character);
	SetOwner(Character);

	// Weapon Crosshair Resolve
	if (CrosshairOverride && OwnerPlayerController) {
		ACoopHUD* HUD = Cast<ACoopHUD>(OwnerPlayerController->GetHUD());
		
		if (HUD)
			HUD->SetCrosshair(CrosshairOverride);
	}

	Character->Weapon = this;
	Character->CarriedWeaponSpeedModifier = CharacterSpeedModifier;

}


void ASWeapon::BeginDestroy()
{
	Super::BeginDestroy();

	
	// InterruptReload();

	if (CharOwner) {
		CharOwner->CarriedWeaponSpeedModifier = 1.0f;
	}
	
	// Weapon Crosshair Resolve
	if (CrosshairOverride && OwnerPlayerController) {
		ACoopHUD* HUD = Cast<ACoopHUD>(OwnerPlayerController->GetHUD());

		if (HUD)
			HUD->RestoreDefaultCrosshair();
	}
}


void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastShot += DeltaTime;
	
	// DBG ammo
	FVector Loc = MeshComp->GetSocketLocation(MuzzleSocketName);
	Loc += GetActorForwardVector() * -30;
	DrawDebugString(GetWorld(), Loc, *FString::Printf(TEXT("%d / %d"), AmmoCurrent, AmmoMax), (AActor*)0, FColor::White, DeltaTime);
	//

	if (CharOwner)
	{
		float VelocityModifierNormalized = CharOwner->GetVelocity().Size() / CharOwner->BaseSpeed;
		CharacterAimPose Pos = CharStateToAimPose(CharOwner->CharacterState);

		SpreadModifiers.SetPosModifier(Pos);
		RecoilModifiers.SetPosModifier(Pos);

		SpreadModifiers.SetVelModifier(VelocityModifierNormalized);
		RecoilModifiers.SetVelModifier(VelocityModifierNormalized);

		SpreadModifiers.AllModifiersChangeOnTick(DeltaTime);
		RecoilModifiers.AllModifiersChangeOnTick(DeltaTime);

		// Recoil fade
		CompensateRecoil(DeltaTime);

		// Set Current Spread Value for Shot Placement and Crosshair HUD dynamic effect
		CurrentSpreadAngle = SpreadBaseAngle * SpreadModifiers.GetTotalModifier();

		// DBG Spread
		if (DebugWeaponDrawing_Spread && GEngine)
			GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 10, FColor::Cyan, *FString::Printf(
				TEXT("[Spread] Vel %f Pos %f Rate %f Aim %f, Total %f, SpreadAngle %f"),
				SpreadModifiers.CurrentVelocityModifier, SpreadModifiers.CurrentPosModifier,
				SpreadModifiers.CurrentFireRateModifier, SpreadModifiers.CurrentAimingModifier,
				SpreadModifiers.CurrentTotalModifier, CurrentSpreadAngle));
		//
	}
}


// RECOIL BLOCK
void ASWeapon::AddRecoil()
{
	if (!OwnerPlayerController) // if character is not player return
		return;

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

	if (!OwnerPlayerController)  // if character is not player return
		return;

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
			GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds() * 10, FColor::Blue, *FString::Printf(TEXT("Compensate Recoil Left (X: %d, y: %d)"), RecoilParams.RecoilAccumulationPitch, RecoilParams.RecoilAccumulationYaw));
		//
	}

}


// SPREAD BLOCK
FRotator ASWeapon::CalcSpread()
{

	float AllowedSpreadRange = CurrentSpreadAngle;
	
	float RandPitch = (FMath::FRand() * 2.0f - 1.0f) * AllowedSpreadRange;

	AllowedSpreadRange = sqrtf(AllowedSpreadRange * AllowedSpreadRange - RandPitch * RandPitch);

	float RandYaw = (FMath::FRand() * 2.0f - 1.0f) * AllowedSpreadRange;

	return FRotator(RandPitch, RandYaw, 0.0f);

}

// FIRE BLOCK
void ASWeapon::SemiAutoFireTimerBind(bool ShotDelayed)
{

	bShotIsDelayed = ShotDelayed;
	bShouldFire = true;
	Fire();
	bShouldFire = false;

}


void ASWeapon::FireLogic()
{
	AActor* MyOwner = GetOwner();

	if (!MyOwner)
		return;

	// FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

	FVector EyeLocation;
	FRotator EyeRotation;
	MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FVector ShotDirection = EyeRotation.Vector();
	// FVector TraceEnd = EyeLocation + (ShotDirection * Range);

	LastHit = FHitResult();

	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(MyOwner);
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;
	

	// Spread Calculation
	// float TempAngle = CurrentSpreadAngle;
	// FRotator ShotSpreadAngle = CalcSpread();
	//FRotator ShotAngle = EyeRotation + CalcSpread();

	FVector ShotEnd = EyeLocation + ((EyeRotation + CalcSpread()).Vector() * Range);
	
	if (GetWorld()->LineTraceSingleByChannel(LastHit, EyeLocation /*MuzzleLoc*/, /*TraceEnd*/ ShotEnd, COLLISION_WEAPON, QueryParams)) {
		// Blocking hit! Process damage
		AActor* HitActor = LastHit.GetActor();

		EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(LastHit.PhysMaterial.Get());

		float CurrentDamage = this->Damage;

		if (SurfaceType == SURFACE_FLESHVULNERABLE) {

			CurrentDamage *= HeadshotDamageMultiplier;
		}

		UGameplayStatics::ApplyPointDamage(HitActor, CurrentDamage, ShotDirection, LastHit, MyOwner->GetInstigatorController(), this, DamageType);

	} // Line Trace

	// Shooting Modifiers On Shoot Modifier Resolve
	SpreadModifiers.SetFireRateModifier(TimeSinceLastShot);
	RecoilModifiers.SetFireRateModifier(TimeSinceLastShot);

	// // DEBUG
	if (DebugWeaponDrawing_Shot > 0) {
		/*
		DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 15.0f, 0, 1.0f);
		DrawDebugLine(GetWorld(), EyeLocation, ShotEnd, FColor::Red, false, 15.0f, 0, 1.0f);

		FVector ShotNormilized = TraceEnd - EyeLocation;
		ShotNormilized.Normalize();

		FVector ShotSpreadNormalized = ShotEnd - EyeLocation;
		ShotSpreadNormalized.Normalize();

		float ActualShotAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(ShotNormilized, ShotSpreadNormalized));

		FString PrintStr = FString::Printf(
			TEXT("Rotator: %s, RotatorSum: %f, RotEuqulid: %f, TempAngle: %f, Calc: %f"),
			*ShotSpreadAngle.ToString(),
			FMath::Abs(ShotSpreadAngle.Pitch) + FMath::Abs(ShotSpreadAngle.Yaw) + FMath::Abs(ShotSpreadAngle.Roll),
			sqrtf(FMath::Square(ShotSpreadAngle.Pitch) + FMath::Square(ShotSpreadAngle.Yaw) + FMath::Square(ShotSpreadAngle.Roll)),
			TempAngle,
			ActualShotAngle
		);

		DrawDebugString(GetWorld(), ShotEnd, *PrintStr, (AActor*)0, FColor::White, FireRate);
		UE_LOG(LogTemp, Warning, TEXT("Rotator: %s, RotatorSum: %f, RotEuqulid: %f, TempAngle: %f, Calc: %f"),
			*ShotSpreadAngle.ToString(),
			FMath::Abs(ShotSpreadAngle.Pitch) + FMath::Abs(ShotSpreadAngle.Yaw) + FMath::Abs(ShotSpreadAngle.Roll),
			sqrtf(FMath::Square(ShotSpreadAngle.Pitch) + FMath::Square(ShotSpreadAngle.Yaw) + FMath::Square(ShotSpreadAngle.Roll)),
			TempAngle,
			ActualShotAngle
		);*/

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

}


void ASWeapon::PlayFireEffects()
{

	EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(LastHit.PhysMaterial.Get());

	UParticleSystem* SelectedEffect;

	switch (SurfaceType) {
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

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, LastHit.ImpactPoint, LastHit.ImpactNormal.Rotation());

	if (MuzzleEffect)
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);

	if (TraceEffect) {

		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TraceEffect, MuzzleLoc);

		if (PSC)
			PSC->SetVectorParameter(TargetTraceEffect, LastHit.ImpactPoint.Size() > 0.0f ? LastHit.ImpactPoint : LastHit.TraceEnd);
	}

	if (FireSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, MeshComp->GetSocketLocation(MuzzleSocketName));

	if (bCameraShaking && CameraShakeEffect && OwnerPlayerController) {
		OwnerPlayerController->ClientPlayCameraShake(CameraShakeEffect, CameraShakeScale);
	}

}


void ASWeapon::Fire()
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
	AddRecoil();

	if (FireMode == WeaponFireMode::Fullauto) {
		GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
		GetWorldTimerManager().SetTimer(TimerHandle_FireCoolDown, this, &ASWeapon::Fire, FireRate);
	} // full auto handle

	ShotCount++;
	AmmoCurrent--;

	TimeSinceLastShot = 0.0f;
	bShotIsDelayed = false;

}


// RELOADING BLOCK
void ASWeapon::InterruptReload()
{

	if (CharacterAnim && ReloadAnimMontage) {

		GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);

		CharacterAnim->Montage_Stop(0.15f, ReloadAnimMontage);

		bIsReloading = false;

	}
}


void ASWeapon::FinishReload()
{

	AmmoCurrent = AmmoMax;

	bIsReloading = false;

}


void ASWeapon::StartReload()
{

	if (bIsReloading || AmmoCurrent == AmmoMax)
		return;

	if (CharacterAnim && ReloadAnimMontage) {
		
		bIsReloading = true;

		float AnimLength = CharacterAnim->Montage_Play(ReloadAnimMontage, ReloadMontageSpeedMultiplier);

		GetWorldTimerManager().ClearTimer(TimerHandle_FireCoolDown);
		GetWorldTimerManager().SetTimer(TimerHandle_FireCoolDown, this, &ASWeapon::FinishReload, AnimLength / ReloadMontageSpeedMultiplier);

	}

}
