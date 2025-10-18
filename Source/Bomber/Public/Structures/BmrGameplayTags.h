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
		/** Event that activates collecting powerup ability*/
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Powerup_Collected);

		/** Event that attempts to activate the bomb ability*/
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bomb_Placed);

		/** Event that fires on death*/
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_Death);
	} // namespace Event

	namespace GameplayEffect
	{
		/** Is added by UPlayerDataAsset::DamageImmunityGameplayEffectInternal, defines a temporary damage immunity effect for the player, e.g., god mode, join game in progress etc. */
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player_DamageImmunity);
	} // namespace GameplayEffect

	namespace SetByCaller
	{
		/** SetByCaller tag to set bomb duration in seconds, modifies original duration to compensate for network latency on server side. */
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bomb_Duration);
	} // namespace SetByCaller

	namespace GameplayCue
	{
		/** Immediate visual feedback executed locally on all clients when bomb detonates to spawn VFXs and SFXs despite damage is server-authority only. */
		BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Bomb_Explosion);
	} // namespace GameplayCue
} // namespace BmrGameplayTags