// Copyright (c) Yevhenii Selivanov

#include "Structures/BmrPowerupTag.h"
//---
#include "NativeGameplayTags.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupTag)

// The Powerup tag that contains nothing chosen by default
const FBmrPowerupTag FBmrPowerupTag::None = EmptyTag;

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Powerup_Skate, "Powerup.Skate", "Increases the player's movement speed");
const FBmrPowerupTag FBmrPowerupTag::Skate = TAG_Powerup_Skate.GetTag();

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Powerup_Bomb, "Powerup.Bomb", "Increases the number of bombs that can be placed at once");
const FBmrPowerupTag FBmrPowerupTag::Bomb = TAG_Powerup_Bomb.GetTag();

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Powerup_Fire, "Powerup.Fire", "Increases the explosion radius of bombs");
const FBmrPowerupTag FBmrPowerupTag::Fire = TAG_Powerup_Fire.GetTag();

// Returns all powerup tags, useful for iterating, wrapped in a function for deferred allocation on first call
const FGameplayTagContainer& FBmrPowerupTag::GetAll()
{
	static const FGameplayTagContainer AllTags = FGameplayTagContainer::CreateFromArray(TArray<FGameplayTag>{
		Skate,
		Bomb,
		Fire
	});
	return AllTags;
}

// Custom constructor to set all members values
FBmrPowerupTag::FBmrPowerupTag(const FGameplayTag& Tag)
	: FGameplayTag(Tag) {}