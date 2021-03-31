// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopPlayerState.h"

void ACoopPlayerState::AddScore(float ScoreDelta)
{
	float NewScore = GetScore() + ScoreDelta;
	SetScore(NewScore);
}