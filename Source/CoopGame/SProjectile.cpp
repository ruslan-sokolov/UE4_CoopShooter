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
#include "Net/UnrealNetwork.h"

ASProjectile::ASProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Components
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComp;
	MeshComp->SetGenerateOverlapEvents(true);
	MeshComp->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

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

	// Projectile Component Defaults
	ProjectileComp->UpdatedComponent = RootComponent;
	ProjectileComp->InitialSpeed = 3000.f;
	ProjectileComp->MaxSpeed = 3000.f;
	ProjectileComp->bRotationFollowsVelocity = true;
	ProjectileComp->bShouldBounce = true;

	// Life span
	InitialLifeSpan = 40.0f;
	
	// Default Damage Type
	DamageType = UDamageType::StaticClass();

	// Replication
	SetReplicates(true);
	SetReplicateMovement(true);

	NetUpdateFrequency = 60.0f;
	MinNetUpdateFrequency = 30.0f;
}

void ASProjectile::BeginPlay()
{
	Super::BeginPlay();

	MeshComp->OnComponentHit.AddDynamic(this, &ASProjectile::OnProjectileHit);

	if (GetLocalRole() == ROLE_Authority)
	{
		ProjectileComp->OnProjectileStop.AddDynamic(this, &ASProjectile::OnProjectileStop);

		if (bCanExplode && DetonationMode == EProjectileDetonationMode::DetonateOnTimerAfterSpawn)
			TriggerDetonation(DetonationTime);
	}
}

// detonation
void ASProjectile::DetonationFX()
{
	if (CameraShakeEffect)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
			PlayerController->ClientPlayCameraShake(CameraShakeEffect, 1.0f);
	}

	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, GetActorLocation());
	
	if (ImpactEffect)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, GetActorLocation());
}

void ASProjectile::Detonate() 
{
	if (bDetonated)
		return;
	bDetonated = true;

	DetonationFX();

	TArray<AActor*> IgnoredActors;
	UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorLocation(), DamageRadius, DamageType, IgnoredActors, GetOwner(), GetInstigatorController(), bDoFullDamage);

	// debug
	auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("COOP.DebugWeapons.Shot"));
	if (CVar)
	{
		int32 Value = CVar->GetInt();
		if (Value > 0) DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 12, FColor::Red, false, 4.0f);
	}
	//

	// Destroy but finish replicatation hack
	// Destroy();
	SetLifeSpan(0.1f);
}


void ASProjectile::TriggerDetonationFX()
{
	PreDetonationSound->Play();
}


void ASProjectile::TriggerDetonation(float Timer)
{
	if (bDetonationTriggered)
		return;
	bDetonationTriggered = true;

	TriggerDetonationFX();

	FTimerHandle EmptyTimeHandle;
	GetWorldTimerManager().SetTimer(EmptyTimeHandle, this, &ASProjectile::Detonate, Timer);
}

// projectile component events
void ASProjectile::OnProjectileStop(const FHitResult& ImpactResult)
{
	MeshComp->SetSimulatePhysics(true);
	MeshComp->AddForceAtLocationLocal(FVector(20.0f, 0.0f, 0.0f), ImpactResult.ImpactPoint);
}

void ASProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetLocalRole() == ROLE_Authority)
		if (bCanExplode)
		{
			if (DetonationMode == EProjectileDetonationMode::DetonateOnTimerAfterHit) 
				TriggerDetonation(DetonationTime);
			
			else if (DetonationMode == EProjectileDetonationMode::DetonateOnHit) 
				Detonate();
		}

	if (BounceSound && !BounceSound->IsPlaying())
		BounceSound->Play();
}

// net replication
void ASProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASProjectile, bDetonated);
	DOREPLIFETIME(ASProjectile, bDetonationTriggered);
}
