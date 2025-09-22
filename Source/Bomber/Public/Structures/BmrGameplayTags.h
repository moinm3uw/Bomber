// Copyright (c) Yevhenii Selivanov.

#pragma once

// UE
#include "NativeGameplayTags.h" // UE_DECLARE_GAMEPLAY_TAG_EXTERN

namespace BmrGameplayTags
{
	namespace UI
	{
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Widget_Settings);
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Widget_Nickname);
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Widget_FpsCounter);
	} // namespace UI

	namespace Event
	{
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Powerup_Collected);
	} // namespace Event
} // namespace BmrGameplayTags