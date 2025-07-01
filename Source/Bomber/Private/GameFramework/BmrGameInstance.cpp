// Copyright (c) Yevhenii Selivanov

#include "GameFramework/BmrGameInstance.h"
//---
#include "AdvancedSteamFriendsLibrary.h"
#include "CreateSessionCallbackProxyAdvanced.h"
#include "Controllers/MyPlayerController.h"
#include "DataAssets/GameStateDataAsset.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrGameInstance)

/*********************************************************************************************
 * Overrides and Events
 ********************************************************************************************* */

// Is called when the game instance is created
void UBmrGameInstance::Init()
{
	Super::Init();

	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnBeginPlay);
}

// Is used to initialize the game instance post world initialization
void UBmrGameInstance::OnBeginPlay(UWorld* World, struct FWorldInitializationValues WorldInitializationValues)
{
	const FString MainLevelName = UGameStateDataAsset::Get().GetMainLevel().GetAssetName();
	const FString CurrentLevelName = GetNameSafe(World);
	if (CurrentLevelName.Contains(MainLevelName))
	{
		OnMainLevelOpened();
	}
}

// Called on begin play of the Main Level
void UBmrGameInstance::OnMainLevelOpened_Implementation()
{
	BIND_ON_LOCAL_CHARACTER_READY(this, ThisClass::OnLocalCharacterReady);
}

// Called when the local player character is spawned, possessed, and replicated
void UBmrGameInstance::OnLocalCharacterReady_Implementation(class APlayerCharacter* PlayerCharacter, int32 CharacterID)
{
	TryCreateSession();
}

/*********************************************************************************************
 * Online Sessions
 ********************************************************************************************* */

// Attempts to create a session
void UBmrGameInstance::TryCreateSession()
{
	if (!UAdvancedSteamFriendsLibrary::IsSteamAvailable())
	{
		// Is disabled in the editor or not available
		return;
	}

	AMyPlayerController* MyPC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!ensureMsgf(MyPC, TEXT("ASSERT: [%i] %hs:\n'MyPC' is null, failed to create session!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	const IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!ensureMsgf(SessionInterface, TEXT("ASSERT: [%i] %hs:\n'SessionInterface' is not valid!"), __LINE__, __FUNCTION__)
	    || SessionInterface->GetNamedSession(NAME_GameSession))
	{
		// Session is already created
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Created session!"));

	UCreateSessionCallbackProxyAdvanced::CreateAdvancedDefaultSession(this, MyPC).Activate();
}

void UBmrGameInstance::TryJoinSession(const FBlueprintSessionResult& SessionToJoin)
{
	AMyPlayerController* MyPC = UMyBlueprintFunctionLibrary::GetLocalPlayerController();
	if (!ensureMsgf(MyPC, TEXT("ASSERT: [%i] %hs:\n'MyPC' is null, failed to create session!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	const IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;
	if (!ensureMsgf(SessionInterface, TEXT("ASSERT: [%i] %hs:\n'SessionInterface' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Joining session!"));

	// Listen when session is destroyed to join the session
	FOnDestroySessionCompleteDelegate OnDestroySessionCompleteDelegate;
	OnDestroySessionCompleteDelegate.BindWeakLambda(this, [WeakSession = SessionInterface.ToWeakPtr(), SessionToJoin](FName SessionName, bool bWasSuccessful)
	{
		// Session is destroyed, join session
		if (const TSharedPtr<IOnlineSession> SessionInterface = WeakSession.Pin())
		{
			FOnlineSessionSearchResult ModResult = SessionToJoin.OnlineResult;
			ModResult.Session.SessionSettings.bUsesPresence = true;
			ModResult.Session.SessionSettings.bUseLobbiesIfAvailable = true;
			SessionInterface->JoinSession(0, NAME_GameSession, ModResult);
		}
	});
	SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

	// Leave previous session if any
	SessionInterface->DestroySession(NAME_GameSession);

	// Unpossess the camera and hide widgets, so the player can see the loading screen
	MyPC->SetViewTargetWithBlend(nullptr);
	UWidgetsSubsystem::Get(MyPC).SetAllWidgetsVisibility(false);
}

// Session callback when a user accepts an invitation
void UBmrGameInstance::OnSessionInviteAcceptedMaster(const bool bWasSuccessful, int32 LocalPlayer, TSharedPtr<const FUniqueNetId> PersonInviting, const FOnlineSessionSearchResult& SessionToJoin)
{
	Super::OnSessionInviteAcceptedMaster(bWasSuccessful, LocalPlayer, PersonInviting, SessionToJoin);

	if (!bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to accept session invite!"));
		return;
	}

	TryJoinSession({SessionToJoin});
}

// Session callback when a user receives an invitation
void UBmrGameInstance::OnSessionInviteReceivedMaster(const FUniqueNetId& PersonInvited, const FUniqueNetId& PersonInviting, const FString& AppId, const FOnlineSessionSearchResult& SessionToJoin)
{
	Super::OnSessionInviteReceivedMaster(PersonInvited, PersonInviting, AppId, SessionToJoin);

	if (!SessionToJoin.IsValid())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Session invite is invalid!"));
		return;
	}

	TryJoinSession({SessionToJoin});
}