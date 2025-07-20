// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Abilities/BmrPowerupCollect.h"
//---
#include "DataAssets/ItemDataAsset.h"
#include "Structures/BmrPowerupTag.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "AbilitySystemComponent.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupCollect)

// Actually activate ability, do not call this directly
void UBmrPowerupCollect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null!"), __LINE__, __FUNCTION__);

	// Apply the collect gameplay effect to increase own attribute
	const FBmrPowerupTag PowerupTag = TriggerEventData ? TriggerEventData->InstigatorTags.GetByIndex(0) : FGameplayTag::EmptyTag;
	const UItemRow* ItemRow = UItemDataAsset::Get().GetRowByItemType(PowerupTag, UMyBlueprintFunctionLibrary::GetLevelType());
	const TSubclassOf<UGameplayEffect> CollectGameplayEffect = ItemRow ? ItemRow->CollectGameplayEffect : nullptr;
	ensureMsgf(CollectGameplayEffect, TEXT("ASSERT: [%i] %hs:\n'CollectGameplayEffect' failed to obtain!"), __LINE__, __FUNCTION__);
	const FGameplayEffectSpecHandle CollectSpecHandle = ASC->MakeOutgoingSpec(CollectGameplayEffect, GetAbilityLevel(), ASC->MakeEffectContext());
	if (const FGameplayEffectSpec* CollectSpec = CollectSpecHandle.Data.Get())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*CollectSpec, ActivationInfo.GetActivationPredictionKey());
	}

	K2_EndAbilityLocally();
}