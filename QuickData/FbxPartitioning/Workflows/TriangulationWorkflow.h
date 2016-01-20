#pragma once

#include "../Types.h"

void workflow_gen_tris( gpu_vertex_array * points, gpu_index_array * indices, gpu_triangle_array ** out_tris )
{
	auto & verts = *points;
	auto & local_indices = *indices;


	*out_tris = new gpu_triangle_array( indices->extent.size( ) / 3 );
	auto & tris = **out_tris;
	tris.discard_data( );

	segmented_parallel_for_each(
		tris,
		[verts, local_indices, tris]( index<1> idx ) restrict( amp )
	{
		auto & tri = tris[idx];

		tri.a = verts[local_indices[idx[0] * 3 + 0]];
		tri.b = verts[local_indices[idx[0] * 3 + 1]];
		tri.c = verts[local_indices[idx[0] * 3 + 2]];
	} );
}