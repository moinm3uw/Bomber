// Copyright (c) Yevhenii Selivanov.

#include "Subsystems/WidgetsSubsystem.h"
//---
#include "Controllers/MyPlayerController.h"
#include "DataAssets/UIDataAsset.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "MyUtilsLibraries/WidgetUtilsLibrary.h"
#include "UI/SettingsWidget.h"
#include "UI/Widgets/HUDWidget.h"
#include "UI/Widgets/PlayerNameWidget.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "Components/Viewport.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(WidgetsSubsystem)

// Returns the pointer the UI Subsystem
UWidgetsSubsystem* UWidgetsSubsystem::GetWidgetsSubsystem(const UObject* OptionalWorldContext/* = nullptr*/)
{
	const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(OptionalWorldContext);
	if (!LocalPlayer)
	{
		const AMyPlayerController* PC = Cast<AMyPlayerController>(OptionalWorldContext);
		if (!PC)
		{
			PC = UMyBlueprintFunctionLibrary::GetLocalPlayerController(OptionalWorldContext);
		}
		LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	}
	return LocalPlayer ? LocalPlayer->GetSubsystem<UWidgetsSubsystem>() : nullptr;
}

// Returns the UI subsystem checked: it will crash if player controller is not initialized yet
UWidgetsSubsystem& UWidgetsSubsystem::Get(const UObject* OptionalWorldContext)
{
	UWidgetsSubsystem* WidgetsSubsystem = GetWidgetsSubsystem(OptionalWorldContext);
	checkf(WidgetsSubsystem, TEXT("%s: 'WidgetsSubsystem' is null, likely controller is not initialized yet!"), *FString(__FUNCTION__));
	return *WidgetsSubsystem;
}

/*********************************************************************************************
 * Widgets Management
 ********************************************************************************************* */

// Create specified widget and add it to Manageable widgets list, so its visibility can be changed globally
UUserWidget* UWidgetsSubsystem::CreateManageableWidget(const FManageableWidgetData& WidgetData, const UObject* OptionalWorldContext/* = nullptr*/)
{
	if (!ensureMsgf(WidgetData.IsValid(), TEXT("ASSERT: [%i] %hs:\n'WidgetData' is not valid: %s"), __LINE__, __FUNCTION__, *WidgetData.ToString()))
	{
		return nullptr;
	}

	UUserWidget* Widget = FWidgetUtilsLibrary::CreateWidgetByClass(WidgetData.WidgetClass, WidgetData.bAddToViewport, WidgetData.ZOrder, OptionalWorldContext);
	AllManageableWidgetsInternal.Add(WidgetData.WidgetTag, Widget);
	return Widget;
}

// Returns the widget instance by its tag
UUserWidget* UWidgetsSubsystem::GetWidgetByTag(FGameplayTag WidgetTag) const
{
	const TSoftObjectPtr<UUserWidget>* WidgetPtr = AllManageableWidgetsInternal.Find(WidgetTag);
	return WidgetPtr ? WidgetPtr->Get() : nullptr;
}

// Removes given widget from the list and destroys it
void UWidgetsSubsystem::DestroyManageableWidget(UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return;
	}

	if (const FGameplayTag* WidgetTag = AllManageableWidgetsInternal.FindKey(Widget))
	{
		AllManageableWidgetsInternal.Remove(*WidgetTag);
	}

	AllHiddenWidgetsInternal.RemoveSwap(Widget);

	FWidgetUtilsLibrary::DestroyWidget(*Widget);
}

// Removes given widget from the list and destroys it by its tag
void UWidgetsSubsystem::DestroyManageableWidgetByTag(FGameplayTag WidgetTag)
{
	if (!ensureMsgf(WidgetTag.IsValid(), TEXT("ASSERT: [%i] %hs:\n'WidgetTag' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const TSoftObjectPtr<UUserWidget>* WidgetPtr = AllManageableWidgetsInternal.Find(WidgetTag);
	UUserWidget* Widget = WidgetPtr ? WidgetPtr->Get() : nullptr;
	if (Widget)
	{
		DestroyManageableWidget(Widget);
	}
}

/*********************************************************************************************
 * Core Widgets Initialization
 ********************************************************************************************* */

// Will try to start the process of initializing all widgets used in game
void UWidgetsSubsystem::TryInitWidgets()
{
	if (UUtilsLibrary::IsViewportInitialized())
	{
		InitWidgets();
	}
	else if (!FViewport::ViewportResizedEvent.IsBoundToObject(this))
	{
		FViewport::ViewportResizedEvent.AddUObject(this, &ThisClass::OnViewportResizedWhenInit);
	}
}

// Create and set widget objects once
void UWidgetsSubsystem::InitWidgets()
{
	if (AreWidgetInitialized())
	{
		return;
	}

	const TArray<FManageableWidgetData>& AllWidgetData = UUIDataAsset::Get().GetAllWidgetData();
	for (const FManageableWidgetData& WidgetDataIt : AllWidgetData)
	{
		// Automatically create and add all widgets to the viewport from the UI Data Asset
		UUserWidget& NewWidget = CreateManageableWidgetChecked(WidgetDataIt);

		// Handle extra initialization logic for some widgets
		if (WidgetDataIt.WidgetTag == TAG_UI_WIDGET_SETTINGS)
		{
			USettingsWidget& SettingsWidget = *CastChecked<USettingsWidget>(&NewWidget);
			SettingsWidget.TryConstructSettings();
		}
		else if (WidgetDataIt.WidgetTag == TAG_UI_WIDGET_NICKNAME)
		{
			static constexpr int32 MaxPlayersNum = 4;
			for (int32 Index = 0; Index < MaxPlayersNum; ++Index)
			{
				UPlayerNameWidget& NicknameWidget = CreateManageableWidgetChecked<UPlayerNameWidget>(WidgetDataIt);
				NicknameWidgetsInternal.Add(&NicknameWidget);
			}
		}
	}

	bAreWidgetInitializedInternal = true;

	if (OnWidgetsInitialized.IsBound())
	{
		OnWidgetsInitialized.Broadcast();
	}
}

// Removes all widgets and transient data
void UWidgetsSubsystem::CleanupWidgets()
{
	while (!AllManageableWidgetsInternal.IsEmpty())
	{
		const FGameplayTag WidgetTag = AllManageableWidgetsInternal.CreateIterator().Key();
		DestroyManageableWidgetByTag(WidgetTag);
		AllManageableWidgetsInternal.Remove(WidgetTag);
	}

	AllManageableWidgetsInternal.Empty();
	AllHiddenWidgetsInternal.Empty();
	NicknameWidgetsInternal.Empty();

	bAreWidgetInitializedInternal = false;
}

/*********************************************************************************************
 * Widgets Visibility
 ********************************************************************************************* */

// If true, changes all visible manageable widgets to hidden
void UWidgetsSubsystem::SetAllWidgetsVisibility(bool bMakeVisible, bool bCanRestoreVisibilityLater/* = true*/)
{
	const ESlateVisibility DesiredVisibility = bMakeVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	const TArray<TSoftObjectPtr<UUserWidget>> WidgetsToProcess = [&]()
	{
		TArray<TSoftObjectPtr<UUserWidget>> Result;
		if (bMakeVisible)
		{
			Result = AllHiddenWidgetsInternal;
		}
		else
		{
			AllManageableWidgetsInternal.GenerateValueArray(Result);
		}
		return Result;
	}();

	if (!bMakeVisible)
	{
		AllHiddenWidgetsInternal.Empty();
	}

	for (const TSoftObjectPtr<UUserWidget>& Widget : WidgetsToProcess)
	{
		if (Widget && Widget->GetVisibility() != DesiredVisibility)
		{
			Widget->SetVisibility(DesiredVisibility);

			if (!bMakeVisible
			    && bCanRestoreVisibilityLater)
			{
				AllHiddenWidgetsInternal.Add(Widget);
			}
		}
	}

	if (bMakeVisible)
	{
		AllHiddenWidgetsInternal.Empty();
	}
}

/*********************************************************************************************
 * FPS Counter
 ********************************************************************************************* */

// Set true to show the FPS counter widget on the HUD
void UWidgetsSubsystem::SetFPSCounterEnabled(bool bEnable)
{
	UUserWidget* FPSCounterWidget = GetWidgetByTag(TAG_UI_WIDGET_FPSCOUNTER);
	if (ensureMsgf(FPSCounterWidget, TEXT("ASSERT: [%i] %hs:\n'FPSCounterWidget' was not found!"), __LINE__, __FUNCTION__))
	{
		const ESlateVisibility NewVisibility = bEnable ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		FPSCounterWidget->SetVisibility(NewVisibility);
		bIsFPSCounterEnabledInternal = bEnable;
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Callback for when the player controller is changed on this subsystem's owning local player
void UWidgetsSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);

	const AMyPlayerController* MyPC = Cast<AMyPlayerController>(NewPlayerController);
	if (!MyPC
	    || MyPC->bIsDebugCameraEnabledInternal)
	{
		// Do not initialize widgets if different controller is possessed, likely Debug Controller, or Debug Camera is enabled
		return;
	}

	if (AreWidgetInitialized())
	{
		// New player controller is set, likely level was changed, so perform cleanup first
		CleanupWidgets();
	}

	TryInitWidgets();
}

// Is called when this Subsystem is removed
void UWidgetsSubsystem::Deinitialize()
{
	Super::Deinitialize();

	CleanupWidgets();
}

// Is called right after the game was started and windows size is set
void UWidgetsSubsystem::OnViewportResizedWhenInit(FViewport* Viewport, uint32 Index)
{
	if (FViewport::ViewportResizedEvent.IsBoundToObject(this))
	{
		FViewport::ViewportResizedEvent.RemoveAll(this);
	}

	InitWidgets();
}