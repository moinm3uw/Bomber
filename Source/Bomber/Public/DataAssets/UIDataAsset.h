// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Data/MyPrimaryDataAsset.h"
//---
#include "Structures/ManageableWidgetData.h"
//---
#include "NativeGameplayTags.h"
//---
#include "UIDataAsset.generated.h"

/** All UI widget tags registered in Widgets Subsystem, used to obtain widget data or widget instance. */
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_HUD);
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_SETTINGS);
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_NICKNAME);
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_FPSCOUNTER);
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_MULTIPLAYER);
BOMBER_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_UI_WIDGET_POWERUPS);

enum class EEndGameState : uint8;
enum class EPlayerType : uint8;

/**
 * Contains in-game UI data.
 */
UCLASS(Blueprintable, BlueprintType)
class BOMBER_API UUIDataAsset final : public UMyPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the UI data asset. */
	static const UUIDataAsset& Get();

	/** Returns widget data associated with the given tag, or invalid widget data if not found. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FManageableWidgetData& GetWidgetDataByTag(FGameplayTag InTag) const;

	/** Returns all widgets to create, each identified by a gameplay tag.
	 * @see UUIDataAsset::AllWidgetData. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE TArray<FManageableWidgetData>& GetAllWidgetData() const { return AllWidgetData; }

	/** Returns the localized texts about specified end game to display on UI.
	 * @see UUIDataAsset::EndGameTexts. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FText& GetEndGameText(EEndGameState EndGameState) const;

	/** Returns the default avatar for the specified player type.
	 * @see UUIDataAsset::DefaultAvatarsInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	class UTexture2D* GetDefaultAvatar(EPlayerType PlayerType) const;

protected:
	/** List of all widgets to create, each identified by a gameplay tag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widgets", meta = (BlueprintProtected))
	TArray<FManageableWidgetData> AllWidgetData;

	/** Contains the localized texts about specified end game to display on UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "End-Game Names"))
	TMap<EEndGameState, FText> EndGameTextsInternal;

	/** Returns the default avatar for the specified player type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Default Avatars"))
	TMap<EPlayerType, TObjectPtr<class UTexture2D>> DefaultAvatarsInternal;
};