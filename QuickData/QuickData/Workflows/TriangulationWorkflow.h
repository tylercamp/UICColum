#pragma once

#include "../Types.h"


void workflow_gen_tris( gpu_vertex_array * points, gpu_index_array * indices, gpu_triangle_array ** out_tris )
{
	auto & verts = *points;
	auto & local_indices = *indices;

//#ifdef _DEBUG
//	for( std::size_t i = 0; i < local_indices.extent.size( ); i++ )
//	{
//		if( local_indices[i] < 0 || local_indices[i] >= verts.extent.size( ) )
//			__debugbreak( );
//	}
//#endif

	*out_tris = new gpu_triangle_array( indices->extent.size( ) / 3 );
	//*out_tris = new gpu_triangle_array( 30000000 );
	auto & tris = **out_tris;
	//tris.discard_data( );

	/*
	for( std::size_t i = 0; i < tris.extent.size( ); i++ )
	{
		auto & tri = tris[i];

		tri.a = verts[local_indices[i * 3 + 0]];
		tri.b = verts[local_indices[i * 3 + 1]];
		tri.c = verts[local_indices[i * 3 + 2]];
	}
	*/

	//	segmented parallel_for_each fails at large meshes here?
	auto total_extent = tris.extent;
	int max_extent_size = 0x7FFF;
	int num_steps = total_extent[0] / max_extent_size + 1;

	for( int step = 0; step < num_steps; step++ )
	{
		int current_offset = step * max_extent_size;
		if( current_offset >= total_extent[0] )
			break;

		extent<1> current_extent( min( total_extent[0] - current_offset, max_extent_size ) );
		parallel_for_each(
			current_extent,
			[=]( index<1> idx ) restrict( amp )
		{
			auto & tri = tris[idx[0] + current_offset];

			tri.a = verts[local_indices[(current_offset + idx[0]) * 3 + 0]];
			tri.b = verts[local_indices[(current_offset + idx[0]) * 3 + 1]];
			tri.c = verts[local_indices[(current_offset + idx[0]) * 3 + 2]];
		} );

		concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );
	}


	/*
	segmented_parallel_for_each(
		tris,
		[verts, local_indices, tris]( index<1> idx ) restrict( amp )
	{
		auto & tri = tris[idx];

		tri.a = verts[local_indices[idx[0] * 3 + 0]];
		tri.b = verts[local_indices[idx[0] * 3 + 1]];
		tri.c = verts[local_indices[idx[0] * 3 + 2]];
	} );
	*/
	
}