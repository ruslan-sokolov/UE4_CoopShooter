// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeaponTracerSimulated.h"

#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "../CoopGame.h"

#include "DrawDebugHelpers.h"


ASWeaponTracerSimulated::ASWeaponTracerSimulated()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = SphereComp;

	SphereComp->SetGenerateOverlapEvents(true);
	SphereComp->SetCollisionProfileName(TRACER_PROFILE_NAME);
	SphereComp->InitSphereRadius(5.0f);
	
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ASWeaponTracerSimulated::OnOverlapBegin);
	SphereComp->OnComponentHit.AddDynamic(this, &ASWeaponTracerSimulated::OnHit);

	ParticleSystemComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComp"));
	ParticleSystemComp->SetupAttachment(SphereComp);

	ProjectileComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));

	// projectile movement defaults
	ProjectileComp->ProjectileGravityScale = 0.0f;
	ProjectileComp->InitialSpeed = 30000.0f;
	ProjectileComp->MaxSpeed = 30000.0f;
	ProjectileComp->bRotationFollowsVelocity = true;
	ProjectileComp->bShouldBounce = true;
	ProjectileComp->bBounceAngleAffectsFriction = true;
	ProjectileComp->Bounciness = 0.2f;
	ProjectileComp->Friction = 1.0f;
	ProjectileComp->BounceVelocityStopSimulatingThreshold = 1000.0f;

	ProjectileComp->OnProjectileBounce.AddDynamic(this, &ASWeaponTracerSimulated::OnProjectileBounce);

	// tracer defaults
	InitialLifeSpan = 4.0f;
	bShouldDestroyOnOverlap = true;
	DestroyOnBounceTime = 0.2f;
	bShouldDestroyOnSecondBounce = true;
	BounceAngleToNormalMin = 75.0f;
	BounceAngleCos = cosf(FMath::DegreesToRadians(BounceAngleToNormalMin));  // initialize reflect cos angle
}	

void ASWeaponTracerSimulated::OnProjectileBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	// debug
	// DrawDebugSphere(GetWorld(), GetActorLocation(), 10.0f, 12, FColor::Red, false, 3.0f);
	
	// UE_LOG(LogTemp, Warning, TEXT("Bounce"));

	SetLifeSpan(DestroyOnBounceTime);

	if (bShouldDestroyOnSecondBounce && bIsBouncedFirstTime)
	{
		Destroy();
		return;
	}

	bIsBouncedFirstTime = true;
}

void ASWeaponTracerSimulated::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// debug
	// DrawDebugSphere(GetWorld(), GetActorLocation(), 10.0f, 12, FColor::Green, false, 3.0f);
	
	// UE_LOG(LogTemp, Warning, TEXT("Overlap"));

	if (bShouldDestroyOnOverlap)
	{
		Destroy();
	}
}

void ASWeaponTracerSimulated::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// handle if tracer should reflect
	if (BounceAngleToNormalMin > 0.0f)
	{
		FVector VelocityNormalized = SphereComp->GetComponentVelocity().GetSafeNormal();
		float NormalAndVelocityDot = FVector::DotProduct(-VelocityNormalized, Hit.ImpactNormal);

		// incoming
		// DrawDebugLine(GetWorld(), Hit.Location, VelocityNormalized * -100.0f + Hit.Location, FColor::Red, true);
		// normal
		// DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactNormal * 100.0f + Hit.ImpactPoint, FColor::Green, true);

		// debug angle
		//float HitAngle = FMath::RadiansToDegrees(acosf(NormalAndVelocityDot));
		//UE_LOG(LogTemp, Warning, TEXT("Dot %f, Angle %f, SHOULD BOUNCE WHEN: Dot %f, Angle %f, %s"),
		//	NormalAndVelocityDot, HitAngle, BounceAngleCos, BounceAngleToNormalMin, 
		//	NormalAndVelocityDot <= BounceAngleCos ? *FString("BOUNCE") : *FString("DESTROY"));

		if (NormalAndVelocityDot <= BounceAngleCos)
		{
			ProjectileComp->bShouldBounce = true;
		}
		else
		{
			Destroy();
		}
	}
}
