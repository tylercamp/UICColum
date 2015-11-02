#pragma once

#include "../Workflow.h"
#include "../Types.h"

class DataGroupingWorkflow : public Workflow
{
	IngressDataBinding<triangle> m_Mesh;
	IngressDataBinding<int> m_Tags;
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
	DataGroupingWorkflow( IngressDataBinding<triangle> mesh, IngressDataBinding<int> tags, OutgressDataBinding<mesh_chunk> partitionedMesh ) :
		m_Mesh( mesh ), m_Tags( tags ),
		m_PartitionedMesh( partitionedMesh )
	{

	}

	void Run( )
	{
		//	TODO - Could parallelize (sort data by partition?)
		cpu_chunk_array & chunks = *out_chunks;
		chunks.resize( partitions.size( ) );

		for( int c = 0; c < chunks.size( ); c++ )
		{
			chunks[c].num_tris = partitionSizes[c];
			chunks[c].bounds = partitions[c];
			chunks[c].tris.reserve( partitionSizes[c] );
		}

		cpu::chunk_mesh( mesh, &chunks );
	}
};