// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeaponTracerSimulated.h"

#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../CoopGame.h"

// Sets default values
ASWeaponTracerSimulated::ASWeaponTracerSimulated()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	//RootComponent = SceneComp;
	
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->SetGenerateOverlapEvents(true);
	CapsuleComp->SetCollisionProfileName(TRACER_PROFILE_NAME);

	//CapsuleComp->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
	RootComponent = CapsuleComp;
	//CapsuleComp->SetupAttachment(RootComponent);

	ParticleSystemComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystemComp"));
	//ParticleSystemComp->SetAbsolute(false, true, false);
	ParticleSystemComp->SetupAttachment(CapsuleComp);
	//ParticleSystemComp->SetWorldRotation(FRotator(90.0f, 0.0f, 0.0f));
	//ParticleSystemComp->SetAbsolute(false, false, false);


	ProjectileComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));

}

// Called when the game starts or when spawned
void ASWeaponTracerSimulated::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASWeaponTracerSimulated::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

