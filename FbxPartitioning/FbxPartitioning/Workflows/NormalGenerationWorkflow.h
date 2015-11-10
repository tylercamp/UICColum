#pragma once

#include "../Types.h"
#include "../Utilities.h"

void workflow_gen_normals( gpu_triangle_array * tris )
{
	using namespace concurrency::fast_math;

	gpu_triangle_array & mesh = *tris;

	parallel_for_each(
		mesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto & tri = mesh[idx];
		auto a_b = tri.b - tri.a;
		auto a_c = tri.c - tri.a;
		auto norm = cross( a_b, a_c );

		float normLength = sqrtf( norm.x * norm.x + norm.y * norm.y + norm.z * norm.z );
		norm /= normLength;

		tri.norm_a = norm;
		tri.norm_b = norm;
		tri.norm_c = norm;
	} );
}
