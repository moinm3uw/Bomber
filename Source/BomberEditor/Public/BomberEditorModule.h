﻿// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "Modules/ModuleInterface.h"
//---
#include "Logging/LogMacros.h"

/** Define Bomber Editor log category. */
BOMBEREDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogBomberEditor, Log, All);

class BOMBEREDITOR_API FBomberEditorModule final : public IModuleInterface
{
public:
	/** Is used to load and unload the Property Editor Module. */
	inline static const FName PropertyEditorModule = TEXT("PropertyEditor");

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 * Load dependent modules here, and they will be guaranteed to be available during ShutdownModule.
	 */
	virtual void StartupModule() override;

	/**
	* Called before the module is unloaded, right before the module object is destroyed.
	* During normal shutdown, this is called in reverse order that modules finish StartupModule().
	* This means that, as long as a module references dependent modules in it's StartupModule(), it
	* can safely reference those dependencies in ShutdownModule() as well.
	*/
	virtual void ShutdownModule() override;
};
