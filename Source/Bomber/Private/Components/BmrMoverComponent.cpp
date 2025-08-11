// Copyright (c) Yevhenii Selivanov

#include "Components/BmrMoverComponent.h"
//---
#include "Bomber.h"
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Components/MapComponent.h"
#include "GameFramework/MyGameStateBase.h"
#include "LevelActors/PlayerCharacter.h"
#include "Structures/BmrMoverSyncState.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
//---
#include "AbilitySystemGlobals.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/InstantMovementEffects/BasicInstantMovementEffects.h"
#include "GameFramework/PlayerController.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrMoverComponent)

// Moves owner in given direction
void UBmrMoverComponent::RequestMoveByIntent(const FVector& Direction)
{
	CurrentMoveInputInternal = Direction;
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Called when the game starts
void UBmrMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	BIND_ON_CHARACTER_READY_PTR(this, ThisClass::OnCharacterReady, GetOwner<APlayerCharacter>());

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);

	UMapComponent* MapComponent = UMapComponent::GetMapComponent(GetOwner());
	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is null!"), __LINE__, __FUNCTION__);
	MapComponent->OnAddedToLevel.AddUniqueDynamic(this, &ThisClass::OnOwnerAddedToLevel);
	MapComponent->OnPostRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPostRemovedFromLevel);

	OnPostMovement.AddUniqueDynamic(this, &ThisClass::OnPostMove);
}

// Consumes cached data (inputs and states) to be processed by other systems such as movement modes
void UBmrMoverComponent::ProduceInput(const int32 DeltaTimeMS, FMoverInputCmdContext* Cmd)
{
	Super::ProduceInput(DeltaTimeMS, Cmd);

	// --- Reference: AMoverExamplesCharacter::OnProduceInput
	// Generate user commands. Called right before the Character movement simulation will tick (for a locally controlled pawn)
	// This code is happening outside of the Character movement simulation. All we are doing
	// is generating the input being fed into that simulation. That said, this means that A) the code below does not run on the server
	// (and non controlling clients) and B) the code is not rerun during reconcile/resimulates.

	const APawn* InOwnerPawn = GetOwner<APawn>();
	FCharacterDefaultInputs& CharacterInputs = Cmd->InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	const AController* OwnedController = InOwnerPawn ? InOwnerPawn->GetController() : nullptr;
	if (!OwnedController)
	{
		if (GetOwnerRole() == ROLE_Authority
		    && GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
		{
			static const FCharacterDefaultInputs DoNothingInput;
			// If we get here, that means this pawn is not currently possessed and we're choosing to provide default do-nothing input
			CharacterInputs = DoNothingInput;
		}

		// We don't have a local controller so we can't run the code below. This is ok. Simulated proxies will just use previous input when extrapolating
		return;
	}

	// Setup control rotation
	CharacterInputs.ControlRotation = FRotator::ZeroRotator;
	const APlayerController* PC = Cast<APlayerController>(OwnedController);
	if (PC)
	{
		CharacterInputs.ControlRotation = PC->GetControlRotation();
	}

	// Setup movement input
	const FRotator LevelGridRotation = UCellsUtilsLibrary::GetLevelGridRotation();
	const FVector FinalDirectionalIntent = LevelGridRotation.RotateVector(CurrentMoveInputInternal);
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, FinalDirectionalIntent);
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Is called when this character is ready to be used
void UBmrMoverComponent::OnCharacterReady_Implementation(APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	checkf(PlayerCharacter == GetOwner(), TEXT("ERROR: [%i] %hs:\n'PlayerCharacter' is not the same as Owner!"), __LINE__, __FUNCTION__);

	// Setup powerups
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerCharacter);
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null, make sure Owner implements ability system interface!"), __LINE__, __FUNCTION__);
	FOnGameplayAttributeValueChange& SkateAttributeDelegate = ASC->GetGameplayAttributeValueChangeDelegate(UBmrPowerupsAttributeSet::GetPowerup_SkateAttribute());
	if (!SkateAttributeDelegate.IsBoundToObject(this))
	{
		// Entered in menu for first time, so bind to the Skate attribute change
		SkateAttributeDelegate.AddUObject(this, &ThisClass::OnSkateAttributeChanged);
		CachedSkatePowerupAttributeInternal = UBmrPowerupsAttributeSet::Get(ASC).GetPowerup_Skate();
	}
}

// Listen to react when entered to different game state
void UBmrMoverComponent::OnGameStateChanged(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
		case ECGS::Menu: // fallthrough
		case ECGS::GameStarting:
		{
			RequestMoveByIntent(FVector::ZeroVector);
			break;
		}

		default:
			break;
	}
}

// Called when owner is added on the Generated Map, on both server and client
void UBmrMoverComponent::OnOwnerAddedToLevel_Implementation(UMapComponent* MapComponent)
{
	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is null!"), __LINE__, __FUNCTION__);;

	// Owner is respawned, teleport it to initial restart location
	const TSharedPtr<FTeleportEffect> TeleportEffect = MakeShared<FTeleportEffect>();
	const UCapsuleComponent* CapsuleComponent = GetUpdatedComponent<UCapsuleComponent>();
	const float CapsuleHalfHeight = CapsuleComponent ? CapsuleComponent->GetScaledCapsuleHalfHeight() : 0.f;
	const FVector CapsuleOffset = FVector::UpVector * CapsuleHalfHeight;
	TeleportEffect->TargetLocation = FVector(MapComponent->GetCell()) + CapsuleOffset;
	TeleportEffect->bUseActorRotation = false;
	TeleportEffect->TargetRotation = FRotator::ZeroRotator;
	QueueInstantMovementEffect(TeleportEffect);
}

// Called when owner is destroyed on the Generated Map
void UBmrMoverComponent::OnPostRemovedFromLevel_Implementation(UMapComponent* MapComponent, UObject* DestroyCauser)
{
	RequestMoveByIntent(FVector::ZeroVector);
}

// Broadcast at the end of a simulation tick after movement has occurred, but allowing additions/modifications to the state
void UBmrMoverComponent::OnPostMove_Implementation(const FMoverTimeStep& TimeStep, FMoverSyncState& SyncState, FMoverAuxStateContext& AuxState)
{
	// Add powerup state to SyncState
	FBmrMoverSyncState& BmrSyncStateRef = SyncState.SyncStateCollection.FindOrAddMutableDataByType<FBmrMoverSyncState>();
	BmrSyncStateRef.SkatePowerupAttribute = CachedSkatePowerupAttributeInternal;
}

// Is called by Move Input Action when player pressed the move input button, e.g: WASD or Arrow keys
void UBmrMoverComponent::OnMoveInputTriggered_Implementation(const FInputActionValue& ActionValue)
{
	const APawn* InOwnerPawn = GetOwner<APawn>();
	if (!InOwnerPawn || InOwnerPawn->IsMoveInputIgnored())
	{
		return;
	}

	RequestMoveByIntent(ActionValue.Get<FVector>());
}

// Is called by Move Input Action when player released the move input button, e.g: WASD or Arrow keys
void UBmrMoverComponent::OnMoveInputCompleted_Implementation(const FInputActionValue& ActionValue)
{
	RequestMoveByIntent(FVector::ZeroVector);
}

// Is called when the Skate attribute is changed, e.g: when player picked up a Skate item
void UBmrMoverComponent::OnSkateAttributeChanged(const FOnAttributeChangeData& OnAttributeChangeData)
{
	CachedSkatePowerupAttributeInternal = OnAttributeChangeData.NewValue;
}