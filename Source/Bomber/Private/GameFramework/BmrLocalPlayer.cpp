// Copyright (c) Yevhenii Selivanov

#include "GameFramework/BmrLocalPlayer.h"
//---
#include "Controllers/MyPlayerController.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrLocalPlayer)

// Default constructor with overridden properties
UBmrLocalPlayer::UBmrLocalPlayer()
{
	// Ensures the correct PlayerController is spawned locally when the client joins a session
	PendingLevelPlayerControllerClass = AMyPlayerController::StaticClass();
}