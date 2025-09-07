// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"

// UE
#include "GameplayTagContainer.h"

#include "WidgetsSubsystem.generated.h"

class UUserWidget;

struct FManageableWidgetData;

/**
 * Is used to manage User Widgets with lifetime of Local Player (similar to HUD).
 * @see Access its data with UUIDataAsset (Content/Bomber/DataAssets/DA_UI).
 */
UCLASS(Config = "GameUserSettings", DefaultConfig)
class BOMBER_API UWidgetsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/** Returns the pointer the UI Subsystem.
	 * It will return null if Local Player is not initialized yet. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	static UWidgetsSubsystem* GetWidgetsSubsystem(const UObject* OptionalWorldContext = nullptr);

	/** Returns the UI subsystem checked: it will crash if player controller is not initialized yet.
	 * @warning don't call it on BeginPlay, do it not earlier than OnLocalCharacterReady */
	static UWidgetsSubsystem& Get(const UObject* OptionalWorldContext = nullptr);

	/*********************************************************************************************
	 * Widgets Management
	 * Widgets using there methods are managed by this subsystem and can be controlled globally.
	 ********************************************************************************************* */
public:
	/** Creates and registers specified widget to the Manageable widgets list, so its visibility can be changed globally. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (WorldContext = "OptionalWorldContext", CallableWithoutWorldContext))
	UUserWidget* CreateManageableWidget(const FManageableWidgetData& WidgetData, const UObject* OptionalWorldContext = nullptr);

	/** Is alternative to CreateManageableWidget, but with templated cast and crashes if widget class is not valid.
	 * E.g: UMyUserWidget* NewWidget = CreateManageableWidgetChecked<UMyUserWidget>(WidgetData); */
	template <typename T = UUserWidget>
	FORCEINLINE T& CreateManageableWidgetChecked(const FManageableWidgetData& WidgetData) { return *CastChecked<T>(CreateManageableWidget(WidgetData)); }

	/** Returns the widget instance by its tag. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	UUserWidget* GetWidgetByTag(FGameplayTag WidgetTag) const;

	/** Is alternative to GetManageableWidgetByTag, but with templated cast.
	 * E.g: const UMyUserWidget* FoundWidget = GetManageableWidgetByTagChecked<UMyUserWidget>(WidgetTag); */
	template <typename T = UUserWidget>
	FORCEINLINE T* GetWidgetByTag(FGameplayTag WidgetTag) const { return Cast<T>(GetWidgetByTag(WidgetTag)); }

	/** Removes given widget from the list and destroys it. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void DestroyManageableWidget(UUserWidget* Widget);

	/** Removes given widget from the list and destroys it by its tag. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void DestroyManageableWidgetByTag(FGameplayTag WidgetTag);

protected:
	/** Contains all widgets that are managed by this subsystem.
	 * Is Soft to allow garbage collection. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "All Managable Widgets"))
	TMap<FGameplayTag, TSoftObjectPtr<UUserWidget>> AllManageableWidgetsInternal;

	/*********************************************************************************************
	 * Core Widgets Initialization
	 * Some core widgets (like HUD) that are created internally by this subsystem.
	 ********************************************************************************************* */
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetsInitialized);

	/** Is called to notify that all widgets were initialized and ready. */
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnWidgetsInitialized OnWidgetsInitialized;

	/** Returns true if widgets ere initialized. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE bool AreWidgetInitialized() const { return bAreWidgetInitializedInternal; }

protected:
	/** Is true if widgets are initialized. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Are Widget Initialized"))
	bool bAreWidgetInitializedInternal = false;

	/** Will try to start the process of initializing all widgets used in game. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void TryInitWidgets();

	/** Create and set widget objects once. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void InitWidgets();

	/** Removes all widgets and transient data. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void CleanupWidgets();

	/*********************************************************************************************
	 * Widgets Visibility
	 ********************************************************************************************* */
public:
	/** Is called to toggle all manageable widgets visibility.
	 * @param bMakeVisible - if true, changes all visible manageable widgets to hidden; false, restores visibility of all previously hidden widgets.
	 * @param bCanRestoreVisibilityLater - if true, original visibilities will be remembered, so they can be restored later if call this function again with reverse value. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetAllWidgetsVisibility(bool bMakeVisible, bool bCanRestoreVisibilityLater = true);

	/** Returns true if all manageable widgets are hidden. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE bool AreAllWidgetsHidden() const { return !AllHiddenWidgetsInternal.IsEmpty(); }

protected:
	/** Contains widgets that globally were requested to hide, but were visible before, so their visibility will be restored when needed.
	 * Is Soft to allow garbage collection. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "All Hidden Widgets"))
	TArray<TSoftObjectPtr<UUserWidget>> AllHiddenWidgetsInternal;

	/*********************************************************************************************
	 * Nickname Widgets
	 ********************************************************************************************* */
public:
	/** Returns the nickname widget by a player index. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE class UPlayerNameWidget* GetNicknameWidget(int32 Index) const { return NicknameWidgetsInternal.IsValidIndex(Index) ? NicknameWidgetsInternal[Index] : nullptr; }

protected:
	/** All nickname widget objects for each player. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "3D Nickname Widgets"))
	TArray<TObjectPtr<class UPlayerNameWidget>> NicknameWidgetsInternal;

	/*********************************************************************************************
	 * FPS Counter
	 ********************************************************************************************* */
public:
	/** Set true to show the FPS counter widget on the HUD. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetFPSCounterEnabled(bool bEnable);

	/** Returns true if the FPS counter widget is shown on the HUD. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE bool IsFPSCounterEnabled() const { return bIsFPSCounterEnabledInternal; }

protected:
	/** If true, shows FPS counter widget on the HUD, is config property. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Config, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Is FPS Counter Enabled"))
	bool bIsFPSCounterEnabledInternal;

	/*********************************************************************************************
	 * Events
	 ********************************************************************************************* */
protected:
	/** Callback for when the player controller is changed on this subsystem's owning local player. */
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	/** Is called when this Subsystem is removed. */
	virtual void Deinitialize() override;

	/** Is called right after the game was started and windows size is set. */
	void OnViewportResizedWhenInit(class FViewport* Viewport, uint32 Index);
};