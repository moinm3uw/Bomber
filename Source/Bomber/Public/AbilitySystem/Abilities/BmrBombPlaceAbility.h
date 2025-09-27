// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Abilities/GameplayAbility.h"

#include "BmrBombPlaceAbility.generated.h"

/**
 * Handles placing a bomb on the map for both player and AI characters.
 * Ability is triggered by the TAG_EVENT_BOMB_PLACED event, where:
 * - EventData.Instigator: the actor placing the bomb.
 * - EventData.EventMagnitude: cell index to place the bomb at (if <=0, bomb is placed at avatar cell).
 */
UCLASS()
class BOMBER_API UBmrBombPlaceAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/** Returns the cell from the event data, or avatar cell if event data is invalid. */
	static struct FCell GetSpawnCellFromEventData(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* TriggerEventData);

protected:
	/** Is overridden to prevent event-based activation if we bomb cannot be placed at the specified cell. */
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* TriggerEventData) const override;

	/** Actually activate ability, do not call this directly. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Is overridden to apply cost with set by caller tag for bomb duration. */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Applies given gameplay effect with bomb duration. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	static FActiveGameplayEffectHandle ApplyDurationalEffect(TSubclassOf<UGameplayEffect> GameplayEffect, const FGameplayAbilityActorInfo& ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo);

	/*********************************************************************************************
	 * Data
	 ********************************************************************************************* */
protected:
	/** Is applied at bomb ability activation, detonates the bomb when removed. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Applied Duration Effect"))
	FActiveGameplayEffectHandle AppliedDurationEffectInternal;
};
