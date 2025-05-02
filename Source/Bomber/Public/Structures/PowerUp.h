// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Misc/EnumRange.h"
//---
#include "PowerUp.generated.h"

// @todo JanSeliv UGi56jhn Use GAS attributes for picked up items instead of enum and struct.

/**
 * Types of items.
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	///< The type was not selected
	None,
	///< Increases speed
	Skate,
	///< increases the amount of bombs
	Bomb,
	///< Increases the range of explosion
	Fire
};

using EIT = EItemType;
#define EIT_FIRST_FLAG TO_FLAG(EIT::Skate)
#define EIT_LAST_FLAG TO_FLAG(EIT::Fire)
ENUM_RANGE_BY_FIRST_AND_LAST(EIT, EIT::Skate, EIT::Fire);

/**
 * Numbers of power-ups that affect the abilities of a player during gameplay.
 */
USTRUCT(BlueprintType, DisplayName = "Power-Ups")
struct BOMBER_API FPowerUp
{
	GENERATED_BODY()

	FPowerUp() = default;

	/** Constructor to set all powerup levels to the specified one. */
	FPowerUp(int32 NewValue);

	/** The number of items, that increases the movement speed of the character */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SkateN = 1;

	/** Maximum number of bombs that can be put at one time */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 BombN = 1;

	/** Current amount of bombs available.
	 * Decreases with every bomb spawn and increases with every bomb explosion.
	 * Is always less or equal to BombN. */
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly)
	int32 BombNCurrent = 1;

	/** The number of items, that increases the bomb blast radius */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 FireN = 1;

	/** Returns the level of the specified powerup. */
	int32 GetLevel(EItemType ItemType) const;

	/** Sets the level of the specified powerup. */
	bool SetLevel(int32 NewLevel, EItemType ItemType);

	/** Compares powerup levels. */
	bool operator==(const FPowerUp& Other) const;
};