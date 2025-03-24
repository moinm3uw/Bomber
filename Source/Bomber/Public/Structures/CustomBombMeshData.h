// // Copyright (c) Valerii Rotermel

#pragma once

#include "CustomBombMeshData.generated.h"

USTRUCT(BlueprintType)
struct BOMBER_API FCustomBombMeshData
{
	GENERATED_BODY()

public:
	/** To Do */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	int32 SkinIndex;

	/** To Do */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	FColor Color;
};