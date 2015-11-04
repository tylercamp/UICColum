#pragma once

#include <FbxSdk.h>
#include <fstream>
#include <iostream>

#include "../Workflow.h"
#include "../Types.h"

class FbxChunkExportWorkflow : public Workflow
{
	IngressDataBinding<mesh_chunk> m_MeshChunks;
	std::string m_TargetBaseFileName;

	void save_chunk( std::string filename, const mesh_chunk & chunk )
	{
		FbxManager * fbxManager = FbxManager::Create( );

		FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
		fbxManager->SetIOSettings( ios );

		FbxExporter * exporter = FbxExporter::Create( fbxManager, "" );
		if( !exporter->Initialize( filename.c_str( ), -1, fbxManager->GetIOSettings( ) ) )
			__debugbreak( );

		FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
		FbxNode * meshNode = FbxNode::Create( scene, "MeshNode" );
		FbxMesh * outputMesh = FbxMesh::Create( scene, "Mesh" );
		meshNode->SetNodeAttribute( outputMesh );
		scene->GetRootNode( )->AddChild( meshNode );

		outputMesh->InitControlPoints( chunk.num_tris * 3 );
		auto * controlPoints = outputMesh->GetControlPoints( );

		for( int i = 0; i < chunk.num_tris; i++ )
		{
			auto & tri = chunk.tris[i];

			controlPoints[i * 3 + 0].Set( tri.a.x, tri.a.y, tri.a.z );
			controlPoints[i * 3 + 1].Set( tri.b.x, tri.b.y, tri.b.z );
			controlPoints[i * 3 + 2].Set( tri.c.x, tri.c.y, tri.c.z );

			outputMesh->BeginPolygon( );
			outputMesh->AddPolygon( i * 3 + 0 );
			outputMesh->AddPolygon( i * 3 + 1 );
			outputMesh->AddPolygon( i * 3 + 2 );
			outputMesh->EndPolygon( );
		}

		exporter->Export( scene );

		exporter->Destroy( );
		fbxManager->Destroy( );
	}

	void save_chunkset_metadata( std::string filename, const cpu_chunk_array & chunkset )
	{
		std::ofstream file( filename );

		//file << "FbxPartitioning"

		file.close( );
	}

public:
	FbxChunkExportWorkflow( const std::string & baseFileName, IngressDataBinding<mesh_chunk> meshChunks ) :
		m_TargetBaseFileName( baseFileName ), m_MeshChunks( meshChunks )
	{

	}

	virtual ~FbxChunkExportWorkflow( ) { }

	void Run( )
	{
		auto & chunks = m_MeshChunks.Resolve<cpu_chunk_array>( );

		std::cout << "Saving chunks... ";
		CreateDirectoryA( ( m_TargetBaseFileName + "_chunks/" ).c_str( ), nullptr );
		for( int i = 0; i < chunks.size( ); i++ )
		{
			std::ostringstream name;
			name << m_TargetBaseFileName << "_chunks/";
			name << m_TargetBaseFileName << "_" << i << ".fbx";
			auto & chunk = chunks[i];
			save_chunk( name.str( ), chunk );
		}

		save_chunkset_metadata( m_TargetBaseFileName + "_chunks/meta", chunks );
	}
};