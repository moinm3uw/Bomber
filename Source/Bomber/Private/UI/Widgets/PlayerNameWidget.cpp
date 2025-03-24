// Copyright (c) Yevhenii Selivanov

#include "UI/Widgets/PlayerNameWidget.h"
//---
#include "LevelActors/PlayerCharacter.h"
#include "UtilityLibraries/MyBlueprintFunctionLibrary.h"
//---
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerNameWidget)

// Is overridden to hide dependent 3D widget components along with this widget
void UPlayerNameWidget::SetVisibility(ESlateVisibility InVisibility)
{
	Super::SetVisibility(InVisibility);

	// Hide 3D widget components if this widget is hidden
	const APlayerCharacter* PlayerOwner = UMyBlueprintFunctionLibrary::GetPlayerCharacter(AssociatedPlayerIdInternal);
	UStaticMeshComponent* NameplateMesh = PlayerOwner ? PlayerOwner->GetNameplateMesh() : nullptr;
	if (NameplateMesh)
	{
		constexpr bool bPropagateToChildren = true;
		const bool bMakeVisible = InVisibility != ESlateVisibility::Collapsed && InVisibility != ESlateVisibility::Hidden;
		NameplateMesh->SetVisibility(bMakeVisible, bPropagateToChildren);
	}
}

/*********************************************************************************************
 * Player Name
 ********************************************************************************************* */

// Returns the player name from the widget
FText UPlayerNameWidget::GetPlayerName() const
{
	return PlayerNameTextWidget ? PlayerNameTextWidget->GetText() : FText::GetEmpty();
}

// Sets player name to the widget
void UPlayerNameWidget::SetPlayerName(const FText& NewPlayerName)
{
	checkf(PlayerNameTextWidget, TEXT("ERROR: [%i] %hs:\n'PlayerNameTextWidget' is null!"), __LINE__, __FUNCTION__);
	if (!PlayerNameTextWidget->GetText().IdenticalTo(NewPlayerName))
	{
		PlayerNameTextWidget->SetText(NewPlayerName);
	}
}

/*********************************************************************************************
 * Player Owner
 ********************************************************************************************* */

// Sets the player character to the widget
void UPlayerNameWidget::SetAssociatedPlayerId(int32 NewPlayerId)
{
	if (ensureMsgf(NewPlayerId >= 0, TEXT("ASSERT: [%i] %hs:\n'NewPlayer' is null!"), __LINE__, __FUNCTION__))
	{
		AssociatedPlayerIdInternal = NewPlayerId;
	}
}