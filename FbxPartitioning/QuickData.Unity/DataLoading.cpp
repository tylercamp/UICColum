
#include <fbxsdk.h>
#include <string>
#include "Unity.h"

#pragma comment( lib, "libfbxsdk.lib" )


//	http://ericeastwood.com/blog/17/unity-and-dlls-c-managed-and-c-unmanaged




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





UNITY_NATIVE_EXPORT float * MeshGetVertices( ParsedMeshStructure * mesh )
{
	return mesh->vertexData;
}

UNITY_NATIVE_EXPORT float * MeshGetNormals( ParsedMeshStructure * mesh )
{
	return mesh->normalData;
}


UNITY_NATIVE_EXPORT ParsedMeshStructure * LoadFbxMesh( const char * targetPath )
{
	FbxManager * fbxManager = FbxManager::Create( );

	FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	fbxManager->SetIOSettings( ios );

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

	fbxManager->Destroy( );

	return parsedMesh;
}




#ifdef _DEBUG

int main( )
{
	LoadFbxMesh( "0.fbx" );

	return 0;
}

#endif
