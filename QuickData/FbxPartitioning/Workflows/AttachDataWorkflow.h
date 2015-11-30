#pragma once

#include "../Types.h"
#include "../Utilities.h"

//	Attaches 'data' to 'mesh', using volumeIndex to index into the data. A mesh has multiple stores for these data, the store to use
//		is determined by storeIndex. For meshes where the volumeIndex is non-0-indexed, a referenceStartIndex can be provided (i.e. rSI=1 if 1-indexed)
//		(Specifically for MSH file volume indices being 1-indexed)
void workflow_attach_data_to_mesh( gpu_triangle_array * mesh, gpu_data_array * data, int storeIndex = 0, int referenceStartIndex = 0 )
{
	auto & rmesh = *mesh;
	auto & rdata = *data;

	parallel_for_each(
		rmesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto tri = rmesh[idx];
		tri.volumeValues[storeIndex] = rdata[tri.volumeIndex - referenceStartIndex];
		rmesh[idx] = tri;
	} );
}