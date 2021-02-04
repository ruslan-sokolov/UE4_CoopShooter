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

	DamageType = UDamageType::StaticClass();

	SetReplicates(true);
	SetReplicateMovement(true);

	NetUpdateFrequency = 60.0f;
	MinNetUpdateFrequency = 30.0f;

	// RELOAD ANIM SPEED MODIFIER INITIALIZE
	if (ReloadSpeed < 0.0f)
		// ReloadMontageSpeedMultiplier = ReloadAnimMontage->SequenceLength / ReloadSpeed;
		ReloadSpeed = ReloadAnimMontage->SequenceLength;

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


void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

}


void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// DBG ammo
	FVector Loc = MeshComp->GetSocketLocation(MuzzleSocketName);
	Loc += GetActorForwardVector() * -30;
	DrawDebugString(GetWorld(), Loc, *FString::Printf(TEXT("%d / %d"), AmmoCurrent, AmmoMax), (AActor*)0, FColor::White, DeltaTime);
	//
}


// Character Attach BLOCK  ////////////////////////////////////////////////////////////////////////////////////////////

void ASWeapon::AttachToASCharacter(ASCharacter* Character)
{
	ServerAttachToASCharacter(Character);

	// Weapon Crosshair Resolve
	if (CrosshairOverride && OwnerPlayerController) {
		ACoopHUD* HUD = Cast<ACoopHUD>(OwnerPlayerController->GetHUD());

		if (HUD)
			HUD->SetCrosshair(CrosshairOverride);
	}

	Character->Weapon = this;
	Character->CarriedWeaponSpeedModifier = CharacterSpeedModifier;

}


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

	AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Character->WeaponSocketName);
	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);

	CharOwner = Character;    // Avoid Casting
	// CharacterAnim = CharOwner->GetMesh()->GetAnimInstance; // DEPR Character Anim PTR For Reload Anim
	OwnerPlayerController = Cast<APlayerController>(CharOwner->GetController()); 	// CAMERA SHAKE INITIALZIE
	SetInstigator(Character);
	SetOwner(Character);

	EnableWeaponLogicTick(true);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// RECOIL BLOCK  //////////////////////////////////////////////////////////////////////////////////////////////////////

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

void ASWeapon::WeaponLogicTick()
{

	if (!CharOwner)  // server check (same as client has)
		return;

	// UE_LOG(LogTemp, Warning, TEXT("WEAPON LOGIC TICK"));

	float DeltaTime = GetWorld()->GetDeltaSeconds();

	TimeSinceLastShot += WEAPON_TICK;

	float VelocityModifierNormalized = CharOwner->GetVelocity().Size() / CharOwner->BaseSpeed;
	CharacterAimPose Pos = CharStateToAimPose(CharOwner->GetState());

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

}  // tick logic


void ASWeapon::EnableWeaponLogicTick(bool Enable)
{

	if (Enable) 
	{
		if (!CharOwner) {
			UE_LOG(LogTemp, Warning, TEXT("Can't Enable Weapon Tick When weapon hasn't attached to character"));
			return;
		}

		GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTick);
		GetWorldTimerManager().SetTimer(TimerHandle_WeaponTick, this, &ASWeapon::WeaponLogicTick, WEAPON_TICK, true);

	}
	else
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_WeaponTick);
	}

}  // weapon logic tick activation, better use on server when attached to character


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

	if (!MyOwner)
		return;

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
	
	if (GetWorld()->LineTraceSingleByChannel(LastHit, EyeLocation, ShotEnd, COLLISION_WEAPON, QueryParams)) {
		// Blocking hit! Process damage
		AActor* HitActor = LastHit.GetActor();

		EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(LastHit.PhysMaterial.Get());

		float CurrentDamage = this->Damage;

		if (SurfaceType == SURFACE_FLESHVULNERABLE) {

			CurrentDamage *= HeadshotDamageMultiplier;
		}

		UGameplayStatics::ApplyPointDamage(HitActor, CurrentDamage, ShotDirection, LastHit, MyOwner->GetInstigatorController(), this, DamageType);

	
		// NET REPLICATION FOR SHOT EFFECT
		if (GetLocalRole() == ROLE_Authority)
		{
			HitScanTrace.ImpactPoint = LastHit.ImpactPoint.Size() > 0.0f ? LastHit.ImpactPoint : LastHit.TraceEnd;
			HitScanTrace.HitSurfaceType = SurfaceType;
		}
	
	
	} // Line Trace


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

	if (bCameraShaking && CameraShakeEffect && OwnerPlayerController) {
		OwnerPlayerController->ClientPlayCameraShake(CameraShakeEffect, CameraShakeScale);
	}

}  // fire FX


void ASWeapon::PlayImpactEffects()
{

	EPhysicalSurface SurfaceType = SurfaceType_Default;
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

	if (SelectedEffect) {
		FVector ImpactPoint = HitScanTrace.ImpactPoint;
		FVector MuzzleLoc = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = ImpactPoint - MuzzleLoc;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, ImpactPoint, ShotDirection.Rotation());

	}

}  // impact surface FX


void ASWeapon::OnRep_HitScanTrace()
{
	// Play cosmetic FX
	PlayFireEffects();
	PlayImpactEffects();

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

		if (CharacterAnim && ReloadAnimMontage) {
			CharacterAnim->Montage_Stop(0.15f, ReloadAnimMontage);
		}
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
	//DOREPLIFETIME_CONDITION(ASWeapon, SpreadModifiers, COND_AutonomousOnly);
	//DOREPLIFETIME_CONDITION(ASWeapon, RecoilModifiers, COND_AutonomousOnly);
	// DOREPLIFETIME_CONDITION(ASWeapon, RecoilParams, COND_AutonomousOnly);
	DOREPLIFETIME_CONDITION(ASWeapon, CurrentSpreadAngle, COND_AutonomousOnly);

	//DOREPLIFETIME_CONDITION(ASWeapon, TimeSinceLastShot, COND_AutonomousOnly);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
