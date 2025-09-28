// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "GameFramework/Actor.h"

// Bomber
#include "Structures/Cell.h"

// UE
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"

#include "BombActor.generated.h"

enum class ELevelType : uint8;

/**
 * Bombs are put by the character to destroy the level actors, trigger other bombs.
 * @see Access its data with UBombDataAsset (Content/Bomber/DataAssets/DA_Bomb).
 */
UCLASS()
class BOMBER_API ABombActor final : public AActor,
                                    public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	/** Sets default values for this actor's properties */
	ABombActor();

protected:
	/** The MapComponent manages this actor on the Generated Map */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Map Component"))
	TObjectPtr<class UMapComponent> MapComponentInternal = nullptr;

	/*********************************************************************************************
	 * Detonation
	 ********************************************************************************************* */
public:
	/** Initiates the explosion: starts countdown and initializes the data (fire radius, explosion cells, etc.).
	 * Can be called on both server and clients.
	 * @param OptionalInstigator - player which placed the bomb, can be accessed as GetInstigator(), is used to track the destroy causer, e.g: scoreboard. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitBomb(APawn* OptionalInstigator = nullptr);

	/** If set, the bomb will detonate when this effect is removed.
	 * Otherwise, the bomb must be manually detonated by destroying the level actor on generated map. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetActiveDurationEffectHandle(const struct FActiveGameplayEffectHandle& InHandle);

	/** Clears the active duration effect handle as part of cleanup process. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void ClearActiveDurationEffectHandle();

	/** Returns cells are going to explode by this bomb. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE TSet<FCell>& GetExplosionCells() const { return LocalExplosionCellsInternal; }

	/** Returns explosion radius from instigator, or -1 if can not be obtained. */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetFireRadius() const;

	/** Show current explosion cells if the bomb type is allowed to be displayed, is not available in shipping build. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (DevelopmentOnly))
	void TryDisplayExplosionCells();

protected:
	/** Is not replicated, is calculated locally on the server and clients from the FireRadiusInternal. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Explosion Cells"))
	TSet<FCell> LocalExplosionCellsInternal = FCell::EmptyCells;

	/** Is applied at bomb ability activation, detonates the bomb when removed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Applied Duration Effect"))
	FActiveGameplayEffectHandle AppliedDurationEffectInternal;

	/** Is server-only, immediately detonates the bomb. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++", meta = (BlueprintProtected))
	void DetonateBomb();

	/** Calculates the explosion cells based on current fire radius. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void UpdateExplosionCells();

	/*********************************************************************************************
	 * Visuals: VFXs, SFXs, Mesh, Materials
	 ********************************************************************************************* */
public:
	/** Spawns VFXs and SFXs, is allowed to call both on server and clients.
	 * Immediate visual feedback executed locally when bomb detonates while damage itself is server-authority only. */
	UFUNCTION(Blueprintable, Category = "C++")
	void PlayExplosionsCue();

	/** Updates current mesh for this bomb actor, based on instigator type, or randomly if no instigator. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ApplyMesh();

	/** Updates current material for this bomb actor, based on this bomb and Player placer types. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void ApplyMaterial();

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Called when an instance of this class is placed (in editor) or spawned */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Returns the Ability System Component from the Instigator or local player if none.
	 * In blueprints, call 'Get Ability System Component' as interface function. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAbilitySystemComponent& GetAbilitySystemComponentChecked() const;

	/** Is overridden to init bomb on clients when instigator is replicated. */
	virtual void OnRep_Instigator() override;

	/*********************************************************************************************
	 * Events
	 ********************************************************************************************* */
protected:
	/** Called when this level actor is reconstructed or added on the Generated Map.
	 * Is used by Level Actors instead of the BeginPlay(). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnAddedToLevel(UMapComponent* MapComponent);

	/** Is called to listen when this bomb is destroyed on the Generated Map by itself or by other actors. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Is called when character leaves the bomb to update collision response. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPlayerCellChanged(UMapComponent* PlayerMapComponent, const FCell& NewCell, const FCell& PreviousCell);

	/*********************************************************************************************
	 * Custom Collision Response
	 ********************************************************************************************* */
public:
	/** Sets actual collision response to all players for this bomb. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitCollisionResponseToAllPlayers();

	/** Takes your container and returns is with new specified response for player by its specified ID.
	 * @param InOutCollisionResponses Will contain requested response.
	 * @param CharacterID Player to set response.
	 * @param NewResponse New response to set. */
	UFUNCTION(BlueprintPure, Category = "C++")
	static void GetCollisionResponseToPlayerByID(FCollisionResponseContainer& InOutCollisionResponses, int32 CharacterID, ECollisionResponse NewResponse);
};