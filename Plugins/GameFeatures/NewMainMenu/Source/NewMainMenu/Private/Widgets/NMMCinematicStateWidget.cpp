// Copyright (c) Yevhenii Selivanov

#include "Widgets/NMMCinematicStateWidget.h"
//---
#include "Controllers/MyPlayerController.h"
#include "Data/NMMDataAsset.h"
#include "Data/NMMTypes.h"
#include "Subsystems/NMMBaseSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
//---
#include "Components/Button.h"
#include "Components/RadialSlider.h"
#include "Components/TextBlock.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(NMMCinematicStateWidget)

// Applies the given time to hold the skip progress to skip the cinematic
void UNMMCinematicStateWidget::SetCurrentHoldTime(float NewHoldTime)
{
	CurrentHoldTimeInternal = NewHoldTime;

	const float MaxHoldTime = UNMMDataAsset::Get().GetSkipCinematicHoldTime();
	const float HoldProgressNormalized = FMath::Clamp(CurrentHoldTimeInternal / MaxHoldTime, 0.f, 1.f);

	checkf(SkipHoldProgress, TEXT("ERROR: [%i] %hs:\n'SkipHoldProgress' is null!"), __LINE__, __FUNCTION__);
	SkipHoldProgress->SetValue(HoldProgressNormalized);

	if (CurrentHoldTimeInternal >= MaxHoldTime)
	{
		OnCinematicSkipFinished();
	}
}

// Reset to default state
void UNMMCinematicStateWidget::ResetWidget()
{
	SetCurrentHoldTime(0.f);
}

// // Called after the underlying slate widget is constructed
void UNMMCinematicStateWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Hide this widget by default
	SetVisibility(ESlateVisibility::Collapsed);

	// Listen Main Menu states
	UNMMBaseSubsystem& BaseSubsystem = UNMMBaseSubsystem::Get();
	BaseSubsystem.OnMainMenuStateChanged.AddUniqueDynamic(this, &ThisClass::OnNewMainMenuStateChanged);
	if (BaseSubsystem.GetCurrentMenuState() != ENMMState::None)
	{
		// State is already set, apply it
		OnNewMainMenuStateChanged(BaseSubsystem.GetCurrentMenuState());
	}

	if (SkipCinematicButton)
	{
		SkipCinematicButton->SetClickMethod(EButtonClickMethod::PreciseClick);
		SkipCinematicButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnCinematicSkipFinished);
	}
}

// Called when the Main Menu state was changed
void UNMMCinematicStateWidget::OnNewMainMenuStateChanged_Implementation(ENMMState NewState)
{
	const bool bIsCinematic = NewState == ENMMState::Cinematic;

	// Hide all other widgets in Cinematic state
	UWidgetsSubsystem::Get().SetAllWidgetsVisibility(!bIsCinematic);

	// Show this widget in Cinematic state
	SetVisibility(bIsCinematic ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	ResetWidget();
}

/*********************************************************************************************
 * Inputs
 ********************************************************************************************* */

// Is calling while the skip holding button is ongoing
void UNMMCinematicStateWidget::OnCinematicSkipOngoing_Implementation()
{
	const UWorld* World = GetWorld();
	checkf(World, TEXT("ERROR: [%i] %hs:\n'World' is null!"), __LINE__, __FUNCTION__);
	const float NewHoldTime = CurrentHoldTimeInternal + World->GetDeltaSeconds();

	SetCurrentHoldTime(NewHoldTime);
}

// Is called on skip cinematic button released (cancelled)
void UNMMCinematicStateWidget::OnCinematicSkipReleased_Implementation()
{
	ResetWidget();
}

// Is called to skip cinematic on finished holding the skip button or clicked on UI
void UNMMCinematicStateWidget::OnCinematicSkipFinished_Implementation()
{
	// Skip cinematic
	if (AMyPlayerController* MyPC = GetOwningPlayer<AMyPlayerController>())
	{
		MyPC->SetGameStartingState();
	}
}
