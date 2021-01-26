// Fill out your copyright notice in the Description page of Project Settings.


#include "SCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "DrawDebugHelpers.h"

#include "SWeapon.h"
#include "CoopGame.h"
#include "Components/SHealthComponent.h"

#include "Net/UnrealNetwork.h"

#include "GameFramework/PlayerState.h"

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

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

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

	if (GetLocalRole() == ROLE_Authority)
	{
		if (WeaponClassToSpawnWith)
			SpawnWeapon(WeaponClassToSpawnWith);
	}


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
		if (CharacterState != ECharacterState::Crouch)
			SetState(ECharacterState::Idle);
	}
	else /* Positive Velocity Magnitude */
	{
		if (CharacterState == ECharacterState::Idle || CharacterState == ECharacterState::Falling ||
			(CharacterState == ECharacterState::Run && bShouldSprinting))
		SetState(ECharacterState::Run);
	}
	//

	// DEBUG STATE
	DrawDebugString(GetWorld(), GetMesh()->GetBoneLocation(FName(TEXT("head"))) + FVector(0.0f, 30.0f, 30.0f), *FString::Printf(TEXT("%s"), *DebugCharState(CharacterState)), (AActor*)0, FColor::White, DeltaTime);
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
	if (GetLocalRole() < ROLE_Authority) {
		ServerBeginSprint();
	}

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


void ASCharacter::ServerBeginSprint_Implementation()
{
	BeginSprint();
}

void ASCharacter::EndSprint()
{
	if (GetLocalRole() < ROLE_Authority) {
		ServerBeginSprint();
	}

	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	bShouldSprinting = false;
	SetState(ECharacterState::Run);
}


void ASCharacter::ServerEndSprint_Implementation()
{
	EndSprint();
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
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, *FString::Printf(
			TEXT("SpawnWeapon")));

	ServerSpawnWeapon(WeaponClass);

}


void ASCharacter::ServerSpawnWeapon_Implementation(TSubclassOf<ASWeapon> WeaponClass)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, *FString::Printf(
			TEXT("ServerSpawnWeapon_Implementation")));

	ASWeapon* SpawnedWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, GetActorLocation(), GetActorRotation());
	SpawnedWeapon->AttachToASCharacter(this);
}


void ASCharacter::SetState(ECharacterState NewCharacterState)
{
	if (CharacterState == ECharacterState::Dead)
		return;

	if (GetLocalRole() == ROLE_AutonomousProxy)
		ServerSetState(NewCharacterState);

	if (CharacterState == NewCharacterState || CharacterState == ECharacterState::Dead)
		return;

	ECharacterState NewCharacterStateFiltered;
	
	// sprint resolve
	if (NewCharacterState == ECharacterState::Run && bShouldSprinting && GetVelocity().Size() > BaseSpeed)
		NewCharacterStateFiltered = ECharacterState::Sprint;
	else
		NewCharacterStateFiltered = NewCharacterState;

	CharacterState = NewCharacterStateFiltered;
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, *FString::Printf(
			TEXT("SetState id: %d, %s"), GetPlayerState()->GetPlayerId(), *DebugCharState(CharacterState)));

	StateTime = 0.0f;
	
}


void ASCharacter::ServerSetState_Implementation(ECharacterState NewCharacterState)
{

	SetState(NewCharacterState);

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



void ASCharacter::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && CharacterState != ECharacterState::Dead)
	{
		// DIE!
		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		StopFire();
		EndZoom();

		SetState(ECharacterState::Dead);

		// DetachFromControllerPendingDestroy();
		Controller->UnPossess();

		GetCharacterMovement()->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;

		SetLifeSpan(10.0f);
	}

}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, Weapon);
	DOREPLIFETIME(ASCharacter, CharacterState);
}