#pragma once

//	Imports a mesh using the Asset Import library
//	http://assimp.sourceforge.net/main_doc.html
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "Types.h"

void workflow_import_assimp( const std::string & file, gpu_vertex_array ** out_verts, gpu_index_array ** out_indices )
{
	auto scene = aiImportFile( file.c_str( ), 0 );

	if( !scene )
		NOT_YET_IMPLEMENTED( );

	std::size_t vertOffset = 0;
	std::size_t indexOffset = 0;

	int numVertices = 0;
	int numIndices = 0;
	for( int m = 0; m < scene->mNumMeshes; m++ )
	{
		numVertices += scene->mMeshes[m]->mNumVertices;
		numIndices += scene->mMeshes[m]->mNumFaces * 3;
	}

	

	cpu_vertex_array vertices( numVertices );
	cpu_index_array indices( numIndices );

	for( int m = 0; m < scene->mNumMeshes; m++ )
	{
		auto mesh = scene->mMeshes[m];

		for( int v = 0; v < mesh->mNumVertices; v++ )
		{
			auto vert = mesh->mVertices[v];
			vertices[v + vertOffset] = float_3( vert.x, vert.y, vert.z );
		}

		for( int f = 0; f < mesh->mNumFaces; f++ )
		{
			auto face = mesh->mFaces[f];
			indices[indexOffset + f * 3 + 0] = face.mIndices[0];
			indices[indexOffset + f * 3 + 1] = face.mIndices[1];
			indices[indexOffset + f * 3 + 2] = face.mIndices[2];
		}

		vertOffset += mesh->mNumVertices;
		indexOffset += mesh->mNumFaces * 3;
	}

	aiReleaseImport( scene );



	*out_verts = bindless_copy( vertices );
	*out_indices = bindless_copy( indices );
}