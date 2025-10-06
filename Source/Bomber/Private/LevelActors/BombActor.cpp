// Copyright (c) Yevhenii Selivanov.

#include "LevelActors/BombActor.h"

// Bomber
#include "AbilitySystem/Attributes/BmrPowerupsAttributeSet.h"
#include "Bomber.h"
#include "Components/MapComponent.h"
#include "DataAssets/BombDataAsset.h"
#include "GameFramework/MyGameStateBase.h"
#include "GeneratedMap.h"
#include "Structures/BmrGameplayTags.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/LevelActorsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"

#if WITH_EDITOR
#include "MyEditorUtilsLibraries/EditorUtilsLibrary.h"
#include "MyUnrealEdEngine.h"
#endif

// UE
#include "AbilitySystemGlobals.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BombActor)

// Sets default values
ABombActor::ABombActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an actor
	bReplicates = true;
	static constexpr float NewNewUpdateFrequency = 10.f;
	SetNetUpdateFrequency(NewNewUpdateFrequency);
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	// Initialize Root Component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));

	// Initialize MapComponent
	MapComponentInternal = CreateDefaultSubobject<UMapComponent>(TEXT("MapComponent"));
}

/*********************************************************************************************
 * Detonation
 ********************************************************************************************* */

// Initiates the explosion: starts countdown and initializes the data (fire radius, explosion cells, etc.)
void ABombActor::InitBomb(APawn* OptionalInstigator /* = nullptr*/)
{
	SetInstigator(OptionalInstigator);

	UpdateExplosionCells();

	InitCollisionResponseToAllPlayers();

	ApplyMesh();

	ApplyMaterial();
}

// If set, the bomb will detonate when this effect is removed
void ABombActor::SetActiveDurationEffectHandle(const FActiveGameplayEffectHandle& InHandle)
{
	if (!HasAuthority()
	    || !ensureMsgf(InHandle.IsValid(), TEXT("ASSERT: [%i] %hs:\n'InHandle' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* OnEffectRemoved = GetInstigatorAbilityComponentChecked().OnGameplayEffectRemoved_InfoDelegate(InHandle);
	if (!ensureMsgf(OnEffectRemoved, TEXT("ASSERT: [%i] %hs:\n'OnEffectRemoved' is not found, can not listen its removal!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	OnEffectRemoved->AddWeakLambda(this, [this](const FGameplayEffectRemovalInfo& RemovalInfo)
	{
		AGeneratedMap::Get().DestroyLevelActor(MapComponentInternal, this);
	});

	AppliedDurationEffectInternal = InHandle;
}

// Clears the active duration effect handle as part of cleanup process
void ABombActor::ClearActiveDurationEffectHandle()
{
	if (!HasAuthority()
	    || !AppliedDurationEffectInternal.IsValid())
	{
		// Is already cleared
		return;
	}

	FOnActiveGameplayEffectRemoved_Info* OnEffectRemoved = GetInstigatorAbilityComponentChecked().OnGameplayEffectRemoved_InfoDelegate(AppliedDurationEffectInternal);
	if (OnEffectRemoved)
	{
		OnEffectRemoved->RemoveAll(this);
	}

	AppliedDurationEffectInternal.Invalidate();
}

// Returns explosion radius from instigator, or -1 if can not be obtained
int32 ABombActor::GetFireRadius() const
{
	const UBmrPowerupsAttributeSet* PowerupsAttributeSet = UBmrPowerupsAttributeSet::GetPowerupsAttributeSet(GetInstigator());
	return PowerupsAttributeSet ? PowerupsAttributeSet->GetPowerup_Fire() : INDEX_NONE;
}

// Show current explosion cells if the bomb type is allowed to be displayed, is not available in shipping build
void ABombActor::TryDisplayExplosionCells()
{
#if !UE_BUILD_SHIPPING
	FDisplayCellsParams Params = FDisplayCellsParams::EmptyParams;
	Params.bClearPreviousDisplays = true;
	Params.TextColor = FLinearColor::Yellow;
	Params.TextSize += 50.f;
	Params.TextHeight += 1.f;
	UCellsUtilsLibrary::DisplayCells(this, LocalExplosionCellsInternal, Params);
#endif // !UE_BUILD_SHIPPING
}

// Destroy bomb and burst explosion cells, calls multicast event
void ABombActor::DetonateBomb()
{
	if (!HasAuthority()
	    || IsHidden()
	    || AMyGameStateBase::GetCurrentGameState() != ECGS::InGame)
	{
		return;
	}

	if (!ensureMsgf(!LocalExplosionCellsInternal.IsEmpty(), TEXT("ASSERT: [%i] %hs:\n'LocalExplosionCellsInternal' is empty!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	// Make sure cells are up-to-date
	UpdateExplosionCells();

	PlayExplosionsCue();

	FMapComponents TargetMapComponents;
	ULevelActorsUtilsLibrary::GetLevelActorsOnCells(/*out*/ TargetMapComponents, LocalExplosionCellsInternal);

	for (UMapComponent* TargetMapComponent : TargetMapComponents)
	{
		const AActor* TargetActor = TargetMapComponent ? TargetMapComponent->GetOwner() : nullptr;
		if (!TargetActor
		    || TargetActor == this)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
		if (!TargetASC)
		{
			AGeneratedMap::Get().DestroyLevelActor(TargetMapComponent, this);
			continue;
		}

		// Actor has ASC: apply damage through GAS
		UAbilitySystemComponent& InstigatorASC = GetInstigatorAbilityComponentChecked();
		FGameplayEffectContextHandle EffectContext = InstigatorASC.MakeEffectContext();
		EffectContext.AddInstigator(GetInstigator(), this);
		const FGameplayEffectSpecHandle DamageSpecHandle = InstigatorASC.MakeOutgoingSpec(UBombDataAsset::Get().GetExplosionDamageEffect(), 1.f, EffectContext);
		if (const FGameplayEffectSpec* DamageSpec = DamageSpecHandle.Data.Get())
		{
			InstigatorASC.ApplyGameplayEffectSpecToTarget(*DamageSpec, TargetASC);
		}
	}
}

// Calculates the explosion cells based on current fire radius
void ABombActor::UpdateExplosionCells()
{
	const int32 FireRadius = GetFireRadius();
	if (FireRadius <= 0
	    || !MapComponentInternal
	    || MapComponentInternal->GetCell().IsInvalidCell())
	{
		// On client some data might be not replicated yet
		return;
	}

	const FCells NewExplosionCells = UCellsUtilsLibrary::GetCellsAround(MapComponentInternal->GetCell(), EPathType::Explosion, FireRadius);

	const bool bIsAlreadySetByDefault = !LocalExplosionCellsInternal.IsEmpty() && FireRadius <= 1 && NewExplosionCells.Num() == LocalExplosionCellsInternal.Num();
	if (bIsAlreadySetByDefault)
	{
		return;
	}

	LocalExplosionCellsInternal = NewExplosionCells;

	TryDisplayExplosionCells();
}

/*********************************************************************************************
 * Cue Visuals: VFXs, SFXs, Materials
 ********************************************************************************************* */

// Spawns VFXs and SFXs, is allowed to call both on server and clients
void ABombActor::PlayExplosionsCue()
{
	if (!(TO_FLAG(AMyGameStateBase::GetCurrentGameState()) & TO_FLAG(ECGS::InGame | ECGS::EndGame))
	    || !ensureMsgf(!LocalExplosionCellsInternal.IsEmpty(), TEXT("ASSERT: [%i] %hs:\n'LocalExplosionCellsInternal' is empty!"), __LINE__, __FUNCTION__))
	{
		// Play cue only during the match (InGame or EndGame states) and if explosion cells are valid
		return;
	}

	FGameplayCueParameters Parameters;
	Parameters.Instigator = this;
	GetInstigatorAbilityComponentChecked().InvokeGameplayCueEvent(BmrGameplayTags::GameplayCue::Bomb_Explosion, EGameplayCueEvent::Executed, Parameters);
}

// Updates current mesh for this bomb actor, based on instigator type, or randomly if no instigator
void ABombActor::ApplyMesh()
{
	// If it has instigator with map component, then associate its mesh with bomb mesh, e.g when each character has own bomb
	// Otherwise just apply random mesh from data asset
	const UMapComponent* InstigatorMapComponent = UMapComponent::GetMapComponent(GetInstigator());
	const ELevelType FinalMeshType = InstigatorMapComponent
	                                     ? InstigatorMapComponent->GetLevelType()
	                                     : TO_ENUM(ELevelType, ELT_FIRST_FLAG << FMath::RandRange(0, FMath::FloorLog2(ELT_LAST_FLAG)));

	const ULevelActorRow* BombRow = UBombDataAsset::Get().GetRowByLevelType(FinalMeshType);
	if (!ensureMsgf(BombRow, TEXT("ASSERT: [%i] %hs:\n'BombRow' is not valid, can not apply mesh!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	MapComponentInternal->SetLocalMesh(BombRow->Mesh);
}

// Updates current material for this bomb actor, based on this bomb and Player placer types
void ABombActor::ApplyMaterial()
{
	TObjectPtr<UMaterialInterface> NewBombMaterial = nullptr;

	// If bot character, override material with the player type
	const APawn* OwnerCharacter = GetInstigator<APawn>();
	const APlayerState* OwnerPlayerState = OwnerCharacter ? OwnerCharacter->GetPlayerState<APlayerState>() : nullptr;
	if (OwnerPlayerState
	    && OwnerCharacter->IsBotControlled())
	{
		// If bot character, set material for its default bomb with the same mesh
		const int32 PlayerIndex = OwnerPlayerState->GetPlayerId();
		const UBombDataAsset& BombDataAsset = UBombDataAsset::Get();
		const int32 BombMaterialsNum = BombDataAsset.GetBombMaterialsNum();
		if (PlayerIndex != INDEX_NONE // Is not debug character
		    && BombMaterialsNum) // As least one bomb material
		{
			const int32 MaterialIndex = FMath::Abs(PlayerIndex) % BombMaterialsNum;
			NewBombMaterial = BombDataAsset.GetBombMaterial(MaterialIndex);
		}
	}
	else
	{
		// Set material by bomb type (default)
		checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
		const UStaticMesh* BombMesh = MapComponentInternal->GetMesh<UStaticMesh>();
		if (ensureMsgf(BombMesh, TEXT("ASSERT: [%i] %hs:\n'BombMesh' is not found"), __LINE__, __FUNCTION__))
		{
			NewBombMaterial = BombMesh->GetMaterial(0);
		}
	}

	// Apply material
	if (NewBombMaterial)
	{
		checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
		MapComponentInternal->SetLocalMeshMaterial(NewBombMaterial);
	}
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Called when an instance of this class is placed (in editor) or spawned.
void ABombActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	MapComponentInternal->OnAddedToLevel.AddUniqueDynamic(this, &ThisClass::OnAddedToLevel);
	AGeneratedMap::Get().AddToGrid(MapComponentInternal);
}

// Returns the Ability System Component from the Instigator or local player if none
UAbilitySystemComponent* ABombActor::GetInstigatorAbilityComponent() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());
	if (!ASC)
	{
		// Instigator is optional for some type of bombs like environmental ones, so try to get local player ASC
		ASC = UMyBlueprintFunctionLibrary::GetLocalAbilitySystemComponent();
	}
	return ASC;
}

// Returns the Ability System Component from the Instigator or local player if none, will crash if can't be obtained
UAbilitySystemComponent& ABombActor::GetInstigatorAbilityComponentChecked() const
{
	UAbilitySystemComponent* ASC = GetInstigatorAbilityComponent();
	checkf(ASC, TEXT("ERROR: [%i] %hs:\n'ASC' is null!"), __LINE__, __FUNCTION__);
	return *ASC;
}

// Is overridden to init bomb on clients when instigator is replicated
void ABombActor::OnRep_Instigator()
{
	if (APawn* InInstigator = GetInstigator())
	{
		InitBomb(InInstigator);
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Called when this level actor is reconstructed or added on the Generated Map
void ABombActor::OnAddedToLevel_Implementation(UMapComponent* MapComponent)
{
	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is null!"), __LINE__, __FUNCTION__);

	if (!GetInstigator())
	{
		// This is likely environmental bomb or not fully replicated one
		// Init bomb by default, if it's spawned from player, it will be reinitialized again
		InitBomb();
	}

	// Listen when this bomb is destroyed on the Generated Map by itself or by other actors
	MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);

#if WITH_EDITOR //[IsEditorNotPieWorld]
	if (FEditorUtilsLibrary::IsEditorNotPieWorld()) // [IsEditorNotPieWorld]
	{
		UMyUnrealEdEngine::GOnAIUpdatedDelegate.Broadcast();
	}
#endif // WITH_EDITOR [IsEditorNotPieWorld]
}

// Called when this level actor is destroyed on the Generated Map
void ABombActor::OnPreRemovedFromLevel_Implementation(UMapComponent* MapComponent, UObject* DestroyCauser)
{
	if (HasAuthority())
	{
		// On server, bomb is removed from Generated Map, detonate it
		DetonateBomb();
	}
	else
	{
		// On client, play explosions cue locally
		UpdateExplosionCells();
		PlayExplosionsCue();
	}

	if (MapComponentInternal)
	{
		MapComponentInternal->OnPreRemovedFromLevel.RemoveAll(this);
		MapComponentInternal->OnCellChanged.RemoveAll(this);
	}

	ClearActiveDurationEffectHandle();

	SetInstigator(nullptr);

	LocalExplosionCellsInternal = FCell::EmptyCells;
}

// Is called when character leaves the bomb to update collision response
void ABombActor::OnPlayerCellChanged_Implementation(UMapComponent* PlayerMapComponent, const FCell& NewCell, const FCell& PreviousCell)
{
	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	const UBoxComponent* BoxCollisionComponent = MapComponentInternal->GetBoxCollisionComponent();
	checkf(BoxCollisionComponent, TEXT("ERROR: [%i] %hs:\n'BoxCollisionComponent' is null!"), __LINE__, __FUNCTION__);
	FCollisionResponseContainer CollisionResponses = BoxCollisionComponent->GetCollisionResponseToChannels();

	// true: player left the bomb, block collision (all other channels and players will stay as they are)
	// false: client player with high ping might be teleported back into blocked bomb, so release collision
	const bool bIsPlayerLeft = NewCell != MapComponentInternal->GetCell();
	const ECollisionResponse NewResponse = bIsPlayerLeft ? ECR_Block : ECR_Overlap;

	checkf(PlayerMapComponent, TEXT("ERROR: [%i] %hs:\n'PlayerMapComponent' is null!"), __LINE__, __FUNCTION__);
	const APawn* Pawn = PlayerMapComponent ? PlayerMapComponent->GetOwner<APawn>() : nullptr;
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<APlayerState>() : nullptr;
	const int32 PlayerId = PlayerState ? PlayerState->GetPlayerId() : INDEX_NONE;

	GetCollisionResponseToPlayerByID(/*InOut*/ CollisionResponses, PlayerId, NewResponse);
	MapComponentInternal->SetCollisionResponses(CollisionResponses);
}

/*********************************************************************************************
 * Custom Collision Response
 ********************************************************************************************* */

// Sets actual collision responses to all players for this bomb
void ABombActor::InitCollisionResponseToAllPlayers()
{
	// Obtain all overlapped level actors on the bomb cell to enable overlap response for players inside the bomb
	FMapComponents OverlapMapComponents;
	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	ULevelActorsUtilsLibrary::GetLevelActorsOnCells(/*out*/ OverlapMapComponents, {MapComponentInternal->GetCell()});

	// Obtain default collision responses
	const UBoxComponent* BoxCollisionComponent = MapComponentInternal ? MapComponentInternal->GetBoxCollisionComponent() : nullptr;
	checkf(BoxCollisionComponent, TEXT("ERROR: [%i] %hs:\n'BoxCollisionComponent' is null!"), __LINE__, __FUNCTION__);
	FCollisionResponseContainer CollisionResponses = BoxCollisionComponent->GetCollisionResponseToChannels();

	// Block all players by default (all non-player channels will remain unchanged)
	static constexpr int32 MaxPlayerID = 3;
	for (int32 CharacterID = 0; CharacterID <= MaxPlayerID; ++CharacterID)
	{
		GetCollisionResponseToPlayerByID(/*InOut*/ CollisionResponses, CharacterID, ECR_Block);
	}

	// Unlock (allow overlap) those players which overlap with this bomb
	for (const UMapComponent* OverlapMapComponentIt : OverlapMapComponents)
	{
		const APawn* Pawn = OverlapMapComponentIt ? OverlapMapComponentIt->GetOwner<APawn>() : nullptr;
		const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<APlayerState>() : nullptr;
		if (!PlayerState)
		{
			// Is different overlapped actor, likely bomb itself
			continue;
		}

		// Change response for the player which overlaps with this bomb (all other channels and players will stay as they are)
		GetCollisionResponseToPlayerByID(/*InOut*/ CollisionResponses, PlayerState->GetPlayerId(), ECR_Overlap);

		// Listen when character end to overlaps with this bomb to block collision
		UMapComponent* PlayerMapComponent = UMapComponent::GetMapComponent(Pawn);
		checkf(PlayerMapComponent, TEXT("ERROR: [%i] %hs:\n'PlayerMapComponent' is null!"), __LINE__, __FUNCTION__);
		PlayerMapComponent->OnCellChanged.AddUniqueDynamic(this, &ThisClass::OnPlayerCellChanged);
	}

	MapComponentInternal->SetCollisionResponses(CollisionResponses);
}

// Takes your container and returns is with new specified response for player by its specified ID
void ABombActor::GetCollisionResponseToPlayerByID(FCollisionResponseContainer& InOutCollisionResponses, int32 CharacterID, ECollisionResponse NewResponse)
{
	if (CharacterID < 0)
	{
		return;
	}

	ECollisionChannel CollisionChannel = ECC_WorldDynamic;
	switch (CharacterID)
	{
		case 0:
			CollisionChannel = ECC_Player0;
			break;
		case 1:
			CollisionChannel = ECC_Player1;
			break;
		case 2:
			CollisionChannel = ECC_Player2;
			break;
		case 3:
			CollisionChannel = ECC_Player3;
			break;
		default:
			break;
	}

	InOutCollisionResponses.SetResponse(CollisionChannel, NewResponse);
}