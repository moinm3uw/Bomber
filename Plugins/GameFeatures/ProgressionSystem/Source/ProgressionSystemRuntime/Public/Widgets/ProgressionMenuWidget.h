// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "ProgressionMenuWidget.generated.h"
enum class ECurrentGameState : uint8;

class UButton;
class UTextBlock;


/**
 * 
 */
UCLASS()
class PROGRESSIONSYSTEMRUNTIME_API UProgressionMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	public:
	
	/** The text to display player current progression achievement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, BindWidget))
	TObjectPtr<UTextBlock> ProgressionState = nullptr;

	// Horizontal Box widget for storing stars
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "C++", meta = (BlueprintProtected, BindWidget))
	TObjectPtr<class UHorizontalBox> HorizontalBox = nullptr;

	UFUNCTION()
	void SetProgressionState(FText text);

	// Function to add images to the Horizontal Box
	UFUNCTION(BlueprintCallable)
	void AddImagesToHorizontalBox(int AmountOfUnlockedPoints, int AmountOfLockedPoints);

	// Function to add images to the Horizontal Box
	UFUNCTION(BlueprintCallable)
	void ClearImagesFromHorizontalBox();

	
	/*********************************************************************************************
	 * Protected functions
	 ********************************************************************************************* */
protected:
	/** Called after the underlying slate widget is constructed.
	 * May be called multiple times due to adding and removing from the hierarchy. */
	virtual void NativeConstruct() override;

	/** Progression System data asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Progression System Data Asset"))
	TObjectPtr<class UProgressionSystemDataAsset> ProgressionSystemDataAssetInternal = nullptr;

	/** Debug purpose */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Amount of Unlocked points debug"))
	int AmountOfStarsUnlockedDebug = 0;

	/** Debug purpose */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Amount of locked points debug"))
	int AmountOfStarsLockedDebug = 0;
	
	
};
