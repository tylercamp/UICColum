#pragma once

#include "../Types.h"

float_3 workflow_get_mesh_center( gpu_triangle_array * mesh )
{
	int numTris = mesh->extent.size( );
	float_3 avg = float_3( 0.0f, 0.0f, 0.0f );

	for( int i = 0; i < numTris; i++ )
		avg += (*mesh)[i].center / numTris;

	return avg;
}

void workflow_recenter_mesh( gpu_triangle_array * mesh, float_3 center )
{
	auto & dev_mesh = *mesh;

	parallel_for_each(
		dev_mesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		dev_mesh[idx].a -= center;
		dev_mesh[idx].b -= center;
		dev_mesh[idx].c -= center;
	} );
}
