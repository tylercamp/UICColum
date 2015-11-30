#pragma once

#include "../Types.h"

void workflow_tag_mesh_volumes( gpu_triangle_array * mesh, gpu_index_array * volumes )
{
	auto & rmesh = *mesh;
	auto & rvolumes = *volumes;

	parallel_for_each(
		mesh->extent,
		[=]( index<1> idx ) restrict( amp )
	{
		rmesh[idx].volumeIndex = rvolumes[idx];
	} );
}