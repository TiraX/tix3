// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TiXDefines.generated.h"

struct FTiXExporterSetting
{
	float TileSize;
	float MeshVertexPositionScale;
	bool bIgnoreMaterial;
	FString ExportPath;
	FString ExtName;

	static FTiXExporterSetting Setting;

	FTiXExporterSetting()
		: TileSize(16.f)
		, MeshVertexPositionScale(0.01f)
		, bIgnoreMaterial(false)
		, ExtName(TEXT(".tasset"))
	{}
};

inline TArray<float> ToArray(const FVector& Vec)
{
	TArray<float> Array;
	Array.Reserve(3);
	Array.Add(Vec.X);
	Array.Add(Vec.Y);
	Array.Add(Vec.Z);
	return Array;
};
inline TArray<float> ToArray(const FVector4& Vec)
{
	TArray<float> Array;
	Array.Reserve(4);
	Array.Add(Vec.X);
	Array.Add(Vec.Y);
	Array.Add(Vec.Z);
	Array.Add(Vec.W);
	return Array;
};
inline TArray<float> ToArray(const FRotator& Rot)
{
	TArray<float> Array;
	Array.Reserve(3);
	Array.Add(Rot.Pitch);
	Array.Add(Rot.Yaw);
	Array.Add(Rot.Roll);
	return Array;
};
inline TArray<float> ToArray(const FQuat& Quat)
{
	TArray<float> Array;
	Array.Reserve(4);
	Array.Add(Quat.X);
	Array.Add(Quat.Y);
	Array.Add(Quat.Z);
	Array.Add(Quat.W);
	return Array;
};
inline TArray<float> ToArray(const FLinearColor& Color)
{
	TArray<float> Array;
	Array.Reserve(4);
	Array.Add(Color.R);
	Array.Add(Color.G);
	Array.Add(Color.B);
	Array.Add(Color.A);
	return Array;
};
inline TArray<int32> ToArray(const FIntPoint& IntPoint)
{
	TArray<int32> Array;
	Array.Reserve(2);
	Array.Add(IntPoint.X);
	Array.Add(IntPoint.Y);
	return Array;
};
inline TArray<float> ToArray(const FBox& Box)
{
	TArray<float> Array;
	Array.Reserve(6);
	Array.Add(Box.Min.X);
	Array.Add(Box.Min.Y);
	Array.Add(Box.Min.Z);
	Array.Add(Box.Max.X);
	Array.Add(Box.Max.Y);
	Array.Add(Box.Max.Z);
	return Array;
};
inline TArray<float> ToArray(const FSHVectorRGB3& SH3)
{
	TArray<float> Array;
	Array.Reserve(TSHVector<3>::NumTotalFloats * 3);
	for (int32 i = 0; i < TSHVector<3>::NumTotalFloats; i++)
	{
		Array.Add(SH3.R.V[i]);
	}
	for (int32 i = 0; i < TSHVector<3>::NumTotalFloats; i++)
	{
		Array.Add(SH3.G.V[i]);
	}
	for (int32 i = 0; i < TSHVector<3>::NumTotalFloats; i++)
	{
		Array.Add(SH3.B.V[i]);
	}
	return Array;
};

// Directional Light
USTRUCT()
struct FTiXDirectionalLight
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	TArray<float> direction;

	UPROPERTY()
	TArray<float> color;

	UPROPERTY()
	float intensity;
};

// Sky Light
USTRUCT()
struct FTiXSkyLight
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	TArray<float> irradiance_sh3;

	UPROPERTY()
	FString cube_source;

	UPROPERTY()
	float cube_angle;
};

// Camera Asset
USTRUCT()
struct FTiXCamera
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> location;

	UPROPERTY()
	TArray<float> target;

	UPROPERTY()
	TArray<float> rotator;

	UPROPERTY()
	float fov;

	UPROPERTY()
	float aspect;
};

// Environment
USTRUCT()
struct FTiXEnvironment
{
	GENERATED_BODY()

	UPROPERTY()
	FTiXDirectionalLight sun_light;

	UPROPERTY()
	FTiXSkyLight sky_light;
};

USTRUCT()
struct FTileIndex
{
	GENERATED_BODY()

	UPROPERTY()
	int32 index[2];

	FTileIndex()
	{
		index[0] = 0;
		index[1] = 0;
	}

	FTileIndex(const FIntPoint& Pt)
	{
		index[0] = Pt.X;
		index[1] = Pt.Y;
	}
};

// Scene
USTRUCT()
struct FTiXSceneDesc
{
	GENERATED_BODY()
		
	UPROPERTY()
	FString name;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	TArray<FTiXCamera> cameras;

	UPROPERTY()
	FTiXEnvironment environment;

	UPROPERTY()
	TArray< FTileIndex > tiles;
};

// Scene Tile Dependency
USTRUCT()
struct FTiXSceneTileDependency
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> textures;

	UPROPERTY()
	TArray<FString> materials;

	UPROPERTY()
	TArray<FString> material_instances;
	
	UPROPERTY()
	TArray<FString> static_meshes;
};

// Static Mesh Instance
USTRUCT()
struct FTiXSMInstance
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<float> position;

	UPROPERTY()
	TArray<float> rotation;

	UPROPERTY()
	TArray<float> scale;
};

// Scene Tile Static Mesh Instance
USTRUCT()
struct FTiXSceneTileSMInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FString linked_mesh;

	UPROPERTY()
	TArray<FTiXSMInstance> instances;
};

// Reflection capture actor
USTRUCT()
struct FTiXReflectionCapture
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString linked_cubemap;

	UPROPERTY()
	int32 cubemap_size;

	UPROPERTY()
	float average_brightness;

	UPROPERTY()
	float brightness;

	UPROPERTY()
	TArray<float> position;
};

// Scene Tile
USTRUCT()
struct FTiXSceneTileDesc
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString level;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	TArray<int32> position;

	UPROPERTY()
	TArray<float> bbox;

	UPROPERTY()
	int32 static_meshes_total;

	UPROPERTY()
	int32 sm_instances_total;
	
	UPROPERTY()
	int32 textures_total;
	
	UPROPERTY()
	int32 reflection_captures_total;

	UPROPERTY()
	FTiXSceneTileDependency dependency;

	UPROPERTY()
	TArray<FTiXReflectionCapture> reflection_captures;

	UPROPERTY()
	TArray<FTiXSceneTileSMInstance> static_mesh_instances;
};

// Static Mesh
USTRUCT()
struct FTiXStaticMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> vs_format;
	
	UPROPERTY()
	TArray<float> vertices;

	UPROPERTY()
	TArray<uint32> indices;
};

USTRUCT()
struct FTiXStaticMeshSection
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;
	
	UPROPERTY()
	FString material_instance;

	UPROPERTY()
	int32 index_start;

	UPROPERTY()
	int32 triangles;
};

USTRUCT()
struct FTiXStaticMesh
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	int32 vertex_count_total;

	UPROPERTY()
	int32 index_count_total;
	
	UPROPERTY()
	int32 texcoord_count;
	
	UPROPERTY()
	int32 total_lod;

	UPROPERTY()
	FTiXStaticMeshData data;

	UPROPERTY()
	TArray<FTiXStaticMeshSection> sections;
};

// Material
USTRUCT()
struct FTiXMaterial
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	TArray<FString> shaders;

	UPROPERTY()
	TArray<FString> vs_format;

	UPROPERTY()
	TArray<FString> ins_format;

	UPROPERTY()
	TArray<FString> rt_colors;

	UPROPERTY()
	FString rt_depth;

	UPROPERTY()
	FString blend_mode;

	UPROPERTY()
	bool depth_write;

	UPROPERTY()
	bool depth_test;

	UPROPERTY()
	bool two_sides;
};

// Material Instance
USTRUCT()
struct FTiXMIParamVector
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString type;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	TArray<float> value;
};

USTRUCT()
struct FTiXMIParamTexture
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString type;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	FString value;
};

USTRUCT()
struct FTiXMaterialInstance
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	FString linked_material;

	UPROPERTY()
	TArray<FTiXMIParamVector> param_vectors;

	UPROPERTY()
	TArray<FTiXMIParamTexture> param_textures;
};


// Texture
USTRUCT()
struct FTiXTexture
{
	GENERATED_BODY()

	UPROPERTY()
	FString name;

	UPROPERTY()
	FString file_type;

	UPROPERTY()
	int32 version;

	UPROPERTY()
	FString desc;

	UPROPERTY()
	FString source;

	UPROPERTY()
	FString texture_type;

	UPROPERTY()
	bool srgb;

	UPROPERTY()
	bool is_normalmap;

	UPROPERTY()
	bool has_mips;

	UPROPERTY()
	bool ibl;

	UPROPERTY()
	int32 width;

	UPROPERTY()
	int32 height;

	UPROPERTY()
	int32 mips;

	UPROPERTY()
	FString address_mode;

	UPROPERTY()
	int32 lod_bias;
};