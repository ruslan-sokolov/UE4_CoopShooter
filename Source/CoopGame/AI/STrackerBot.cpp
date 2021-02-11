// Fill out your copyright notice in the Description page of Project Settings.


#include "STrackerBot.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = MeshComp;
	MeshComp->SetCanEverAffectNavigation(false);
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	GetNextPathPoint();
}


FVector ASTrackerBot::GetNextPathPoint()
{
	// Hack, to get player location
	AActor* PlayerPawn  = Cast<AActor>(UGameplayStatics::GetPlayerCharacter(this, 0));

	/*UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);*/

	FVector NextPoint;
	/*
	if (NavPath->PathPoints.Num() > 1)
	{
		// next point in the path
		NextPoint = NavPath->PathPoints[1];
	}
	else
	{
		// Failed to find path
		NextPoint = GetActorLocation();
	}

	// debug
	DrawDebugSphere(GetWorld(), NextPoint, 100.0, 12, FColor::Cyan, true, 1.0f);*/

	return NextPoint;
}


// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
