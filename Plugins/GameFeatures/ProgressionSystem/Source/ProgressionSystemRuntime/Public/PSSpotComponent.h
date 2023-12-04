// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "ProgressionSystemDataAsset.h"
#include "Components/ActorComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "PSSpotComponent.generated.h"

/**
 * Represents a spot where a character can be selected in the Main Menu.
 * Is added dynamically to the My Skeletal Mesh actors on the level.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROGRESSIONSYSTEMRUNTIME_API UPSSpotComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPSSpotComponent();

	/** Returns the Progression System data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	static const UProgressionSystemDataAsset* GetProgressionSystemDataAsset() {return &UProgressionSystemDataAsset::Get(); }

	/** Returns the Skeletal Mesh of the Bomber character. */
	UFUNCTION(BlueprintPure, Category = "C++")
	class UMySkeletalMeshComponent* GetMySkeletalMeshComponent() const;
	class UMySkeletalMeshComponent& GetMeshChecked() const;
	 

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

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

		/** Locks the player spot when progression for level achieved */
	UFUNCTION(BlueprintCallable,Category= "C++", meta=(BlueprintProtected))
	void ChangeSpotVisibilityStatus();

	/** Progression System data asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression System Data Asset"))
	TObjectPtr<UProgressionSystemDataAsset> ProgressionSystemDataAssetInternal;
	
	/** A Progression System Component */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression Data Table", ShowOnlyInnerProperties))
	TObjectPtr<UProgressionSystemComponent> MyProgressionSystemComponentInternal = nullptr;

	/** A player skeletal mesh actor */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression Data Table", ShowOnlyInnerProperties))
	TObjectPtr<UMySkeletalMeshComponent> PlayerSpotOnLevelInternal = nullptr;

public:	
	
		
};
