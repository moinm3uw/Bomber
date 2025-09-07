// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Blueprint/UserWidget.h"

// Bomber
#include "Structures/BmrPowerupTag.h" // ItemType

#include "BmrPowerupWidget.generated.h"

/**
 * Widget that represents the powerup item in the UI.
 */
UCLASS(Abstract)
class BOMBER_API UBmrPowerupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Updates the blends slider target to which widget will interpolate.
	 * @param NewValue The new value to set the slider to, should be in range [0, MaxValue].
	 * @param MaxValue The maximum value for the slider, used to display the percentage of the powerup level.
	 * @param bImmediateUpdate If true, the slider will be updated immediately without interpolation. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void SetTargetValue(float NewValue, float MaxValue, bool bImmediateUpdate = false);

protected:
	/** Exposed property to be set in Details Panel of the type of item this UI or data element is associated with (e.g., Speed, BombCount, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Design", meta = (BlueprintProtected, DisplayName = "Item Type", ExposeOnSpawn = "true"))
	FBmrPowerupTag ItemTypeInternal = FBmrPowerupTag::None;

	/** Exposed property to be set in Details Panel of the duration of the interpolation when updating visual feedback (e.g., slider value change) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Design", meta = (BlueprintProtected, DisplayName = "Lerp Duration", ClampMin = "0.01"))
	float LerpDurationInternal = 0.5f;

	/** The radial slider UI widget used to display or adjust the power-up level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Transient, Category = "C++", meta = (BlueprintProtected, BindWidget))
	TObjectPtr<class URadialSlider> RadialSlider = nullptr;

	/** Target value to interpolate toward when updating the slider */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Target Value"))
	float TargetValueInternal = 0.f;

	/** Time elapsed since starting the interpolation toward the new target value */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Elapsed Lerp Time"))
	float ElapsedLerpTimeInternal = 0.f;

	/** Whether the slider value needs to be updated based on target and elapsed time */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Needs Update"))
	bool bNeedsUpdateInternal = false;

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Called after the underlying slate widget is constructed.
	 * May be called multiple times due to adding and removing from the hierarchy. */
	virtual void NativeConstruct() override;

	/** Is executed every tick when widget is enabled. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/*********************************************************************************************
	 * Events
	 ********************************************************************************************* */
protected:
	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/** Called when the power-up data is updated and the UI should reflect new values */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Powerups", meta = (BlueprintProtected))
	void OnPowerUpsChanged(float NewValue, float MaxValue, struct FBmrPowerupTag PowerupType);

	/** Is called when the Skate attribute is changed, e.g: when player picked up a Skate item. */
	void OnSkateAttributeChanged(const struct FOnAttributeChangeData& OnAttributeChangeData);

	/** Is called when the Fire attribute is changed, e.g: when player picked up a Fire item. */
	void OnFireAttributeChanged(const struct FOnAttributeChangeData& OnAttributeChangeData);

	/** Is called when the Bomb attribute is changed, e.g: when player picked up a Bomb item. */
	void OnBombAttributeChanged(const struct FOnAttributeChangeData& OnAttributeChangeData);
};