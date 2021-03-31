// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopGameState.h"

#include "Net/UnrealNetwork.h"

void ACoopGameState::OnRep_WaveState(EWaveState OldState)
{
	WaveStateChanged(WaveState, OldState);
}

void ACoopGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACoopGameState, WaveState);
}

void ACoopGameState::SetWaveState(EWaveState NewState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		EWaveState OldState = WaveState;

		WaveState = NewState;

		OnRep_WaveState(OldState); // call on rep wave state for server explicitly
	}
}
