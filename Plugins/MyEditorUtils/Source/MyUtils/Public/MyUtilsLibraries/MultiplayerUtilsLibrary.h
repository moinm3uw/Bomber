// Copyright (c) Yevhenii Selivanov

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MultiplayerUtilsLibrary.generated.h"

/**
 * Function library with multiplayer-related helpers.
 *
 * Other useful multiplayer functions:
 * - UKismetSystemLibrary::IsServer
 * - UKismetSystemLibrary::HasMultipleLocalPlayers
 */
UCLASS()
class MYUTILS_API UMultiplayerUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns true if any client is connected to the game. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static FORCEINLINE bool IsMultiplayerGame() { return GetPlayersInMultiplayerNum() > 1; }

	/** Returns true if running from the server or party-leader client
	 * If false, then caller is joined client. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static bool IsPartyLeader();

	/** Returns true in editor game if running from client-only mode, so caller is single-player client or party-leader client. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static bool IsAuthoritativeClient();

	/** Returns amount of players (host + clients) playing this game. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static int32 GetPlayersInMultiplayerNum();

	/** Returns the index of the current player during multiplayer.
	 * 0 is the server or party-leader client.
	 * 1 (or higher) is joined client. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static int32 GetPlayerId();

	/** Returns the ping in milliseconds to the server in milliseconds.
	 * Is only valid on the local client, is 0 on the server or in standalone mode. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	static float GetPingMs();
};

/**
 * Overrides the replication condition of an inherited property, including private ones.
 *
 * Useful when reusing actors like APlayerState, where certain engine properties are marked
 * as COND_InitialOnly and do not replicate again after initial connection. This macro allows
 * changing their replication condition (e.g., to COND_None) so they replicate normally.
 *
 * Example: Enable APlayerState::bIsABot replication, which is originally COND_InitialOnly:
 *     DOREPLIFETIME_OVERRIDE_CONDITION(Super, bIsABot, COND_None);
 *
 * @param SuperClass     The class where the property is originally declared (e.g., APlayerState).
 * @param PropertyName   The name of the property to override.
 * @param NewCondition   The new replication condition to apply (e.g., COND_None).
 */
#define DOREPLIFETIME_OVERRIDE_CONDITION(SuperClass, PropertyName, NewCondition)                                                               \
	{                                                                                                                                          \
		static const FName PropertyName##_PrivateName = TEXT(#PropertyName);                                                                   \
		ResetReplicatedLifetimeProperty(StaticClass(), SuperClass::StaticClass(), PropertyName##_PrivateName, NewCondition, OutLifetimeProps); \
	}
