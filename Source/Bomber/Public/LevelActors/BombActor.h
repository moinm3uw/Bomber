// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "GameFramework/Actor.h"

// Bomber
#include "Structures/Cell.h"

#include "BombActor.generated.h"

/**
 * Is part of the UBmrBombPlaceAbility and is responsible mainly for registration and visuals:
 * - Contains the Map Component to register own location on the Generated Map, so it can be seen by other systems (like AI, other bombs, etc.).
 * - Applies mesh and material based on the instigator (player who placed the bomb).
 * - Applies collisions to block other players.
 *
 * It does NOT handle detonation timer, damage application, explosion VFXs/SFXs, etc. - all that is handled by ability, gameplay effects and cues.
 *
 * @see Access its data with UBombDataAsset (Content/Bomber/DataAssets/DA_Bomb).
 */
UCLASS()
class BOMBER_API ABombActor final : public AActor
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
	 * @param InInstigator - player which placed the bomb, can be accessed as GetInstigator(), is used to track the destroy causer, e.g: scoreboard. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void InitBomb(APawn* InInstigator);

	/** Returns cells are going to explode by this bomb. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE TSet<FCell>& GetExplosionCells() const { return ExplosionCellsInternal; }

	/** Returns explosion radius from instigator, or -1 if can not be obtained. */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetFireRadius() const;

	/** Show current explosion cells if the bomb type is allowed to be displayed, is not available in shipping build. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (DevelopmentOnly))
	void TryDisplayExplosionCells();

protected:
	/** Is not replicated, is calculated locally on the server-only from the Fire Radius attribute of the instigator,which placed the bomb.
	 * Is not used for detonation logic, but only for AI perception and debug visuals. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Explosion Cells"))
	TSet<FCell> ExplosionCellsInternal = FCell::EmptyCells;

	/** Calculates the explosion cells based on current fire radius. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++", meta = (BlueprintProtected))
	void UpdateExplosionCells();

	/*********************************************************************************************
	 * Visuals: VFXs, SFXs, Mesh, Materials
	 ********************************************************************************************* */
public:
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

	/** Is used for cleaning up the bomb's data after it was removed from the level. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostRemovedFromLevel(UMapComponent* MapComponent, UObject* DestroyCauser);

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