// Copyright (c) Yevhenii Selivanov

#include "UI/ViewModel/MVVM_MyGameViewModel.h"
//---
#include "Components/MouseActivityComponent.h"
#include "DataAssets/UIDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "LevelActors/PlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "Engine/World.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(MVVM_MyGameViewModel)

/*********************************************************************************************
 * Current Game State
 ********************************************************************************************* */

// Called when the current game state was changed
void UMVVM_MyGameViewModel::OnGameStateChanged_Implementation(ECurrentGameState InGameState)
{
	SetCurrentGameState(InGameState);

	SetCanRestartGame(AMyGameStateBase::Get().CanStartGame());
}

/*********************************************************************************************
 * End-Game State
 ********************************************************************************************* */

// Called when the player state was changed
void UMVVM_MyGameViewModel::OnEndGameStateChanged_Implementation(EEndGameState NewEndGameState)
{
	const ESlateVisibility NewVisibility = NewEndGameState == EEndGameState::None ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	SetEndGameStateVisibility(NewVisibility);

	SetEndGameResult(UUIDataAsset::Get().GetEndGameText(NewEndGameState));
}

/*********************************************************************************************
 * Countdown timers
 ********************************************************************************************* */

// Called when the 'Three-two-one-GO' timer was updated
void UMVVM_MyGameViewModel::OnStartingTimerSecRemainChanged_Implementation(float NewStartingTimerSecRemain)
{
	const int32 Value = FMath::CeilToInt(NewStartingTimerSecRemain);
	SetStartingTimerSecRemain(FText::AsNumber(Value));
}

// Called when remain seconds to the end of the match timer was updated
void UMVVM_MyGameViewModel::OnInGameTimerSecRemainChanged_Implementation(float NewInGameTimerSecRemain)
{
	const int32 Value = FMath::CeilToInt(NewInGameTimerSecRemain);
	SetInGameTimerSecRemain(FText::AsNumber(Value));
}

/*********************************************************************************************
 * PowerUps
 ********************************************************************************************* */

// Called when power-ups were updated on local character
void UMVVM_MyGameViewModel::OnPowerUpsChanged_Implementation(const FBmrPowerUpsContainer& NewPowerUps, const FBmrPowerUpsContainer& PrevPowerUps)
{
	SetPowerUpSkateN(FText::AsNumber(NewPowerUps.Get(EIT::Skate)));
	SetPowerUpBombN(FText::AsNumber(NewPowerUps.Get(EIT::Bomb)));
	SetPowerUpFireN(FText::AsNumber(NewPowerUps.Get(EIT::Fire)));

	// Display powerups in percentage
	SetPowerUpBombPercent(NewPowerUps.Get(EIT::Bomb).GetMaxLevelPercent());
	SetPowerUpBombCurrentPercent(NewPowerUps.Get(EIT::Bomb).GetCurrentLevelPercent());
	SetPowerUpSkatePercent(NewPowerUps.Get(EIT::Skate).GetMaxLevelPercent());
	SetPowerUpFirePercent(NewPowerUps.Get(EIT::Fire).GetMaxLevelPercent());
}

/*********************************************************************************************
 * Mouse Visibility
 ********************************************************************************************* */

// Called when mouse became shown or hidden
void UMVVM_MyGameViewModel::OnMouseVisibilityChanged_Implementation(bool bIsShown)
{
	const ESlateVisibility NewVisibility = bIsShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	SetMouseVisibility(NewVisibility);
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Is called when the view is constructed
void UMVVM_MyGameViewModel::OnViewModelConstruct_Implementation(const UUserWidget* UserWidget)
{
	Super::OnViewModelConstruct_Implementation(UserWidget);

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);

	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);

	BIND_ON_GAME_STATE_CREATED(this, ThisClass::OnGameStateCreated);
}

// Is called when this View Model is destructed
void UMVVM_MyGameViewModel::OnViewModelDestruct_Implementation()
{
	Super::OnViewModelDestruct_Implementation();

	if (UGlobalEventsSubsystem* GlobalEventsSubsystem = UGlobalEventsSubsystem::GetGlobalEventsSubsystem())
	{
		GlobalEventsSubsystem->BP_OnGameStateChanged.RemoveAll(this);
	}

	if (AMyGameStateBase* MyGameState = UMyBlueprintFunctionLibrary::GetMyGameState())
	{
		MyGameState->OnStartingTimerSecRemainChanged.RemoveAll(this);
		MyGameState->OnInGameTimerSecRemainChanged.RemoveAll(this);
	}
}

// Called when Game State was created in current world
void UMVVM_MyGameViewModel::OnGameStateCreated_Implementation(AGameStateBase* GameState)
{
	AMyGameStateBase& MyGameState = *CastChecked<AMyGameStateBase>(GameState);
	MyGameState.OnStartingTimerSecRemainChanged.AddUniqueDynamic(this, &ThisClass::OnStartingTimerSecRemainChanged);
	MyGameState.OnInGameTimerSecRemainChanged.AddUniqueDynamic(this, &ThisClass::OnInGameTimerSecRemainChanged);
	MyGameState.GetWorld()->GameStateSetEvent.RemoveAll(this);
}

// Called when the local player character is spawned, possessed, and replicated
void UMVVM_MyGameViewModel::OnLocalCharacterReady_Implementation(APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);
	PlayerCharacter->OnPowerUpsChanged.AddUniqueDynamic(this, &ThisClass::OnPowerUpsChanged);

	const FBmrPowerUpsContainer PrevPowerups{1, *PlayerCharacter};
	FBmrPowerUpsContainer CurrentPowerUps = PrevPowerups;
	CurrentPowerUps.SetLevel(PlayerCharacter->GetPowerUp(EItemType::Fire), EIT::Fire);
	CurrentPowerUps.SetLevel(PlayerCharacter->GetPowerUp(EItemType::Bomb), EIT::Bomb);
	CurrentPowerUps.SetLevel(PlayerCharacter->GetPowerUp(EItemType::Skate), EIT::Skate);
	OnPowerUpsChanged(CurrentPowerUps, PrevPowerups);

	AMyPlayerState* PlayerState = PlayerCharacter->GetPlayerState<AMyPlayerState>();
	checkf(PlayerState, TEXT("ERROR: [%i] %hs:\n'PlayerState' is null!"), __LINE__, __FUNCTION__);
	PlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);

	UMouseActivityComponent* MouseActivityComponent = UMyBlueprintFunctionLibrary::GetMouseActivityComponent();
	checkf(MouseActivityComponent, TEXT("ERROR: [%i] %hs:\n'MouseActivityComponent' is null!"), __LINE__, __FUNCTION__);
	MouseActivityComponent->OnMouseVisibilityChanged.AddUniqueDynamic(this, &ThisClass::OnMouseVisibilityChanged);
}