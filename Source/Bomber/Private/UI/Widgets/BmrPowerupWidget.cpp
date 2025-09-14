// Copyright (c) Yevhenii Selivanov

#include "UI/Widgets/BmrPowerupWidget.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "DataAssets/UIDataAsset.h"
#include "LevelActors/PlayerCharacter.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// UE
#include "Components/Image.h"
#include "Components/RadialSlider.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupWidget)

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Updates the blends slider target to which widget will interpolate
void UBmrPowerupWidget::SetTargetValue(float NewValue, float MaxValue, bool bImmediateUpdate)
{
	NewValue = FMath::Max(NewValue, 0.f);
	MaxValue = FMath::Max(MaxValue, NewValue);
	TargetValueInternal = NewValue / MaxValue;

	if (bImmediateUpdate)
	{
		checkf(RadialSlider, TEXT("ERROR: [%i] %hs:\n'RadialSlider' is null!"), __LINE__, __FUNCTION__);
		RadialSlider->SetValue(TargetValueInternal);
	}
	else
	{
		// Start the blend
		bNeedsUpdateInternal = true;
		ElapsedLerpTimeInternal = 0.f;
	}
}

// Updates the icon of the powerup item in the UI
void UBmrPowerupWidget::SetPowerupIcon(FBmrPowerupTag NewItemType)
{
	if (!ensureMsgf(PowerUpIcon, TEXT("ASSERT: [%i] %hs:\n'PowerUpIcon' is not constructed!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	ItemTypeInternal = NewItemType;

	UTexture2D* IconTexture = UUIDataAsset::Get().GetPowerupIcon(NewItemType);
	ensureMsgf(PowerUpIcon, TEXT("ASSERT: [%i] %hs:\n'PowerUpIcon' is not set in UI Data Asset"), __LINE__, __FUNCTION__);
	FSlateBrush CurrentBrush = PowerUpIcon->GetBrush();
	CurrentBrush.SetResourceObject(IconTexture);
	PowerUpIcon->SetBrush(CurrentBrush);
}

// Called before the underlying slate widget is constructed to update widget at design time
void UBmrPowerupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Update the icon brush in the editor when tag property is changed
	SetPowerupIcon(ItemTypeInternal);
}

// Called after the underlying slate widget is constructed
void UBmrPowerupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ensureMsgf(ItemTypeInternal != FBmrPowerupTag::None, TEXT("ASSERT: [%i] %hs:\n'ItemType' is not set!"), __LINE__, __FUNCTION__))
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

// Called when the local player state is initialized and its assigned character is ready
void UBmrPowerupWidget::OnLocalCharacterReady_Implementation(APlayerCharacter* Character, int32 CharacterID)
{
	checkf(Character, TEXT("ERROR: [%i] %hs:\n'Character' is null!"), __LINE__, __FUNCTION__);

	// Bind to the attribute change and set the initial values
	constexpr bool bImmediateUpdate = true;
	UAbilitySystemComponent& ASC = Character->GetAbilitySystemComponentChecked();
	const UBmrPowerupsAttributeSet& PowerupsAttributeSet = UBmrPowerupsAttributeSet::Get(&ASC);
	if (ItemTypeInternal == FBmrPowerupTag::Skate)
	{
		ASC.GetGameplayAttributeValueChangeDelegate(UBmrPowerupsAttributeSet::GetPowerup_SkateAttribute()).AddUObject(this, &ThisClass::OnSkateAttributeChanged);
		SetTargetValue(PowerupsAttributeSet.GetPowerup_Skate(), PowerupsAttributeSet.GetPowerup_MaxSkate(), bImmediateUpdate);
	}
	else if (ItemTypeInternal == FBmrPowerupTag::Fire)
	{
		ASC.GetGameplayAttributeValueChangeDelegate(UBmrPowerupsAttributeSet::GetPowerup_FireAttribute()).AddUObject(this, &ThisClass::OnFireAttributeChanged);
		SetTargetValue(PowerupsAttributeSet.GetPowerup_Fire(), PowerupsAttributeSet.GetPowerup_MaxFire(), bImmediateUpdate);
	}
	else if (ItemTypeInternal == FBmrPowerupTag::Bomb)
	{
		ASC.GetGameplayAttributeValueChangeDelegate(UBmrPowerupsAttributeSet::GetPowerup_BombsAvailableAttribute()).AddUObject(this, &ThisClass::OnBombAttributeChanged);
		SetTargetValue(PowerupsAttributeSet.GetPowerup_BombsAvailable(), PowerupsAttributeSet.GetPowerup_MaxBombs(), bImmediateUpdate);
	}
}

// Called when the power-up data is updated and the UI should reflect new values
void UBmrPowerupWidget::OnPowerUpsChanged_Implementation(float NewValue, float MaxValue, FBmrPowerupTag PowerupType)
{
	SetTargetValue(NewValue, MaxValue);
}

// Is called when the Skate attribute is changed, e.g: when player picked up a Skate item
void UBmrPowerupWidget::OnSkateAttributeChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	const UBmrPowerupsAttributeSet& PowerupsAttributeSet = UBmrPowerupsAttributeSet::Get(GetOwningPlayerPawn());
	const float MaxValue = PowerupsAttributeSet.GetPowerup_MaxSkate();
	OnPowerUpsChanged(OnAttributeChangeData.NewValue, MaxValue, FBmrPowerupTag::Skate);
}

// Is called when the Fire attribute is changed, e.g: when player picked up a Fire item
void UBmrPowerupWidget::OnFireAttributeChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	const UBmrPowerupsAttributeSet& PowerupsAttributeSet = UBmrPowerupsAttributeSet::Get(GetOwningPlayerPawn());
	const float MaxValue = PowerupsAttributeSet.GetPowerup_MaxFire();
	OnPowerUpsChanged(OnAttributeChangeData.NewValue, MaxValue, FBmrPowerupTag::Fire);
}

// Is called when the Bomb attribute is changed, e.g: when player picked up a Bomb item
void UBmrPowerupWidget::OnBombAttributeChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	const UBmrPowerupsAttributeSet& PowerupsAttributeSet = UBmrPowerupsAttributeSet::Get(GetOwningPlayerPawn());
	const float MaxValue = PowerupsAttributeSet.GetPowerup_MaxBombs();
	OnPowerUpsChanged(OnAttributeChangeData.NewValue, MaxValue, FBmrPowerupTag::Bomb);
}