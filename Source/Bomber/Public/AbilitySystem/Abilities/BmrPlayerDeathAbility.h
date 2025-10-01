// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Abilities/GameplayAbility.h"

#include "BmrPlayerDeathAbility.generated.h"

/**
 * Handles player death state and cleanup when health reaches zero.
 * Ability is triggered by the BmrGameplayTags::Event::Player_Dead event, where:
 * - EventData.Instigator: the actor that caused the death (bomb, environment, etc.).
 */
UCLASS()
class BOMBER_API UBmrPlayerDeathAbility : public UGameplayAbility
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Data
	 ********************************************************************************************* */
protected:
	/** The actor that caused the death, usually a bomb. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "Death Causer"))
	TObjectPtr<AActor> DeathCauserInternal = nullptr;

	/*********************************************************************************************
	 * Overrides and events
	 ********************************************************************************************* */
protected:
	/** Actually activate ability, do not call this directly. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Called if an ability ends normally or abnormally. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Called when the death montage completed or interrupted. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "C++", meta = (BlueprintProtected))
	void OnMontageEnd();
};