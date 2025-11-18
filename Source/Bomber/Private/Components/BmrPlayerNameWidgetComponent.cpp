// Copyright (c) Yevhenii Selivanov

#include "Components/BmrPlayerNameWidgetComponent.h"

// Bomber
#include "GameFramework/MyGameStateBase.h"
#include "GameFramework/MyPlayerState.h"
#include "Structures/BmrGameplayTags.h"
#include "Subsystems/GlobalEventsSubsystem.h"
#include "Subsystems/WidgetsSubsystem.h"
#include "UI/Widgets/PlayerNameWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BmrPlayerNameWidgetComponent)

// Sets default values for this component's properties
UBmrPlayerNameWidgetComponent::UBmrPlayerNameWidgetComponent()
{
	// Is ticking to render the widget
	PrimaryComponentTick.bCanEverTick = true;

	// Position and orientation setup
	SetUsingAbsoluteRotation(true);
	static const FVector DefaultRelativeLocation(0.f, 0.f, 220.f);
	SetRelativeLocation_Direct(DefaultRelativeLocation);
	static const FRotator WidgetRelativeRotation(90.f, -90.f, 180.f);
	SetRelativeRotation_Direct(WidgetRelativeRotation);

	// Widget display configuration
	static const FVector2D DefaultDrawSize(100.f, 30.f);
	SetDrawSize(DefaultDrawSize);
	static const FVector2D DefaultPivot(0.5f, 0.4f);
	SetPivot(DefaultPivot);
	SetWidgetSpace(EWidgetSpace::Screen);

	// Collision and interaction settings
	SetGenerateOverlapEvents(false);
}

/*********************************************************************************************
 * Player ID
 ********************************************************************************************* */

// Activates the widget component with further syncing it with the player state's name and PlayerId
void UBmrPlayerNameWidgetComponent::Init(AMyPlayerState* PlayerState)
{
	if (!ensureMsgf(PlayerState, TEXT("ASSERT: [%i] %hs:\n'PlayerState' is not valid!"), __LINE__, __FUNCTION__))
	{
		return;
	}

	UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem();
	if (!WidgetsSubsystem)
	{
		// UI Subsystem is not initialized yet or called on remote clients
		return;
	}

	AssociatedPlayerStateInternal = PlayerState;

	const int32 PlayerId = PlayerState->GetPlayerId();
	UPlayerNameWidget* PlayerNameWidget = WidgetsSubsystem->GetWidgetByTag<UPlayerNameWidget>(BmrGameplayTags::UI::Widget_Nickname, PlayerId);
	if (!PlayerNameWidget)
	{
		// Widget is not created yet for specified player ID, request it now
		PlayerNameWidget = &WidgetsSubsystem->CreateManageableWidgetByTagChecked<UPlayerNameWidget>(BmrGameplayTags::UI::Widget_Nickname);
	}

	// Configure widget content and association
	PlayerNameWidget->SetPlayerName(FText::FromString(PlayerState->GetPlayerName()));
	PlayerNameWidget->SetAssociatedPlayerId(PlayerState->GetPlayerId());

	// Update widget component if changed
	const UUserWidget* CurrentWidget = GetWidget();
	if (CurrentWidget != PlayerNameWidget)
	{
		SetWidget(PlayerNameWidget);
	}

	UpdateVisibility();

	// Listen further updates
	PlayerState->OnPlayerNameChanged.AddUniqueDynamic(this, &ThisClass::OnPlayerNameChanged);
	PlayerState->OnPlayerIdChanged.AddUniqueDynamic(this, &ThisClass::OnPlayerIdChanged);
	PlayerState->OnCharacterDeadChanged.AddUniqueDynamic(this, &ThisClass::OnCharacterDeadChanged);
}

// Applies new widget visibility based on the current game state
void UBmrPlayerNameWidgetComponent::UpdateVisibility()
{
	UUserWidget* InWidget = GetWidget();
	if (!InWidget)
	{
		// Widget is not set yet, might be called before UI Subsystem is initialized
		return;
	}

	const bool bMakeVisible = AMyGameStateBase::GetCurrentGameState() != ECGS::Menu
	                          && AssociatedPlayerStateInternal
	                          && !AssociatedPlayerStateInternal->IsCharacterDead();

	const ESlateVisibility NewVisibility = bMakeVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (InWidget->GetVisibility() != NewVisibility)
	{
		InWidget->SetVisibility(NewVisibility);
	}
}

/*********************************************************************************************
 * Events
 ********************************************************************************************* */

// Called when the game starts or when spawned
void UBmrPlayerNameWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWidgetsSubsystem* WidgetsSubsystem = UWidgetsSubsystem::GetWidgetsSubsystem()) // Is null on remote clients
	{
		WidgetsSubsystem->OnWidgetsInitialized.AddUniqueDynamic(this, &ThisClass::OnWidgetsInitialized);
		if (WidgetsSubsystem->AreWidgetInitialized())
		{
			OnWidgetsInitialized();
		}
	}

	BIND_ON_GAME_STATE_CHANGED(this, ThisClass::OnGameStateChanged);
}

// Listen to manage the component visibility
void UBmrPlayerNameWidgetComponent::OnGameStateChanged_Implementation(ECurrentGameState CurrentGameState)
{
	UpdateVisibility();
}

// Is called when all game widgets are initialized to handle UI-related logic
void UBmrPlayerNameWidgetComponent::OnWidgetsInitialized_Implementation()
{
	// If the player state was set before the widgets were initialized, we can safely initialize the widget component now
	if (AssociatedPlayerStateInternal)
	{
		Init(AssociatedPlayerStateInternal);
	}
}

// Called when changed Character's name to update the widget
void UBmrPlayerNameWidgetComponent::OnPlayerNameChanged_Implementation(FName NewNickname)
{
	UPlayerNameWidget* InWidget = Cast<UPlayerNameWidget>(GetWidget());
	if (ensureMsgf(InWidget, TEXT("ASSERT: [%i] %hs:\n'Widget' is not valid!"), __LINE__, __FUNCTION__))
	{
		InWidget->SetPlayerName(FText::FromName(NewNickname));
	}
}

// Called when changed Character's PlayerId to update the widget
void UBmrPlayerNameWidgetComponent::OnPlayerIdChanged_Implementation(int32 NewPlayerId)
{
	UPlayerNameWidget* InWidget = Cast<UPlayerNameWidget>(GetWidget());
	if (ensureMsgf(InWidget, TEXT("ASSERT: [%i] %hs:\n'Widget' is not valid!"), __LINE__, __FUNCTION__))
	{
		InWidget->SetAssociatedPlayerId(NewPlayerId);
	}
}

// Called when changed character Dead status is changed to update the widget visibility
void UBmrPlayerNameWidgetComponent::OnCharacterDeadChanged_Implementation(bool bIsCharacterDead)
{
	UpdateVisibility();
}
