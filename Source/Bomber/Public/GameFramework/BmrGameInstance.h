// Copyright (c) Yevhenii Selivanov

#pragma once

#include "AdvancedFriendsGameInstance.h"
//---
#include "BmrGameInstance.generated.h"

/**
 * The game instance class that is primarily used to manage the online sessions (create, join, destroy etc).
 */
UCLASS()
class BOMBER_API UBmrGameInstance : public UAdvancedFriendsGameInstance
{
	GENERATED_BODY()

	/*********************************************************************************************
	 * Overrides and Events
	 ********************************************************************************************* */
protected:
	/** Is called when the game instance is created. */
	virtual void Init() override;

	/** Is used to initialize the game instance post world initialization. */
	void OnBeginPlay(UWorld* World, struct FWorldInitializationValues WorldInitializationValues);

	/** Called on begin play of the Main Level. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnMainLevelOpened();

	/** Called when the local player character is spawned, possessed, and replicated. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void OnLocalCharacterReady(class APlayerCharacter* PlayerCharacter, int32 CharacterID);

	/*********************************************************************************************
	 * Online Sessions
	 ********************************************************************************************* */
public:
	/** Attempts to create a session. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void TryCreateSession();

	/** Attempts to join a session. */
	UFUNCTION(BlueprintCallable, Category = "C++")
	void TryJoinSession(const struct FBlueprintSessionResult& SessionToJoin);

protected:
	/** Session callback when a user accepts an invitation. */
	virtual void OnSessionInviteAcceptedMaster(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInviting, const FOnlineSessionSearchResult& SessionToJoin) override;

	/** Session callback when a user receives an invitation. */
	virtual void OnSessionInviteReceivedMaster(const FUniqueNetId& PersonInvited, const FUniqueNetId& PersonInviting, const FString& AppId, const FOnlineSessionSearchResult& SessionToJoin) override;
};