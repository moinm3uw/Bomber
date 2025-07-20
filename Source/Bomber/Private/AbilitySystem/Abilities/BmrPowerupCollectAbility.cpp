// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Abilities/BmrPowerupCollectAbility.h"
//---
#include "GeneratedMap.h"
#include "Components/MapComponent.h"
#include "DataAssets/ItemDataAsset.h"
#include "Structures/BmrPowerupTag.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "AbilitySystemComponent.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupCollectAbility)

// Actually activate ability, do not call this directly
void UBmrPowerupCollectAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	check(ActorInfo && TriggerEventData);

	// Destroy powerup actor on overlapping
	UMapComponent* InstigatorMapComponent = UMapComponent::GetMapComponent(TriggerEventData->Instigator);
	AGeneratedMap::Get().DestroyLevelActor(InstigatorMapComponent, ActorInfo->AvatarActor.Get());

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null!"), __LINE__, __FUNCTION__);

	// Apply the collect gameplay effect to increase own attribute
	const FBmrPowerupTag PowerupTag = TriggerEventData->InstigatorTags.GetByIndex(0);
	const UItemRow* ItemRow = UItemDataAsset::Get().GetRowByItemType(PowerupTag, UMyBlueprintFunctionLibrary::GetLevelType());
	const TSubclassOf<UGameplayEffect> CollectGameplayEffect = ItemRow ? ItemRow->CollectGameplayEffect : nullptr;
	ensureMsgf(CollectGameplayEffect, TEXT("ASSERT: [%i] %hs:\n'CollectGameplayEffect' failed to obtain!"), __LINE__, __FUNCTION__);
	const FGameplayEffectSpecHandle CollectSpecHandle = ASC->MakeOutgoingSpec(CollectGameplayEffect, GetAbilityLevel(), ASC->MakeEffectContext());
	if (const FGameplayEffectSpec* CollectSpec = CollectSpecHandle.Data.Get())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*CollectSpec);
	}

	K2_EndAbilityLocally();
}