// Copyright Epic Games, Inc. All Rights Reserved.


#include "CoopGameGameModeBase.h"

#include "CoopHUD.h"
#include "CoopGameState.h"
#include "SCharacter.h"

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

}