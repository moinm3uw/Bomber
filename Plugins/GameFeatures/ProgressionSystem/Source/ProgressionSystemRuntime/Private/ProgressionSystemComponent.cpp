// Copyright (c) Yevhenii Selivanov

#include "ProgressionSystemComponent.h"
//---
#include "Bomber.h"
#include "Controllers/MyPlayerController.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
#include "ProgressionSystemDataAsset.h"
#include "ProgressionSystemRuntimeModule.h"
#include "Blueprint/WidgetTree.h"
#include "UI/MyHUD.h"
#include "Widgets/ProgressionMenuWidget.h"
#include "Widgets/ProgressionSaveWidget.h"
#include "ModuleStructures.h"
#include "Components/Image.h"
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

	CurrentPlayCharacterInternal = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();
	ProgressionSystemDataAssetInternal->SetProgressionSystemComponent(this);
}

// Save the progression depends on EEndGameState
void UProgressionSystemComponent::SavePoints(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState)
{
	SavedProgressionRowDataInternal.CurrentLevelProgression += GetProgressionReward(Map, Character, EndGameState);
	UpdateProgressionRowName();
}

// Checks and updates the progression row name if points to unlock for the next progression reached. 
void UProgressionSystemComponent::UpdateProgressionRowName()
{
	FName CurrentProgressionRowName = GetProgressionRowName(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal);
	FProgressionRowData* ProgressionRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(CurrentProgressionRowName, "Finding a needed row");
	ProgressionRowData->CurrentLevelProgression = SavedProgressionRowDataInternal.CurrentLevelProgression;

	if (ProgressionRowData->CurrentLevelProgression >= ProgressionRowData->PointsToUnlock)
	{
		NextLevelProgressionRowData();
	}
}

int32 UProgressionSystemComponent::GetProgressionReward(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState)
{
	FName CurrentProgressionRowName = GetProgressionRowName(Map, Character);
	FProgressionRowData* ProgressionRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(CurrentProgressionRowName, "Finding a needed row");
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

FProgressionRowData UProgressionSystemComponent::GetProgressionRowData(ELevelType Map, FPlayerTag Character)
{
	FName CurrentProgressionRowName = GetProgressionRowName(Map, Character);
	FProgressionRowData* ProgressionRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(CurrentProgressionRowName, "Finding a needed row");
	return *ProgressionRowData;
}

FName UProgressionSystemComponent::GetProgressionRowName(ELevelType Map, FPlayerTag Character)
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

FProgressionRowData UProgressionSystemComponent::GetCurrentLevelProgressionRowData()
{
	FProgressionRowData ProgressionRowData = GetProgressionRowData(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal);
	return ProgressionRowData;
}

void UProgressionSystemComponent::UnlockNextLevel()
{
	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
	
	int32 CurrentRowIndex = RowNames.Find(SavedProgressionNameInternal);
	
	if (CurrentRowIndex != INDEX_NONE && CurrentRowIndex > 0)
	{
		FName PreviousRowName = RowNames[CurrentRowIndex -1];
		FProgressionRowData* PreviousRow = ProgressionDataTableInternal->FindRow<FProgressionRowData>(PreviousRowName, TEXT("Getting previouse level progression row data at Progressino System Component"));
		if (PreviousRow)
		{
			if (PreviousRow->CurrentLevelProgression >= PreviousRow->PointsToUnlock)
			{
				DisplayLevelUIOverlay(false);
			}  else
			{
				DisplayLevelUIOverlay(true);
			}
		}
	}
}

void UProgressionSystemComponent::NextLevelProgressionRowData()
{
	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
	int32 CurrentRowIndex = RowNames.Find(GetProgressionRowName(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal));
	if (CurrentRowIndex != INDEX_NONE && CurrentRowIndex + 1 < RowNames.Num())
	{
		FName NextRowName = RowNames[CurrentRowIndex + 1];
		FProgressionRowData* NextRow = ProgressionDataTableInternal->FindRow<FProgressionRowData>(NextRowName, TEXT("NextLevelProgressionRowData Context"));
			NextRow->IsLevelLocked = false;	
	}
}

FPlayerTag UProgressionSystemComponent::GetFirstPlayerTag()
{
	TArray<FName> RowNames = ProgressionDataTableInternal->GetRowNames();
	FPlayerTag FirstPlayerTagInTable = FPlayerTag::None;

	if (RowNames.Num() > 0 )
	{
		FName FirstRowName = RowNames[0];
		FProgressionRowData* FirstRowData = ProgressionDataTableInternal->FindRow<FProgressionRowData>(FirstRowName, "Getting first row from data table at Progression System Component");
		if (FirstRowData)
		{
			FirstPlayerTagInTable = FirstRowData->Character;
		}
	}
	return  FirstPlayerTagInTable;
}

void UProgressionSystemComponent::OnGameStateChanged(ECurrentGameState CurrentGameState)
{
	// Show Progression Menu widget in Main Menu
	ProgressionMenuWidgetInternal->SetVisibility(CurrentGameState == ECurrentGameState::Menu ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (CurrentGameState == ECurrentGameState::Menu)
	{
		UpdateProgressionWidgetForPlayer();
	}
}

void UProgressionSystemComponent::OnEndGameStateChanged(EEndGameState EndGameState)
{
	if (EndGameState != EEndGameState::None)
	{
		SavePoints(UMyBlueprintFunctionLibrary::GetLevelType(), CurrentPlayerTagInternal, EndGameState);
	}
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
	CurrentPlayerTagInternal = PlayerTag;
	CurrentPlayCharacterInternal = UMyBlueprintFunctionLibrary::GetLocalPlayerCharacter();
	UpdateProgressionWidgetForPlayer();
}

void UProgressionSystemComponent::UpdateProgressionWidgetForPlayer()
{
	ProgressionMenuWidgetInternal->ClearImagesFromHorizontalBox();
	
	SavedProgressionRowDataInternal = GetCurrentLevelProgressionRowData();
	if (SavedProgressionRowDataInternal.CurrentLevelProgression >= SavedProgressionRowDataInternal.PointsToUnlock)
	{
		ProgressionMenuWidgetInternal->AddImagesToHorizontalBox(SavedProgressionRowDataInternal.PointsToUnlock, 0);
	} else
	{
		ProgressionMenuWidgetInternal->AddImagesToHorizontalBox(SavedProgressionRowDataInternal.CurrentLevelProgression, SavedProgressionRowDataInternal.PointsToUnlock - SavedProgressionRowDataInternal.CurrentLevelProgression);
	}
	
	DisplayLevelUIOverlay(SavedProgressionRowDataInternal.IsLevelLocked);
}

void UProgressionSystemComponent::DisplayLevelUIOverlay(bool IsLevelLocked)
{
	if(IsLevelLocked)
	{
		ProgressionMenuWidgetInternal->PSCBackgroundOverlay->SetVisibility(ESlateVisibility::Visible);
		ProgressionMenuWidgetInternal->PSCBackgroundIconLock->SetVisibility(ESlateVisibility::Visible);
	} else
	{
		ProgressionMenuWidgetInternal->PSCBackgroundOverlay->SetVisibility(ESlateVisibility::Collapsed);
		ProgressionMenuWidgetInternal->PSCBackgroundIconLock->SetVisibility(ESlateVisibility::Collapsed);
	}
}