// Copyright (c) Yevhenii Selivanov.

#include "Structures/BmrGameplayTags.h"

namespace BmrGameplayTags
{
	namespace UI
	{
		UE_DEFINE_GAMEPLAY_TAG(Widget_Settings, "UI.Widget.Settings");
		UE_DEFINE_GAMEPLAY_TAG(Widget_Nickname, "UI.Widget.Nickname");
		UE_DEFINE_GAMEPLAY_TAG(Widget_FpsCounter, "UI.Widget.FPSCounter");
	} // namespace UI

	namespace Event
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Powerup_Collected, "Event.Powerup.Collected", "Event that activates collecting powerup ability.");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bomb_Placed, "Event.Bomb.Placed", "Event that attempts to activate the bomb ability");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Player_Death, "Event.Player.Death", "Event that fires on death");
	} // namespace Event

	namespace GameplayEffect
	{
		namespace Block
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(IncomingDamage, "GameplayEffect.Block.IncomingDamage", "Is added by UPlayerDataAsset::BlockIncomingDamageEffectInternal, defines a temporary immunity effect for incoming damage, e.g., god mode, join game in progress etc");
		} // namespace Block
	} // namespace GameplayEffect

	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bomb_Duration, "SetByCaller.Bomb.Duration", "SetByCaller tag to set bomb duration in seconds, modifies original duration to compensate for network latency on server side");
	} // namespace SetByCaller

	namespace GameplayCue
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bomb_Explosion, "GameplayCue.Bomb.Explosion", "Immediate visual feedback executed locally on all clients when bomb detonates to spawn VFXs and SFXs despite damage is server-authority only");
	} // namespace GameplayCue
} // namespace BmrGameplayTags