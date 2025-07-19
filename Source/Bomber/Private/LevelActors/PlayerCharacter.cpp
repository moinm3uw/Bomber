// Copyright (c) Yevhenii Selivanov.

#include "LevelActors/PlayerCharacter.h"
//---
#include "Bomber.h"
#include "GeneratedMap.h"
#include "Components/BmrPlayerNameWidgetComponent.h"
#include "Components/MapComponent.h"
#include "Components/MySkeletalMeshComponent.h"
#include "Controllers/MyAIController.h"
#include "Controllers/MyPlayerController.h"
#include "DataAssets/ItemDataAsset.h"
#include "DataAssets/PlayerDataAsset.h"
#include "GameFramework/MyGameModeBase.h"
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "LevelActors/BombActor.h"
#include "LevelActors/ItemActor.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/LevelActorsUtilsLibrary.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "InputActionValue.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CurveTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerCharacter)

/*********************************************************************************************
 * Powerups
 ********************************************************************************************* */

// Set powerups levels all at once, can be called only on the server
void APlayerCharacter::SetPowerups(int32 NewLevel)
{
	if (!HasAuthority())
	{
		return;
	}

	// Go through each powerup type and set its new level
	for (const FBmrPowerupTag ItemTypeIt : FBmrPowerupTag::GetAll())
	{
		PowerupsInternal.SetLevel(NewLevel, ItemTypeIt);
	}
}

// Resets powerups levels to the default ones, can be called only on the server
void APlayerCharacter::SetDefaultPowerups()
{
	if (!HasAuthority())
	{
		return;
	}

	const FBmrPowerUpsContainer PrevPowerups = PowerupsInternal;

	// Attempt to get defaults from the Curve Table by current Player Tag
	const UCurveTable* DefaultPowerupsCurveTable = UItemDataAsset::Get().GetDefaultPowerupsCurveTable();
	ensureMsgf(DefaultPowerupsCurveTable, TEXT("ASSERT: [%i] %hs:\n'DefaultPowerupsCurveTable' is not set, 1 will be used as default!"), __LINE__, __FUNCTION__);
	const FRealCurve* DefaultItemLevelsCurve = DefaultPowerupsCurveTable ? DefaultPowerupsCurveTable->FindCurve(GetPlayerTag().GetTagName(), __FUNCTION__, /*bWarnIfNotFound*/false) : nullptr;
	if (DefaultItemLevelsCurve)
	{
		// Go through each powerup type and set its level from the curve table
		for (int32 Idx = 0; Idx < FBmrPowerupTag::GetAll().Num(); ++Idx)
		{
			const FBmrPowerupTag ItemTypeIt = FBmrPowerupTag::GetAll().GetByIndex(Idx);
			const int32 ItemLevel = FMath::RoundToInt(DefaultItemLevelsCurve->Eval(static_cast<float>(Idx + 1)));
			PowerupsInternal.SetLevel(ItemLevel, ItemTypeIt);
		}
	}
	else
	{
		// If the curve table is not set for the character (like bot), reset all powerups to first level by default 
		SetPowerups(1);
	}
}

// Apply effect of picked up powerups, can be called both on server and clients
void APlayerCharacter::ApplyPowerups(const FBmrPowerUpsContainer& PrevPowerups)
{
	// Apply speed
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		static constexpr float SpeedMultiplier = 100.F;
		const float SkateAdditiveStrength = UItemDataAsset::Get().GetSkateAdditiveStrength();
		const int32 SkateN = PowerupsInternal.Get(FBmrPowerupTag::Skate) * SpeedMultiplier + SkateAdditiveStrength;
		MovementComponent->MaxWalkSpeed = SkateN;
	}

	// Here you might to apply others types of powerups
	// ...

	// Notify listeners
	if (OnPowerUpsChanged.IsBound())
	{
		OnPowerUpsChanged.Broadcast(PowerupsInternal, PrevPowerups);
	}
}

// Is called on clients to apply powerups
void APlayerCharacter::OnRep_Powerups(const FBmrPowerUpsContainer& PrevPowerups)
{
	ApplyPowerups(PrevPowerups);
}

/** ---------------------------------------------------
 *		Public functions
 * --------------------------------------------------- */

// Sets default values
APlayerCharacter::APlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMySkeletalMeshComponent>(MeshComponentName)) // Init UMySkeletalMeshComponent instead of USkeletalMeshComponent
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Replicate an actor
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(true);

	// Set the default AI controller class
	AIControllerClass = AMyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Do not rotate player by camera
	bUseControllerRotationYaw = false;

	// Initialize MapComponent
	MapComponentInternal = CreateDefaultSubobject<UMapComponent>(TEXT("MapComponent"));

	// Initialize default powerup attributes
	PowerupsInternal = FBmrPowerUpsContainer(1, *this);

	// Initialize skeletal mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
	checkf(SkeletalMeshComponent, TEXT("ERROR: [%i] %hs:\n'SkeletalMeshComponent' is null!"), __LINE__, __FUNCTION__);
	static const FVector MeshRelativeLocation(0, 0, -90.f);
	SkeletalMeshComponent->SetRelativeLocation_Direct(MeshRelativeLocation);
	static const FRotator MeshRelativeRotation(0, -90.f, 0);
	SkeletalMeshComponent->SetRelativeRotation_Direct(MeshRelativeRotation);
	SkeletalMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	// Enable all lighting channels, so it's clearly visible in the dark
	SkeletalMeshComponent->SetLightingChannels(/*bChannel0*/true, /*bChannel1*/true, /*bChannel2*/true);
	MapComponentInternal->SetMeshComponent(SkeletalMeshComponent);

	// Initialize 3D widget component for the player name
	PlayerName3DWidgetComponentInternal = CreateDefaultSubobject<UBmrPlayerNameWidgetComponent>(TEXT("PlayerName3DWidgetComponent"));
	PlayerName3DWidgetComponentInternal->SetupAttachment(RootComponent);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Rotate player by movement
		MovementComponent->bOrientRotationToMovement = true;
		static const FRotator RotationRate(0.f, 540.f, 0.f);
		MovementComponent->RotationRate = RotationRate;

		// Do not push out clients from collision
		MovementComponent->MaxDepenetrationWithGeometryAsProxy = 0.f;
	}

	if (UCapsuleComponent* RootCapsuleComponent = GetCapsuleComponent())
	{
		// Setup collision to allow overlap players with each other, but block all other actors
		RootCapsuleComponent->CanCharacterStepUpOn = ECB_Yes;
		RootCapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		RootCapsuleComponent->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player0, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player1, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player2, ECR_Overlap);
		RootCapsuleComponent->SetCollisionResponseToChannel(ECC_Player3, ECR_Overlap);
	}
}

// Returns level type associated with player, e.g: Water level type for Roger character
ELevelType APlayerCharacter::GetPlayerType() const
{
	const UPlayerRow* PlayerRow = MapComponentInternal ? MapComponentInternal->GetMeshRow<UPlayerRow>() : nullptr;
	return PlayerRow ? PlayerRow->LevelType : ELT::None;
}

// Returns the Player Tag associated with player
const FPlayerTag& APlayerCharacter::GetPlayerTag() const
{
	const UPlayerRow* PlayerRow = MapComponentInternal ? MapComponentInternal->GetMeshRow<UPlayerRow>() : nullptr;
	return PlayerRow ? PlayerRow->PlayerTag : FPlayerTag::None;
}

// Returns the Ability System Component from the Player State
UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	const AMyPlayerState* InPlayerState = GetPlayerState<AMyPlayerState>();
	return InPlayerState ? InPlayerState->GetAbilitySystemComponent() : nullptr;
}

/*********************************************************************************************
 * Overrides
 ********************************************************************************************* */

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	// Call to super
	Super::BeginPlay();

	// Set the animation blueprint on very first character spawn
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const TSubclassOf<UAnimInstance> AnimInstanceClass = UPlayerDataAsset::Get().GetAnimInstanceClass();
		MeshComp->SetAnimInstanceClass(AnimInstanceClass);
	}

	// Attempt to posses player or AI on very first spawn
	TryPossessController(EPlayerType::Any);

	if (HasAuthority())
	{
		// Listen to handle possessing logic
		FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &ThisClass::OnPostLogin);
	}

	BIND_ON_PLAYER_STATE_READY(this, ThisClass::OnPlayerStateReady, GetPlayerId());
}

// Called when an instance of this class is placed (in editor) or spawned
void APlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	MapComponentInternal->OnAddedToLevel.AddUniqueDynamic(this, &ThisClass::OnAddedToLevel);
	AGeneratedMap::Get().AddToGrid(MapComponentInternal);
}

// Called every frame, is disabled on start, tick interval is decreased
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLocation();
}

// Returns properties that are replicated for the lifetime of the actor channel
void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PowerupsInternal, Params);
}

// Is overriden to handle the client login when is set new player state
void APlayerCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);

	if (AMyPlayerState* MyPlayerState = Cast<AMyPlayerState>(NewPlayerState))
	{
		MyPlayerState->OnPlayerStateInit();
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Called when this level actor is reconstructed or added on the Generated Map
void APlayerCharacter::OnAddedToLevel_Implementation(UMapComponent* MapComponent)
{
	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is null!"), __LINE__, __FUNCTION__);
	MapComponent->OnPreRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPreRemovedFromLevel);
	MapComponent->OnPostRemovedFromLevel.AddUniqueDynamic(this, &ThisClass::OnPostRemovedFromLevel);
	MapComponent->OnCellChanged.AddUniqueDynamic(this, &ThisClass::OnCellChanged);
	MapComponent->OnActorTypeChanged.AddUniqueDynamic(this, &ThisClass::OnActorTypeChanged);

	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnPlayerBeginOverlap);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	if (!MapComponentInternal->GetReplicatedMeshData().IsValid())
	{
		SetDefaultPlayerMeshData();
	}

	if (AMyPlayerState* MyPlayerState = GetPlayerState<AMyPlayerState>())
	{
		checkf(PlayerName3DWidgetComponentInternal, TEXT("ERROR: [%i] %hs:\n'PlayerName3DWidgetComponentInternal' is null!"), __LINE__, __FUNCTION__);
		PlayerName3DWidgetComponentInternal->Init(MyPlayerState);
	}

	UpdateCollisionObjectType();

	// Spawn or destroy controller of specific ai with enabled visualization
#if WITH_EDITOR // [IsEditorNotPieWorld]
	if (UUtilsLibrary::IsEditorNotPieWorld()                                 // [IsEditorNotPieWorld] only
	    && ULevelActorsUtilsLibrary::GetIndexByLevelActor(MapComponent) > 0) // Is a bot
	{
		AIControllerInternal = Cast<AAIController>(GetController());
		if (!MapComponent->bShouldShowRenders)
		{
			if (AIControllerInternal)
			{
				AIControllerInternal->Destroy();
			}
		}
		else if (!AIControllerInternal) // Is a bot with debug visualization and AI controller is not created yet
		{
			SpawnDefaultController();
			if (AController* PlayerController = GetController())
			{
				PlayerController->bIsEditorOnlyActor = true;
			}
		}
	}
#endif	// WITH_EDITOR [IsEditorNotPieWorld]

	TryPossessController(EPlayerType::Any);

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);

	SetDefaultPowerups();

	UGlobalEventsSubsystem::Get().OnCharactersReadyHandler.Broadcast_OnCharacterAdded(*this);
}

// Is called when the Row from current Data Asset is changed for owner on the level, on both server and clients
void APlayerCharacter::OnActorTypeChanged_Implementation(UMapComponent* MapComponent, const class ULevelActorRow* NewRow, const class ULevelActorRow* PreviousRow)
{
	// Update powerup attributes based on changed player type
	SetDefaultPowerups();
}

// Triggers when this player character starts something overlap.
void APlayerCharacter::OnPlayerBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	const AItemActor* OverlappedItem = Cast<AItemActor>(OtherActor);
	if (OverlappedItem)
	{
		PowerupsInternal.AddLevel(1, OverlappedItem->GetItemType());
	}
}

// Listen to manage the tick
void APlayerCharacter::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	switch (CurrentGameState)
	{
		case ECurrentGameState::Menu: // fallthrough
		case ECurrentGameState::GameStarting:
		{
			SetActorTickEnabled(false);
			break;
		}
		case ECurrentGameState::InGame:
		{
			SetActorTickEnabled(true);
			break;
		}
		default:
			break;
	}
}

// Is called on server when ANY human player joined the session
void APlayerCharacter::OnPostLogin_Implementation(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	TryPossessController(EPlayerType::Human);

	// If successfully replaced the bot by human, update the mesh
	if (GetController() == NewPlayer)
	{
		SetDefaultPlayerMeshData();
	}
}

// Is called on server when human player, previously possessed by this character, left the session
void APlayerCharacter::OnPostLogout_Implementation(APlayerController* ExitingPlayer)
{
	// Player is leaving, possess the bot and update the mesh
	TryPossessController(EPlayerType::Bot);
	SetDefaultPlayerMeshData();
}

// Is called when the player was destroyed
void APlayerCharacter::OnPreRemovedFromLevel_Implementation(UMapComponent* MapComponent, UObject* DestroyCauser)
{
	if (AMyGameStateBase::GetCurrentGameState() != ECurrentGameState::InGame)
	{
		// Ignore, is not gameplay destroy, likely level is regenerated
		return;
	}

	// Mark this player as dead in own PlayerState
	if (AMyPlayerState* InPlayerState = GetPlayerState<AMyPlayerState>())
	{
		InPlayerState->SetCharacterDead(true);
	}

	// In the KillerPlayerState, mark this player as killed by DestroyCauser
	AMyPlayerState* KillerPlayerState = [DestroyCauser]
	{
		const APlayerCharacter* CauserCharacter = Cast<APlayerCharacter>(DestroyCauser);
		if (!CauserCharacter)
		{
			const ABombActor* Bomb = DestroyCauser ? Cast<ABombActor>(DestroyCauser) : nullptr;
			CauserCharacter = Bomb ? Cast<APlayerCharacter>(Bomb->GetBombPlacer()) : nullptr;
		}
		return CauserCharacter ? CauserCharacter->GetPlayerState<AMyPlayerState>() : nullptr;
	}();
	if (KillerPlayerState)
	{
		KillerPlayerState->SetOpponentKilled(this);
	}
}

// Is used for cleaning up the character's data after it was removed from the level
void APlayerCharacter::OnPostRemovedFromLevel_Implementation(UMapComponent* MapComponent, UObject* DestroyCauser)
{
	// -- Handle cleanup after removed player from the level

	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is null!"), __LINE__, __FUNCTION__);
	MapComponent->OnPostRemovedFromLevel.RemoveAll(this);
	MapComponent->OnCellChanged.RemoveAll(this);
	MapComponent->OnActorTypeChanged.RemoveAll(this);

	OnActorBeginOverlap.RemoveAll(this);

	UGlobalEventsSubsystem::Get().BP_OnGameStateChanged.RemoveAll(this);

	GetMeshChecked().SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	SetDefaultPowerups();
}

// Is called for everytime when character changed its cell on the Generated Map
void APlayerCharacter::OnCellChanged_Implementation(UMapComponent* MapComponent, const FCell& NewCell, const FCell& PreviousCell)
{
	checkf(MapComponent, TEXT("ERROR: [%i] %hs:\n'MapComponent' is guaranteed to be valid!"), __LINE__, __FUNCTION__);
	if (HasActorBegunPlay())
	{
		// Visualize the cell changes during the gameplay
		MapComponentInternal->TryDisplayOwnedCell(/*bClearPrevious*/true);
	}
}

// Is called when the player state is fully initialized
void APlayerCharacter::OnPlayerStateReady_Implementation(AMyPlayerState* InPlayerState, int32 CharacterID)
{
	if (GetPlayerId() != CharacterID)
	{
		// Is not this character
		return;
	}

	checkf(PlayerName3DWidgetComponentInternal, TEXT("ERROR: [%i] %hs:\n'PlayerName3DWidgetComponentInternal' is null!"), __LINE__, __FUNCTION__);
	PlayerName3DWidgetComponentInternal->Init(InPlayerState);
}

/*********************************************************************************************
 * Protected functions
 ********************************************************************************************* */

// Updates collision object type by current character ID
void APlayerCharacter::UpdateCollisionObjectType()
{
	const int32 PlayerId = GetPlayerId();
	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	if (!ensureMsgf(CapsuleComp, TEXT("ASSERT: [%i] %hs:\n'CapsuleComp' is not valid!"), __LINE__, __FUNCTION__)
	    || PlayerId < 0) // Might be replicating yet
	{
		return;
	}

	// Set the object collision type
	ECollisionChannel CollisionObjectType = CapsuleComp->GetCollisionObjectType();
	switch (PlayerId)
	{
		case 0:
			CollisionObjectType = ECC_Player0;
			break;
		case 1:
			CollisionObjectType = ECC_Player1;
			break;
		case 2:
			CollisionObjectType = ECC_Player2;
			break;
		case 3:
			CollisionObjectType = ECC_Player3;
			break;
		default:
			break;
	}

	CapsuleComp->SetCollisionObjectType(CollisionObjectType);
}

/*********************************************************************************************
 * Controller (AI/Player)
 ********************************************************************************************* */

// Is overridden to determine additional conditions for the player-controlled character
bool APlayerCharacter::IsPlayerControlled() const
{
	if (Super::IsPlayerControlled())
	{
		return true;
	}

	// Player state is not initialized yet (so Super returned false), but 0 is always a player
	return !GetPlayerState() && GetPlayerId() == 0;
}

// Possess a player or AI controller in dependence of current Character ID
void APlayerCharacter::TryPossessController(EPlayerType PlayerType)
{
	if (!HasAuthority()
	    || !IsActorInitialized() // Engine doesn't allow possess before BeginPlay\PostInitializeComponents
	    || UUtilsLibrary::IsEditorNotPieWorld()
	    || !ensureMsgf(PlayerType != EPlayerType::None, TEXT("ASSERT: [%i] %hs:\n'PlayerType' is None, can't possess!"), __LINE__, __FUNCTION__))
	{
		// Should not possess in PIE
		return;
	}

	const int32 PlayerId = GetPlayerId();
	const AMyGameModeBase* MyGameMode = UMyBlueprintFunctionLibrary::GetMyGameMode();
	if (!ensureMsgf(PlayerId >= 0, TEXT("ASSERT: [%i] %hs:\n'PlayerId' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(MyGameMode, TEXT("ASSERT: [%i] %hs:\n'MyGameMode' is not valid! Make sure '%s' class is assigned to the '%s' level"), __LINE__, __FUNCTION__, *AMyGameModeBase::StaticClass()->GetName(), *GetWorld()->GetMapName()))
	{
		return;
	}

	if (PlayerType == EPlayerType::Any)
	{
		PlayerType = IsPlayerControlled() ? EPlayerType::Human : EPlayerType::Bot;
	}

	AController* ControllerToPossess = nullptr;
	switch (PlayerType)
	{
		default: checkNoEntry(); // Fallthrough

		case EPlayerType::Human:
		{
			AMyPlayerController* MyPC = UMyBlueprintFunctionLibrary::GetMyPlayerController(PlayerId);
			if (MyPC
			    && !MyPC->bCinematicMode) // Don't possess player if it's the Render Movie
			{
				ControllerToPossess = MyPC;
			}
			break;
		}

		case EPlayerType::Bot:
		{
			if (!AIControllerInternal                             // Is not spawned yet
			    || !AIControllerInternal->IsA(AIControllerClass)) // Spawned, but wrong AI controller assigned
			{
				// Spawn AI controller
				AIControllerInternal = GetWorld()->SpawnActor<AAIController>(AIControllerClass, GetActorTransform());
			}

			ControllerToPossess = AIControllerInternal;
		}
		break;
	}

	if (!ControllerToPossess
	    || ControllerToPossess == Controller)
	{
		return;
	}

	if (Controller)
	{
		// At first, unpossess previous controller
		Controller->UnPossess();
	}

	ControllerToPossess->Possess(this);
}

// Move the player character
void APlayerCharacter::MovePlayer(const FInputActionValue& ActionValue)
{
	const AController* OwnedController = GetController();
	if (!OwnedController
	    || OwnedController->IsMoveInputIgnored())
	{
		return;
	}

	// input is a Vector2D
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();

	// Find out which way is forward
	const FRotator ForwardRotation = UCellsUtilsLibrary::GetLevelGridRotation();

	// Get forward vector
	const FVector ForwardDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::X);

	// Get right vector
	const FVector RightDirection = FRotationMatrix(ForwardRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

// Takes the player current vector location and updates it on the level as a cell
void APlayerCharacter::UpdateLocation()
{
	if (HasAuthority())
	{
		// On server, update a player location on the Generated Map
		AGeneratedMap::Get().SetNearestCell(MapComponentInternal);
	}
	else if (IsLocallyControlled())
	{
		// On local client, directly set a player location for responsiveness while server replicates it
		checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
		const FCell SnappedCell = UCellsUtilsLibrary::SnapActorOnLevel(this);
		MapComponentInternal->SetCell(SnappedCell);
	}
}

/*********************************************************************************************
 * Player ID
 ********************************************************************************************* */

// Returns own character ID, e.g: 0, 1, 2, 3
int32 APlayerCharacter::GetPlayerId() const
{
	if (const AMyPlayerState* MyPlayerState = GetPlayerState<AMyPlayerState>())
	{
		return MyPlayerState->GetPlayerId();
	}

	// Player state is not initialized yet, return it directly from the order on the level
	return ULevelActorsUtilsLibrary::GetIndexByLevelActor(MapComponentInternal);
}

/*********************************************************************************************
 * Player Mesh
 ********************************************************************************************* */

// Returns the Skeletal Mesh of bombers
UMySkeletalMeshComponent* APlayerCharacter::GetMySkeletalMeshComponent() const
{
	return MapComponentInternal ? MapComponentInternal->GetMeshComponent<UMySkeletalMeshComponent>() : nullptr;
}

UMySkeletalMeshComponent& APlayerCharacter::GetMeshChecked() const
{
	return *CastChecked<UMySkeletalMeshComponent>(GetMesh());
}

// Set and apply default skeletal mesh for this player
void APlayerCharacter::SetDefaultPlayerMeshData(bool bForcePlayerSkin/* = false*/)
{
	if (!HasAuthority())
	{
		return;
	}

	const UPlayerDataAsset& PlayerDataAsset = UPlayerDataAsset::Get();
	const int32 MeshesNum = PlayerDataAsset.GetRowsNum();
	const int32 PlayerId = GetPlayerId();
	if (!ensureMsgf(MeshesNum > 0, TEXT("ASSERT: [%i] %hs:\n'MeshesNum' is empty!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(PlayerId >= 0, TEXT("ASSERT: [%i] %hs:\n'PlayerId' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const bool bIsPlayer = IsPlayerControlled() || PlayerId == 0;
	const ELevelType PlayerFlag = UMyBlueprintFunctionLibrary::GetLevelType();
	constexpr ELevelType AIFlag = ELT::None;
	ELevelType LevelType = bIsPlayer ? PlayerFlag : AIFlag;

	if (bForcePlayerSkin)
	{
		// Force each bot to look like different player
		LevelType = static_cast<ELevelType>(1 << PlayerId);
	}

	const UPlayerRow* Row = PlayerDataAsset.GetRowByLevelType<UPlayerRow>(TO_ENUM(ELevelType, LevelType));
	if (!ensureMsgf(Row, TEXT("ASSERT: [%i] %hs:\n'Row' is not found!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const int32 SkinsNum = Row->GetSkinTexturesNum();
	FBmrMeshData MeshData = FBmrMeshData::Empty;
	MeshData.Row = Row;
	MeshData.SkinIndex = PlayerId % SkinsNum;

	checkf(MapComponentInternal, TEXT("ERROR: [%i] %hs:\n'MapComponentInternal' is null!"), __LINE__, __FUNCTION__);
	MapComponentInternal->SetReplicatedMeshData(MeshData);
}

/*********************************************************************************************
 * Bomb Placement
 ********************************************************************************************* */

// Spawns bomb on character position
void APlayerCharacter::ServerSpawnBomb_Implementation(bool bForce/* = false*/)
{
	const AController* OwnedController = GetController();
	if (!ensureMsgf(OwnedController, TEXT("ASSERT: [%i] %hs:\n'OwnedController' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(MapComponentInternal, TEXT("ASSERT: [%i] %hs:\n'MapComponentInternal' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	if (!bForce)
	{
		if (UUtilsLibrary::IsEditorNotPieWorld()                   // Should not spawn bomb in PIE
		    || PowerupsInternal.Get(FBmrPowerupTag::Fire).IsZero() // Null length of explosion
		    || PowerupsInternal.Get(FBmrPowerupTag::Bomb).IsZero() // No more bombs
		    || OwnedController->IsMoveInputIgnored())              // controller is blocked
		{
			return;
		}
	}

	// Bomb is spawned on the current location, so make sure it's synced
	UpdateLocation();

	const TFunction<void(UMapComponent&)> OnBombSpawned = [WeakThis = TWeakObjectPtr(this)](UMapComponent& MapComponent)
	{
		APlayerCharacter* PlayerCharacter = WeakThis.Get();
		if (!PlayerCharacter)
		{
			return;
		}

		PlayerCharacter->PowerupsInternal.AddCurrentLevel(-1, FBmrPowerupTag::Bomb);

		// Init Bomb
		ABombActor& BombActor = *CastChecked<ABombActor>(MapComponent.GetOwner());
		BombActor.InitBomb(PlayerCharacter);

		// Start listening this bomb
		MapComponent.OnPostRemovedFromLevel.AddUniqueDynamic(PlayerCharacter, &ThisClass::OnBombDestroyed);
	};

	// Spawn bomb
	AGeneratedMap::Get().SpawnActorByType(EAT::Bomb, MapComponentInternal->GetCell(), OnBombSpawned);
}

// Event triggered when the bomb has been explicitly destroyed.
void APlayerCharacter::OnBombDestroyed_Implementation(UMapComponent* MapComponent, UObject* DestroyCauser/* = nullptr*/)
{
	if (!MapComponent
	    || MapComponent->GetActorType() != EAT::Bomb)
	{
		return;
	}

	// Stop listening this bomb
	MapComponent->OnPostRemovedFromLevel.RemoveAll(this);

	// Increment current bomb count back
	PowerupsInternal.AddCurrentLevel(1, FBmrPowerupTag::Bomb);
}