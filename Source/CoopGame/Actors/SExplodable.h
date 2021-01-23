// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SExplodable.generated.h"

class USHealthComponent;
class UAudioComponent;
class UParticleSystemComponent;
class URadialForceComponent;
class UMaterial;


USTRUCT(BlueprintType)
struct FExplodableLaunchImpulse
{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMinX = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMaxX = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMinY = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMaxY = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMinZ = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Impulse")
		float LaunchImpulseMaxZ = 600.0f;

	FORCEINLINE FVector GenerateRandImpulse() {

		return FVector(
			FMath::FRand() * (LaunchImpulseMaxX - LaunchImpulseMinX) + LaunchImpulseMinX,
			FMath::FRand() * (LaunchImpulseMaxY - LaunchImpulseMinY) + LaunchImpulseMinY,
			FMath::FRand() * (LaunchImpulseMaxZ - LaunchImpulseMinZ) + LaunchImpulseMinZ
		);
	}
	
	FExplodableLaunchImpulse() {}

	explicit FExplodableLaunchImpulse(float MinX, float MaxX, float MinY, float MaxY, float MinZ, float MaxZ) :
		LaunchImpulseMinX(MinX), LaunchImpulseMaxX(MaxX), LaunchImpulseMinY(MinY), LaunchImpulseMaxY(MaxY), LaunchImpulseMinZ(MinZ), LaunchImpulseMaxZ(MaxZ)
	{
		FExplodableLaunchImpulse();
	}

};

UCLASS()
class COOPGAME_API ASExplodable : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASExplodable();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UStaticMeshComponent* MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		USHealthComponent* HealthComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		URadialForceComponent* RadialForceComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UAudioComponent* SoundHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UAudioComponent* SoundExplode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UParticleSystemComponent* ExplodeFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterial* ExplodedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explode Launch Impulse")
		FExplodableLaunchImpulse LaunchImpulseParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Damage")
		bool bEnableRadialDamage = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial Damage", meta = (EditCondition = "bEnableRadialDamage"))
		TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Damage", meta = (EditCondition = "bEnableRadialDamage"))
		float Damage = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Damage", meta = (EditCondition = "bEnableRadialDamage"))
		float DamageRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial Damage", meta = (EditCondition = "bEnableRadialDamage"))
		bool bDoFullDamage;

	void explode();

	bool bExploded;

public:

	UFUNCTION()
		void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta,
			const class UDamageType* InstigatedDamageType, class AController* InstigatedBy, AActor* DamageCauser);

};
