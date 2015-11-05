#pragma once

#include "../Workflow.h"
#include "../Types.h"



/*
Algorithm: Multi-pass spatial partition

INITIALIZATION
1. Generate a list of triangles and a list of partition descriptors (partition bounds)
2. Allocate an integer co-array for the list of triangles (same extent as triangles)

DETERMINE EFFECTIVENESS OF "CURRENT" PARTITIONS LIST
1. For each triangle, tag it with the index of the containing partition descriptor (store in co-array)
2. (Tiled) Generate the amount of triangles tagged to each partition list
3. If any partition exceeds MAX_CHUNK_VERTICES, re-assess overflowing partitions (subdivide), repeat



ONCE EFFECTIVE PARTITION LIST IS DISCOVERED
1. In-place sort triangle list by partition index
2. Copy back to CPU, copy into partition buffers, write to disk
*/

#define MAX_CHUNK_TRIANGLES 0xFFFF


class PartitioningWorkflow : public Workflow
{
	IngressDataBinding<triangle> m_Mesh;
	IngressDatumBinding<float_3> m_MinExtents;
	IngressDatumBinding<float_3> m_MaxExtents;
	OutgressDataBinding<int> m_MeshTags;
	OutgressDataBinding<mesh_partition_descriptor> m_MeshPartitions;
	OutgressDataBinding<int> m_PartitionSizes;

	//	Splits on 3D space
	cpu_partition_descriptor_array split_partition( mesh_partition_descriptor & partition, int num_splits )
	{
		//	Todo - would be more effective if I sorted the partitions per axis and placed partitions based on percentiles

		cpu_partition_descriptor_array result;
		result.reserve( pow( num_splits + 1, 3 ) );

		auto range_min = partition.bounds_start;
		auto range_max = partition.bounds_end;

		float x_base = range_min.x;
		float y_base = range_min.y;
		float z_base = range_min.z;


		float x_step = ( range_max.x - range_min.x ) / num_splits;
		float y_step = ( range_max.y - range_min.y ) / num_splits;
		float z_step = ( range_max.z - range_min.z ) / num_splits;
		for( int z = 0; z < num_splits + 1; z++ )
		{
			for( int y = 0; y < num_splits + 1; y++ )
			{
				for( int x = 0; x < num_splits + 1; x++ )
				{
					mesh_partition_descriptor desc;
					desc.bounds_start.x = x_base + x * x_step;
					desc.bounds_start.y = y_base + y * y_step;
					desc.bounds_start.z = z_base + z * z_step;

					desc.bounds_end.x = x_base + ( x + 1 ) * x_step;
					desc.bounds_end.y = y_base + ( y + 1 ) * y_step;
					desc.bounds_end.z = z_base + ( z + 1 ) * z_step;

					result.push_back( desc );
				}
			}
		}

		return result;
	}

	void cpu_tag_mesh_partitions( const cpu_partition_descriptor_array & chunks, const cpu_triangle_array & mesh, cpu_index_array & out_tags )
	{
		int num_partitions = chunks.size( );

		for( int t = 0; t < mesh.size( ); t++ )
		{
			float_3 tri_center = mesh[t].center;

			int owner_chunk = -1;

			for( int i = 0; i < num_partitions; i++ )
			{
				mesh_partition_descriptor chunk = chunks[i];
				if( chunk.contains_point( tri_center ) )
				{
					owner_chunk = i;
					break;
				}
			}

			out_tags[t] = owner_chunk;
		}
	}

	//	Tags all triangles in the mesh with their containing partition
	void gpu_tag_mesh_partitions( cpu_partition_descriptor_array & chunks, const gpu_triangle_array & mesh, gpu_index_array & out_tags )
	{
		gpu_partition_descriptor_array dev_partitions( chunks );

		int num_partitions = (int) chunks.size( );

		segmented_parallel_for_each(
			mesh,
			[dev_partitions, mesh, out_tags, num_partitions]( index<1> idx ) restrict( amp )
		{
			float_3 tri_center = mesh[idx].center;

			int owner_chunk = -1;

			for( int i = 0; i < num_partitions; i++ )
			{
				mesh_partition_descriptor chunk = dev_partitions[i];
				if( chunk.contains_point( tri_center ) )
				{
					owner_chunk = i;
					break;
				}
			}

			out_tags[idx] = owner_chunk;
		} );

		dev_partitions.synchronize( );
	}

public:
	PartitioningWorkflow( IngressDataBinding<triangle> mesh, IngressDatumBinding<float_3> minExtents, IngressDatumBinding<float_3> maxExtents,
						  OutgressDataBinding<int> meshTags, OutgressDataBinding<mesh_partition_descriptor> partitions, OutgressDataBinding<int> partitionCounts ) :
		m_Mesh( mesh ), m_MinExtents( minExtents ), m_MaxExtents( maxExtents ),
		m_MeshTags( meshTags ), m_MeshPartitions( partitions ), m_PartitionSizes( partitionCounts )
	{

	}

	void Run( )
	{
		auto & mesh = m_Mesh.Resolve<gpu_triangle_array>( );
		cpu_triangle_array cpu_tris;
		for( int i = 0; i < mesh.extent.size( ); i++ )
			cpu_tris.push_back( mesh[i] );

		//	Options
		int axisSplits = 2; // when we split a partition, how many times should we split it? (on X, Y, Z axis)

		mesh_partition_descriptor basePartition = {
			m_MinExtents.Resolve( ),
			m_MaxExtents.Resolve( )
		};

		//	Record of our "current" partitioning attempt
		cpu_partition_descriptor_array partitions = split_partition( basePartition, axisSplits );
		std::vector<int> partitionSizes;

		for( ;; )
		{
			std::cout << "Attempting mesh partitioning using " << partitions.size( ) << " partitions" << std::endl;

			std::cout << "\t(GPU partition tagging... ";
			gpu_index_array dev_meshtags( mesh.extent );
			dev_meshtags.discard_data( );

			gpu_tag_mesh_partitions( partitions, mesh, dev_meshtags );
			//cpu::tag_mesh_partitions( partitions, cpu_tris, meshtags );

			dev_meshtags.synchronize( );

			partitionSizes.clear( );
			partitionSizes.assign( partitions.size( ), 0 );

			std::cout << "done)" << std::endl;


			std::cout << "\t(CPU counting partition membership... ";
			for( int i = 0; i < cpu_tris.size( ); i++ )
			{
				int partitionIndex = dev_meshtags[i];
#ifdef _DEBUG
				triangle tri = cpu_tris[i];
				if( partitionIndex < 0 )
					__debugbreak( );
#endif
				++partitionSizes[partitionIndex];
			}

			//	Check if the current partitioning scheme is valid (satisfies max chunk size)

			cpu_partition_descriptor_array newPartitions;

			std::cout << "done)" << std::endl;


			//	If there are any partitions that were larger than MAX_CHUNK_TRIANGLES, split those into smaller partitions and try again

			std::cout << "\t(CPU discovering new partitions... ";
			bool schemeWasValid = true;
			int largestViolation = 0;
			int numRemovedPartitions = 0;
			for( int i = 0; i < partitions.size( ); i++ )
			{
				int violation = fmax( 0, partitionSizes[i] - MAX_CHUNK_TRIANGLES );
				largestViolation = fmax( largestViolation, violation );

				if( violation > 0 )
				{
					//	Break partition
					auto missingPartitions = split_partition( partitions[i], axisSplits );
					for( auto p : missingPartitions )
						newPartitions.emplace_back( p );

					partitions.erase( partitions.begin( ) + i );
					partitionSizes.erase( partitionSizes.begin( ) + i );

					--i;

					schemeWasValid = false;
				}
				else if( partitionSizes[i] == 0 )
				{
					//	Remove partitions that aren't useful
					partitions.erase( partitions.begin( ) + i );
					partitionSizes.erase( partitionSizes.begin( ) + i );
					--i;
					numRemovedPartitions++;
				}
			}

			std::cout << "done)" << std::endl;

			std::cout << "Discovered " << newPartitions.size( ) << " new partitions, removed " << numRemovedPartitions << " unused partitions, largest violation was " << largestViolation << " tris" << std::endl;

			for( auto newPartition : newPartitions )
				partitions.emplace_back( newPartition );

			if( schemeWasValid )
			{
				m_MeshTags.Assign( dev_meshtags );
				m_MeshPartitions.Assign( partitions );
				m_PartitionSizes.Assign( partitionSizes );
				break;
			}

			std::cout << std::endl;
		}
	}
};

/*
//	(I think this already has a name?)
ALGORITHM FOR GENERATING STATISTICAL PARTITION SPLITS

With vertex data on the GPU, sort vertex data on sign and magnitude per dimension (spatial_sort)
Copy vertex data to the CPU, sample at quartiles


*/



