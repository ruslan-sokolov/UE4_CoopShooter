// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeaponTracerSimulated.h"

#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "../CoopGame.h"

#include "DrawDebugHelpers.h"
#include "../SWeapon.h"

#include "Kismet/GameplayStatics.h"

ASWeaponTracerSimulated::ASWeaponTracerSimulated()
{
	PrimaryActorTick.bCanEverTick = false;
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

void ASWeaponTracerSimulated::BeginPlay()
{
	Super::BeginPlay();

	// Cast WeaponOwner
	WeaponOwner = Cast<ASWeapon>(GetOwner());

	if (WeaponOwner != nullptr)
	{

		float SimTime = 1.0f; // 1 sec
		FVector InitLocation = GetActorLocation();
		FVector InitVelocity = SphereComp->GetForwardVector() * ProjectileComp->InitialSpeed;
		float Gravity = ProjectileComp->GetGravityZ();
		float Radius = SphereComp->GetScaledSphereRadius();

		FPredictProjectilePathParams PredictParams;
		PredictParams.StartLocation = InitLocation;
		PredictParams.LaunchVelocity = InitVelocity;
		PredictParams.bTraceWithCollision = true;
		PredictParams.ProjectileRadius = Radius;
		PredictParams.MaxSimTime = SimTime;
		PredictParams.TraceChannel = COLLISION_TRACER;
		PredictParams.SimFrequency = 10.0f;
		PredictParams.OverrideGravityZ = Gravity;
		PredictParams.DrawDebugType = EDrawDebugTrace::ForDuration;
		PredictParams.DrawDebugTime = SimTime;

		FPredictProjectilePathResult PredictResult;

		UGameplayStatics::PredictProjectilePath(GetWorld(), PredictParams, PredictResult);
		
		// compute velocity:
		// v = v0 + a*t

		// compute movement delta
		// Velocity Verlet integration (http://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet)
		// The addition of p0 is done outside this method, we are just computing the delta.
		// p = p0 + v0*t + 1/2*a*t^2

		// We use ComputeVelocity() here to infer the acceleration, to make it easier to apply custom velocities.
		// p = p0 + v0*t + 1/2*((v1-v0)/t)*t^2
		// p = p0 + v0*t + 1/2*((v1-v0))*t

		FVector OneSecVelocity = InitVelocity + FVector(0.0f, 0.0f, Gravity) * SimTime;
		FVector OneSecLocation = InitLocation + InitVelocity * SimTime + (OneSecVelocity - InitVelocity) * (0.5f * SimTime);

		DrawDebugSphere(GetWorld(), OneSecLocation, Radius * 1.5f, 12, FColor::Red, false, SimTime);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ASWeaponTracerSimulated::BeginPlay() GetOwner cast to ASWeapon fail, add Owner Weapon as SpawnParameter"));
	}
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

ASWeaponTracerSimulated* ASWeaponTracerSimulated::SpawnFromWeapon(ASWeapon* Weapon)
{
	check(Weapon);
	
	FVector MuzzleLoc = Weapon->GetMeshComp()->GetSocketLocation(Weapon->MuzzleSocketName);

	FVector ShotDirection = Weapon->HitScanTrace.ImpactPoint - MuzzleLoc;
	ShotDirection.Normalize();

	// Normal Spawn
	FActorSpawnParameters SpawnParamters;
	// SpawnParamters.Instigator = Weapon->GetInstigator();
	SpawnParamters.Owner = Weapon;
	
	ASWeaponTracerSimulated* TracerSimulated = Weapon->GetWorld()->SpawnActor<ASWeaponTracerSimulated>(
		Weapon->TracerSimulatedClass, MuzzleLoc, ShotDirection.Rotation(), SpawnParamters);
	//
	
	// Spawn Deferred
	/*const FTransform Transform(ShotDirection.Rotation(), MuzzleLoc, FVector::OneVector);

	ASWeaponTracerSimulated* TracerSimulated = Weapon->GetWorld()->SpawnActorDeferred<ASWeaponTracerSimulated>(
		Weapon->TracerSimulatedClass, Transform);

	// Do on spawn stuff here
	TracerSimulated->WeaponOwner = Weapon;

	TracerSimulated->FinishSpawning(Transform);*/
	//

	return TracerSimulated;
}
