#pragma once

#include "../Types.h"

#include "../BinaryMesh.h"
#include <map>

void workflow_import_binary_mesh( const std::string & filename, mesh_chunk ** out_chunk )
{
	BinaryMesh mesh( filename );
	
	mesh_chunk * result = new mesh_chunk( );
	//	TODO - LOAD BOUNDS
	result->num_tris = mesh.numTris;
	//	TODO - OPTIMIZE - Shouldn't need to do a copy
	
	auto & tris = result->tris;
	tris.resize( result->num_tris );
	for( int i = 0; i < result->num_tris; i++ )
	{
		auto & currentTri = tris[i];
		currentTri.a = mesh.vertices[i * 3 + 0];
		currentTri.b = mesh.vertices[i * 3 + 1];
		currentTri.c = mesh.vertices[i * 3 + 2];

		currentTri.norm_a = mesh.normals[i * 3 + 0];
		currentTri.norm_b = mesh.normals[i * 3 + 1];
		currentTri.norm_c = mesh.normals[i * 3 + 2];
	}

	*out_chunk = result;
}

void workflow_import_binary_mesh_set( std::string sourceFolder, cpu_chunk_array ** out_chunks )
{
	cpu_chunk_array * result = new cpu_chunk_array( );

	//	Can't assume mesh files have been sorted by numeric name, do this sorting ourselves
	std::vector<std::pair<int, mesh_chunk *>> mappedChunks;
	std::vector<std::string> meshFiles = findFiles( sourceFolder, "*.binmesh" );
	for( auto file : meshFiles )
	{
		auto name = getFileName( file );
		int index = strtol( name.c_str( ), nullptr, 10 );
		mesh_chunk * newChunk;
		workflow_import_binary_mesh( file, &newChunk );

		//	TODO - OPTIMIZE
		mappedChunks.push_back( std::pair<int, mesh_chunk *>( index, newChunk ) );
	}

	std::sort( mappedChunks.begin( ), mappedChunks.end( ), []( std::pair<int, mesh_chunk *> a, std::pair<int, mesh_chunk *> b )
	{
		return a.first < b.first;
	} );

	auto partitionSchemeFiles = findFiles( sourceFolder, "*.binmeshscheme" );
	if( partitionSchemeFiles.size( ) > 0 )
	{
		auto partitionSchemeFile = partitionSchemeFiles[0];
		MeshPartitionScheme scheme;
		scheme.LoadFrom( partitionSchemeFile );

		for( int i = 0; i < mappedChunks.size( ); i++ )
		{
			auto & chunk = *mappedChunks[i].second;
			chunk.bounds = scheme.descriptors[i];
		}
	}

	//	Is this "correct"? 
	for( auto mappedChunk : mappedChunks )
	{
		result->emplace_back( std::move( *mappedChunk.second ) );
		delete mappedChunk.second;
	}

	*out_chunks = result;
}