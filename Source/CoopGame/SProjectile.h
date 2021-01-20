// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SProjectile.generated.h"

class UProjectileMovementComponent;
class UCapsuleComponent;
class UAudioComponent;
class UParticleSystem;
class USceneComponent;
class UParticleSystemComponent;
class USoundBase;
class UCameraShake;

UENUM(BlueprintType)
enum class ProjectileDetonationMode : uint8
{

	DetonateOnHit,
	DetonateOnTimerAfterSpawn,
	DetonateOnTimerAfterHit,

};

UCLASS()
class COOPGAME_API ASProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
		UStaticMeshComponent* MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UProjectileMovementComponent* ProjectileComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
		UParticleSystem* ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UAudioComponent* BounceSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
		USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UAudioComponent* PreDetonationSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		bool bCanExplode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		ProjectileDetonationMode DetonationMode = ProjectileDetonationMode::DetonateOnTimerAfterHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
		float DetonationTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
		TSubclassOf<UCameraShake> CameraShakeEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
		TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		float Damage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		float DamageRadius = 300.0f;

	bool bDetonationTriggered;

public:	
	UFUNCTION()
		void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
		void OnProjectileStop(const FHitResult& ImpactResult);
	
	void TriggerDetonation(float Timer);
	void Detonate();
};
