// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Abilities/BmrPowerupCollectAbility.h"

// Bomber
#include "Components/MapComponent.h"
#include "DataAssets/ItemDataAsset.h"
#include "GeneratedMap.h"
#include "LevelActors/ItemActor.h"
#include "Structures/BmrPowerupTag.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

// UE
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPowerupCollectAbility)

// Actually activate ability, do not call this directly
void UBmrPowerupCollectAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	check(ActorInfo && TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null!"), __LINE__, __FUNCTION__);
	const AItemActor& ItemActor = *CastChecked<AItemActor>(TriggerEventData->Instigator);

	// Apply the collect gameplay effect to increase own attribute
	const FBmrPowerupTag PowerupTag = TriggerEventData->InstigatorTags.GetByIndex(0);
	const UItemRow* ItemRow = UItemDataAsset::Get().GetRowByItemType(PowerupTag, UMyBlueprintFunctionLibrary::GetLevelType());
	const TSubclassOf<UGameplayEffect> CollectGameplayEffect = ItemRow ? ItemRow->CollectGameplayEffect : nullptr;
	if (ensureMsgf(CollectGameplayEffect, TEXT("ASSERT: [%i] %hs:\n'CollectGameplayEffect' failed to obtain!"), __LINE__, __FUNCTION__))
	{
		FGameplayEffectContextHandle CollectContext = ASC->MakeEffectContext();
		CollectContext.AddSourceObject(TriggerEventData->Instigator);
		const FPredictionKey PredictionKey = ASC->GetPredictionKeyForNewAction();
		ASC->ApplyGameplayEffectToSelf(CollectGameplayEffect.GetDefaultObject(), GetAbilityLevel(), CollectContext, PredictionKey);
	}

	// @TODO JanSeliv uL3AzYIa - BEGIN: remove next once provided support for predicted destroy pooled actors
	if (!ItemActor.HasAuthority())
	{
		const_cast<AItemActor&>(ItemActor).SetActorHiddenInGame(true);
	}
	// @TODO JanSeliv uL3AzYIa - END
	else
	{
		// Finally, destroy powerup actor at the end
		UMapComponent* InstigatorMapComponent = UMapComponent::GetMapComponent(&ItemActor);
		AGeneratedMap::Get().DestroyLevelActor(InstigatorMapComponent, ActorInfo->AvatarActor.Get());
	}

	K2_EndAbilityLocally();
}