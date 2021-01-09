// Copyright Epic Games, Inc. All Rights Reserved.

#include "GGGamesGameMode.h"
#include "GGGamesCharacter.h"

AGGGamesGameMode::AGGGamesGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = AGGGamesCharacter::StaticClass();	
}
