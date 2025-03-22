// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Data/MyPrimaryDataAsset.h"
//---
#include "Structures/ManageableWidgetData.h"
//---
#include "UIDataAsset.generated.h"

enum class EEndGameState : uint8;

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

	/** Returns data for the in-game widget.
	 * @see UUIDataAsset::HUDWidgetDataInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FManageableWidgetData& GetHUDWidgetData() const { return HUDWidgetDataInternal; }

	/** Returns data for the settings widget.
	 * @see UUIDataAsset::SettingsWidgetDataInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FManageableWidgetData& GetSettingsWidgetData() const { return SettingsWidgetDataInternal; }

	/** Returns data for the nickname widget.
	 * @see UUIDataAsset::NicknameWidgetDataInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FManageableWidgetData& GetNicknameWidgetData() const { return NicknameWidgetDataInternal; }

	/** Returns data for the FPS counter widget.
	 * @see UUIDataAsset::FPSCounterWidgetDataInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FManageableWidgetData& GetFPSCounterWidgetData() const { return FPSCounterWidgetDataInternal; }

	/** Returns data for the multiplayer widget.
	 * @see UUIDataAsset::MultiplayerWidgetDataInternal. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FManageableWidgetData& GetMultiplayerWidgetData() const { return MultiplayerWidgetDataInternal; }

	/** Returns the localized texts about specified end game to display on UI.
	 * @see UUIDataAsset::EndGameTexts. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FText& GetEndGameText(EEndGameState EndGameState) const;

protected:
	/** Data for the In-Game Widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "HUD Widget"))
	FManageableWidgetData HUDWidgetDataInternal;

	/** Data for the Settings Widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Settings Widget"))
	FManageableWidgetData SettingsWidgetDataInternal;

	/** Data for the Nickname Widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "3D Nickname Widget"))
	FManageableWidgetData NicknameWidgetDataInternal;

	/** Data for the FPS Counter Widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "FPS Counter Widget"))
	FManageableWidgetData FPSCounterWidgetDataInternal;

	/** Data for the Multiplayer Widget. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Multiplayer Widget"))
	FManageableWidgetData MultiplayerWidgetDataInternal;

	/** Contains the localized texts about specified end game to display on UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "End-Game Names"))
	TMap<EEndGameState, FText> EndGameTextsInternal;
};