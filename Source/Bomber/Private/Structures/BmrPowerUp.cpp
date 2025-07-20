// Copyright (c) Yevhenii Selivanov

#include "Structures/BmrPowerUp.h"
//---
#include "DataAssets/ItemDataAsset.h"
#include "LevelActors/PlayerCharacter.h"
//---
#include "Net/Core/PushModel/PushModel.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerUp)

// Constructor to set powerup levels to the specified one
FBmrPowerUp::FBmrPowerUp(FBmrPowerupTag InItemType, int32 NewLevel)
{
	ItemType = InItemType;
	SetLevel(NewLevel);
}

// Assigns and clamps the powerup level in valid range
void FBmrPowerUp::SetLevel(int32 NewLevel)
{
	SetMaxLevel(NewLevel);
	SetCurrentLevel(NewLevel);
}

void FBmrPowerUp::SetMaxLevel(int32 NewMaxLevel)
{
	constexpr int32 ClampMin = 0;
	constexpr int32 MaxAllowedItemsNum = 5;
	MaxLevel = FMath::Clamp(NewMaxLevel, ClampMin, MaxAllowedItemsNum);
}

void FBmrPowerUp::SetCurrentLevel(int32 NewCurrentLevel)
{
	constexpr int32 ClampMin = 0;
	CurrentLevel = FMath::Clamp(NewCurrentLevel, ClampMin, MaxLevel);
}

// Adds the specified number of items to the current and max levels
void FBmrPowerUp::AddLevel(int32 AdditionalLevel)
{
	SetMaxLevel(MaxLevel + AdditionalLevel);
	SetCurrentLevel(CurrentLevel + AdditionalLevel);
}

// Converts given item level to its equivalent percent value
float FBmrPowerUp::Conv_PowerUpLevelToPercent(int32 ItemLevel)
{
	constexpr float MinPowerUps = 1.f;
	constexpr float MaxPowerUps = FMath::Max(5.f, MinPowerUps);

	const int32 CurrentItemLevel = static_cast<float>(ItemLevel);
	return CurrentItemLevel / MaxPowerUps;
}

// Compares powerup max and current levels
bool FBmrPowerUp::operator==(const FBmrPowerUp& Other) const
{
	return ItemType == Other.ItemType
	       && MaxLevel == Other.MaxLevel
	       && CurrentLevel == Other.CurrentLevel;
}

// Constructor to set all powerup levels to the specified one
FBmrPowerUpsContainer::FBmrPowerUpsContainer(int32 NewLevel, class APlayerCharacter& InOwner)
{
	for (FBmrPowerupTag ItemIt : FBmrPowerupTag::GetAll())
	{
		PowerUps.Add({ItemIt, NewLevel});
	}

	SetOwner(InOwner);
}

// Returns the powerup by its type
const FBmrPowerUp& FBmrPowerUpsContainer::Get(FBmrPowerupTag ItemType) const
{
	const FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
	checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
	return *PowerUpPtr;
}

// Assigns and clamps the powerup level in valid range
void FBmrPowerUpsContainer::SetLevel(int32 NewLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->SetLevel(NewLevel);
		MarkDirty(PrevPowerups);
	}
}

void FBmrPowerUpsContainer::SetMaxLevel(int32 NewMaxLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->SetMaxLevel(NewMaxLevel);
		MarkDirty(PrevPowerups);
	}
}

void FBmrPowerUpsContainer::SetCurrentLevel(int32 NewCurrentLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->SetCurrentLevel(NewCurrentLevel);
		MarkDirty(PrevPowerups);
	}
}

// Adds the specified number of items to the current and max levels
void FBmrPowerUpsContainer::AddLevel(int32 AdditionalLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->AddLevel(AdditionalLevel);
		MarkDirty(PrevPowerups);
	}
}

void FBmrPowerUpsContainer::AddMaxLevel(int32 AdditionalLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->AddMaxLevel(AdditionalLevel);
		MarkDirty(PrevPowerups);
	}
}

void FBmrPowerUpsContainer::AddCurrentLevel(int32 AdditionalLevel, FBmrPowerupTag ItemType)
{
	const APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		const FBmrPowerUpsContainer PrevPowerups = *this;
		FBmrPowerUp* PowerUpPtr = PowerUps.FindByKey(ItemType);
		checkf(PowerUpPtr, TEXT("ERROR: [%i] %hs:\n'PowerupPtr' is null!"), __LINE__, __FUNCTION__);
		PowerUpPtr->AddCurrentLevel(AdditionalLevel);
		MarkDirty(PrevPowerups);
	}
}

// Marks this container as dirty to push changes for replication, if valid
void FBmrPowerUpsContainer::MarkDirty(const FBmrPowerUpsContainer& PrevPowerups)
{
	APlayerCharacter* InPlayerCharacter = OwnerInternal.Get();
	if (InPlayerCharacter && InPlayerCharacter->HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(APlayerCharacter, PowerupsInternal, InPlayerCharacter);
	}
}

// Assigns owner to this container for replication
void FBmrPowerUpsContainer::SetOwner(class APlayerCharacter& InOwner)
{
	if (InOwner.HasAuthority())
	{
		// Set it only on the server as it's used internally only for replication purposes
		OwnerInternal = &InOwner;
	}
}