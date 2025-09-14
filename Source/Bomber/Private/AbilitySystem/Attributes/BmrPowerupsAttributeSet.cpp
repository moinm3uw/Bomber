// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"

// UE
#include "AbilitySystemGlobals.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "Structures/BmrPowerupTag.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupsAttributeSet)

// Returns the powerups attribute set for the specified owner. It will return nullptr if can't be obtained
const UBmrPowerupsAttributeSet* UBmrPowerupsAttributeSet::GetPowerupsAttributeSet(const UObject* InOwner)
{
	const UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(InOwner);
	if (!ASC)
	{
		ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AActor>(InOwner));
	}
	const UAttributeSet* AttributeSet = ASC ? ASC->GetAttributeSet(StaticClass()) : nullptr;
	return Cast<UBmrPowerupsAttributeSet>(AttributeSet);
}

// Returns the powerups attribute set for the specified owner. It will crash if can't be obtained
const UBmrPowerupsAttributeSet& UBmrPowerupsAttributeSet::Get(const UObject* InOwner)
{
	const UBmrPowerupsAttributeSet* PowerupsAttributeSet = GetPowerupsAttributeSet(InOwner);
	checkf(PowerupsAttributeSet, TEXT("ERROR: [%i] %hs:\n'PowerupsAttributeSet' is null!"), __LINE__, __FUNCTION__);
	return *PowerupsAttributeSet;
}

// Returns the powerup attribute associated with the given tag, or an empty attribute if not found
FGameplayAttribute UBmrPowerupsAttributeSet::GetPowerupBaseAttributeByTag(FBmrPowerupTag InTag)
{
	if (!InTag.IsValid())
	{
		return FGameplayAttribute();
	}

	if (InTag == FBmrPowerupTag::Fire)
	{
		return GetPowerup_FireAttribute();
	}

	if (InTag == FBmrPowerupTag::Skate)
	{
		return GetPowerup_SkateAttribute();
	}

	if (InTag == FBmrPowerupTag::Bomb)
	{
		return GetPowerup_BombsAvailableAttribute();
	}

	ensureMsgf(false, TEXT("ASSERT: [%i] %hs:\n'%s' powerup tag is not recognized!"), __LINE__, __FUNCTION__, *InTag.ToString());
	return FGameplayAttribute();
}

// Returns the max powerup attribute associated with the given tag, or an empty attribute if not found
FGameplayAttribute UBmrPowerupsAttributeSet::GetPowerupMaxAttributeByTag(struct FBmrPowerupTag InTag)
{
	if (!InTag.IsValid())
	{
		return FGameplayAttribute();
	}

	if (InTag == FBmrPowerupTag::Fire)
	{
		return GetPowerup_MaxFireAttribute();
	}

	if (InTag == FBmrPowerupTag::Skate)
	{
		return GetPowerup_MaxSkateAttribute();
	}

	if (InTag == FBmrPowerupTag::Bomb)
	{
		return GetPowerup_MaxBombsAttribute();
	}

	ensureMsgf(false, TEXT("ASSERT: [%i] %hs:\n'%s' powerup tag is not recognized!"), __LINE__, __FUNCTION__, *InTag.ToString());
	return FGameplayAttribute();
}

// Returns the value of the powerup attribute associated with the given tag, or -1 if not found
float UBmrPowerupsAttributeSet::GetPowerupValueByTag(FBmrPowerupTag InTag) const
{
	const FGameplayAttribute PowerupAttribute = GetPowerupBaseAttributeByTag(InTag);
	constexpr float InvalidPowerupValue = -1.f;
	return PowerupAttribute.IsValid() ? PowerupAttribute.GetNumericValue(this) : InvalidPowerupValue;
}

// Returns the value of the max powerup attribute associated with the given tag, or -1 if not found
float UBmrPowerupsAttributeSet::GetPowerupMaxValueByTag(FBmrPowerupTag InTag) const
{
	const FGameplayAttribute PowerupMaxAttribute = GetPowerupMaxAttributeByTag(InTag);
	constexpr float InvalidPowerupValue = -1.f;
	return PowerupMaxAttribute.IsValid() ? PowerupMaxAttribute.GetNumericValue(this) : InvalidPowerupValue;
}

/*********************************************************************************************
 * OnRep notifies
 ********************************************************************************************* */

void UBmrPowerupsAttributeSet::OnRep_Powerup_Fire(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_Fire, OldValue);
}

void UBmrPowerupsAttributeSet::OnRep_Powerup_MaxFire(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_MaxFire, OldValue);
}

void UBmrPowerupsAttributeSet::OnRep_Powerup_Skate(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_Skate, OldValue);
}

void UBmrPowerupsAttributeSet::OnRep_Powerup_MaxSkate(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_MaxSkate, OldValue);
}

void UBmrPowerupsAttributeSet::OnRep_Powerup_BombsAvailable(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_BombsAvailable, OldValue);
}

void UBmrPowerupsAttributeSet::OnRep_Powerup_MaxBombs(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Powerup_MaxBombs, OldValue);
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Returns properties that are replicated for the lifetime of the actor channel
void UBmrPowerupsAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.RepNotifyCondition = REPNOTIFY_Always;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_Fire, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_MaxFire, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_Skate, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_MaxSkate, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_BombsAvailable, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Powerup_MaxBombs, Params);
}

// Called just before any modification happens to an attribute
void UBmrPowerupsAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// E.g: if base attribute became larger than max, then clamp the base attribute to its max
	constexpr float MinPowerup = 0.f;
	if (Attribute == GetPowerup_FireAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinPowerup, GetPowerup_MaxFire());
	}
	else if (Attribute == GetPowerup_SkateAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinPowerup, GetPowerup_MaxSkate());
	}
	else if (Attribute == GetPowerup_BombsAvailableAttribute())
	{
		NewValue = FMath::Clamp(NewValue, MinPowerup, GetPowerup_MaxBombs());
	}
}

// Is overridden to reclamp after changing dynamic max attributes
void UBmrPowerupsAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null!"), __LINE__, __FUNCTION__);

	// E.g: if max attribute was dynamically decreased, so base attribute became larger than max, then clamp the base attribute to new max
	if (Attribute == GetPowerup_MaxFireAttribute()
	    && GetPowerup_Fire() > NewValue)
	{
		ASC->ApplyModToAttribute(GetPowerup_FireAttribute(), EGameplayModOp::Override, NewValue);
	}
	else if (Attribute == GetPowerup_MaxSkateAttribute()
	         && GetPowerup_Skate() > NewValue)
	{
		ASC->ApplyModToAttribute(GetPowerup_SkateAttribute(), EGameplayModOp::Override, NewValue);
	}
	else if (Attribute == GetPowerup_MaxBombsAttribute()
	         && GetPowerup_BombsAvailable() > NewValue)
	{
		ASC->ApplyModToAttribute(GetPowerup_BombsAvailableAttribute(), EGameplayModOp::Override, NewValue);
	}
}