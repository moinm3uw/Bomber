// Copyright (c) Yevhenii Selivanov

#include "DataAssets/UIDataAsset.h"
//---
#include "Bomber.h"
#include "DataAssets/DataAssetsContainer.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(UIDataAsset)

// All UI widget tags registered in Widgets Subsystem, used to obtain widget data or widget instance
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_HUD, "UI.Widget.HUD");
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_SETTINGS, "UI.Widget.Settings");
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_NICKNAME, "UI.Widget.Nickname");
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_FPSCOUNTER, "UI.Widget.FPSCounter");
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_MULTIPLAYER, "UI.Widget.Multiplayer");
UE_DEFINE_GAMEPLAY_TAG(TAG_UI_WIDGET_POWERUPS, "UI.Widget.Powerups");

// Returns the UI data asset
const UUIDataAsset& UUIDataAsset::Get()
{
	const UUIDataAsset* UIDataAsset = UDataAssetsContainer::GetUIDataAsset();
	checkf(UIDataAsset, TEXT("The UI Data Asset is not valid"));
	return *UIDataAsset;
}

// Returns widget data associated with the given tag, or invalid widget data if not found
const FManageableWidgetData& UUIDataAsset::GetWidgetDataByTag(FGameplayTag InTag) const
{
	const FManageableWidgetData* FoundWidgetData = AllWidgetData.FindByKey(InTag);
	return FoundWidgetData ? *FoundWidgetData : FManageableWidgetData::Empty;
}

// Returns the localized texts about specified end game to display on UI.
const FText& UUIDataAsset::GetEndGameText(EEndGameState EndGameState) const
{
	if (EndGameState == EEndGameState::None)
	{
		return FText::GetEmpty();
	}

	return EndGameTextsInternal.FindChecked(EndGameState);
}

// Returns the default avatar for the specified player type
UTexture2D* UUIDataAsset::GetDefaultAvatar(EPlayerType PlayerType) const
{
	if (PlayerType == EPlayerType::None)
	{
		return nullptr;
	}

	const TObjectPtr<UTexture2D>* FoundTexturePtr = DefaultAvatarsInternal.Find(PlayerType);
	return FoundTexturePtr ? *FoundTexturePtr : nullptr;
}