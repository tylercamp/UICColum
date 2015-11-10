#pragma once

#include <string>

#include "../Workflow.h"
#include "../Types.h"

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

FbxMesh * findMeshData( FbxScene * scene )
{
	auto root = scene->GetRootNode( );

	return SearchNode( root );
}

void workflow_import_fbx( const std::string & fileName, gpu_vertex_array ** out_vertices, gpu_index_array ** out_indices )
{
	FbxManager * fbxManager = FbxManager::Create( );

	FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	fbxManager->SetIOSettings( ios );

	FbxImporter * importer = FbxImporter::Create( fbxManager, "" );
	if( !importer->Initialize( fileName.c_str( ), -1, fbxManager->GetIOSettings( ) ) )
		__debugbreak( );

	FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
	importer->Import( scene );
	importer->Destroy( );

	FbxMesh * mesh = findMeshData( scene );






	int numVerts = mesh->GetControlPointsCount( );

	int numIndices = mesh->GetPolygonVertexCount( );
	int * indices = mesh->GetPolygonVertices( );

	cpu_vertex_array cpu_vertices;
	cpu_vertices.resize( numVerts );
	cpu_index_array cpu_indices;
	cpu_indices.resize( numIndices );

	//	Begin copy process
	for( int i = 0; i < numVerts; i++ )
	{
		FbxVector4 vert = mesh->GetControlPointAt( i );
		cpu_vertices[i].x = (float)vert.mData[0];
		cpu_vertices[i].y = (float)vert.mData[1];
		cpu_vertices[i].z = (float)vert.mData[2];
	}

	for( int i = 0; i < numIndices; i++ )
	{
		cpu_indices[i] = indices[i];
	}

	fbxManager->Destroy( );


}





class FbxImportWorkflow : public Workflow
{
	std::string m_TargetFileName;
	OutgressDataBinding<float_3> m_Vertices;
	OutgressDataBinding<int> m_Indices;

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

	FbxMesh * findMeshData( FbxScene * scene )
	{
		auto root = scene->GetRootNode( );

		return SearchNode( root );
	}

public:
	FbxImportWorkflow( const std::string & fileName, OutgressDataBinding<float_3> & vertices, OutgressDataBinding<int> indices ) :
		m_TargetFileName( fileName ), m_Vertices( vertices ), m_Indices( indices )
	{

	}

	virtual ~FbxImportWorkflow( ) { }

	void Run( )
	{
		FbxManager * fbxManager = FbxManager::Create( );

		FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
		fbxManager->SetIOSettings( ios );

		FbxImporter * importer = FbxImporter::Create( fbxManager, "" );
		if( !importer->Initialize( m_TargetFileName.c_str( ), -1, fbxManager->GetIOSettings( ) ) )
			__debugbreak( );

		FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
		importer->Import( scene );
		importer->Destroy( );

		FbxMesh * mesh = findMeshData( scene );






		int numVerts = mesh->GetControlPointsCount( );

		int numIndices = mesh->GetPolygonVertexCount( );
		int * indices = mesh->GetPolygonVertices( );

		cpu_vertex_array cpu_vertices;
		cpu_vertices.resize( numVerts );
		cpu_index_array cpu_indices;
		cpu_indices.resize( numIndices );

		//	Begin copy process
		for( int i = 0; i < numVerts; i++ )
		{
			FbxVector4 vert = mesh->GetControlPointAt( i );
			cpu_vertices[i].x = (float) vert.mData[0];
			cpu_vertices[i].y = (float) vert.mData[1];
			cpu_vertices[i].z = (float) vert.mData[2];
		}

		for( int i = 0; i < numIndices; i++ )
		{
			cpu_indices[i] = indices[i];
		}

		fbxManager->Destroy( );

		m_Vertices.Assign( cpu_vertices );
		m_Indices.Assign( cpu_indices );
	}
};