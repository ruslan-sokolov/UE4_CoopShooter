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

	GetMesh()->SetGenerateOverlapEvents(true);  // static mesh with charactermesh collision will response to COLLISION_TRACER

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	CapsuleComp->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	CapsuleComp->SetCollisionResponseToChannel(COLLISION_TRACER, ECR_Ignore);

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(GetMesh());

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	bBoostNoAmmoActive = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();

	// movements defaults todo: resolve server client only actions
	BaseSpeed = GetCharacterMovement()->MaxWalkSpeed;

	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;

	DefaultFOV = CameraComp->FieldOfView;
	//

	// spawn weapon from server
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
		if (CharacterState == ECharacterState::Run)
		{
			if (bShouldSprinting && GetVelocity().Size() > BaseSpeed)
				SetState(ECharacterState::Sprint);
		}
		else if (CharacterState == ECharacterState::Idle || CharacterState == ECharacterState::Falling)
			SetState(ECharacterState::Run);
	}
	//

	// DEBUG STATE TODO: debug param on screen
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
		Weapon->SetShouldAim(true);
	}
}

void ASCharacter::EndZoom()
{

	bWantsToZoom = false;

	if (Weapon)
	{
		Weapon->SetShouldAim(false);
	}
}

void ASCharacter::Fire()
{

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("CHARACTER FIRE")));

	if (Weapon && !bShouldSprinting) {

		Weapon->SetShouldFire(true);
		Weapon->Fire();
	}

}

void ASCharacter::StopFire()
{

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, *FString::Printf(TEXT("CHARACTER STOP FIRE")));

	if (Weapon) {

		Weapon->SetShouldFire(false);

	}

}

void ASCharacter::BeginSprint()
{
	if (GetLocalRole() == ROLE_AutonomousProxy) {
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
	if (GetLocalRole() == ROLE_AutonomousProxy) {
		ServerEndSprint();
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
	ServerSpawnWeapon(WeaponClass);
}

void ASCharacter::ServerSpawnWeapon_Implementation(TSubclassOf<ASWeapon> WeaponClass)
{
	ASWeapon* SpawnedWeapon = GetWorld()->SpawnActor<ASWeapon>(WeaponClass, GetActorLocation(), GetActorRotation());
	SpawnedWeapon->ServerAttachToASCharacter(this);
}


void ASCharacter::SetState(ECharacterState NewCharacterState)
{
	if (NewCharacterState == CharacterState || CharacterState == ECharacterState::Dead)
		return;

	CharacterState = NewCharacterState;

	if (GetLocalRole() == ROLE_AutonomousProxy)
		ServerSetState(NewCharacterState);

	StateTime = 0.0f;

	/*if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan, *FString::Printf(
			TEXT("SetState id: %d, %s"), GetPlayerState()->GetPlayerId(), *DebugCharState(NewCharacterState)));*/
	
}


void ASCharacter::ServerSetState_Implementation(ECharacterState NewCharacterState)
{

	CharacterState = NewCharacterState;

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

		// ToDo: use detach weapon
		if (Weapon)
		{
			Weapon->Destroy();
		}
	}

}


void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, Weapon);
	DOREPLIFETIME_CONDITION(ASCharacter, CharacterState, COND_SimulatedOnly);
	DOREPLIFETIME_CONDITION(ASCharacter, CarriedWeaponSpeedModifier, COND_AutonomousOnly);
}