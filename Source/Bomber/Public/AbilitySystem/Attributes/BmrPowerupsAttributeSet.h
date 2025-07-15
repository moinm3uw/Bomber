// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AttributeSet.h"
//---
#include "AbilitySystemComponent.h" // ATTRIBUTE_ACCESSORS_BASIC
//---
#include "BmrPowerupsAttributeSet.generated.h"

/**
 * Attribute set for powerup-related attributes (items pick-up, character stats, etc.).
 */
UCLASS()
class BOMBER_API UBmrPowerupsAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	/** Returns the powerups attribute set for the specified owner. It will return nullptr if can't be obtained. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++", meta = (DefaultToSelf = "InOwner"))
	static const UBmrPowerupsAttributeSet* GetPowerupsAttributeSet(const UObject* InOwner);

	/** Returns the powerups attribute set for the specified owner. It will crash if can't be obtained. */
	static const UBmrPowerupsAttributeSet& Get(const UObject& InOwner);

	/*********************************************************************************************
	 * Data attributes
	 ********************************************************************************************* */
protected:
	/** Current explosion radius enhancement from fire powerups. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_Fire", Category = "C++")
	FGameplayAttributeData Powerup_Fire;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_Fire)

	/** Maximum allowed explosion radius. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_MaxFire", Category = "C++")
	FGameplayAttributeData Powerup_MaxFire;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_MaxFire)

	/** Amount of skate items collected (each adds +100 speed to base movement speed). */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_Skate", Category = "C++")
	FGameplayAttributeData Powerup_Skate;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_Skate)

	/** Maximum allowed skate items. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_MaxSkate", Category = "C++")
	FGameplayAttributeData Powerup_MaxSkate;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_MaxSkate)

	/** Current available bombs for placement: decremented when placed, incremented when exploded. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_BombsAvailable", Category = "C++")
	FGameplayAttributeData Powerup_BombsAvailable;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_BombsAvailable)

	/** Maximum bomb capacity. */
	UPROPERTY(BlueprintReadOnly, Transient, ReplicatedUsing = "OnRep_Powerup_MaxBombs", Category = "C++")
	FGameplayAttributeData Powerup_MaxBombs;
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, Powerup_MaxBombs)

	/*********************************************************************************************
	 * OnRep notifies
	 ********************************************************************************************* */
protected:
	UFUNCTION()
	void OnRep_Powerup_Fire(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Powerup_MaxFire(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Powerup_Skate(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Powerup_MaxSkate(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Powerup_BombsAvailable(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Powerup_MaxBombs(const FGameplayAttributeData& OldValue) const;

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Returns properties that are replicated for the lifetime of the actor channel. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called just before any modification happens to an attribute. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Called just after a GameplayEffect is executed to modify the base value of an attribute. No more changes can be made. */
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};