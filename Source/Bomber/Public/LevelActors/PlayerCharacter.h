// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "GameFramework/Character.h"
//---
#include "AbilitySystemInterface.h"
#include "Structures/BmrPowerUp.h"
//---
#include "PlayerCharacter.generated.h"

enum class ELevelType : uint8;
enum class ECurrentGameState : uint8;
enum class EPlayerType : uint8;

/**
 * Players and AI, whose goal is to remain the last survivor for the win.
 * @see Access Player's data with UPlayerDataAsset (Content/Bomber/DataAssets/DA_Player).
 * @see Access AI's data with UAIDataAsset (Content/Bomber/DataAssets/DA_AI).
 */
UCLASS()
class BOMBER_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Powerups
	 * @todo JanSeliv UGi56jhn Replace all powerup-related logic by GAS attributes (delegates, setters, getters, etc.)
	 ********************************************************************************************* */
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPowerUpsChanged, const struct FBmrPowerUpsContainer&, NewPowerUps, const FBmrPowerUpsContainer&, PrevPowerUps);

	/** Called when this character picked up any power-up or they were reset.*/
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Transient, Category = "C++")
	FOnPowerUpsChanged OnPowerUpsChanged;

	/** Returns current powerup levels */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FBmrPowerUp& GetPowerUp(EItemType ItemType) const { return PowerupsInternal.Get(ItemType); }

	/** Set powerups levels all at once, can be called only on the server. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++", meta = (AutoCreateRefTerm = "NewPowerups"))
	void SetPowerups(int32 NewLevel);

	/** Resets powerups levels to the default ones, can be called only on the server.
	 * Gathers all default levels from the Curve Table by current player type, where:
	 * - Columns (Item Type): Skate, Bomb, Fire
	 * - Rows (Player Tag): 'Player.Bastet' etc */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetDefaultPowerups();

	/** Apply effect of picked up powerups, can be called both on server and clients.
	 * @param PrevPowerups - previous powerups levels before applying new ones (assuming new ones are already set). */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (AutoCreateRefTerm = "PrevPowerups"))
	void ApplyPowerups(const FBmrPowerUpsContainer& PrevPowerups);

protected:
	friend FBmrPowerUpsContainer;

	/** Count of items that affect on a player during gameplay. Can be overriden by the Cheat Manager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, ReplicatedUsing = "OnRep_Powerups", Category = "C++", meta = (BlueprintProtected, DisplayName = "Powerups", ShowOnlyInnerProperties))
	FBmrPowerUpsContainer PowerupsInternal;

	/** Is called on clients to apply powerups for this character. */
	UFUNCTION()
	void OnRep_Powerups(const FBmrPowerUpsContainer& PrevPowerups);

	/** ---------------------------------------------------
	 *		Public functions
	 * --------------------------------------------------- */
public:
	/** Sets default values for this character's properties */
	APlayerCharacter(const FObjectInitializer& ObjectInitializer);

	/** Returns level type associated with player, e.g: Water level type for Roger character. */
	UFUNCTION(BlueprintPure, Category = "C++")
	ELevelType GetPlayerType() const;

	/** Returns the Player Tag associated with player. */
	UFUNCTION(BlueprintPure, Category = "C++")
	const struct FPlayerTag& GetPlayerTag() const;

	/** Returns the Ability System Component from the Player State. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	/** The MapComponent manages this actor on the Generated Map */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Map Component"))
	TObjectPtr<class UMapComponent> MapComponentInternal = nullptr;

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Called when an instance of this class is placed (in editor) or spawned */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Called every frame, is disabled on start, tick interval is decreased. */
	virtual void Tick(float DeltaTime) override;

	/** Returns properties that are replicated for the lifetime of the actor channel. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Is overriden to handle the client login when is set new player state. */
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;

	/*********************************************************************************************
	 * Events
	 ********************************************************************************************* */
public:
	/** Is called on server when ANY human player joined the session. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostLogin(class AGameModeBase* GameMode, class APlayerController* NewPlayer);

	/** Is called on server when human player, previously possessed by this character, left the session. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostLogout(class APlayerController* ExitingPlayer);

protected:
	/** Called when this level actor is reconstructed or added on the Generated Map.
	 * Is used by Level Actors instead of the BeginPlay(). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnAddedToLevel(UMapComponent* MapComponent);

	/** Is called when the Row from current Data Asset is changed for owner on the level, on both server and clients. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnActorTypeChanged(UMapComponent* MapComponent, const class ULevelActorRow* NewRow, const class ULevelActorRow* PreviousRow);

	/**
	 * Triggers when this player character starts something overlap.
	 * With item overlapping Increases +1 to numbers of character's powerups (Skate/Bomb/Fire).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPlayerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** Listen to manage the tick. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnGameStateChanged(ECurrentGameState CurrentGameState);

	/** Called right before owner actor going to remove from the Generated Map, on both server and clients.
	 * Is used for handling the in-game dying logic before this character is removed from the level. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPreRemovedFromLevel(class UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Called each time after owner actor was removed from Generated Map, on both server and clients.
	 * Is used for cleaning up the character's data after it was removed from the level. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPostRemovedFromLevel(class UMapComponent* MapComponent, UObject* DestroyCauser);

	/** Is called for everytime when character changed its position on the Generated Map. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnCellChanged(class UMapComponent* MapComponent, const struct FCell& NewCell, const struct FCell& PreviousCell);

	/** Is called when the player state is fully initialized. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnPlayerStateReady(class AMyPlayerState* InPlayerState, int32 CharacterID);

	/*********************************************************************************************
	 * Protected functions
	 ********************************************************************************************* */
protected:
	/** Updates collision object type by current character ID. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void UpdateCollisionObjectType();

	/*********************************************************************************************
	 * Controller (AI/Player)
	 ********************************************************************************************* */
public:
	/** Is overridden to determine additional conditions for the player-controlled character. */
	virtual bool IsPlayerControlled() const override;

	/** Possess a player or AI controller in dependence of current Character ID.
	 * @param PlayerType the type of player to possess: human or bot; or 'Any' to automatically detect.*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void TryPossessController(EPlayerType PlayerType);

	/** Move the player character. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected, AutoCreateRefTerm = "ActionValue"))
	void MovePlayer(const struct FInputActionValue& ActionValue);

	/** Takes the player current vector location and updates it on the level as a cell. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void UpdateLocation();

protected:
	/** The character's AI controller */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Transient, AdvancedDisplay, Category = "C++", meta = (BlueprintProtected, DisplayName = "My AI Controller"))
	TObjectPtr<class AAIController> AIControllerInternal = nullptr;

	/*********************************************************************************************
	 * Nickname
	 ********************************************************************************************* */
public:
	/** Returns the 3D widget component that displays the player name above the character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE class UBmrPlayerNameWidgetComponent* GetPlayerName3DWidgetComponent() const { return PlayerName3DWidgetComponentInternal; }

protected:
	/** 3D widget component that displays the player name above the character. */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Player Name 3D Widget Component"))
	TObjectPtr<class UBmrPlayerNameWidgetComponent> PlayerName3DWidgetComponentInternal = nullptr;

	/*********************************************************************************************
	 * Player ID
	 ********************************************************************************************* */
public:
	/** Returns own character ID, e.g: 0, 1, 2, 3 */
	UFUNCTION(BlueprintPure, Category = "C++")
	int32 GetPlayerId() const;

	/*********************************************************************************************
	 * Player Mesh
	 ********************************************************************************************* */
public:
	friend class UMyCheatManager;

	/** Returns the Skeletal Mesh of bombers. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	class UMySkeletalMeshComponent* GetMySkeletalMeshComponent() const;
	UMySkeletalMeshComponent& GetMeshChecked() const;

	/** Set and apply default skeletal mesh for this player.
	 * @param bForcePlayerSkin If true, will force the bot to change own skin to look like a player. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "C++")
	void SetDefaultPlayerMeshData(bool bForcePlayerSkin = false);

	/*********************************************************************************************
	 * Bomb Placement
	 ********************************************************************************************* */
public:
	/** Spawns bomb on character position.
	 * @param bForce If true, will force spawning bomb without any checks, might be useful for testing or modding. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "C++")
	void ServerSpawnBomb(bool bForce = false);

protected:
	/** Event triggered when the bomb has been explicitly destroyed. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnBombDestroyed(class UMapComponent* MapComponent, UObject* DestroyCauser = nullptr);
};