// Fill out your copyright notice in the Description page of Project Settings.
#include "LoaderBPFunctionLibrary.h"
#include "RuntimeMeshLoader.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshAttributes.h"

#include "ProceduralMeshConversion.h"

#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistryModule.h"


#include "Engine.h"

#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags




void FindMeshInfo(const aiScene* scene, aiNode* node, FReturnedData& result,const FTransform &tran = FTransform())
{
	//transform...
	aiMatrix4x4 TranMat,tempMat;
	
	
	bool bTran = false;
	if ( !tran.GetLocation().Equals(FVector{ 0.0f }, 0.01f ) )
	{
		bTran = true;
		TranMat = TranMat.Translation(aiVector3D{ tran.GetLocation().X, tran.GetLocation().Y, tran.GetLocation().Z }, tempMat);
	}

	if ( !tran.GetScale3D().Equals( FVector{ 0.0f }, 0.01f ) )
	{
		bTran = true;
		TranMat = TranMat.Scaling(aiVector3D{ tran.GetScale3D().X, tran.GetScale3D().Y, tran.GetScale3D().Z }, tempMat);
	}

	if ( !tran.GetRotation().Equals( FRotator{ 0.0f }.Quaternion(), 0.01f ) )
	{
		bTran = true;
		TranMat = TranMat.RotationX( PI / 180.f * tran.GetRotation().Rotator().Roll	, tempMat  );
		TranMat = TranMat.RotationY( PI / 180.f * tran.GetRotation().Rotator().Yaw	, tempMat  );
		TranMat = TranMat.RotationZ( PI / 180.f * tran.GetRotation().Rotator().Pitch	, tempMat  );
	}

	for (uint32 i = 0; i < node->mNumMeshes; i++)
	{
		std::string TestString = node->mName.C_Str();

		FString Fs = FString(TestString.c_str());

		UE_LOG(LogTemp, Warning, TEXT("FindMeshInfo. %s\n"), *Fs);

		int meshidx = *node->mMeshes;

		aiMesh	  *mesh = scene->mMeshes  [ meshidx ];
		FMeshInfo &mi	= result.meshInfo [ meshidx ];
		aiMatrix4x4 tempTrans = node->mTransformation;
		//如果变换
		if (bTran)
		{
			tempTrans = tempTrans * TranMat;
		}

		FMatrix tempMatrix;

		// e.g
		// _______________
		// | A0,B0,C0,D0 |
		// | A1,B1,C1,D1 |
		// | A2,B2,C2,D2 |
		// | A3,B3,C3,D3 |
		// |_____________|
		// 
		tempMatrix.M[0][0]	 =	tempTrans.a1; tempMatrix.M[0][1] = tempTrans.b1; tempMatrix.M[0][2] = tempTrans.c1; tempMatrix.M[0][3] = tempTrans.d1;
		tempMatrix.M[1][0]	 =	tempTrans.a2; tempMatrix.M[1][1] = tempTrans.b2; tempMatrix.M[1][2] = tempTrans.c2; tempMatrix.M[1][3] = tempTrans.d2;
		tempMatrix.M[2][0]	 =	tempTrans.a3; tempMatrix.M[2][1] = tempTrans.b3; tempMatrix.M[2][2] = tempTrans.c3; tempMatrix.M[2][3] = tempTrans.d3;
		tempMatrix.M[3][0]	 =	tempTrans.a4; tempMatrix.M[3][1] = tempTrans.b4; tempMatrix.M[3][2] = tempTrans.c4; tempMatrix.M[3][3] = tempTrans.d4;

		// Mesh transform on scene
		mi.RelativeTransform =	FTransform(tempMatrix);

		// fill Mesh Vertices 填充Mesh顶点
		for (uint32 j = 0; j < mesh->mNumVertices; ++j)
		{
			FVector vertex = FVector (
				mesh->mVertices[j].x ,
				mesh->mVertices[j].y ,
				mesh->mVertices[j].z );

			vertex = mi.RelativeTransform.TransformFVector4(vertex);
			// vertex = mi.RelativeTransform.Trans
			mi.Vertices.Push( vertex );

			//Normal
			if (mesh->HasNormals())
			{
				FVector normal = FVector(
					mesh->mNormals[j].x ,
					mesh->mNormals[j].y ,
					mesh->mNormals[j].z  );

				// normal = mi.RelativeTransform.TransformFVector4(normal);
				mi.Normals.Push(normal);
			}
			else
			{
				mi.Normals.Push(FVector::ZeroVector);
			}

			// UV0 Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(0))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[0][j].x, -mesh->mTextureCoords[0][j].y);
				mi.UV0.Add(uv);
			}
			// UV1 Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(1))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[1][j].x, -mesh->mTextureCoords[1][j].y);
				mi.UV1.Add(uv);
			}
			// UV2 Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(2))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[2][j].x, -mesh->mTextureCoords[2][j].y);
				mi.UV2.Add(uv);
			}

			// UV3 Coordinates - inconsistent coordinates
			if (mesh->HasTextureCoords(3))
			{
				FVector2D uv = FVector2D(mesh->mTextureCoords[3][j].x, -mesh->mTextureCoords[3][j].y);
				mi.UV3.Add(uv);
			}

			// Tangent /切线
			if (mesh->HasTangentsAndBitangents())
			{
				FProcMeshTangent meshTangent = FProcMeshTangent(
					mesh->mTangents[j].x,
					mesh->mTangents[j].y,
					mesh->mTangents[j].z
				);

				mi.Tangents.Push(meshTangent);
			}

			//Vertex color
			if (mesh->HasVertexColors(0))
			{
				FLinearColor color = FLinearColor(
					mesh->mColors[0][j].r,
					mesh->mColors[0][j].g,
					mesh->mColors[0][j].b,
					mesh->mColors[0][j].a
				);
				mi.VertexColors.Push(color);
			}
		}
	}
}

void FindMesh(const aiScene* scene, aiNode* node, FReturnedData& retdata, const  FTransform &tran)
{
	FindMeshInfo(scene, node, retdata);

	// tree node 
	for ( uint32 m = 0; m < node->mNumChildren; ++m )
	{
		FindMesh(scene, node->mChildren[m], retdata, tran);
	}
}

#pragma region copy


TMap<UMaterialInterface*, FPolygonGroupID> BuildMaterialMapEx(UProceduralMeshComponent* ProcMeshComp, FMeshDescription& MeshDescription)
{
	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials;
	const int32 NumSections = ProcMeshComp->GetNumSections();

	UniqueMaterials.Reserve(NumSections);

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();

	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection = ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface* Material = ProcMeshComp->GetMaterial(SectionIdx);

		if (!UniqueMaterials.Contains(Material))
		{
			FPolygonGroupID NewPolygonGroup = MeshDescription.CreatePolygonGroup();
			UniqueMaterials.Add(Material, NewPolygonGroup);
			PolygonGroupNames[NewPolygonGroup] = Material->GetFName();
		}
	}
	return UniqueMaterials;
}


FMeshDescription BuildMeshDescriptionEx(UProceduralMeshComponent* ProcMeshComp)
{
	FMeshDescription MeshDescription;

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	AttributeGetter.Register();

	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	TVertexAttributesRef<FVector> VertexPositions = AttributeGetter.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector> Tangents = AttributeGetter.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns = AttributeGetter.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector> Normals = AttributeGetter.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4> Colors = AttributeGetter.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2D> UVs = AttributeGetter.GetVertexInstanceUVs();

	// Materials to apply to new mesh
	const int32 NumSections = ProcMeshComp->GetNumSections();
	int32 VertexCount = 0;
	int32 VertexInstanceCount = 0;
	int32 PolygonCount = 0;

	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials = BuildMaterialMapEx(ProcMeshComp, MeshDescription);
	TArray<FPolygonGroupID> PolygonGroupForSection;
	PolygonGroupForSection.Reserve(NumSections);

	// Calculate the totals for each ProcMesh element type
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		VertexCount += ProcSection->ProcVertexBuffer.Num();
		VertexInstanceCount += ProcSection->ProcIndexBuffer.Num();
		PolygonCount += ProcSection->ProcIndexBuffer.Num() / 3;
	}
	MeshDescription.ReserveNewVertices(VertexCount);
	MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
	MeshDescription.ReserveNewPolygons(PolygonCount);
	MeshDescription.ReserveNewEdges(PolygonCount * 2);
	UVs.SetNumIndices(4);

	// Create the Polygon Groups
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		UMaterialInterface* Material = ProcMeshComp->GetMaterial(SectionIdx);
		FPolygonGroupID* PolygonGroupID = UniqueMaterials.Find(Material);
		check(PolygonGroupID != nullptr);
		PolygonGroupForSection.Add(*PolygonGroupID);
	}


	// Add Vertex and VertexInstance and polygon for each section
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FProcMeshSection* ProcSection =
			ProcMeshComp->GetProcMeshSection(SectionIdx);
		FPolygonGroupID PolygonGroupID = PolygonGroupForSection[SectionIdx];
		// Create the vertex
		int32 NumVertex = ProcSection->ProcVertexBuffer.Num();
		TMap<int32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(NumVertex);
		
		for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
		{
			FProcMeshVertex& Vert = ProcSection->ProcVertexBuffer[VertexIndex];
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = Vert.Position;
			VertexIndexToVertexID.Add(VertexIndex, VertexID);
		}
		// Create the VertexInstance
		int32 NumIndices = ProcSection->ProcIndexBuffer.Num();
		int32 NumTri = NumIndices / 3;
		TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
		IndiceIndexToVertexInstanceID.Reserve(NumVertex);
		for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
		{
			const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
			const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
			const FVertexInstanceID VertexInstanceID =
				MeshDescription.CreateVertexInstance(VertexID);
			IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);

			FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[VertexIndex];

			Tangents[VertexInstanceID] = ProcVertex.Tangent.TangentX;
			Normals[VertexInstanceID] = ProcVertex.Normal;
			BinormalSigns[VertexInstanceID] =
				ProcVertex.Tangent.bFlipTangentY ? -1.f : 1.f;

			Colors[VertexInstanceID] = FLinearColor(ProcVertex.Color);

			UVs.Set(VertexInstanceID, 0, ProcVertex.UV0);
			UVs.Set(VertexInstanceID, 1, ProcVertex.UV1);
			UVs.Set(VertexInstanceID, 2, ProcVertex.UV2);
			UVs.Set(VertexInstanceID, 3, ProcVertex.UV3);
		}

		// Create the polygons for this section
		for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
		{
			FVertexID VertexIndexes[3];
			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);

			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;
				const int32 VertexIndex = ProcSection->ProcIndexBuffer[IndiceIndex];
				VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
				VertexInstanceIDs[CornerIndex] =
					IndiceIndexToVertexInstanceID[IndiceIndex];
			}

			// Insert a polygon into the mesh
			MeshDescription.CreatePolygon(PolygonGroupID, VertexInstanceIDs);
		}
	}
	return MeshDescription;
}

/**
 *
 */
TMap<UMaterialInterface*, FPolygonGroupID> BuildMaterialMapExchange(FReturnedData& ReturnedData, /* UProceduralMeshComponent* ProcMeshComp ,*/ FMeshDescription& MeshDescription)
{
	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials;
	const int32 NumSections =  ReturnedData.meshInfo.Num(); //ProcMeshComp->GetNumSections();
	UniqueMaterials.Reserve(NumSections);

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();

	for ( int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++ )
	{
		FMeshInfo MeshInfo = ReturnedData.meshInfo[SectionIdx];
		// MeshInfo.Normals
		UMaterialInterface* Material = UMaterial::GetDefaultMaterial(MD_Surface);
		
		if ( !UniqueMaterials.Contains(Material) )
		{
			FPolygonGroupID NewPolygonGroup = MeshDescription.CreatePolygonGroup();
			
			UniqueMaterials.Add(Material, NewPolygonGroup);
			PolygonGroupNames[NewPolygonGroup] = Material->GetFName();
		}
	}
	
	return UniqueMaterials;
}


/**
 *
 */
FMeshDescription BuildMeshDescriptionExtend( FReturnedData& MeshsData /* UProceduralMeshComponent* ProcMeshComp */)
{
	FMeshDescription MeshDescription;

	FStaticMeshAttributes AttributeGetter(MeshDescription);
	AttributeGetter.Register();

	TPolygonGroupAttributesRef<FName> PolygonGroupNames = AttributeGetter.GetPolygonGroupMaterialSlotNames();
	TVertexAttributesRef<FVector> VertexPositions		= AttributeGetter.GetVertexPositions();
	TVertexInstanceAttributesRef<FVector> Tangents		= AttributeGetter.GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> BinormalSigns	= AttributeGetter.GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector> Normals		= AttributeGetter.GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector4> Colors		= AttributeGetter.GetVertexInstanceColors();
	TVertexInstanceAttributesRef<FVector2D> UVs			= AttributeGetter.GetVertexInstanceUVs();

	// Materials to apply to new mesh
	
	const int32 NumSections = MeshsData.meshInfo.Num(); // ProcMeshComp->GetNumSections();
	int32 VertexCount = 0;
	int32 VertexInstanceCount = 0;
	int32 PolygonCount = 0;

	TMap<UMaterialInterface*, FPolygonGroupID> UniqueMaterials = BuildMaterialMapExchange(MeshsData, MeshDescription);
	TArray<FPolygonGroupID> PolygonGroupForSection;
	PolygonGroupForSection.Reserve(NumSections);
	// Calculate the totals for each ProcMesh element type
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FMeshInfo MeshInfo = MeshsData.meshInfo[SectionIdx];

		VertexCount			+=  MeshInfo.Vertices.Num ()	 ; // ProcSection->ProcVertexBuffer.Num();
		VertexInstanceCount +=  MeshInfo.Triangles.Num()	 ; // ProcSection->ProcIndexBuffer.Num();
		PolygonCount		+=  MeshInfo.Triangles.Num() / 3 ; // ProcSection->ProcIndexBuffer.Num() / 3;
	}

	MeshDescription.ReserveNewVertices(VertexCount);
	MeshDescription.ReserveNewVertexInstances(VertexInstanceCount);
	MeshDescription.ReserveNewPolygons( PolygonCount );
	MeshDescription.ReserveNewEdges( PolygonCount * 2);
	UVs.SetNumIndices(4);
	
	// Create the Polygon Groups
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FMeshInfo MeshInfo = MeshsData.meshInfo[SectionIdx];

		UMaterialInterface* Material = UMaterial::GetDefaultMaterial(MD_Surface);

		FPolygonGroupID* PolygonGroupID = UniqueMaterials.Find(Material);
		check( PolygonGroupID != nullptr );
		PolygonGroupForSection.Add(*PolygonGroupID);
	}
	
	// Add Vertex and VertexInstance and polygon for each section
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FMeshInfo MeshInfo = MeshsData.meshInfo[SectionIdx];
		FPolygonGroupID PolygonGroupID = PolygonGroupForSection[SectionIdx];
		// Create the vertex
		int32 NumVertex = MeshInfo.Vertices.Num();
		TMap<int32, FVertexID> VertexIndexToVertexID;
		VertexIndexToVertexID.Reserve(NumVertex);

		for (int32 VertexIndex = 0; VertexIndex < NumVertex; ++VertexIndex)
		{
			FVector Vert = MeshInfo.Vertices[VertexIndex];

			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = Vert;
			VertexIndexToVertexID.Add(VertexIndex, VertexID);
		}

		// Create the VertexInstance
		int32 NumIndices = MeshInfo.Triangles.Num();

		int32 NumTri = NumIndices / 3;
		TMap<int32, FVertexInstanceID> IndiceIndexToVertexInstanceID;
		IndiceIndexToVertexInstanceID.Reserve(NumVertex);
		for (int32 IndiceIndex = 0; IndiceIndex < NumIndices; IndiceIndex++)
		{
			const int32 VertexIndex = MeshInfo.Triangles[IndiceIndex];
			const FVertexID VertexID = VertexIndexToVertexID[VertexIndex];
			const FVertexInstanceID VertexInstanceID = MeshDescription.CreateVertexInstance(VertexID);
			IndiceIndexToVertexInstanceID.Add(IndiceIndex, VertexInstanceID);

			FVector ProcVertex = MeshInfo.Vertices[VertexIndex];		// FProcMeshVertex& ProcVertex = ProcSection->ProcVertexBuffer[VertexIndex];
			FProcMeshTangent VertexTanents = MeshInfo.Tangents[VertexIndex];
			
			FLinearColor VertexColor = MeshInfo.VertexColors.Num() > VertexIndex ? MeshInfo.VertexColors[VertexIndex] : FLinearColor(1.0, 0.0, 0.0);

			Tangents[VertexInstanceID] = VertexTanents.TangentX;		// ProcVertex.Tangent.TangentX;
			Normals[VertexInstanceID] = MeshInfo.Normals[VertexIndex];	// ProcVertex.Normal;
			BinormalSigns[VertexInstanceID] = VertexTanents.bFlipTangentY ? -1.f : 1.f;

			Colors[VertexInstanceID] = VertexColor; //FLinearColor(ProcVertex.Color);
			
			if ( MeshInfo.UV0.Num() > VertexIndex)
			{
				UVs.Set(VertexInstanceID, 0, MeshInfo.UV0[VertexIndex]);
			}

			if ( MeshInfo.UV1.Num() > VertexIndex)
			{
				UVs.Set(VertexInstanceID, 1, MeshInfo.UV1[VertexIndex]);
			}

			if ( MeshInfo.UV2.Num() > VertexIndex )
			{
				UVs.Set(VertexInstanceID, 2, MeshInfo.UV2[VertexIndex]);
			}
			
			if ( MeshInfo.UV3.Num() > VertexIndex )
			{
				UVs.Set(VertexInstanceID, 3, MeshInfo.UV3[VertexIndex]);
			}
		}

		// Create the polygons for this section
		for (int32 TriIdx = 0; TriIdx < NumTri; TriIdx++)
		{
			FVertexID VertexIndexes[3];
			TArray<FVertexInstanceID> VertexInstanceIDs;
			VertexInstanceIDs.SetNum(3);

			for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
			{
				const int32 IndiceIndex = (TriIdx * 3) + CornerIndex;
				const int32 VertexIndex =  MeshInfo.Triangles[IndiceIndex]; //ProcSection->ProcIndexBuffer[IndiceIndex];
				VertexIndexes[CornerIndex] = VertexIndexToVertexID[VertexIndex];
				VertexInstanceIDs[CornerIndex] =
					IndiceIndexToVertexInstanceID[IndiceIndex];
			}

			// Insert a polygon into the mesh
			MeshDescription.CreatePolygon(PolygonGroupID, VertexInstanceIDs);
		}
	}
	return MeshDescription;
}

#pragma endregion copy


FReturnedData ULoaderBPFunctionLibrary::LoadMesh(const FString& filepath, const FTransform& tran, EPathType type )
{
	FReturnedData result;
	result.bSuccess = false;
	result.meshInfo.Empty();
	result.NumMeshes = 0;
	
	if (filepath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: filepath is empty.\n"));
		return result;
	}

	std::string file;
	switch (type)
	{
		case EPathType::Absolute:
		{
			file = TCHAR_TO_UTF8(*filepath);
			break;
		}
		case EPathType::Relative:
		{
			file = TCHAR_TO_UTF8(*FPaths::Combine(FPaths::ProjectContentDir(), filepath));
			break;
		}
	}

	Assimp::Importer mImporter;

	const aiScene* mScenePtr = mImporter.ReadFile(file, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes);

	if (mScenePtr == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: Read mesh file failure.\n"));
		return result;
	}

	if (mScenePtr->HasMeshes())
	{
		result.meshInfo.SetNum(mScenePtr->mNumMeshes, false);

		FindMesh(mScenePtr, mScenePtr->mRootNode, result, tran);

		for ( uint32 i = 0; i < mScenePtr->mNumMeshes; ++i )
		{
			// Triangle number
			for ( uint32 l = 0; l < mScenePtr->mMeshes[i]->mNumFaces; ++l )
			{
				for ( uint32 m = 0; m < mScenePtr->mMeshes[i]->mFaces[l].mNumIndices; ++m )
				{
					result.meshInfo[i].Triangles.Push(mScenePtr->mMeshes[i]->mFaces[l].mIndices[m]);
				}
			}
		}

		result.bSuccess = true;
	}

	return result;
}

void ULoaderBPFunctionLibrary::LoadMeshToProceduralMesh(UProceduralMeshComponent* target, const FString& filepath, const FTransform& tran, EPathType type)
{
	FReturnedData &&MeshInfo = ULoaderBPFunctionLibrary::LoadMesh(filepath,  tran,type );
	if (MeshInfo.bSuccess)
	{
		for ( int i = 0; i < MeshInfo.meshInfo.Num(); i++ )
		{
			FMeshInfo Info = MeshInfo.meshInfo[i];
			

			TArray<FVector2D> EmptyArray;
			// FOccluderVertexArray
			target->CreateMeshSection_LinearColor(	i					, 
													Info.Vertices		,
													Info.Triangles		, 
													Info.Normals		, 
													Info.UV0			, 
													Info.UV1			,
													Info.UV2			,
													Info.UV3			,
													Info.VertexColors	, 
													Info.Tangents		,
													true				 );
		}

	}
}

UStaticMesh* ULoaderBPFunctionLibrary::LoadMeshToStaticMesh( UObject* WorldContextObject, 
															 const FString& filepath, 
															 const FTransform& tran,
															 EPathType type /* = EPathType::Absolute */ 
															  )
{
	FReturnedData&& MeshInfo = ULoaderBPFunctionLibrary::LoadMesh(filepath,  tran, type);

	FString NewNameSuggestion = FString(TEXT("ProcMesh"));
	FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
	FString Name;
	FString UserPackageName = TEXT("");
	FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

	// Check if the user inputed a valid asset name, if they did not, give it the generated default name
	if (MeshName == NAME_None)
	{
		// Use the defaults that were already generated.
		UserPackageName = PackageName;
		MeshName = *Name;
	}

	FMeshDescription MeshDescription = BuildMeshDescriptionExtend(MeshInfo);

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(WorldContextObject, MeshName, RF_Public | RF_Standalone);
	
	StaticMesh->InitResources();
	StaticMesh->LightingGuid = FGuid::NewGuid();

	TArray<const FMeshDescription*> arr;
	arr.Add(&MeshDescription);
	StaticMesh->BuildFromMeshDescriptions(arr, false);
	
	//// MATERIALS
	TSet<UMaterialInterface*> UniqueMaterials;
	
	const int32 NumSections = 1;
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		UMaterialInterface* Material = UMaterial::GetDefaultMaterial(MD_Surface);
		UniqueMaterials.Add(Material);
		
	}

	// Copy materials to new mesh
	int32 MaterialID = 0;
	for (UMaterialInterface* Material : UniqueMaterials)
	{
		// Material
		FStaticMaterial&& StaticMat = FStaticMaterial(Material);

		StaticMat.UVChannelData.bInitialized = true;
		StaticMesh->StaticMaterials.Add(StaticMat);		

#pragma region 模拟填充 FMeshSectionInfo

		FStaticMeshRenderData* const RenderData = StaticMesh->RenderData.Get();

		int32 LODIndex = 0;
		int32 MaxLODs = RenderData->LODResources.Num();

		for (; LODIndex < MaxLODs; ++LODIndex)
		{
			FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];

			for (int32 SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
			{
				FStaticMeshSection& Section = LOD.Sections[SectionIndex];
				Section.MaterialIndex = MaterialID;
				Section.bEnableCollision = true;
				Section.bCastShadow = true;
				Section.bForceOpaque = false;
			}
		}

#pragma endregion 	
		MaterialID++;
	}

	return StaticMesh;
}


UStaticMesh* ULoaderBPFunctionLibrary::LoadMeshToStaticMeshFromProceduralMesh(UObject* WorldContextObject, UProceduralMeshComponent* ProcMeshComp)
{
	FString NewNameSuggestion = FString(TEXT("ProcMesh"));
	FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
	FString Name;
	FString UserPackageName = TEXT("");

	// FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	// AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

	FName MeshName( *FPackageName::GetLongPackageAssetName( UserPackageName ) );

	// FName MeshName( *FPackageName::GetLongPackageAssetName( UserPackageName ) );
	// Check if the user inputed a valid asset name, if they did not, give it the generated default name
	if (MeshName == NAME_None)
	{
		// Use the defaults that were already generated.
		UserPackageName = PackageName;
		MeshName = *Name;
	}

	FMeshDescription MeshDescription = BuildMeshDescriptionEx(ProcMeshComp);

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(WorldContextObject, MeshName, RF_Public | RF_Standalone);

	StaticMesh->InitResources();
	StaticMesh->LightingGuid = FGuid::NewGuid();

	TArray<const FMeshDescription*> arr;

	arr.Add(&MeshDescription);
	StaticMesh->BuildFromMeshDescriptions(arr, false);

	//// MATERIALS
	TSet<UMaterialInterface*> UniqueMaterials;

	const int32 NumSections = 1;
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		UMaterialInterface* Material = UMaterial::GetDefaultMaterial(MD_Surface);
		UniqueMaterials.Add(Material);
	}

	#pragma region material test
	for (UMaterialInterface* Material : UniqueMaterials)
	{
		// Material
		FStaticMaterial&& StaticMat = FStaticMaterial(Material);

		StaticMat.UVChannelData.bInitialized = true;
		StaticMesh->StaticMaterials.Add(StaticMat);

	}
	#pragma region 模拟填充 FMeshSectionInfo

		FStaticMeshRenderData* const RenderData = StaticMesh->RenderData.Get();


		int32 LODIndex = 0;
		int32 MaxLODs = RenderData->LODResources.Num();
		for (int32 MaterialID = 0; LODIndex < MaxLODs; ++LODIndex)
		{
			FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];

			for (int32 SectionIndex = 0; SectionIndex < LOD.Sections.Num(); ++SectionIndex)
			{
				FStaticMeshSection& Section = LOD.Sections[SectionIndex];
				Section.MaterialIndex = MaterialID;
				Section.bEnableCollision = true;
				Section.bCastShadow = true;
				Section.bForceOpaque = false;
				MaterialID++;
			}
		}

	#pragma endregion 


	

	#pragma endregion material test
	
	//StaticMesh->Build(false);
	return StaticMesh;
}

UStaticMesh* ULoaderBPFunctionLibrary::TryNewStaticMesh(UObject* WorldContextObject, UProceduralMeshComponent* ProcMeshComp)
{
	FString NewNameSuggestion = FString(TEXT("ProcMesh"));
	FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
	FString Name;
	FString UserPackageName = TEXT("");
	FName MeshName( *FPackageName::GetLongPackageAssetName(UserPackageName) );

	// FName MeshName( *FPackageName::GetLongPackageAssetName( UserPackageName ) )
	// Check if the user inputed a valid asset name, if they did not, give it the generated default name
	if (MeshName == NAME_None)
	{
		// Use the defaults that were already generated.
		UserPackageName = PackageName;
		MeshName = *Name;
	}

	FMeshDescription MeshDescription = BuildMeshDescription(ProcMeshComp);

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(WorldContextObject, MeshName, RF_Public | RF_Standalone);
	StaticMesh->InitResources();
	StaticMesh->LightingGuid = FGuid::NewGuid();

	TArray<const FMeshDescription*> arr;
	arr.Add(&MeshDescription);
	StaticMesh->BuildFromMeshDescriptions(arr, false);

	return StaticMesh;
}
