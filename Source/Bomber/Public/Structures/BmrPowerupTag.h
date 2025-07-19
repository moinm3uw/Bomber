// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameplayTagContainer.h"
//---
#include "BmrPowerupTag.generated.h"

/**
* The tag that represents specific powerup type.
*/
USTRUCT(BlueprintType, DisplayName = "Powerup Tag", meta = (Categories = "Powerup"))
struct BOMBER_API FBmrPowerupTag : public FGameplayTag
{
	GENERATED_BODY()

	/** The Powerup tag that contains nothing chosen by default. */
	static const FBmrPowerupTag None;

	/** Increases the player's movement speed. */
	static const FBmrPowerupTag Skate;

	/** Increases the number of bombs that can be placed at once. */
	static const FBmrPowerupTag Bomb;

	/** Increases the explosion radius of bombs. */
	static const FBmrPowerupTag Fire;

	/** Returns all powerup tags, useful for iterating, wrapped in a function for deferred allocation on first call. */
	static const FGameplayTagContainer& GetAll();

	/** Default constructor. */
	FBmrPowerupTag() = default;

	/** Custom constructor to set all members values. */
	FBmrPowerupTag(const FGameplayTag& Tag);
};