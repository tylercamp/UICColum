#pragma once

#include <vector>
#include <string>

#define UNITY_NATIVE_EXPORT extern "C" __declspec(dllexport)

#define NOT_YET_IMPLEMENTED( ) __debugbreak( )

struct ParsedMeshStructure
{
	std::string path;

	float * vertexData;
	float * normalData;
	float * colorData;

	int numTris;

	ParsedMeshStructure( ) : vertexData( nullptr ), normalData( nullptr ), colorData( nullptr ), numTris( 0 )
	{

	}
};

struct MeshDataset
{
	std::vector<ParsedMeshStructure *> meshes;
};

ParsedMeshStructure * LoadFbxMesh( const char * targetPath );
UNITY_NATIVE_EXPORT MeshDataset * LoadMeshSet( const char * searchPath, const char * searchString );