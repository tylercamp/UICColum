
//#include <fbxsdk.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "QuickData.Unity.h"

#include "../FbxPartitioning/BinaryMesh.h"
#include "../FbxPartitioning/VolumeMeshTimeline.h"

#pragma comment( lib, "libfbxsdk.lib" )


//	http://ericeastwood.com/blog/17/unity-and-dlls-c-managed-and-c-unmanaged




ParsedMeshStructure * LoadBinaryMesh( const char * targetPath )
{
	BinaryMesh mesh( targetPath );

	ParsedMeshStructure * result = new ParsedMeshStructure( );
	result->path = targetPath;
	result->numTris = mesh.numTris;
	//	float_3 data should just be contiguous floats
	result->colorData = reinterpret_cast<float *>( mesh.colors );
	result->normalData = reinterpret_cast<float *>( mesh.normals );
	result->vertexData = reinterpret_cast<float *>( mesh.vertices );
	result->volumeData = mesh.volumes;

	//	Take ownership of these arrays, avoid copy
	mesh.colors = nullptr;
	mesh.normals = nullptr;
	mesh.vertices = nullptr;
	mesh.volumes = nullptr;


	return result;
}







bool FileExists( const std::string & path )
{
	std::ifstream f( path );
	bool exists = f.is_open( );
	f.close( );
	return exists;
}

//#ifdef _DEBUG
int main( )
{
	auto start = clock( );
	//auto meshSet = LoadMeshSet( "E:\\Dropbox\\HotRepos\\UICColum\\QuickData\\FbxPartitioning\\bsCSF\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "C:\\Users\\algor\\Dropbox\\hotrepos\\UICColum\\QuickData\\FbxPartitioning\\IanArteries5.GAMBIT\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "E:\\Dropbox\\HotRepos\\UICColum\\QuickData\\FbxPartitioning\\IanArteries5.GAMBIT\\volumes\\", "*.binmesh" );
	//LoadMeshSet( "MESHES\\IanArteries5.GAMBIT\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "../FbxPartitioning/patients/bs/bsCSF/surfaces/", "*.binmesh" );

	//VolumeMeshTimeline timeline;
	//timeline.LoadFrom( "FullCNS_Drug_CellCenters_T1.dynamic.binvolumes" );

	//std::cout << "Loaded " << meshSet->meshes.size( ) << " meshes" << std::endl;

	std::cout << "Took " << (clock( ) - start) << "ms";

	std::string j;
	std::getline( std::cin, j );

	return 0;
}

//#endif
