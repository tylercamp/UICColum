#pragma once

#include "../Types.h"
#include "../Utilities.h"

void workflow_gen_normals( gpu_triangle_array * tris, bool reverse = false )
{
	using namespace concurrency::fast_math;

	gpu_triangle_array & mesh = *tris;

	parallel_for_each(
		mesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto & tri = mesh[idx];
		if( reverse )
		{
			auto a = tri.a;
			tri.a = tri.c;
			tri.c = a;
		}

		auto a_b = tri.b - tri.a;
		auto a_c = tri.c - tri.a;
		auto normal = norm( cross( a_b, a_c ) );

		tri.norm_a = normal;
		tri.norm_b = normal;
		tri.norm_c = normal;
	} );
}
