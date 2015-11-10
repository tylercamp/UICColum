#pragma once

#include <iostream>

#include "../Workflow.h"
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





class MeshChunkingWorkflow : public Workflow
{
	IngressDataBinding<triangle> m_Mesh;
	IngressDataBinding<int> m_Tags;
	IngressDataBinding<int> m_PartitionCounts;
	IngressDataBinding<mesh_partition_descriptor> m_Partitions;
	OutgressDataBinding<mesh_chunk> m_PartitionedMesh;

	void chunk_mesh( gpu_triangle_array mesh, cpu_chunk_array * out_chunks )
	{
		//	TODO - should be multithreaded copy
		auto & chunks = *out_chunks;

		for( int i = 0; i < mesh.extent.size( ); i++ )
		{
			if( i % ( mesh.extent.size( ) / 10 ) == 0 )
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

public:
	MeshChunkingWorkflow( IngressDataBinding<triangle> mesh, IngressDataBinding<int> tags, IngressDataBinding<int> partitionCounts, IngressDataBinding<mesh_partition_descriptor> partitions,
						  OutgressDataBinding<mesh_chunk> partitionedMesh ) :
		m_Mesh( mesh ), m_Tags( tags ), m_PartitionCounts( partitionCounts ), m_Partitions( partitions ),
		m_PartitionedMesh( partitionedMesh )
	{

	}

	void Run( )
	{
		//	TODO - Could parallelize (sort data by partition?)
		auto & mesh = m_Mesh.Resolve<gpu_triangle_array>( );
		auto & partitions = m_Partitions.Resolve<cpu_partition_descriptor_array>( );
		auto & partitionSizes = m_PartitionCounts.Resolve<cpu_index_array>( );
		
		cpu_chunk_array chunks;
		chunks.resize( partitions.size( ) );

		for( int c = 0; c < chunks.size( ); c++ )
		{
			chunks[c].num_tris = partitionSizes[c];
			chunks[c].bounds = partitions[c];
			chunks[c].tris.reserve( partitionSizes[c] );
		}

		chunk_mesh( mesh, &chunks );

		m_PartitionedMesh.Assign( chunks );
	}
};