// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
//---
#include "AbilitySystemGlobals.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupsAttributeSet)

// Returns the powerups attribute set for the specified owner. It will return nullptr if can't be obtained
const UBmrPowerupsAttributeSet* UBmrPowerupsAttributeSet::GetPowerupsAttributeSet(const UObject* InOwner)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<AActor>(InOwner));
	const UAttributeSet* AttributeSet = ASC ? ASC->GetAttributeSet(StaticClass()) : nullptr;
	return Cast<UBmrPowerupsAttributeSet>(AttributeSet);
}

// Returns the powerups attribute set for the specified owner. It will crash if can't be obtained
const UBmrPowerupsAttributeSet& UBmrPowerupsAttributeSet::Get(const UObject& InOwner)
{
	const UBmrPowerupsAttributeSet* PowerupsAttributeSet = GetPowerupsAttributeSet(&InOwner);
	checkf(PowerupsAttributeSet, TEXT("ERROR: [%i] %hs:\n'PowerupsAttributeSet' is null!"), __LINE__, __FUNCTION__);
	return *PowerupsAttributeSet;
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

// Called just after a GameplayEffect is executed to modify the base value of an attribute. No more changes can be made
void UBmrPowerupsAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	constexpr float MinPowerup = 0.f;
	if (Data.EvaluatedData.Attribute == GetPowerup_FireAttribute())
	{
		SetPowerup_Fire(FMath::Clamp(GetPowerup_Fire(), MinPowerup, GetPowerup_MaxFire()));
	}
	else if (Data.EvaluatedData.Attribute == GetPowerup_SkateAttribute())
	{
		SetPowerup_Skate(FMath::Clamp(GetPowerup_Skate(), MinPowerup, GetPowerup_MaxSkate()));
	}
	else if (Data.EvaluatedData.Attribute == GetPowerup_BombsAvailableAttribute())
	{
		SetPowerup_BombsAvailable(FMath::Clamp(GetPowerup_BombsAvailable(), MinPowerup, GetPowerup_MaxBombs()));
	}
}