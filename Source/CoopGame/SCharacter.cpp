// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "DrawDebugHelpers.h"

#include "SWeapon.h"

// Sets default values
ASCharacter::ASCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(GetMesh());

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseSpeed = GetCharacterMovement()->MaxWalkSpeed;

	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	DefaultFOV = CameraComp->FieldOfView;

	if (WeaponClassToSpawnWith)
		SpawnWeapon(WeaponClassToSpawnWith);

}

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// FOV Zoom
	float ZoomFOV;

	if (Weapon && Weapon->ZoomFOV != 0.0f)
		ZoomFOV = Weapon->ZoomFOV;
	else
		ZoomFOV = ZoomedFOV;

	float TargetFOV = bWantsToZoom ? ZoomFOV : DefaultFOV;

	// FOV Zoom Speed
	float ZoomSpeed = Weapon && Weapon->ZoomInterpSpeed != 0.0f ? Weapon->ZoomInterpSpeed : ZoomInterpSpeed;

	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomSpeed);

	CameraComp->SetFieldOfView(NewFOV);
	//

	// Debug velocity
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("Velocity: %s size: %f"), *GetVelocity().ToString(), GetVelocity().Size()));
	//

	// Movement State
	StateTime += DeltaTime;

	if (GetVelocity().Z < 0.0f) /* Falling */
	{
		SetState(ECharacterState::Falling);
	}
	else if (GetVelocity().Size() < 0.01f) /* Zero Velocity Magnitude */
	{
		if (State != ECharacterState::Crouch)
			SetState(ECharacterState::Idle);
	}
	else /* Positive Velocity Magnitude */
	{
		if (State == ECharacterState::Idle || State == ECharacterState::Falling)
			SetState(ECharacterState::Run);
	}
	//

	// DEBUG STATE
	DrawDebugString(GetWorld(), GetMesh()->GetBoneLocation(FName(TEXT("head"))) + FVector(0.0f, 30.0f, 30.0f), *FString::Printf(TEXT("%s"), *DebugCharState(State)), (AActor*)0, FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), GetMesh()->GetBoneLocation(FName(TEXT("head"))) + FVector(0.0f, -30.0f, 30.0f), *FString::Printf(TEXT("%d"), (int32)GetVelocity().Size()), (AActor*)0, FColor::White, DeltaTime);
	//
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::BeginJump);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASCharacter::BeginSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASCharacter::EndSprint);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::Fire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::BeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::EndZoom);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);

}

void ASCharacter::Reload()
{

	if (!Weapon)
		return;

	if (Weapon->bIsReloading) {

		Weapon->InterruptReload();

	}
	else {

		Weapon->StartReload();
	}

}

void ASCharacter::BeginZoom()
{

	bWantsToZoom = true;

	if (Weapon)
	{
		Weapon->SpreadModifiers.SetAimingModifier(true);
		Weapon->RecoilModifiers.SetAimingModifier(true);
		//Weapon->SpreadModifiers.DropFireRateModifier();
		//Weapon->RecoilModifiers.DropFireRateModifier();
	}
}

void ASCharacter::EndZoom()
{

	bWantsToZoom = false;

	if (Weapon)
	{
		Weapon->SpreadModifiers.SetAimingModifier(false);
		Weapon->RecoilModifiers.SetAimingModifier(false);
		//Weapon->SpreadModifiers.DropFireRateModifier();
		//Weapon->RecoilModifiers.DropFireRateModifier();
	}
}

void ASCharacter::Fire()
{

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("CHARACTER FIRE")));

	if (Weapon && !bShouldSprinting) {

		Weapon->bShouldFire = true;
		Weapon->Fire();
	}

}

void ASCharacter::StopFire()
{

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("CHARACTER STOP FIRE")));

	if (Weapon) {

		Weapon->bShouldFire = false;

	}

}

void ASCharacter::BeginSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
	bShouldSprinting = true;
	SetState(ECharacterState::Sprint);

	StopFire();

	//if (Weapon)
		//Weapon->SpreadModifiers.DropFireRateModifier();
		//Weapon->RecoilModifiers.DropFireRateModifier();
}


void ASCharacter::EndSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	bShouldSprinting = false;
	SetState(ECharacterState::Run);
}

FVector ASCharacter::GetPawnViewLocation() const
{

	if (CameraComp)
	{

		return CameraComp->GetComponentLocation();

	}

	return Super::GetPawnViewLocation();

}


void ASCharacter::MoveForward(float Val)
{
	
	FVector FwdVec = GetControlRotation().Vector();
	FwdVec.Z = 0.0f;
	FwdVec.Normalize();

	AddMovementInput(FwdVec * Val * CarriedWeaponSpeedModifier);

}


void ASCharacter::MoveRight(float Val)
{
	AddMovementInput(FRotationMatrix(GetControlRotation()).GetScaledAxis(EAxis::Y) * Val * CarriedWeaponSpeedModifier);

}

void ASCharacter::BeginCrouch()
{
	
	if (GetMovementComponent()->IsFalling())
		return;

	Crouch();

	SetState(ECharacterState::Crouch);

}

void ASCharacter::EndCrouch()
{

	UnCrouch();

	SetState(ECharacterState::Run);
}


void ASCharacter::SpawnWeapon(TSubclassOf<ASWeapon> WeaponClass)
{
	ASWeapon* SpawnedWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, GetActorLocation(), GetActorRotation());
	SpawnedWeapon->AttachToASCharacter(this);
}

void ASCharacter::SetState(ECharacterState NewState)
{

	if (State == NewState)
		return;

	ECharacterState NewStateFiltered;

	// sprint resolve
	if (NewState == ECharacterState::Run && bShouldSprinting && GetVelocity().Size() > BaseSpeed)
		NewStateFiltered = ECharacterState::Sprint;
	else
		NewStateFiltered = NewState;

	// next state after jumping can only falling
	if (State == ECharacterState::Jumping && (NewState != ECharacterState::Falling || NewState != ECharacterState::Dead))
	{
		//return;
	}
	else if (State == ECharacterState::Dead)
	{
		return;
	}
	else
	{

	}

	// print
	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("[Char State] Old: %s New: %s"), *DebugCharState(State), *DebugCharState(NewStateFiltered)));
	//

	State = NewStateFiltered;

	StateTime = 0.0f;
	
}

void ASCharacter::BeginJump()
{

	if (GetMovementComponent()->IsCrouching())
	{
		EndCrouch();
		return;
	}

	ACharacter::Jump();

	SetState(ECharacterState::Jumping);

}
