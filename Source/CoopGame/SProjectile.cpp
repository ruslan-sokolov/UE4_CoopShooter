// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Components/SceneComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Gameframework/DamageType.h"
#include "DrawDebugHelpers.h"

// Sets default values
ASProjectile::ASProjectile()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComp;
	MeshComp->SetGenerateOverlapEvents(true);
	MeshComp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	MeshComp->OnComponentHit.AddDynamic(this, &ASProjectile::OnProjectileHit);

	ProjectileComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile"));

	BounceSound = CreateDefaultSubobject<UAudioComponent>(TEXT("BounceSound"));
	BounceSound->SetupAttachment(RootComponent);
	BounceSound->bAutoActivate = false;

	PreDetonationSound = CreateDefaultSubobject<UAudioComponent>(TEXT("PreDetonationSound"));
	PreDetonationSound->SetupAttachment(RootComponent);
	PreDetonationSound->bAutoActivate = false;

	// Players can't walk on it
	MeshComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	MeshComp->CanCharacterStepUpOn = ECB_No;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileComp->UpdatedComponent = RootComponent;
	ProjectileComp->InitialSpeed = 3000.f;
	ProjectileComp->MaxSpeed = 3000.f;
	ProjectileComp->bRotationFollowsVelocity = true;
	ProjectileComp->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 40.0f;
	
	DamageType = UDamageType::StaticClass();

}

// Called when the game starts or when spawned
void ASProjectile::BeginPlay()
{
	Super::BeginPlay();
	ProjectileComp->OnProjectileStop.AddDynamic(this, &ASProjectile::OnProjectileStop);

	if (bCanExplode && !bDetonationTriggered && DetonationMode == ProjectileDetonationMode::DetonateOnTimerAfterSpawn)
	{

		bDetonationTriggered = true;
		TriggerDetonation(DetonationTime);
	
	}

}

void ASProjectile::Detonate() 
{

	if (CameraShakeEffect) {

		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

		if (PlayerController)
		{

			PlayerController->ClientPlayCameraShake(CameraShakeEffect, 1.0f);

		}
	}

	// resolve detonation here
	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	if (ImpactEffect)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, GetActorLocation());

	TArray<AActor*> IgnoredActors;

	UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorLocation(), DamageRadius, DamageType, IgnoredActors, GetOwner(), GetInstigatorController(), true);

	//DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 12, FColor::Red, false, 2.0f);

	Destroy();
}

void ASProjectile::TriggerDetonation(float Timer)
{

	FTimerHandle EmptyTimeHandle;

	GetWorldTimerManager().SetTimer(EmptyTimeHandle, this, &ASProjectile::Detonate, Timer);

}

void ASProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Purple, *FString::Printf(TEXT("STOP PROJECTILE")));
	
	MeshComp->SetSimulatePhysics(true);
	MeshComp->AddForceAtLocationLocal(FVector(20.0f, 0.0f, 0.0f), ImpactResult.ImpactPoint);
}

void ASProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bCanExplode && !bDetonationTriggered)
	{

		if (DetonationMode == ProjectileDetonationMode::DetonateOnTimerAfterHit) {

			bDetonationTriggered = true;
			TriggerDetonation(DetonationTime);
			PreDetonationSound->Play();
		}

		else if (DetonationMode == ProjectileDetonationMode::DetonateOnHit) {

			bDetonationTriggered = true;
			Detonate();

		}
	}

	if (BounceSound) {

		if (!BounceSound->IsPlaying())
			BounceSound->Play();
	}

}
