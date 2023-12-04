// Copyright (c) Yevhenii Selivanov

#pragma once

#include "ModuleStructures.h"
#include "ProgressionSystemDataAsset.h"
#include "Components/ActorComponent.h"
#include "Structures/PlayerTag.h"
//---
#include "ProgressionSystemComponent.generated.h"

/**
 * Implements the core logic on project about Progression System.
 */

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROGRESSIONSYSTEMRUNTIME_API UProgressionSystemComponent final : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Sets default values for this component's properties. */
	UProgressionSystemComponent();

	/** Returns the Progression System data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	static const UProgressionSystemDataAsset* GetProgressionSystemDataAsset() {return &UProgressionSystemDataAsset::Get(); }

	/** Returns current saved progression. */
	UFUNCTION(BlueprintCallable, Category="C++")
	FORCEINLINE FProgressionRowData GetSavedProgressionRowData() const { return SavedProgressionRowDataInternal; }

	/** Returns current saved progression anme . */
	UFUNCTION(BlueprintCallable, Category="C++")
	FORCEINLINE FName GetSavedProgressionName() const { return  SavedProgressionNameInternal; }

	/** The current Saved Progression of a player. */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Saved Progression", ShowOnlyInnerProperties))
	FProgressionRowData SavedProgressionRowDataInternal;
	
	/** Returns the endgame reward. */
	UFUNCTION(BlueprintCallable, Category="C++")
	FORCEINLINE int32 GetCurrenTotalScore() const { return SavedProgressionRowDataInternal.CurrentLevelProgression; }

	/** Save the progression depends on EEndGameState. */
	UFUNCTION(BlueprintCallable, Category="C++")
	void SavePoints(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState);

	/** Checks and updates the progression row name if points to unlock for the next progression reached. */
	UFUNCTION(BlueprintCallable, Category="C++")
	void UpdateProgressionRowName();

	/** Returns the endgame reward. */
	UFUNCTION(BlueprintCallable, Category="C++")
	int32 GetProgressionReward(ELevelType Map, FPlayerTag Character, EEndGameState EndGameState);
	
protected:
	
	/** Called when the game starts. */
	virtual void BeginPlay() override;

	/** Returns a current progression row name */
	UFUNCTION(BlueprintPure, Category="C++")
	FName GetProgressionRowName(ELevelType Map, FPlayerTag Character);

	/** Returns a current progression row name */
	UFUNCTION(BlueprintPure, Category="C++")
	FProgressionRowData GetProgressionRowData(ELevelType Map, FPlayerTag Character);

	/** Returns the current level progression row data structure. */
	UFUNCTION(BlueprintCallable, Category="C++")
	FProgressionRowData GetCurrentLevelProgressionRowData();

	/** Returns the next level from current progression row data structure. Returns last item if there is no next level. */
	UFUNCTION(BlueprintCallable, Category="C++")
	void UnlockNextLevel();

	UFUNCTION(BlueprintCallable, Category="C++")
	void NextLevelProgressionRowData();

	/** Returns the first player tag from the data table list **/
	UFUNCTION(Blueprintable, Category="C++")
	FPlayerTag GetFirstPlayerTag();

	/** Current Player Character */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Current PlayerCharacter"))
	TObjectPtr<ACharacter> CurrentPlayCharacterInternal = nullptr; 

	/** Progression System data asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression System Data Asset"))
	TObjectPtr<UProgressionSystemDataAsset> ProgressionSystemDataAssetInternal;

	/** Created Main Menu widget. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Main Menu Widget"))
	TObjectPtr<class UProgressionMenuWidget> ProgressionMenuWidgetInternal = nullptr;

	/** Created Save points widget. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Main Menu Widget"))
	TObjectPtr<class UProgressionSaveWidget> ProgressionSaveWidgetInternal = nullptr;

	UPROPERTY(EditAnywhere, meta = (DisplayName = "Saved Progression", ShowOnlyInnerProperties))
	FName SavedProgressionNameInternal;

	/** The current selected player */
	UPROPERTY(EditDefaultsOnly, meta = (DisplayName = "Currnet Player Tag", ShowOnlyInnerProperties))
	FPlayerTag CurrentPlayerTagInternal;

	/** The Progression Data Table that is responsible for progression configuration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression Data Table", ShowOnlyInnerProperties))
	TObjectPtr<UDataTable> ProgressionDataTableInternal = nullptr;
	
	/** Called when the current game state was changed. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Called when the end game state was changed. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnEndGameStateChanged(EEndGameState EndGameState);
	
	/** Is called to prepare the widget for Menu game state. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void HandleGameState(class AMyGameStateBase* MyGameState);

	/** Is called to prepare the widget for handling end game state. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void HandleEndGameState(class AMyPlayerState* MyPlayerState);

	/** Is called when a player has been changed */
	UFUNCTION(BlueprintCallable, Category= "C++", meta = (BlueprintProtected))
	void OnPlayerTypeChanged(FPlayerTag PlayerTag);

	/** Updates the progression menu widget when player changed */
	UFUNCTION(BlueprintCallable, Category= "C++", meta = (BlueprintProtected))
	void UpdateProgressionWidgetForPlayer();

	/** Show locked level ui overlay */
	UFUNCTION(BlueprintCallable, Category= "C++", meta = (BlueprintProtected))
	void DisplayLevelUIOverlay(bool IsLevelLocked);
};
