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
	} // namespace Event

} // namespace BmrGameplayTags