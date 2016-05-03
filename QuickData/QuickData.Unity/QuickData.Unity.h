#pragma once

#include <vector>
#include <string>

#define UNITY_NATIVE_EXPORT extern "C" __declspec(dllexport)

#ifdef _DEBUG
# define NOT_YET_IMPLEMENTED( ) __debugbreak( )
#else
# define NOT_YET_IMPLEMENTED( )
#endif

struct ParsedMeshStructure
{
	std::string path;

	float * vertexData;
	float * normalData;
	float * colorData;

	int * volumeData;

	int numTris;

	ParsedMeshStructure( ) : vertexData( nullptr ), normalData( nullptr ), colorData( nullptr ), volumeData( nullptr ), numTris( 0 )
	{

	}
};

struct MeshDataset
{
	std::vector<ParsedMeshStructure *> meshes;
};

ParsedMeshStructure * LoadBinaryMesh( const char * targetPath );

UNITY_NATIVE_EXPORT bool LoadMeshSet( const char * searchPath, const char * searchString );
UNITY_NATIVE_EXPORT bool LoadVoxelMatrix( const char * storePath );
