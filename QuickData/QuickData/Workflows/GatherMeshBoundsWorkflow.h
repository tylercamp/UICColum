#pragma once

#include "../Types.h"

void workflow_gather_mesh_bounds( gpu_triangle_array * mesh, float_3 * out_min, float_3 * out_max )
{
	using namespace concurrency::fast_math;
	//	TODO: Look into tile operations
	const int target_size = 100;

	auto & dev_tris = *mesh;
	auto base_extent = dev_tris.extent;

#define TILE_SIZE 256

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
	

	//	FUTURE TODO: USE 2D TILE EXTENTS
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

	*out_min = min;
	*out_max = max;
}

