// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplodable.h"

#include "Components/AudioComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "../Components/SHealthComponent.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
ASExplodable::ASExplodable()
{

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MeshComp->SetSimulatePhysics(true);
	MeshComp->BodyInstance.bNotifyRigidBodyCollision = true;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->DefaultHealth = 100.f;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplodable::OnHealthChanged);

	
	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->SetAutoActivate(false);
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->Radius = 250.0f;
	RadialForceComp->bIgnoreOwningActor = true;

	SoundHit = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundHit"));
	SoundHit->SetupAttachment(RootComponent);
	SoundHit->SetAutoActivate(false);

	SoundExplode = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundExplode"));
	SoundExplode->SetupAttachment(RootComponent);
	SoundExplode->SetAutoActivate(false);

	ExplodeFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ExplodeFX"));
	ExplodeFX->SetupAttachment(RootComponent);
	ExplodeFX->SetAutoActivate(false);

	DamageType = UDamageType::StaticClass();

}

// Called when the game starts or when spawned
void ASExplodable::BeginPlay()
{
	Super::BeginPlay();
	
}

void ASExplodable::explode()
{
	if (bExploded == true)
		return;
	
	bExploded = true;

	// effects
	if (ExplodedMaterial)
		MeshComp->SetMaterial(0, ExplodedMaterial);

	SoundExplode->Play();
	ExplodeFX->Activate(true);
	//

	// launch mesh
	MeshComp->AddImpulse(LaunchImpulseParams.GenerateRandImpulse(), NAME_None, true);
	
	// radial impulse
	RadialForceComp->FireImpulse();

	// radial damage
	if (bEnableRadialDamage)
	{

		TArray<AActor*> IgnoredActors;
		UGameplayStatics::ApplyRadialDamage(GetWorld(), Damage, GetActorLocation(), DamageRadius, 
			DamageType, IgnoredActors, GetOwner(), GetInstigatorController(), bDoFullDamage);
	
	}

}


void ASExplodable::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, 
	float HealthDelta, const UDamageType* InstigatedDamageType, AController* InstigatedBy, AActor* DamageCauser)
{

	if (Health == 0.0f)
		explode();
		return;

	SoundHit->Play();

}

