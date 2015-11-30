#pragma once

#include <iostream>

#include "../Types.h"
#include "../../QuickData.Unity/ThreadPool.h"


void chunk_mesh_mt( gpu_triangle_array mesh, cpu_chunk_array * bounded_chunks )
{
	auto & chunks = *bounded_chunks;
	std::atomic_size_t numProcessed = 0;
	std::atomic_size_t numJoined = 0;
	
	//	BROKEN: Vector not thread-safe

	auto kernel = [mesh, &numProcessed, &numJoined]( mesh_chunk & chunk )
	{
		auto & bounds = chunk.bounds;

		auto meshData = mesh.data( );

		for( int i = 0; i < mesh.extent.size( ); i++ )
		{
			if( bounds.contains_point( meshData[i].center ) )
				chunk.tris.push_back( meshData[i] );
			++numProcessed;
		}

		++numJoined;
	};

	ThreadPool pool( 4 );

	std::atomic_size_t totalJobs = chunks.size( ) * static_cast<unsigned long long>(mesh.extent.size( ));

	for( int i = 0; i < chunks.size( ); i++ )
		pool.enqueue( kernel, chunks[i] );

	//	Join all, show progress
	float lastCompletion = 0.0f;
	auto progressPoints = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f };
	while( numJoined < chunks.size( ) )
	{
		int currentCompleted = numProcessed;
		float currentCompletion = currentCompleted / (double)totalJobs;

		Sleep( 10 );

		for( auto point : progressPoints )
		{
			if( lastCompletion < point && currentCompletion > point )
				std::cout << (int)round( point * 100.0f ) << "%.. ";
		}

		lastCompletion = currentCompletion;
	}

	std::cout << "Done." << std::endl;
}

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
			if( chunks.data()[p].num_tris == chunks.data()[p].tris.size( ) )
				continue;

			auto & desc = chunks.data()[p].bounds;

			if( desc.contains_point( mesh.data()[i].center ) )
			{
				chunks[p].tris.push_back( mesh.data()[i] );
				break;
			}
		}
	}
}

void workflow_chunk_from_partitions( gpu_triangle_array * tris, gpu_partition_descriptor_array * partitions, gpu_index_array * tags, gpu_index_array * partitionSizes, cpu_chunk_array ** out_chunks )
{
	//	TODO - Could GPU parallelize (sort data by partition?)
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
	//chunk_mesh_mt( dev_mesh, chunks );

	*out_chunks = chunks;
}


