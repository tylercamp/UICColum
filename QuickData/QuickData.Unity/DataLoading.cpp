
#include <fbxsdk.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "QuickData.Unity.h"

#include "../FbxPartitioning/BinaryMesh.h"

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





FbxMesh * SearchNode( FbxNode* pNode )
{
	auto mesh = pNode->GetMesh( );
	if( mesh )
		return mesh;

	for( int j = 0; j < pNode->GetChildCount( ); j++ )
	{
		mesh = SearchNode( pNode->GetChild( j ) );
		if( mesh )
			return mesh;
	}

	return nullptr;
}





bool FileExists( const std::string & path )
{
	std::ifstream f( path );
	bool exists = f.is_open( );
	f.close( );
	return exists;
}

ParsedMeshStructure * LoadFbxMesh( const char * targetPath )
{
	if( !FileExists( targetPath ) )
		return nullptr;

	FbxManager * fbxManager = FbxManager::Create( );

	//FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	//fbxManager->SetIOSettings( ios );

	FbxImporter * importer = FbxImporter::Create( fbxManager, "" );
	if( !importer->Initialize( targetPath, -1, fbxManager->GetIOSettings( ) ) )
		__debugbreak( );

	FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
	importer->Import( scene );
	importer->Destroy( );

	FbxMesh * mesh = SearchNode( scene->GetRootNode( ) );
	if( !mesh->IsTriangleMesh( ) )
		return nullptr;

	int numData = mesh->GetPolygonVertexCount( );
	int numTris = numData / 3;
	auto layerElementNormal = mesh->GetElementNormal( );

	auto mappingMode = layerElementNormal->GetMappingMode( );
	auto referenceMode = layerElementNormal->GetReferenceMode( );

	if( mappingMode != FbxLayerElement::eByControlPoint )
		return nullptr;
	if( referenceMode != FbxLayerElement::eDirect )
		return nullptr;

	
	ParsedMeshStructure * parsedMesh = new ParsedMeshStructure( );
	parsedMesh->vertexData = new float[numData * 3];
	parsedMesh->normalData = new float[numData * 3];
	parsedMesh->numTris = numTris;

	for( int i = 0; i < numData; i++ )
	{
		const auto vert = mesh->GetControlPointAt( i );
		const auto norm = layerElementNormal->GetDirectArray( ).GetAt( i );

		parsedMesh->vertexData[i * 3 + 0] = vert[0];
		parsedMesh->vertexData[i * 3 + 1] = vert[1];
		parsedMesh->vertexData[i * 3 + 2] = vert[2];

		parsedMesh->normalData[i * 3 + 0] = norm[0];
		parsedMesh->normalData[i * 3 + 1] = norm[1];
		parsedMesh->normalData[i * 3 + 2] = norm[2];
	}

	//fbxManager->Destroy( );

	return parsedMesh;
}




//#ifdef _DEBUG
int main( )
{
	auto start = clock( );
	//auto meshSet = LoadMeshSet( "E:\\Dropbox\\HotRepos\\UICColum\\QuickData\\FbxPartitioning\\bsCSF\\surfaces\\", "*.binmesh" );
	LoadMeshSet( "C:\\Users\\algor\\Dropbox\\hotrepos\\UICColum\\QuickData\\FbxPartitioning\\IanArteries5.GAMBIT\\volumes\\", "*.binmesh" );

	//std::cout << "Loaded " << meshSet->meshes.size( ) << " meshes" << std::endl;

	std::cout << "Took " << (clock( ) - start) << "ms";

	std::string j;
	std::getline( std::cin, j );

	return 0;
}

//#endif
