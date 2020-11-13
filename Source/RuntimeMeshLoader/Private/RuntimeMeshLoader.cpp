// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RuntimeMeshLoader.h"

#define LOCTEXT_NAMESPACE "FRuntimeMeshLoaderModule"



void FRuntimeMeshLoaderModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	//FPlatformProcess::AddDllDirectory();
	const FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("RuntimeMeshLoader"))->GetBaseDir();

	const FString AssimpBinPath = PluginDir / TEXT ( PREPROCESSOR_TO_STRING(ThirdParty/assimp/Win64/Debug)	);
	const FString DLLPath		= AssimpBinPath   / TEXT ( PREPROCESSOR_TO_STRING(assimp-vc141-mtd.dll)		);

	FPlatformProcess::PushDllDirectory(*AssimpBinPath);
	
	this->AssimpDllHandle = FPlatformProcess::GetDllHandle(*DLLPath);
	
	FPlatformProcess::PopDllDirectory(*AssimpBinPath);
}

void FRuntimeMeshLoaderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	if(this->AssimpDllHandle)
	{
		FPlatformProcess::FreeDllHandle(this->AssimpDllHandle);
	}
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRuntimeMeshLoaderModule, RuntimeMeshLoader)