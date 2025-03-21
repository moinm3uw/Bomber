// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Blueprint/UserWidget.h"
//---
#include "PlayerNameWidget.generated.h"

/**
 * Represents the player nickname, is used by human and AI characters, both in 3D and 2D UI.
 */
UCLASS(Abstract)
class BOMBER_API UPlayerNameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/** Is overridden to hide dependent 3D widget components along with this widget. */
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

	/*********************************************************************************************
	 * Player Name
	 ********************************************************************************************* */
public:
	/** Returns the player name from the widget. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FText GetPlayerName() const;

	/** Sets player name to the widget. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetPlayerName(const FText& NewPlayerName);

protected:
	/** The text block with player name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, BindWidget))
	TObjectPtr<class UTextBlock> PlayerNameTextWidget = nullptr;

	/*********************************************************************************************
	 * Player ID
	 ********************************************************************************************* */
public:
	/** Returns ID of the player character owner with which this widget is associated. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE int32 GetAssociatedPlayerId() const { return AssociatedPlayerIdInternal; }

	/** Sets the player ID to the widget.
	 * It's essential to set since it's used by AI characters as well, so it's only a way to get correct owner. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetAssociatedPlayerId(int32 NewPlayerId);

protected:
	/** ID of the player character owner with which this widget is associated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, DisplayName = "Associated Player ID"))
	int32 AssociatedPlayerIdInternal = INDEX_NONE;
};
