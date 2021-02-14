// Fill out your copyright notice in the Description page of Project Settings.


#include "STrackerBot.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "../Components/SHealthComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "Components/AudioComponent.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetNotifyRigidBodyCollision(true);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::OnHealthChange_HandleTakeDamage);

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(RootComponent);
	RadialForceComp->SetAutoActivate(false);
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->Radius = 200.0f;
	RadialForceComp->ImpulseStrength = 500.0f;
	RadialForceComp->bIgnoreOwningActor = true;

	BounceSound = CreateDefaultSubobject<UAudioComponent>(TEXT("BounceSound"));
	BounceSound->SetupAttachment(RootComponent);
	BounceSound->bAutoActivate = false;

	MeshComp->OnComponentHit.AddDynamic(this, &ASTrackerBot::OnMeshCompHit);

	// defaults move
	bUseVelocityChange = true;
	MovementForce = 1000.0f;
	RequiredDistanceToTarget = 100.0f;
	MoveControlTick = 0.1f;

	// defauls launch on stuck
	bUseLaunchWhenStuck = true;
	LaunchOnStuckImpulse = FVector(100.0f, 0.0f, 1000.0f);
	TimeToConsiderStuck = 1.0f;
	StuckDistanceDelta = 50.0f;

	// defaults explosion
	ExplosionDamage = 70.0f;
	ExplosionRadius = 250.0f;
	bDoFullDamage = false;
	DamageType = UDamageType::StaticClass();
	bExplosionApplyRadialImpulse = true;

	// defaults sound
	BounceSound_HitImpulseTrashhold = 4000.0f;

	// defaults jump and explode
	bJumpAndExplodeNearChasedCharacter = true;
	DistanceToJumpAndExplode = 200.0f;
	ChasedCharacterBoneJumpTo = FName("head");
	JumpExtraHeight = 50.0f;
	JumpHeightReachExplosionDelay = 0.5f;
	bAddAngularImpulseOnJumpExplosion = true;
	AngularVelocityOnJumpExplosion = FVector(0.0f, 0.0f, 3000.0f);

}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	// Create and set dynamic material instance
	MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));

	// Hack Init Chased Character 
	ChasedCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);

	// Find initial move to
	NextPathPoint = GetNextPathPoint();

	// Move Tick Enable
	GetWorldTimerManager().SetTimer(TimerHandle_MoveControl, this, &ASTrackerBot::OnTimer_MoveControl, MoveControlTick, true);
}


FVector ASTrackerBot::GetNextPathPoint()
{
	// check
	if (ChasedCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASTrackerBot::GetNextPathPoint(), ChasedCharacter is nullptr!!!"));
	}

	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), ChasedCharacter);

	FVector NextPoint;
	
	if (NavPath->PathPoints.Num() > 1)
	{
		// next point in the path
		NextPoint = NavPath->PathPoints[1];
	}
	else
	{
		// Failed to find path
		NextPoint = ChasedCharacter->GetActorLocation();
	}

	// debug
	if (debugParam > 0)
		DrawDebugSphere(GetWorld(), NextPoint, 100.0, 12, FColor::Cyan, false, 3.0f);

	return NextPoint;
}


bool ASTrackerBot::IsStuckDesisionMake()
{
	float StuckDistanceMeasure = 0.0f;

	if (bPossiblyStuck)
	{
		// check actual possible stuck location
		StuckDistanceMeasure = (GetActorLocation() - PossibleStuckLocation).Size();
		
		// debug
		if (debugParam > 0)
			DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), 
				FString::Printf(TEXT("Delta PossibleStuckLocation: %s,  CountDown TimeToDesideStuck: %s"), 
					*FString::SanitizeFloat(StuckDistanceMeasure), *FString::SanitizeFloat(TimeToConsiderStuckAccomulation)), (AActor*)0, FColor::Red, MoveControlTick);
		//
	}
	else
	{
		// try init new stuck location
		StuckDistanceMeasure = (GetActorLocation() - StuckCheck_PrevActorLocation).Size();
		
		// debug
		if (debugParam > 0)
			DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.0f, 0.0f, 100.0f),
				FString::Printf(TEXT("Delta PrevLocation: %s,  CountDown TimeToDesideStuck: %s"),
					*FString::SanitizeFloat(StuckDistanceMeasure), *FString::SanitizeFloat(TimeToConsiderStuckAccomulation)), (AActor*)0, FColor::Orange, MoveControlTick);
		//
	}

	// check if possibly stuck
	if (StuckDistanceMeasure <= StuckDistanceDelta)
	{
		if (!bPossiblyStuck)  // save possible stuck location only once
		{
			PossibleStuckLocation = StuckCheck_PrevActorLocation;
			bPossiblyStuck = true;
		}

		// timer to make final stuck verdict
		TimeToConsiderStuckAccomulation += MoveControlTick;

		if (TimeToConsiderStuckAccomulation >= TimeToConsiderStuck)
		{
			TimeToConsiderStuckAccomulation = 0.0f;  // block multiple execution
			return true;
		}
	}
	else
	{
		TimeToConsiderStuckAccomulation = 0.0f;
		bPossiblyStuck = false;
	}

	return false;
}


void ASTrackerBot::LaunchOnStuck()
{

	FVector Direction = NextPathPoint - GetActorLocation();
	Direction.Normalize();
	FVector ImpulseTowardsNextPathPoint = Direction.Rotation().RotateVector(LaunchOnStuckImpulse);

	MeshComp->AddImpulse(ImpulseTowardsNextPathPoint, NAME_None, bUseVelocityChange);

	// debug
	if (debugParam > 0)
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ImpulseTowardsNextPathPoint, 32, FColor::Blue, false, 3.0f, 0, 2.0f);
}


void ASTrackerBot::HandleStuck_OnTimer_MoveControl()
{
	if (bUseLaunchWhenStuck)
	{
		if (IsStuckDesisionMake())
		{
			LaunchOnStuck();
		}
		else
		{
			// update prev actor location
			StuckCheck_PrevActorLocation = GetActorLocation();
		}
	}
}


void ASTrackerBot::UpdateNextPathPoint_WhenReach()
{
	float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

	if (DistanceToTarget <= RequiredDistanceToTarget)
	{
		// Calc new path point to move to
		NextPathPoint = GetNextPathPoint();

		// debug
		if (debugParam > 0)
			DrawDebugString(GetWorld(), GetActorLocation(), FString::Printf(TEXT("Reached! Next Target Point: %s"), *NextPathPoint.ToString()), (AActor*)0, FColor::Cyan, MoveControlTick, true);
	}
}


void ASTrackerBot::CheckExplodeActivation_OnTimer_MoveControl()
{
	if (ChasedCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASTrackerBot::CheckExplodeActivation_OnTimer_MoveControl(), ChasedCharacter is nullptr!"));
		return;
	}

	float DistanceToCharacter = (GetActorLocation() - ChasedCharacter->GetActorLocation()).Size();

	if (DistanceToCharacter <= DistanceToJumpAndExplode)
	{
		bDelayedDetonationActivated = true;
	}
}


void ASTrackerBot::RunDelayedDetonation()
{
	if (ChasedCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASTrackerBot::RunDelayedDetonation(), ChasedCharacter is nullptr!"));
		return;
	}

	if (bDelayedDetonationActivated)
	{
		float Gravity = GetWorld()->GetDefaultGravityZ();
		float JumpZHeight = ChasedCharacter->GetMesh()->GetBoneLocation(ChasedCharacterBoneJumpTo).Z + JumpExtraHeight;
		float JumpZVelocity = sqrtf(-2.0f * Gravity * JumpZHeight);

		// make TrackerBot jump to ChasedCharacter Selected Bone Z location
		MeshComp->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, JumpZVelocity));

		float JumpZTime = 2.0f * JumpZHeight / JumpZVelocity;

		if (bAddAngularImpulseOnJumpExplosion)
			MeshComp->SetPhysicsAngularVelocity(AngularVelocityOnJumpExplosion);

		if (MatInst)
		{
			MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
			MatInst->SetScalarParameterValue("PulseMult", 512.0f);
		}

		// make TrackerBot explode after Selected Bone Z Location Reached
		FTimerHandle EmptyTimerHandler;
		GetWorldTimerManager().SetTimer(EmptyTimerHandler, this, &ASTrackerBot::SelfDestruct, JumpZTime + JumpHeightReachExplosionDelay, false);
	}

}


void ASTrackerBot::OnTimer_MoveControl()
{
	// check
	if (!MeshComp->IsSimulatingPhysics())
		UE_LOG(LogTemp, Warning, TEXT("ASTrackerBot::MoveToNextPathPoint(), MeshComp Physics Simulation is disabled! Please, enable"));

	// if delayed detonation activated stop movement
	if (bDelayedDetonationActivated)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle_MoveControl);
		RunDelayedDetonation();
		return;
	}

	// check if NextPathPoint reached then update NextPathPoint
	UpdateNextPathPoint_WhenReach();

	// check if stuck then launch
	HandleStuck_OnTimer_MoveControl();

	// check if character close then activate delayed detonation
	CheckExplodeActivation_OnTimer_MoveControl();
}


void ASTrackerBot::OnTick_MoveToNextPathPoint()
{
	// stop movement force when delayed detonation activated
	if (bDelayedDetonationActivated)
	{
		return;
	}

	// Keep moving towards next target
	FVector ForceDirection = NextPathPoint - GetActorLocation();
	ForceDirection.Normalize();
	ForceDirection *= MovementForce;

	MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

	// debug
	if (debugParam > 0)
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 2.0f);
}


// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OnTick_MoveToNextPathPoint();
}


void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	// Apply Damage!
	UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage, GetActorLocation(), ExplosionRadius, DamageType,
		IgnoredActors, nullptr, nullptr, bDoFullDamage);

	// Apply Radial Impulse!
	if (bExplosionApplyRadialImpulse)
	{
		RadialForceComp->FireImpulse();
	}

	// debug
	if (debugParam > 0)
		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 5.0f, 0, 1.0f);

	// Delete Actor immediately
	Destroy();
}


void ASTrackerBot::OnHealthChange_HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, 
	const UDamageType* InstigatedDamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}


void ASTrackerBot::OnMeshCompHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (NormalImpulse.Size() > BounceSound_HitImpulseTrashhold)
	{
		BounceSound->Play();
	}
}