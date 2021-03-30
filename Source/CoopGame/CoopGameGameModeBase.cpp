// Copyright Epic Games, Inc. All Rights Reserved.


#include "CoopGameGameModeBase.h"

#include "CoopHUD.h"
#include "CoopGameState.h"
#include "SCharacter.h"

#include "Components/SHealthComponent.h"

ACoopGameGameModeBase::ACoopGameGameModeBase()
{

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Blueprints/BP_Player"));
	if (PlayerPawnClassFinder.Succeeded())
		DefaultPawnClass = PlayerPawnClassFinder.Class;
	else
		DefaultPawnClass = ASCharacter::StaticClass();

	// use our custom HUD class
	static ConstructorHelpers::FClassFinder<AHUD> HUDClassFinder(TEXT("/Game/Blueprints/GameMode/BP_HUD"));
	if (HUDClassFinder.Succeeded())
		HUDClass = HUDClassFinder.Class;
	else
		HUDClass = ACoopHUD::StaticClass();

	// custom game state
	static ConstructorHelpers::FClassFinder<AGameStateBase> GameStateFinder(TEXT("/Game/Blueprints/GameMode/BP_GameState"));
	if (GameStateFinder.Succeeded())
		GameStateClass = GameStateFinder.Class;
	else
		GameStateClass = ACoopGameState::StaticClass();

	// bot spawn defaults
	NrOfBotsToSpawn = 0;
	NrOfBotsToSpawnMult = 2;
	MaxNrOfBotsToSpawnPerWave = 10;
	WaveCount = 0;
	MaxWaveNr = 0;
	TimeBetweenBotSpawnInWave = 1.0f;
	TimeBetweenWaves = 2.0f;

	// tick
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;
}

void ACoopGameGameModeBase::SpawnBotTimerElapsed()
{
	SpawnNewBot();

	NrOfBotsToSpawn--;

	if (NrOfBotsToSpawn <= 0)
	{
		// stop inf loop of actual bot spawn
		EndWave();
	}
}

void ACoopGameGameModeBase::StartWave()
{
	WaveCount++;

	NrOfBotsToSpawn = NrOfBotsToSpawnMult * WaveCount;
	if (NrOfBotsToSpawn > MaxNrOfBotsToSpawnPerWave) NrOfBotsToSpawn = MaxNrOfBotsToSpawnPerWave;

	// start inf loop of actual bot spawn
	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ACoopGameGameModeBase::SpawnBotTimerElapsed, TimeBetweenBotSpawnInWave, true, 0.0);
}

void ACoopGameGameModeBase::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
}

void ACoopGameGameModeBase::PrepareForNextWave()
{
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ACoopGameGameModeBase::StartWave, TimeBetweenWaves, false);
}

void ACoopGameGameModeBase::CheckWaveState()
{
	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);

	if (NrOfBotsToSpawn > 0 || bIsPreparingForWave)
	{
		return;
	}

	bool bIsAnyBotAlive = false;

	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		APawn* TestPawn = It->Get();
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled())
		{
			continue;
		}

		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));

		if (HealthComp && HealthComp->GetHealth() > 0.0f)
		{
			bIsAnyBotAlive = true;
			break;
		}
	}

	if (!bIsAnyBotAlive)
	{
		// debug
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, "[GameMode] Prep next spawn wave");
		//

		PrepareForNextWave();
	}
}

void ACoopGameGameModeBase::StartPlay()
{
	Super::StartPlay();

	PrepareForNextWave();  // first wave inititialize
}

void ACoopGameGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckWaveState();
}
