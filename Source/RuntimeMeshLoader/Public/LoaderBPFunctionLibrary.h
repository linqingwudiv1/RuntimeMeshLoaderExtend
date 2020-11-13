// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "ProceduralMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoaderBPFunctionLibrary.generated.h"


class UProceduralMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EPathType : uint8
{
	Absolute UMETA(DisplayName = "Absolute"),
	Relative UMETA(DisplayName = "Relative")
};

USTRUCT(BlueprintType)
struct FMeshInfo
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		/** Vertices index */
		TArray<int32> Triangles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector> Normals;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector2D> UV0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector2D> UV1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector2D> UV2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector2D> UV3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FLinearColor> VertexColors;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FProcMeshTangent> Tangents;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		FTransform RelativeTransform;
};

USTRUCT(BlueprintType)
struct FReturnedData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		/**/
		bool bSuccess;

	/** Contain Mesh Count  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		int32 NumMeshes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FMeshInfo> meshInfo;
};




/**
 * 
 */
UCLASS()
class RUNTIMEMESHLOADER_API ULoaderBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	/**  */
	UFUNCTION( BlueprintCallable, Category="MeshLoader")
		static FReturnedData LoadMesh(const FString& filepath,EPathType type= EPathType:: Absolute, const FTransform& tran = FTransform());

	/** only static mesh */
	UFUNCTION(BlueprintCallable, Category="MeshLoader" )
		static void LoadMeshToProceduralMesh(UProceduralMeshComponent* target, const FString& filepath, EPathType type, const FTransform& tran = FTransform());

	/**  */
	UFUNCTION( BlueprintCallable, Category = "MeshLoader", meta = ( HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject" )  )
		static UStaticMesh* LoadMeshToStaticMesh( UObject* WorldContextObject, 
												  const FString& filepath, 
												  EPathType type = EPathType::Absolute,
												  const FTransform &tran = FTransform() );
	/**  */
	UFUNCTION( BlueprintCallable, Category = "MeshLoader", meta = ( HidePin = "WorldContextObject",DefaultToSelf = "WorldContextObject" )  )
		static UStaticMesh* LoadMeshToStaticMeshFromProceduralMesh(UObject* WorldContextObject, UProceduralMeshComponent* ProcMeshComp);


	UFUNCTION( BlueprintCallable, Category = "MeshLoader", meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject") )
		static UStaticMesh* TryNewStaticMesh( UObject* WorldContextObject, UProceduralMeshComponent* ProcMeshComp);
};
