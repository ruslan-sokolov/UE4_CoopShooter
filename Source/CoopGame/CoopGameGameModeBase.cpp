// Copyright Epic Games, Inc. All Rights Reserved.


#include "CoopGameGameModeBase.h"

#include "CoopHUD.h"
#include "CoopGameState.h"
#include "SCharacter.h"
#include "CoopPlayerState.h"

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

	// custom player state
	static ConstructorHelpers::FClassFinder<APlayerState> PlayerStateFinder(TEXT("/Game/Blueprints/GameMode/BP_PlayerState"));
	if (PlayerStateFinder.Succeeded())
		PlayerStateClass = PlayerStateFinder.Class;
	else
		PlayerStateClass = ACoopPlayerState::StaticClass();

	// bot spawn defaults
	bWaveSpawnEnableOnStart = false;
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

	SetWaveState(EWaveState::WaveInProgress);
}

void ACoopGameGameModeBase::EndWave()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);

	SetWaveState(EWaveState::WaitingToComplete);
}

void ACoopGameGameModeBase::PrepareForNextWave()
{
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ACoopGameGameModeBase::StartWave, TimeBetweenWaves, false);

	SetWaveState(EWaveState::PreparingNextWave);

	RestartDeadPlayers();
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

		SetWaveState(EWaveState::WaveComplete);

		PrepareForNextWave();
	}
}

void ACoopGameGameModeBase::CheckAnyPlayerAlive()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		if (PC)
		{
			APawn* PlayerPawn = PC->GetPawn();

			if (PlayerPawn)
			{
				USHealthComponent* HealthComp =  Cast<USHealthComponent>(PlayerPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			
				if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f)
				{
					// A Player is still alive
					return;
				}
			}
		}
	}

	// No Player alive
	GameOver();
}

void ACoopGameGameModeBase::GameOver()
{
	EndWave();

	// @ToDo: finish up the match, present "Game over" to players

	// debug
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, "[GameMode] Game over. All players are dead.");
	//

	SetWaveState(EWaveState::GameOver);
}

void ACoopGameGameModeBase::SetWaveState(EWaveState NewState)
{
	ACoopGameState* GS = GetGameState<ACoopGameState>();
	if (ensureAlways(GS))
	{
		GS->SetWaveState(NewState);
	}
}

void ACoopGameGameModeBase::RestartDeadPlayers()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn() == nullptr)
		{
			RestartPlayer(PC);
		}
	}
}

void ACoopGameGameModeBase::StartPlay()
{
	Super::StartPlay();

	if (bWaveSpawnEnableOnStart)
	{
		SetWaveState(EWaveState::WaitingToStart);

		PrepareForNextWave();  // first wave inititialize
	}
}

void ACoopGameGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bWaveSpawnEnableOnStart)
	{
		CheckWaveState();
		CheckAnyPlayerAlive();
	}
}
