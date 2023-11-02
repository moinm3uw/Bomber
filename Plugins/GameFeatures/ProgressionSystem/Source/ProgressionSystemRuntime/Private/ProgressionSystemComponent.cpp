// Copyright (c) Yevhenii Selivanov

#include "ProgressionSystemComponent.h"
//---
#include "Bomber.h"
#include "Controllers/MyPlayerController.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
#include "ProgressionSystemDataAsset.h"
#include "ProgressionSystemRuntimeModule.h"
#include "UnrealWidgetFwd.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/ScaleBox.h"
//#include "UI/MainMenuWidget.h"
#include "UI/MyHUD.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
#include "Widgets/ProgressionMenuWidget.h"
#include "Widgets/ProgressionSaveWidget.h"
#include "ModuleStructures.h"
#include "DataAssets/UIDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "LevelActors/PlayerCharacter.h"

// Sets default values for this component's properties
UProgressionSystemComponent::UProgressionSystemComponent()
{	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Called when the game starts
void UProgressionSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogProgressionSystem, Warning, TEXT("--- I'm running ---"));
	
	AMyHUD* MyHUD = Cast<AMyHUD>(GetOwner());
	const AMyHUD& HUD = *MyHUD;
	ProgressionMenuWidgetInternal = HUD.CreateWidgetByClass<UProgressionMenuWidget>(ProgressionSystemDataAssetInternal->GetProgressionMenuWidget(), true, 1);
	ProgressionSaveWidgetInternal = HUD.CreateWidgetByClass<UProgressionSaveWidget>(ProgressionSystemDataAssetInternal->GetProgressionSaveWidget(), true, 10);
	if (ProgressionMenuWidgetInternal)
	{
		FString myString = FString::Printf(TEXT("%d"), GetCurrenTotalScore());
		ProgressionMenuWidgetInternal->SetProgressionState(FText::FromString(myString));
	}
	ProgressionDataTableInternal = ProgressionSystemDataAssetInternal->GetProgressionDataTable();

	// Listen states to spawn widgets
	if (AMyGameStateBase* MyGameState = UMyBlueprintFunctionLibrary::GetMyGameState())
	{
		HandleGameState(MyGameState);	
	}
	else if (AMyPlayerController* MyPC = GetOwner<AMyPlayerController>())
	{
		MyPC->OnGameStateCreated.AddUniqueDynamic(this, &ThisClass::HandleGameState);
	}

	if (AMyPlayerState* MyPlayerState = UMyBlueprintFunctionLibrary::GetLocalPlayerState())
	{
		HandleEndGameState(MyPlayerState);	
	}
	if (APlayerCharacter* MyPlayerCharacter = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter())
	{
		MyPlayerCharacter->OnPlayerTypeChanged.AddUniqueDynamic(this, &ThisClass::OnPlayerTypeChanged);
	}
}

// Save the progression depends on EEndGameState
void UProgressionSystemComponent::SavePoints(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState)
{
	SavedProgressionInternal.Points += GetProgressionReward(Map, Character, EndGameState);
	UpdateProgressionRowName();
}

// Checks and updates the progression row name if points to unlock for the next progression reached. 
void UProgressionSystemComponent::UpdateProgressionRowName()
{
	FProgressionRowData* NextRow = nullptr;
	FName NextRowName;
	int CurrentRowIndex;

	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
	for (int i = 0; i < RowNames.Num(); i++)
	{
		if (RowNames[i] == SavedProgressionInternal.ProgressionRowName)
		{
			CurrentRowIndex = i;
			if (CurrentRowIndex + 1 <= RowNames.Num() - 1)
			{
				NextRowName = RowNames[CurrentRowIndex + 1];
			}
			else
			{
				NextRowName = SavedProgressionInternal.ProgressionRowName;
			}
			break;
		}
	}
	
	NextRow = ProgressionDataTableInternal->FindRow<FProgressionRowData>(NextRowName, "Finding next progression row");
	if (NextRow)
	{
		if (NextRow->PointsToUnlock <= SavedProgressionInternal.Points)
		{
			SavedProgressionInternal.ProgressionRowName = NextRowName;
		}
	}
}

int UProgressionSystemComponent::GetProgressionReward(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState)
{
	FName CurrentProgressionRowName = GetProgressionRow(Map, Character);
	FProgressionRowData* ProgressionRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(
		CurrentProgressionRowName, "Finding a needed row");
	if (!ProgressionRowData)
	{
		return 0; 
	}
	
	switch (EndGameState)
	{
	case EEndGameState::Win:
		return ProgressionRowData->WinReward;
	case EEndGameState::Draw:
		return ProgressionRowData->DrawReward;
	case EEndGameState::Lose:
		return ProgressionRowData->LossReward;
	default:
		return 0;
	}
}

FName UProgressionSystemComponent::GetProgressionRow(ELevelType Map, FPlayerTag Character)
{
	FName CurrentProgressionRowName;
	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
	for (auto RowName : RowNames)
	{
		FProgressionRowData* Row = ProgressionDataTableInternal->FindRow<FProgressionRowData>(
			RowName, "Finding progression row name");
		if (Row)
		{
			if (Row->Map == Map && Row->Character == Character)
			{
				CurrentProgressionRowName = RowName;
			}
		}
	}
	return CurrentProgressionRowName;
}

FProgressionRowData UProgressionSystemComponent::GetProgressionRowData(ELevelType Map, FPlayerTag Character)
{
	FName CurrentProgressionRowName = GetProgressionRow(Map, Character);
	FProgressionRowData* ProgressionRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(
		CurrentProgressionRowName, "Finding a needed row");
	return *ProgressionRowData;
}

// to improve: make it to work per map.
int UProgressionSystemComponent::GetPointsToUnlockNextLevel(ELevelType Map, FPlayerTag Character)
{
	FName SelectedCharacterRowName = GetProgressionRow(Map, Character);
	int PointToUnlockNextLevel = 0;
	
	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
		 
	for (int i = 0; i < RowNames.Num(); i++)
	{
		int nextElementIndex = i; 
		/* finding selected character row name */ 
		if (SelectedCharacterRowName == RowNames[i])
		{
			if (++nextElementIndex < RowNames.Num())
			{
				PointToUnlockNextLevel = ProgressionDataTableInternal->FindRow<FProgressionRowData>(RowNames[nextElementIndex], "Finding a needed row")->PointsToUnlock;
			} else
				PointToUnlockNextLevel = ProgressionDataTableInternal->FindRow<FProgressionRowData>(RowNames.Last(), "Finding a needed row")->PointsToUnlock;
			break;
		}
	}
	return PointToUnlockNextLevel;
}

void UProgressionSystemComponent::OnGameStateChanged(ECurrentGameState CurrentGameState)
{
	// Hide Save widget in when game not End Game
	ProgressionSaveWidgetInternal->SetVisibility(CurrentGameState == ECurrentGameState::EndGame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	
	// Show Progression Menu widget in Main Menu
	ProgressionMenuWidgetInternal->SetVisibility(CurrentGameState == ECurrentGameState::Menu ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (CurrentGameState == ECurrentGameState::Menu)
	{
		UpdateProgressionWidgetForPlayer();
	}
}

void UProgressionSystemComponent::OnEndGameStateChanged(EEndGameState EndGameState)
{
	// Hide this widget in when game not End Game
	//ProgressionSaveWidgetInternal->SetVisibility(EndGameState != EEndGameState::None ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	FString myString = FString::Printf(TEXT("%d"), GetCurrenTotalScore());
	ProgressionMenuWidgetInternal->SetProgressionState(FText::FromString(myString));
	if (EndGameState != EEndGameState::None)
	{
		SavePoints(ELevelType::First, CurrentPlayerTagInternal, EndGameState);
		FString progressionReward = FString::Printf(TEXT("+%d"), GetProgressionReward(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal, EndGameState));
		FString totalScore = FString::Printf(TEXT("%d"),GetCurrenTotalScore());
		ProgressionSaveWidgetInternal->ConfigureWidgetText(UUIDataAsset::Get().GetEndGameText(EndGameState), FText::FromString(progressionReward), FText::FromString(totalScore));
	}
	UpdateProgressionWidgetForPlayer();
}

void UProgressionSystemComponent::HandleGameState(AMyGameStateBase* MyGameState)
{
	checkf(MyGameState, TEXT("ERROR: 'MyGameState' is null!"));

	// Listen states to handle this widget behavior
	MyGameState->OnGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnGameStateChanged);

	if (MyGameState->GetCurrentGameState() == ECurrentGameState::Menu)
	{
		// Handle current game state initialized with delay
		OnGameStateChanged(ECurrentGameState::Menu);
	}
	else
	{
		// Enter the game in Menu game state
		MyGameState->ServerSetGameState(ECurrentGameState::Menu);
	}
}

void UProgressionSystemComponent::HandleEndGameState(AMyPlayerState* MyPlayerState)
{
	checkf(MyPlayerState, TEXT("ERROR: 'MyGameState' is null!")); 
	MyPlayerState->OnEndGameStateChanged.AddUniqueDynamic(this, &ThisClass::OnEndGameStateChanged);
}

void UProgressionSystemComponent::OnPlayerTypeChanged(FPlayerTag PlayerTag)
{
	UE_LOG(LogTemp,Warning, TEXT("OnPlayerTypeChanged"));
	CurrentPlayerTagInternal = PlayerTag;
	UpdateProgressionWidgetForPlayer();
}

void UProgressionSystemComponent::UpdateProgressionWidgetForPlayer()
{
	ProgressionMenuWidgetInternal->ClearImagesFromHorizontalBox();
	
	FString myString = FString::Printf(TEXT("%d"), GetCurrenTotalScore()); //debug
	ProgressionMenuWidgetInternal->SetProgressionState(FText::FromString(myString)); // debug
	
	int PointsToUnlockNextLevel = GetPointsToUnlockNextLevel(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal);
	if (PointsToUnlockNextLevel >= GetCurrenTotalScore())
	{
		//AmountLockedLevels
		int AmountLockedLevels = PointsToUnlockNextLevel - GetCurrenTotalScore();
		int PointsRequiredForCurrentLevel = GetProgressionRowData(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal).PointsToUnlock;
		int PointsDifferenceBetweenLevels = PointsToUnlockNextLevel - PointsRequiredForCurrentLevel;
		
		if (AmountLockedLevels > PointsDifferenceBetweenLevels)
		{
			AmountLockedLevels = PointsDifferenceBetweenLevels;
		} 
			int AmountOfUnlockedLevels = PointsToUnlockNextLevel - PointsRequiredForCurrentLevel - AmountLockedLevels;
		if (AmountOfUnlockedLevels < 0 )
		{
			AmountOfUnlockedLevels = 0;
		} 
		UE_LOG(LogTemp, Warning, TEXT("AmountOfUnlockedLevels: %d"), AmountOfUnlockedLevels); // debug
		UE_LOG(LogTemp, Warning, TEXT("AmountLockedLevels: %d"), AmountLockedLevels); // debug
		ProgressionMenuWidgetInternal->AddImagesToHorizontalBox(AmountOfUnlockedLevels, AmountLockedLevels);
	} else if (PointsToUnlockNextLevel < GetCurrenTotalScore())
	{
		ProgressionMenuWidgetInternal->AddImagesToHorizontalBox(PointsToUnlockNextLevel, 0);
	}	
}
