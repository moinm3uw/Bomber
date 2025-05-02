// Copyright (c) Yevhenii Selivanov

#include "Structures/PowerUp.h"
//---
#include "DataAssets/ItemDataAsset.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PowerUp)

// Constructor to set all powerup levels to the specified one
FPowerUp::FPowerUp(int32 NewValue)
{
	SetLevel(NewValue, EItemType::Skate);
	SetLevel(NewValue, EItemType::Bomb);
	SetLevel(NewValue, EItemType::Fire);
}

// Returns the level of the specified powerup
int32 FPowerUp::GetLevel(EItemType ItemType) const
{
	switch (ItemType)
	{
		case EItemType::Skate:
			return SkateN;
		case EItemType::Bomb:
			return BombN;
		case EItemType::Fire:
			return FireN;
		default:
			break;
	}

	return INDEX_NONE;
}

// Sets the level of the specified powerup
bool FPowerUp::SetLevel(int32 NewLevel, EItemType ItemType)
{
	auto SetClamped = [](int32& InAttributeRef, int32 NewNum, int32 ClampMax = INDEX_NONE)
	{
		const int32 MaxAllowedItemsNum = UItemDataAsset::Get().GetMaxAllowedItemsNum();
		const int32 NewNumClamped = FMath::Clamp(NewNum, 0, ClampMax == INDEX_NONE ? MaxAllowedItemsNum : ClampMax);
		if (InAttributeRef != NewNumClamped)
		{
			InAttributeRef = NewNumClamped;
			return true;
		}
		return false;
	};

	switch (ItemType)
	{
		case EItemType::Skate:
		{
			return SetClamped(SkateN, NewLevel);
		}
		case EItemType::Bomb:
		{
			const bool bIsSet = SetClamped(BombN, NewLevel);
			return bIsSet | SetClamped(BombNCurrent, BombN);
		}
		case EItemType::Fire:
		{
			return SetClamped(FireN, NewLevel);
		}
		default:
			break;
	}

	return false;
}

// Compares powerup levels
bool FPowerUp::operator==(const FPowerUp& Other) const
{
	return SkateN == Other.SkateN
	       && BombN == Other.BombN
	       && BombNCurrent == Other.BombNCurrent
	       && FireN == Other.FireN;
}