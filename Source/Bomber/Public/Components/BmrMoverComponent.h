// Copyright (c) Yevhenii Selivanov

#pragma once

#include "MoverComponent.h"
//---
#include "BmrMoverComponent.generated.h"

enum class ECurrentGameState : uint8;

/*
 * Replaces the Character Movement Component (CMR) for next purposes:
 * - Better local movement prediction causing less jittering.
 * - Modular movement modes that can be easily extended or injected.
 * It caches gameplay-related data (such as inputs, states etc) and provides to other systems such as movement modes.
 * All the settings are set in Details Panel of pawn blueprint due to instanced properties.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BOMBER_API UBmrMoverComponent : public UMoverComponent
{
	GENERATED_BODY()

public:
	/** Moves owner in given direction.
	* Is called by both player (from input) and AI.
	* @param Direction - Normalized direction to move in the world space, can be zero to stop moving. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void RequestMoveByIntent(const FVector& Direction);

	/*********************************************************************************************
	 * Data
	 ********************************************************************************************* */
protected:
	/** Cached move input for the current simulation frame.
	 * This is used to accumulate user input or AI state into a single vector that can be processed by the movement system. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Current Move Input"))
	FVector CurrentMoveInputInternal = FVector::ZeroVector;

	/** Cached skate power-up attribute value.
	 * Is used in walking mode to increase the movement speed when player picked up a Skate item. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "C++", meta = (BlueprintProtected, DisplayName = "Cached Skate Powerup Value"))
	float CachedSkatePowerupAttributeInternal = 0.f;

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Called when the game starts. */
	virtual void BeginPlay() override;

	/** Consumes cached inputs to be processed by other systems such as movement modes. */
	virtual void ProduceInput(const int32 DeltaTimeMS, FMoverInputCmdContext* Cmd) override;

	/*********************************************************************************************
	 * Events
	 ********************************************************************************************* */
protected:
	/** Is called when this character is ready to be used. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Listen to react when entered to different game state. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Called when owner is added on the Generated Map, on both server and client. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnOwnerAddedToLevel(class UMapComponent* MapComponent);

	/** Called when owner is destroyed on the Generated Map. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostRemovedFromLevel(class UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Broadcast at the end of a simulation tick after movement has occurred, but allowing additions/modifications to the state. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostMove(const FMoverTimeStep& TimeStep, FMoverSyncState& SyncState, FMoverAuxStateContext& AuxState);

	/** Is called by Move Input Action when player pressed the move input button, e.g: WASD or Arrow keys.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnMoveInputTriggered(const struct FInputActionValue& ActionValue);

	/** Is called by Move Input Action when player released the move input button, e.g: WASD or Arrow keys. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnMoveInputCompleted(const struct FInputActionValue& ActionValue);

	/** Is called when the Skate attribute is changed, e.g: when player picked up a Skate item. */
	void OnSkateAttributeChanged(const struct FOnAttributeChangeData& OnAttributeChangeData);
};