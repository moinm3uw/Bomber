// Copyright (c) Yevhenii Selivanov

#include "MyUtilsLibraries/GameplayUtilsLibrary.h"

// MyUtils
#include "MyUtilsLibraries/UtilsLibrary.h"

// UE
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CurveTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFeaturesSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayUtilsLibrary)

/*********************************************************************************************
 * Actor Helpers
 ********************************************************************************************* */

// Abstract getter that allows to obtain the static or skeletal mesh from given mesh component (base class of both)
class UStreamableRenderAsset* UGameplayUtilsLibrary::GetMesh(const UMeshComponent* MeshComponent)
{
	if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		return SkeletalMeshComponent->GetSkeletalMeshAsset();
	}

	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
	{
		return StaticMeshComponent->GetStaticMesh();
	}

	return nullptr;
}

// Abstract method that allows set both static and skeletal meshes to the specified mesh component
void UGameplayUtilsLibrary::SetMesh(UMeshComponent* MeshComponent, UStreamableRenderAsset* MeshAsset)
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(MeshAsset));
	}
	else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
	{
		StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(MeshAsset));
	}
}

// Returns the first child actor of the specified class
AActor* UGameplayUtilsLibrary::GetAttachedActorByClass(const AActor* ParentActor, TSubclassOf<AActor> ChildActorClass, bool bIncludeDescendants /* = false*/)
{
	if (!ensureMsgf(ParentActor, TEXT("ASSERT: [%i] %s:\n'!ParentActor' is not valid!"), __LINE__, *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);
	if (AttachedActors.IsEmpty())
	{
		return nullptr;
	}

	for (AActor* It : AttachedActors)
	{
		if (It && It->IsA(ChildActorClass))
		{
			return It;
		}

		if (bIncludeDescendants)
		{
			if (AActor* FoundActor = GetAttachedActorByClass(It, ChildActorClass, bIncludeDescendants))
			{
				return FoundActor;
			}
		}
	}

	return nullptr;
}

// Is useful for animating actor's transform from values stored in the Curve Table
bool UGameplayUtilsLibrary::ApplyTransformFromCurveTable(AActor* InActor, const FTransform& CenterWorldTransform, UCurveTable* CurveTable, float TotalSecondsSinceStart)
{
	/* Example data (can be imported as csv into your Curve Table):

	    Name,0,0.1,0.2,0.5
	    LocationX,0,0.0,0.0,0
	    LocationY,0,0.0,0.0,0
	    LocationZ,0,0.0,0.0,0
	    RotationPitch,0,0.0,0.0,0
	    RotationYaw,0,0.0,0.0,0
	    RotationRoll,0,0.0,0.0,0
	    ScaleX,1,0.9,0.7,0
	    ScaleY,1,0.9,0.7,0
	    ScaleZ,1,0.9,0.7,0

	Where ScaleZ will change from `1` (at 0 sec) to `0` (at 0.5 sec) */

	if (!ensureMsgf(CurveTable, TEXT("ASSERT: [%i] %hs:\n'CurveTable' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(InActor, TEXT("ASSERT: [%i] %hs:\n'InActor' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(TotalSecondsSinceStart >= 0.f, TEXT("ASSERT: [%i] %hs:\n'TotalSecondsSinceStart' must be greater or equal to 0!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(CenterWorldTransform.IsValid(), TEXT("ASSERT: [%i] %hs:\n'CenterWorldTransform' is not valid, it should be initial transform of actor to apply animation on it!"), __LINE__, __FUNCTION__))
	{
		return false;
	}

	static const TArray<FName> LocationRows = {FName("LocationX"), FName("LocationY"), FName("LocationZ")};
	static const TArray<FName> RotationRows = {FName("RotationPitch"), FName("RotationYaw"), FName("RotationRoll")};
	static const TArray<FName> ScaleRows = {FName("ScaleX"), FName("ScaleY"), FName("ScaleZ")};

	FVector NewLocation = FVector::ZeroVector;
	FVector NewScale = FVector::OneVector;
	FVector RotationValues = FVector::ZeroVector;

	float EvaluatedValue = 0.f;

	auto EvaluateCurveRow = [](UCurveTable* CurveTable, FName RowName, float InXY, float& OutValue) -> bool
	{
		FCurveTableRowHandle Handle;
		Handle.CurveTable = CurveTable;
		Handle.RowName = RowName;

		const FString ContextString = RowName.ToString();
		const FRealCurve* Curve = CurveTable ? Handle.CurveTable->FindCurve(RowName, ContextString) : nullptr;
		if (!Curve)
		{
			return false;
		}

		float MinTime = 0.f;
		float MaxTime = 0.f;
		Curve->GetTimeRange(MinTime, MaxTime);
		if (InXY >= MaxTime)
		{
			// The curve is finished
			return false;
		}

		return Handle.Eval(InXY, &OutValue, ContextString);
	};

	for (int32 Index = 0; Index < LocationRows.Num(); ++Index)
	{
		if (!EvaluateCurveRow(CurveTable, LocationRows[Index], TotalSecondsSinceStart, EvaluatedValue))
		{
			return false;
		}
		NewLocation[Index] = EvaluatedValue;
	}

	for (int32 Index = 0; Index < RotationRows.Num(); ++Index)
	{
		if (!EvaluateCurveRow(CurveTable, RotationRows[Index], TotalSecondsSinceStart, EvaluatedValue))
		{
			return false;
		}
		RotationValues[Index] = EvaluatedValue;
	}
	const FRotator NewRotation = FRotator::MakeFromEuler(RotationValues);

	for (int32 Index = 0; Index < ScaleRows.Num(); ++Index)
	{
		if (!EvaluateCurveRow(CurveTable, ScaleRows[Index], TotalSecondsSinceStart, EvaluatedValue))
		{
			return false;
		}
		NewScale[Index] = EvaluatedValue;
	}

	FTransform WorldTransform = FTransform::Identity;
	WorldTransform.SetLocation(CenterWorldTransform.GetLocation() + NewLocation);
	WorldTransform.SetRotation(CenterWorldTransform.GetRotation() * NewRotation.Quaternion());
	WorldTransform.SetScale3D(CenterWorldTransform.GetScale3D() * NewScale);

	InActor->SetActorTransform(WorldTransform);

	return true;
}

/*********************************************************************************************
 * Level Helpers
 ********************************************************************************************* */

// Returns true if the specified level is opened in the current world
bool UGameplayUtilsLibrary::IsLevelOpened(const TSoftObjectPtr<UWorld>& Level)
{
	const UWorld* World = UUtilsLibrary::GetPlayWorld();
	const FString CurrentLevelName = GetNameSafe(World);
	const FString LevelName = Level.GetAssetName();
	return CurrentLevelName.Contains(LevelName);
}

// Opens the specified level as listen server: Level?listen
void UGameplayUtilsLibrary::OpenListenServerLevel(const TSoftObjectPtr<UWorld>& Level, bool bForceLoad /* = false*/)
{
	if (!ensureMsgf(!Level.IsNull(), TEXT("ASSERT: [%i] %hs:\n'Level' is null!"), __LINE__, __FUNCTION__)
	    || (!bForceLoad && IsLevelOpened(Level)))
	{
		// Already in main level
		return;
	}

	constexpr bool bAbsolute = true;
	static const FString Options = TEXT("listen");
	UGameplayStatics::OpenLevelBySoftObjectPtr(UUtilsLibrary::GetPlayWorld(), Level, bAbsolute, Options);
}

/*********************************************************************************************
 * Modular Game Features (MGF)
 ********************************************************************************************* */

void UGameplayUtilsLibrary::SetGameFeaturesEnabled(bool bEnable, const TArray<FName>& GameFeatures)
{
	if (GameFeatures.IsEmpty())
	{
		return;
	}

	UGameFeaturesSubsystem& GameFeaturesSubsystem = UGameFeaturesSubsystem::Get();
	for (const FName GameFeatureName : GameFeatures)
	{
		if (GameFeatureName.IsNone())
		{
			continue;
		}

		FString GameFeatureURL;
		GameFeaturesSubsystem.GetPluginURLByName(GameFeatureName.ToString(), /*out*/ GameFeatureURL);
		if (!ensureMsgf(!GameFeatureURL.IsEmpty(), TEXT("ASSERT: [%i] %hs:\n'%s' game feature state can not be changed!"), __LINE__, __FUNCTION__, *GameFeatureName.ToString()))
		{
			continue;
		}

		static const FGameFeaturePluginLoadComplete EmptyCallback{};
		if (bEnable)
		{
			GameFeaturesSubsystem.LoadAndActivateGameFeaturePlugin(GameFeatureURL, EmptyCallback);
		}
		else
		{
			GameFeaturesSubsystem.UnloadGameFeaturePlugin(GameFeatureURL, EmptyCallback, UUtilsLibrary::IsEditor());
		}
	}
}

/*********************************************************************************************
 * Global functions
 ********************************************************************************************* */

void AsyncTaskGameThread(const UObject* WorldContextObject, TFunction<void()> Function)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!ensureMsgf(World, TEXT("ASSERT: [%i] %hs:\n'World' is not valid!"), __LINE__, __FUNCTION__)
	    || !ensureMsgf(Function, TEXT("ASSERT: [%i] %hs:\n'Function' is not bound!"), __LINE__, __FUNCTION__))
	{
		return;
	}

#if WITH_EDITOR
	if (UUtilsLibrary::IsEditor())
	{
		// In editor, redirect the callback to the correct world context to avoid issues with PIE multiplayer
		AsyncTask(ENamedThreads::GameThread, [WeakWorld = TWeakObjectPtr(World), Function = MoveTemp(Function)]() mutable -> void
		{
			if (const UWorld* InWorld = WeakWorld.Get())
			{
				InWorld->GetTimerManager().SetTimerForNextTick([Function = MoveTemp(Function)]() mutable -> void
				{
					Function();
				});
			}
		});
		return;
	}
#endif

	// Direct dispatch in non-editor builds
	AsyncTask(ENamedThreads::GameThread, MoveTemp(Function));
}
