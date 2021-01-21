// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Blueprint/UserWidget.h"


#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class FString;

class USHealthComponent;

UENUM(BlueprintType)
enum class ECharacterState : uint8
{

	Idle UMETA(DisplayName = "Idle"),
	Walk UMETA(DisplayName = "Walk"), // not used yet
	Run UMETA(DisplayName = "Run"),
	Sprint UMETA(DisplayName = "Sprint"),

	Crouch UMETA(DisplayName = "Crouch"),
	Proning UMETA(DisplayName = "Proning"), // not used yet
	
	Jumping UMETA(DisplayName = "Jumping"),
	Falling UMETA(DisplayName = "Falling"),

	Cover UMETA(DisplayName = "Cover"), // not used yet
	VehiclePilot UMETA(DisplayName = "VehiclePilot"), // not used yet
	VehiclePassenger UMETA(DisplayName = "VehiclePassenger"), // not used yet

	Dead UMETA(DisplayName = "Dead"), // not used yet

};

FORCEINLINE FString DebugCharState(ECharacterState State) {

	if (State == ECharacterState::Idle)
		return FString(TEXT("Idle"));

	else if (State == ECharacterState::Walk)
		return FString(TEXT("Walk"));

	else if (State == ECharacterState::Run)
		return FString(TEXT("Run"));

	else if (State == ECharacterState::Sprint)
		return FString(TEXT("Sprint"));

	else if (State == ECharacterState::Crouch)
		return FString(TEXT("Crouch"));

	else if (State == ECharacterState::Proning)
		return FString(TEXT("Proning"));

	else if (State == ECharacterState::Jumping)
		return FString(TEXT("Jumping"));

	else if (State == ECharacterState::Falling)
		return FString(TEXT("Falling"));

	else if (State == ECharacterState::Cover)
		return FString(TEXT("Cover"));

	else if (State == ECharacterState::VehiclePilot)
		return FString(TEXT("VehiclePilot"));

	else if (State == ECharacterState::VehiclePassenger)
		return FString(TEXT("VehiclePassenger"));

	else
		return FString("None");
};

UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
		UCameraComponent* CameraComp;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
		USpringArmComponent* SpringArmComp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
		USHealthComponent* HealthComp;

	/** Character Speed while sprinting **/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Movement: Sprint")
		float SprintSpeed = 720.0f;

	/** Default weapon class to spawn with player  **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character: Combat")
		TSubclassOf<ASWeapon> WeaponClassToSpawnWith;

	bool bWantsToZoom;

	/** Change FOV when aiming can be overriden by weapon settings **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character: Player")
	float ZoomedFOV;

	/** Change FOV speed. Bigger value - smoother can be overriden by weapon settings**/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character: Player", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
		float ZoomInterpSpeed = 20.0f;

	/* Default FOV Set During Begin Play */
	float DefaultFOV;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Val);
	void MoveRight(float Val);

	void BeginCrouch();
	void EndCrouch();

	void BeginSprint();
	void EndSprint();

	void BeginZoom();
	void EndZoom();

	void Fire();

	void Reload();

	void StopFire();

	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintCallable, Category = "Chracter: Combat")
		void SpawnWeapon(TSubclassOf<ASWeapon> WeaponClass);

	UFUNCTION()
		void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
			const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(BlueprintReadOnly, Category = "Chracter: State")
		ECharacterState CharacterState;

	UFUNCTION(BlueprintCallable, Category = "Chracter: State")
		void SetState(ECharacterState NewState);

	void BeginJump();

	/** Current State Active Time* */
	UPROPERTY(BlueprintReadOnly, Category = "Chracter: State")
		float StateTime;

	/** Carried Weapon Speed modifier **/
	UPROPERTY(BlueprintReadOnly, Category = "Chracter: Combat")
		float CarriedWeaponSpeedModifier = 1.0f;

	bool bShouldSprinting;

	float BaseSpeed;

	/** Weapon Attachment SocketName **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Chracter: Combat")
		FName WeaponSocketName = "WeaponSocket";

	/** Current Character Weapon  **/
	UPROPERTY(BlueprintReadOnly, Category = "Chracter: Combat")
		ASWeapon* Weapon;

};