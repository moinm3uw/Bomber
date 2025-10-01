// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Abilities/BmrPlayerDeathAbility.h"

// Bomber
#include "Components/MapComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "DataAssets/PlayerDataAsset.h"
#include "GeneratedMap.h"
#include "LevelActors/PlayerCharacter.h"

// UE
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPlayerDeathAbility)

// Actually activate ability, do not call this directly
void UBmrPlayerDeathAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	check(ActorInfo && TriggerEventData);

	DeathCauserInternal = const_cast<AActor*>(TriggerEventData->Instigator.Get());

	if (APlayerController* PlayerController = ActorInfo->PlayerController.Get())
	{
		PlayerController->SetIgnoreMoveInput(true);
	}

	const APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());
	const UPlayerRow* PlayerRow = PlayerCharacter ? UPlayerDataAsset::Get().GetRowByPlayerTag(PlayerCharacter->GetPlayerTag()) : nullptr;
	UAnimMontage* DeathMontage = PlayerRow ? PlayerRow->DeathMontage : nullptr;
	if (ensureMsgf(DeathMontage, TEXT("ASSERT: [%i] %hs:\n'DeathMontage' failed to play!"), __LINE__, __FUNCTION__))
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DeathMontage);
		MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageEnd);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageEnd);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageEnd);
		MontageTask->ReadyForActivation();
	}
	else
	{
		K2_EndAbilityLocally();
	}
}

// Called if an ability ends normally or abnormally
void UBmrPlayerDeathAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	AGeneratedMap::Get().DestroyLevelActor(UMapComponent::GetMapComponent(ActorInfo->AvatarActor.Get()), DeathCauserInternal);
	DeathCauserInternal = nullptr;
}

// Called when the death montage completed or interrupted
void UBmrPlayerDeathAbility::OnMontageEnd_Implementation()
{
	K2_EndAbilityLocally();
}