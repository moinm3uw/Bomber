// Copyright (c) Yevhenii Selivanov

#pragma once

#include "InputAction.h"
//---
#include "Structures/BmrInputActionBinding.h"
//---
#include "BmrInputAction.generated.h"

/**
  * Is inherited data asset, has additional data to setup input action.
  */
UCLASS(Blueprintable, Const, AutoExpandCategories=("C++"))
class BOMBER_API UBmrInputAction final : public UInputAction
{
	GENERATED_BODY()

public:
	/** Returns the number of input action bindings configured for this action
	 * @see ::InputActionBindingsInternal */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	FORCEINLINE int32 GetInputActionBindingsNum() const { return InputActionBindingsInternal.Num(); }

	/** Returns the input action binding by index
	 * @see ::InputActionBindingsInternal */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "C++")
	const FORCEINLINE FBmrInputActionBinding& GetInputActionBinding(int32 Index) const { return InputActionBindingsInternal.IsValidIndex(Index) ? InputActionBindingsInternal[Index] : FBmrInputActionBinding::Empty; }

#if WITH_EDITOR
	/** Validates bound functions to this input action. */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif // WITH_EDITOR

protected:
	/** Contains all input action binding configurations for this action */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Input Action Bindings", ShowOnlyInnerProperties))
	TArray<FBmrInputActionBinding> InputActionBindingsInternal;
};