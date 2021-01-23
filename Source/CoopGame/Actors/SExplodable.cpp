// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplodable.h"

#include "Components/AudioComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "../Components/SHealthComponent.h"



// Sets default values
ASExplodable::ASExplodable()
{

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MeshComp->SetSimulatePhysics(true);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->DefaultHealth = 100.f;
	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplodable::OnHealthChanged);

	
	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->SetAutoActivate(false);

	SoundHit = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundHit"));
	SoundHit->SetupAttachment(RootComponent);
	SoundHit->SetAutoActivate(false);

	SoundExplode = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundExplode"));
	SoundExplode->SetupAttachment(RootComponent);
	SoundExplode->SetAutoActivate(false);

	ExplodeFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ExplodeFX"));
	ExplodeFX->SetupAttachment(RootComponent);
	ExplodeFX->SetAutoActivate(false);

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
	UE_LOG(LogTemp, Warning, TEXT("EXPLODE"));


	RadialForceComp->Activate();
	if (ExplodedMaterial)
		MeshComp->SetMaterial(0, ExplodedMaterial);

	MeshComp->AddImpulse(LaunchImpulseParams.GenerateRandImpulse(), NAME_None, true);

	SoundExplode->Play();

	ExplodeFX->Activate(true);
	
}


void ASExplodable::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, 
	float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{

	if (Health == 0.0f)
		explode();
		return;

	SoundHit->Play();

}

