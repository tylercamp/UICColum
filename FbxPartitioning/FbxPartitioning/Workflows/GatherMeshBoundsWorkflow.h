#pragma once

#include "../Workflow.h"
#include "../Types.h"

class GatherMeshBoundsWorkflow : public Workflow
{
	IngressDataBinding<triangle> m_Mesh;
	OutgressDatumBinding<float_3> m_MinExtents;
	OutgressDatumBinding<float_3> m_MaxExtents;

	void cpu_mesh_get_range( gpu_triangle_array & dev_tris, float_3 * out_min, float_3 * out_max )
	{
		float_3 min = dev_tris[0].center;
		float_3 max = dev_tris[0].center;

		for( int i = 0; i < dev_tris.extent.size( ); i++ )
		{
			float_3 tri = dev_tris[i].center;
			min.x = fminf( min.x, tri.x );
			min.y = fminf( min.y, tri.y );
			min.z = fminf( min.z, tri.z );

			max.x = fmaxf( max.x, tri.x );
			max.y = fmaxf( max.y, tri.y );
			max.z = fmaxf( max.z, tri.z );
		}

		*out_min = min;
		*out_max = max;
	}

public:
	GatherMeshBoundsWorkflow( IngressDataBinding<triangle> mesh, OutgressDatumBinding<float_3> min, OutgressDatumBinding<float_3> max ) :
		m_Mesh( mesh ),
		m_MinExtents( min ), m_MaxExtents( max )
	{

	}

	void Run( )
	{
		using namespace concurrency::fast_math;
		//	TODO: Look into tile operations
		const int target_size = 100;

		auto & dev_tris = m_Mesh.Resolve<gpu_triangle_array>( );
		auto base_extent = dev_tris.extent;

#define TILE_SIZE 128

		array_view<float_3> mins( base_extent / TILE_SIZE + 1 );
		mins.discard_data( );
		array_view<float_3> maxs( base_extent / TILE_SIZE + 1 );
		maxs.discard_data( );

		//	Repeat while size > target_size

		int tris_per_tile = base_extent[0] / TILE_SIZE;
		if( tris_per_tile > 0xFFFF )
		{
			throw "Mesh too large, tiling size must be increased for processing";
		}

		parallel_for_each(
			base_extent.tile<TILE_SIZE>( ).pad( ),
			[dev_tris, mins, maxs]( tiled_index<TILE_SIZE> t_idx ) restrict( amp )
		{
			tile_static float_3 tile_ranges[TILE_SIZE];

			if( t_idx.global[0] < dev_tris.extent[0] )
				tile_ranges[t_idx.local[0]] = dev_tris[t_idx].center;

			t_idx.barrier.wait( );

			if( t_idx.local[0] == 0 )
			{
				float_3 min = tile_ranges[0], max = tile_ranges[0];

				for( int i = 1; i < TILE_SIZE; i++ )
				{
					if( t_idx.global[0] + i >= dev_tris.extent[0] )
						break;

					float_3 & current = tile_ranges[i];
					min.x = fminf( min.x, current.x );
					min.y = fminf( min.y, current.y );
					min.z = fminf( min.z, current.z );

					max.x = fmaxf( max.x, current.x );
					max.y = fmaxf( max.y, current.y );
					max.z = fmaxf( max.z, current.z );
				}

				mins[t_idx.tile[0]] = min;
				maxs[t_idx.tile[0]] = max;
			}
		} );

#undef TILE_SIZE

		mins.synchronize( );
		maxs.synchronize( );

		float_3 min, max;
		min = mins[0];
		max = maxs[0];

		for( int i = 0; i < mins.extent[0]; i++ )
		{
			float_3 current = mins[i];
			min.x = fminf( min.x, current.x );
			min.y = fminf( min.y, current.y );
			min.z = fminf( min.z, current.z );
		}

		for( int i = 0; i < mins.extent[0]; i++ )
		{
			float_3 current = maxs[i];
			max.x = fmaxf( max.x, current.x );
			max.y = fmaxf( max.y, current.y );
			max.z = fmaxf( max.z, current.z );
		}

		float_3 range = max - min;
		//	Extend working bounds to offset error
		min -= range * 0.01f;
		max += range * 0.01f;

		m_MinExtents.Assign( min );
		m_MaxExtents.Assign( max );
	}
};