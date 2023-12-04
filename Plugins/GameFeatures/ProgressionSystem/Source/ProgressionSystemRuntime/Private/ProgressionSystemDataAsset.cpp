// Copyright (c) Yevhenii Selivanov


#include "ProgressionSystemDataAsset.h"

#include <string>

#include "ProgressionSystemComponent.h"


void UProgressionSystemDataAsset::SetProgressionSystemComponent(UProgressionSystemComponent* ProgressionSystemComponent)
{
	if (ProgressionSystemComponent != nullptr)
	{
		ProgressionSystemComponentInternal = ProgressionSystemComponent;
	}
}
