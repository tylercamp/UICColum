#pragma once

#include <FbxSdk.h>
#include <fstream>
#include <iostream>
#include <Shlobj.h>

#include "../Types.h"

void save_chunk_fbx( std::string filename, const mesh_chunk & chunk )
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
	outputMesh->InitNormals( chunk.num_tris * 3 );
	auto * controlPoints = outputMesh->GetControlPoints( );

	for( int i = 0; i < chunk.num_tris; i++ )
	{
		auto & tri = chunk.tris[i];

		controlPoints[i * 3 + 0].Set( tri.a.x, tri.a.y, tri.a.z );
		controlPoints[i * 3 + 1].Set( tri.b.x, tri.b.y, tri.b.z );
		controlPoints[i * 3 + 2].Set( tri.c.x, tri.c.y, tri.c.z );

		outputMesh->SetControlPointNormalAt( FbxVector4( tri.norm_a.x, tri.norm_a.y, tri.norm_a.z, 0.0f ), i * 3 + 0 );
		outputMesh->SetControlPointNormalAt( FbxVector4( tri.norm_b.x, tri.norm_b.y, tri.norm_b.z, 0.0f ), i * 3 + 1 );
		outputMesh->SetControlPointNormalAt( FbxVector4( tri.norm_c.x, tri.norm_c.y, tri.norm_c.z, 0.0f ), i * 3 + 2 );

		outputMesh->BeginPolygon( );
		outputMesh->AddPolygon( i * 3 + 0 );
		outputMesh->AddPolygon( i * 3 + 1 );
		outputMesh->AddPolygon( i * 3 + 2 );
		outputMesh->EndPolygon( );
	}

	//outputMesh->GenerateNormals( );

	exporter->Export( scene );

	exporter->Destroy( );
	fbxManager->Destroy( );
}

void save_chunkset_metadata( std::string filename, cpu_chunk_array * chunkset )
{
	std::ofstream file( filename );

	//file << "FbxPartitioning"
	//NOT_YET_IMPLEMENTED( );

	file.close( );
}

void workflow_chunk_export_fbx( const std::string & targetFolder, cpu_chunk_array * chunks )
{
	std::cout << "Saving chunks... ";
	CreateDirectoryA( targetFolder.c_str( ), nullptr );
	//	http://stackoverflow.com/questions/1530760/how-do-i-recursively-create-a-folder-in-win32
	//system( ("mkdir " + targetFolder).c_str( ) );
	//SHCreateDirectoryExA( NULL, targetFolder.c_str( ), NULL );
	for( int i = 0; i < chunks->size( ); i++ )
	{
		std::ostringstream pathName;
		pathName << targetFolder << "/" << i << ".fbx";
		auto & chunk = chunks->at( i );
		save_chunk_fbx( pathName.str( ), chunk );
	}

	save_chunkset_metadata( targetFolder, chunks );
}


