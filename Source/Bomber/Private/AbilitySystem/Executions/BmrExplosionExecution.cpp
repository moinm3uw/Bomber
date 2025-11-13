// Copyright (c) Yevhenii Selivanov

#include "AbilitySystem/Executions/BmrExplosionExecution.h"

// Bomber
#include "AbilitySystem/Attributes/BmrHealthAttributeSet.h"
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Bomber.h"
#include "Components/MapComponent.h"
#include "DataAssets/BombDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "GeneratedMap.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/LevelActorsUtilsLibrary.h"

// UE
#include "AbilitySystemGlobals.h"
#include "Structures/BmrGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrExplosionExecution)

// Capture definitions for damage attributes
struct FBmrExplosionStatics
{
	FGameplayEffectAttributeCaptureDefinition OutcomingDamageDef;

	FBmrExplosionStatics()
	{
		OutcomingDamageDef = FGameplayEffectAttributeCaptureDefinition(UBmrHealthAttributeSet::GetOutcomingDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FBmrExplosionStatics& ExplosionStatics()
{
	static FBmrExplosionStatics Statics;
	return Statics;
}

// Sets default capture
UBmrExplosionExecution::UBmrExplosionExecution()
{
	RelevantAttributesToCapture.Add(ExplosionStatics().OutcomingDamageDef);
}

// Executes explosion with chain reaction across all affected cells
void UBmrExplosionExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	TSubclassOf<UGameplayEffect> ExplosionDamageEffect = UBombDataAsset::Get().GetExplosionDamageEffect();
	if (!SourceASC || !SourceASC->GetOwner()->HasAuthority()
	    || !(TO_FLAG(AMyGameStateBase::GetCurrentGameState()) & TO_FLAG(ECGS::InGame | ECGS::EndGame))
	    || !ensureMsgf(ExplosionDamageEffect, TEXT("ASSERT: [%i] %hs:\n'ExplosionDamageEffect' is not set, can not apply explosion damage!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(Spec.GetEffectContext().HasOrigin(), TEXT("ASSERT: [%i] %hs:\n'Origin' cell location is not set in effect context, can not apply explosion damage!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const FCell OriginCell = Spec.GetEffectContext().GetOrigin();
	if (!UCellsUtilsLibrary::IsCellHasAnyMatchingActor(OriginCell, TO_FLAG(EAT::Bomb)))
	{
		// No bomb at origin, which is likely already exploded by another chain reaction
		return;
	}

	const UBmrPowerupsAttributeSet* PowerupsAttributeSet = UBmrPowerupsAttributeSet::GetPowerupsAttributeSet(SourceASC);
	const int32 FireRadius = PowerupsAttributeSet ? PowerupsAttributeSet->GetPowerup_Fire() : INDEX_NONE;
	if (!ensureMsgf(FireRadius > 0, TEXT("ASSERT: [%i] %hs:\n'FireRadius' is less than 1!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Capture outcoming damage from source
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	float OutcomingDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ExplosionStatics().OutcomingDamageDef, EvaluateParameters, OutcomingDamage);
	const float DamageDone = FMath::Max(OutcomingDamage, 0.f);
	if (DamageDone <= 0.f)
	{
		return;
	}

	// Apply damage to all explosion cells
	FMapComponents TargetMapComponents;
	const FCells ExplosionCells = UCellsUtilsLibrary::GetCellsAround(OriginCell, EPathType::Explosion, FireRadius);
	ULevelActorsUtilsLibrary::GetLevelActorsOnCells(TargetMapComponents, ExplosionCells);
	for (UMapComponent* TargetMapComponent : TargetMapComponents)
	{
		AActor* TargetActor = TargetMapComponent ? TargetMapComponent->GetOwner() : nullptr;
		if (!TargetActor)
		{
			continue;
		}

		// Remove bomb effects from chained bombs: this way their effects interrupt immediately, causing this execution and chain reaction
		const FCell& TargetCellIt = TargetMapComponent->GetCell();
		const bool bIsChainedBomb = TargetCellIt != OriginCell && TargetMapComponent->GetActorType() == EAT::Bomb;
		if (bIsChainedBomb)
		{
			UAbilitySystemComponent* BombInstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor->GetInstigator());
			if (!BombInstigatorASC)
			{
				continue;
			}

			FGameplayEffectQuery Query;
			Query.CustomMatchDelegate.BindLambda([&TargetCellIt](const FActiveGameplayEffect& ActiveEffect)
			{
				return ActiveEffect.Spec.GetContext().GetOrigin() == TargetCellIt.Location; // The same cell in context
			});
			BombInstigatorASC->RemoveActiveEffects(Query);
		}

		// If actor does not have ASC, destroy it directly (e.g. boxes)
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		if (!TargetASC)
		{
			AGeneratedMap::Get().DestroyLevelActor(TargetMapComponent, SourceASC->GetAvatarActor());
			continue;
		}

		// Actor has ASC: apply damage through GAS (e.g. pawns)
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(ExplosionDamageEffect, /*Level*/ 1.f, SourceASC->MakeEffectContext());
		if (FGameplayEffectSpec* SpecPtr = SpecHandle.Data.Get())
		{
			SpecPtr->SetSetByCallerMagnitude(BmrGameplayTags::SetByCaller::Bomb_Damage, DamageDone);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecPtr, TargetASC);
		}
	}
}