// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplodable.h"

#include "Components/AudioComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Particles/ParticleSystemComponent.h"

#include "../Components/SHealthComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Net/UnrealNetwork.h"

// Sets default values
ASExplodable::ASExplodable()
{

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetNotifyRigidBodyCollision(true);
	MeshComp->SetGenerateOverlapEvents(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->DefaultHealth = 100.f;

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

	SetReplicates(true);
	SetReplicateMovement(true);

}

// Called when the game starts or when spawned
void ASExplodable::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
		HealthComp->OnHealthChanged.AddDynamic(this, &ASExplodable::OnHealthChanged);
	else
		HealthComp->OnHealthChangedClient.AddDynamic(this, &ASExplodable::OnHealthChangedClient);
	
}

void ASExplodable::Explode()
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


void ASExplodable::ServerExplode_Implementation()
{
	Explode();
}


void ASExplodable::OnRep_Exploded()
{
	// effects
	if (ExplodedMaterial)
		MeshComp->SetMaterial(0, ExplodedMaterial);

	SoundExplode->Play();
	ExplodeFX->Activate(true);
}

void ASExplodable::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health,
	float HealthDelta, const UDamageType* InstigatedDamageType, AController* InstigatedBy, AActor* DamageCauser)
{

	if (Health == 0.0f)
	{
		ServerExplode();
		return;
	}

	SoundHit->Play();

}

void ASExplodable::OnHealthChangedClient()
{
	if (HealthComp->GetHealth() != 0.0f)
		SoundHit->Play();
}


void ASExplodable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASExplodable, bExploded);
}