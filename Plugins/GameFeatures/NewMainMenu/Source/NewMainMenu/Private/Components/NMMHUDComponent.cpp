// Copyright (c) Yevhenii Selivanov

#include "Components/NMMHUDComponent.h"
//---
#include "Data/NMMDataAsset.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
#include "Widgets/NewMainMenuWidget.h"
#include "Widgets/NMMCinematicStateWidget.h"
//---
#include "NativeGameplayTags.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMHUDComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_WIDGET_NEWMAINMENU_MENU, TEXT("UI.Widget.NewMainMenu.Menu"));
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_WIDGET_NEWMAINMENU_CINEMATIC, TEXT("UI.Widget.NewMainMenu.Cinematic"));

// Default constructor
UNMMHUDComponent::UNMMHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// Returns the Main Menu widget
UNewMainMenuWidget* UNMMHUDComponent::GetMainMenuWidget() const
{
	const UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem();
	return WidgetsSubsystem ? WidgetsSubsystem->GetWidgetByTag<UNewMainMenuWidget>(TAG_UI_WIDGET_NEWMAINMENU_MENU) : nullptr;
}

// Returns the In Cinematic State widget
UNMMCinematicStateWidget* UNMMHUDComponent::GetInCinematicStateWidget() const
{
	const UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem();
	return WidgetsSubsystem ? WidgetsSubsystem->GetWidgetByTag<UNMMCinematicStateWidget>(TAG_UI_WIDGET_NEWMAINMENU_CINEMATIC) : nullptr;
}

// Called when a component is registered, after Scene is set, but before CreateRenderState_Concurrent or OnCreatePhysicsState are called
void UNMMHUDComponent::OnRegister()
{
	Super::OnRegister();

	// Listen to register widgets OnLocalCharacterReady to guarantee that the player controller is initialized, so we can use Widgets Subsystem
	BIND_ON_LOCAL_CHARACTER_READY(this, UNMMHUDComponent::OnLocalCharacterReady);
}

// Clears all transient data created by this component
void UNMMHUDComponent::OnUnregister()
{
	// --- Destroy Main Menu widgets

	if (UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem())
	{
		WidgetsSubsystem->DestroyManageableWidgetByTag(TAG_UI_WIDGET_NEWMAINMENU_MENU);
		WidgetsSubsystem->DestroyManageableWidgetByTag(TAG_UI_WIDGET_NEWMAINMENU_CINEMATIC);
	}

	Super::OnUnregister();
}

// Called when the local player character is spawned, possessed, and replicated
void UNMMHUDComponent::OnLocalCharacterReady_Implementation(class APlayerCharacter* Character, int32 CharacterID)
{
	UWidgetsSubsystem::Get().CreateManageableWidgetChecked(UNMMDataAsset::Get().GetMainMenuWidgetData());
	UWidgetsSubsystem::Get().CreateManageableWidgetChecked(UNMMDataAsset::Get().GetInCinematicStateWidgetData());
}
