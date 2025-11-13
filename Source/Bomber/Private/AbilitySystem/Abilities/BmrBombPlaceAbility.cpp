// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Abilities/BmrBombPlaceAbility.h"

// Bomber
#include "Bomber.h"
#include "Components/MapComponent.h"
#include "GeneratedMap.h"
#include "LevelActors/BombActor.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"

// UE
#include "AbilitySystemComponent.h"
#include "DataAssets/BombDataAsset.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrBombPlaceAbility)

// Is overridden to prevent event-based activation if we bomb cannot be placed at the specified cell
bool UBmrBombPlaceAbility::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* TriggerEventData) const
{
	if (!Super::ShouldAbilityRespondToEvent(ActorInfo, TriggerEventData))
	{
		return false;
	}

	const FCell SpawnCell = TriggerEventData ? UCellsUtilsLibrary::GetCellByIndexOnLevel(TriggerEventData->EventMagnitude) : FCell::InvalidCell;
	if (!ensureMsgf(SpawnCell.IsValid(), TEXT("ASSERT: [%i] %hs:\n'SpawnCell' is not passed to event magnitude, can not spawn bomb!"), __LINE__, __FUNCTION__)
	    || UCellsUtilsLibrary::IsCellHasAnyMatchingActor(SpawnCell, TO_FLAG(~EAT::Player)))
	{
		// Cell is occupied by other level actors (except players), cannot place bomb here
		return false;
	}

	return true;
}

// Actually activate ability, do not call this directly
void UBmrBombPlaceAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	check(ActorInfo && TriggerEventData);

	CommitAbility(Handle, ActorInfo, ActivationInfo);

	const FCell SpawnCell = UCellsUtilsLibrary::GetCellByIndexOnLevel(TriggerEventData->EventMagnitude);

	// Start bomb timer, providing own cell location in context
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	FGameplayEffectContextHandle EffectContext = ASC ? ASC->MakeEffectContext() : FGameplayEffectContextHandle();
	EffectContext.AddOrigin(SpawnCell);
	ApplyDurationalEffect(UBombDataAsset::Get().GetDurationGameplayEffect(), EffectContext, *ActorInfo, ActivationInfo);

	const TWeakObjectPtr InInstigator = const_cast<AActor*>(TriggerEventData->Instigator.Get());
	const TFunction<void(UMapComponent&)> OnBombSpawned = [WeakThis = TWeakObjectPtr(this), InInstigator](const UMapComponent& MapComponent)
	{
		UBmrBombPlaceAbility* This = WeakThis.Get();
		if (!This)
		{
			return;
		}

		ABombActor* BombActor = CastChecked<ABombActor>(MapComponent.GetOwner());
		BombActor->InitBomb(Cast<APawn>(InInstigator.Get()));

		This->K2_EndAbility();
	};

	// Spawn bomb
	AGeneratedMap::Get().SpawnActorByType(EAT::Bomb, SpawnCell, OnBombSpawned);
}

// Is overridden to apply cost with set by caller tag for bomb duration
void UBmrBombPlaceAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Super is intentionally not called to apply custom cost

	if (const UGameplayEffect* CostGE = GetCostGameplayEffect())
	{
		check(ActorInfo);
		ApplyDurationalEffect(CostGE->GetClass(), FGameplayEffectContextHandle(), *ActorInfo, ActivationInfo);
	}
}

// Applies given gameplay effect with bomb duration
FActiveGameplayEffectHandle UBmrBombPlaceAbility::ApplyDurationalEffect(const TSubclassOf<UGameplayEffect> GameplayEffect, const FGameplayEffectContextHandle& EffectContext, const FGameplayAbilityActorInfo& ActorInfo, const FGameplayAbilityActivationInfo& ActivationInfo)
{
	UAbilitySystemComponent* ASC = ActorInfo.AbilitySystemComponent.Get();
	if (!ASC || !ASC->HasAuthorityOrPredictionKey(&ActivationInfo)
	    || !ensureMsgf(GameplayEffect, TEXT("ASSERT: [%i] %hs:\n'GameplayEffect' is null!"), __LINE__, __FUNCTION__))
	{
		return FActiveGameplayEffectHandle();
	}

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GameplayEffect, /*Level*/ 1.f, EffectContext);
	FGameplayEffectSpec* DurationSpec = SpecHandle.Data.Get();
	if (!ensureMsgf(DurationSpec, TEXT("ASSERT: [%i] %hs:\n'DurationSpec' is not valid!"), __LINE__, __FUNCTION__))
	{
		return FActiveGameplayEffectHandle();
	}

	// Set duration
	float BombDuration = UBombDataAsset::Get().GetDuration();
	if (ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Authority
	    && !ActorInfo.IsLocallyControlled())
	{
		// Apply lag compensation on server side, so bombs on clients will detonate at the same time as on server
		const APlayerState* Owner = ASC->GetOwner<APlayerState>();
		checkf(Owner, TEXT("ERROR: [%i] %hs:\n'Owner' is null!"), __LINE__, __FUNCTION__);
		constexpr float MaxCompensatedPing = 0.5f; // 500 ms
		const float PlayerPing = Owner->GetPingInMilliseconds() * 0.001f;
		BombDuration = FMath::Max(MaxCompensatedPing, BombDuration - PlayerPing);
	}
	DurationSpec->SetSetByCallerMagnitude(BmrGameplayTags::SetByCaller::Bomb_Duration, BombDuration);

	// Apply effect itself
	const FPredictionKey PredictionKey = ASC->GetPredictionKeyForNewAction();
	return ASC->ApplyGameplayEffectSpecToSelf(*DurationSpec, PredictionKey);
}
