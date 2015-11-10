#pragma once

#include <iostream>

#include "../Types.h"



void chunk_mesh( gpu_triangle_array mesh, cpu_chunk_array * out_chunks )
{
	//	TODO - should be multithreaded copy
	auto & chunks = *out_chunks;

	for( int i = 0; i < mesh.extent.size( ); i++ )
	{
		if( i % (mesh.extent.size( ) / 10) == 0 )
			std::cout << ceilf( i * 100.0f / mesh.extent.size( ) ) << "%.. ";

		for( int p = 0; p < chunks.size( ); p++ )
		{
			if( chunks[p].num_tris == chunks[p].tris.size( ) )
				continue;

			auto & desc = chunks[p].bounds;

			if( desc.contains_point( mesh[i].center ) )
			{
				chunks[p].tris.push_back( mesh[i] );
				break;
			}
		}
	}
}

void workflow_chunk_from_partitions( gpu_triangle_array * tris, gpu_partition_descriptor_array * partitions, gpu_index_array * tags, gpu_index_array * partitionSizes, cpu_chunk_array ** out_chunks )
{
	//	TODO - Could parallelize (sort data by partition?)
	auto & dev_mesh = *tris;
	auto & dev_partitions = *partitions;
	auto & dev_partitionSizes = *partitionSizes;

	cpu_chunk_array * chunks = new cpu_chunk_array( );
	chunks->resize( dev_partitions.extent.size( ) );

	for( int c = 0; c < chunks->size( ); c++ )
	{
		(*chunks)[c].num_tris = dev_partitionSizes[c];
		(*chunks)[c].bounds = dev_partitions[c];
		(*chunks)[c].tris.reserve( dev_partitionSizes[c] );
	}

	chunk_mesh( dev_mesh, chunks );

	*out_chunks = chunks;
}


