// Copyright (c) Yevhenii Selivanov

#include "UI/Widgets/BmrPowerupWidget.h"
//---
#include "DataAssets/ItemDataAsset.h"
#include "LevelActors/PlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "Components/RadialSlider.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupWidget)

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Called after the underlying slate widget is constructed
void UBmrPowerupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ensureMsgf(ItemTypeInternal != EItemType::None, TEXT("ASSERT: [%i] %hs:\n'ItemType' is not set!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Is executed every tick when widget is enabled
void UBmrPowerupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bNeedsUpdateInternal
	    || LerpDurationInternal <= 0.f)
	{
		return;
	}

	const float Current = RadialSlider->GetValue();
	ElapsedLerpTimeInternal += InDeltaTime;

	const float Alpha = FMath::Clamp(ElapsedLerpTimeInternal / LerpDurationInternal, 0.f, 1.f);
	const float NewValue = FMath::Lerp(Current, TargetValueInternal, Alpha);

	RadialSlider->SetValue(NewValue);

	if (FMath::IsNearlyEqual(NewValue, TargetValueInternal, KINDA_SMALL_NUMBER)
	    || Alpha >= 1.f)
	{
		RadialSlider->SetValue(TargetValueInternal);
		bNeedsUpdateInternal = false;
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Called when the local player character is spawned, possessed, and replicated
void UBmrPowerupWidget::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	checkf(PlayerCharacter, TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is null!"), __LINE__, __FUNCTION__);
	PlayerCharacter->OnPowerUpsChanged.AddUniqueDynamic(this, &ThisClass::OnPowerUpsChanged);
	TargetValueInternal = PlayerCharacter->GetPowerUp(ItemTypeInternal).GetCurrentLevelPercent();

	// Set the initial values
	checkf(RadialSlider, TEXT("ERROR: [%i] %hs:\n'' is null!"), __LINE__, __FUNCTION__);
	RadialSlider->SetValue(TargetValueInternal);
}

// Called when the power-up data is updated and the UI should reflect new values
void UBmrPowerupWidget::OnPowerUpsChanged_Implementation(const FBmrPowerUpsContainer& NewPowerUps, const FBmrPowerUpsContainer& PrevPowerUps)
{
	if (NewPowerUps.Get(ItemTypeInternal) == PrevPowerUps.Get(ItemTypeInternal))
	{
		// No change in current powerup
		return;
	}

	bNeedsUpdateInternal = true;
	ElapsedLerpTimeInternal = 0.f;

	// Set the new target value
	TargetValueInternal = NewPowerUps.Get(ItemTypeInternal).GetCurrentLevelPercent();
}