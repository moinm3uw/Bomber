// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Misc/EnumRange.h"
//---
#include "BmrPowerUp.generated.h"

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
struct BOMBER_API FBmrPowerUp
{
	GENERATED_BODY()

	/** Default constructor that initializes all powerup levels to 1. */
	FBmrPowerUp() = default;

	/** Constructor to set powerup levels to the specified one. */
	FBmrPowerUp(EItemType InItemType, int32 NewLevel);

	/** Returns the level of the powerup, e.g: 1, 2, 3 */
	FORCEINLINE int32 GetMaxLevel() const { return MaxLevel; }
	FORCEINLINE int32 GetCurrentLevel() const { return CurrentLevel; }

	/** Returns the percent of the level, e.g: 0.4f for 50% (level 2 of 5) */
	FORCEINLINE float GetMaxLevelPercent() const { return Conv_PowerUpLevelToPercent(MaxLevel); }
	FORCEINLINE float GetCurrentLevelPercent() const { return Conv_PowerUpLevelToPercent(CurrentLevel); }

	/** Assigns and clamps the powerup level in valid range. */
	void SetLevel(int32 NewLevel);
	void SetMaxLevel(int32 NewMaxLevel);
	void SetCurrentLevel(int32 NewCurrentLevel);

	/** Adds the specified number of items to the current and max levels. */
	void AddLevel(int32 AdditionalLevel);
	void AddMaxLevel(int32 AdditionalLevel) { SetMaxLevel(MaxLevel + AdditionalLevel); }
	void AddCurrentLevel(int32 AdditionalLevel) { SetCurrentLevel(CurrentLevel + AdditionalLevel); }

	/** Returns true is the powerup's current or max level is zero. */
	FORCEINLINE bool IsZero() const { return CurrentLevel <= 0 || MaxLevel <= 0; }

	/** Converts given item level to its equivalent percent value. */
	static float Conv_PowerUpLevelToPercent(int32 ItemLevel);

	/** Compares powerup max and current levels. */
	bool operator==(const FBmrPowerUp& Other) const;
	friend BOMBER_API bool operator==(const FBmrPowerUp& A, EItemType B) { return A.ItemType == B; }
	FORCEINLINE operator int32() const { return CurrentLevel; }

private:
	/** The item type associated with this powerup */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	EItemType ItemType = EItemType::None;

	/** The number of items, that increases the attribute of the character */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 MaxLevel = 1;

	/** The number of items, that player currently has */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	int32 CurrentLevel = 1;
};

/**
 * Contains all powerups that player currently has.
 */
USTRUCT(BlueprintType, DisplayName = "Power-Ups")
struct BOMBER_API FBmrPowerUpsContainer
{
	GENERATED_BODY()

	/** Default constructor that initializes all powerup levels to 1. */
	FBmrPowerUpsContainer() = default;

	/** Constructor to set all powerup levels to the specified one. */
	FBmrPowerUpsContainer(int32 NewLevel, class APlayerCharacter& InOwner);

	/** Returns the powerup by its type */
	const FBmrPowerUp& Get(EItemType ItemType) const;

	/** Assigns and clamps the powerup level in valid range. */
	void SetLevel(int32 NewLevel, EItemType ItemType);
	void SetMaxLevel(int32 NewMaxLevel, EItemType ItemType);
	void SetCurrentLevel(int32 NewCurrentLevel, EItemType ItemType);

	/** Adds the specified number of items to the current and max levels. */
	void AddLevel(int32 AdditionalLevel, EItemType ItemType);
	void AddMaxLevel(int32 AdditionalLevel, EItemType ItemType);
	void AddCurrentLevel(int32 AdditionalLevel, EItemType ItemType);

	/** Marks this container as dirty to push changes for replication, if valid. */
	void MarkDirty(const FBmrPowerUpsContainer& PrevPowerups);

	/** Assigns owner to this container for replication. */
	void SetOwner(class APlayerCharacter& InOwner);

private:
	/** The number of items, that increases the movement speed of the character */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (DisplayName = "PowerUps", AllowPrivateAccess = "true"))
	TArray<FBmrPowerUp> PowerUps;

	/** The character associated with this powerups (is not replicated, server-only) */
	TWeakObjectPtr<class APlayerCharacter> OwnerInternal = nullptr;
};