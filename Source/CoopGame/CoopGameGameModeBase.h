// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CoopGameGameModeBase.generated.h"

enum class EWaveState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnActorKilled, AActor*, VictimActor, AActor*, KillerActor, AController*, KillerController);


/**
 * 
 */
UCLASS()
class COOPGAME_API ACoopGameGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	ACoopGameGameModeBase();

protected:

	FTimerHandle TimerHandle_BotSpawner;
	FTimerHandle TimerHandle_NextWaveStart;

	/** Bots mult num to spawn in single wave, total bots spawn num in wave = WaveCount * NrOfBotsToSpawnMult */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BotSpawn")
		int32 NrOfBotsToSpawnMult;

	/** Bots num to spawn left in current wave */
	UPROPERTY(BlueprintReadOnly, Category = "BotSpawn")
		int32 NrOfBotsToSpawn;

	/** Max number of bots to spawn per wave */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BotSpawn")
		int32 MaxNrOfBotsToSpawnPerWave;
	
	/** Current wave number */
	UPROPERTY(BlueprintReadOnly, Category = "BotSpawn")
		int32 WaveCount;
	
	// Max number of spawn waves. If 0, then no spawn wave limit
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BotSpawn")
		int32 MaxWaveNr;

	/** Time wait in wave to spawn single bot in wave*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BotSpawn")
		float TimeBetweenBotSpawnInWave;

	/** Time wait to start next spawn wave */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BotSpawn")
		float TimeBetweenWaves;

	// Hook for BP spawn a single bot
	UFUNCTION(BlueprintImplementableEvent, Category = "BotSpawn")
	void SpawnNewBot();

	// call BP Hook for spawn a single bot
	void SpawnBotTimerElapsed();

	// start actual bot spawn in spawn wave
	void StartWave();
	
	// stop actual bot spawn in spawn wave
	void EndWave();

	// Set timer for next startwave
	void PrepareForNextWave();

	// check and if can prepare next wave, exec if so
	void CheckWaveState();

	void CheckAnyPlayerAlive();

	void GameOver();

	void SetWaveState(EWaveState NewState);

	void RestartDeadPlayers();

public:

	/** Transitions to calls BeginPlay on actors. */
	virtual void StartPlay() override;

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintAssignable, Category = "GameMode")
		FOnActorKilled OnActorKilled;

};
