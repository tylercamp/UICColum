#pragma once

#include <map>
#include "../Types.h"

inline int veckey( const float_3 & vec ) restrict( cpu, amp )
{
	int x = *( (int *) &vec + 0 );
	int y = *( (int *) &vec + 1 );
	int z = *( (int *) &vec + 2 );
	return x ^ y ^ z;
}

// (BROKEN)	Assumes workflow_generate_normals has been called on a pretriangulated mesh
void workflow_smooth_normals( gpu_triangle_array * mesh, const gpu_vertex_array * originalVertices, const gpu_index_array * originalIndices )
{
	using namespace concurrency;

	auto & rmesh = *mesh;
	auto & rindices = *originalIndices;

	auto s = clock( );

	//	Generate list of unique vertices (assumes originalVertices is not/might not be unique)
	std::map<int, float_3> uniqueVertices;
	for( int i = 0; i < originalVertices->extent[0]; i++ )
	{
		const auto & v = originalVertices->data()[i];
		int key = veckey( v );
		uniqueVertices[key] = v;
	}

	std::map<int, int> vertexIndices;
	cpu_vertex_array cpuUniqueVertices( uniqueVertices.size( ) );
	int i = 0;
	for( const auto & kv : uniqueVertices )
	{
		cpuUniqueVertices[i] = kv.second;
		vertexIndices[kv.first] = i++;
	}

	cpu_index_array cpuMergedIndices( originalIndices->extent[0] );
	for( int i = 0; i < originalIndices->extent[0]; i++ )
	{
		const auto & v = originalVertices->data( )[originalIndices->data( )[i]];
		cpuMergedIndices[i] = vertexIndices[veckey( v )];
	}



	//	Sum all normals associated with every vertex, keep track of vertex reference count
	gpu_vertex_array optimizedVertices( cpuUniqueVertices );
	gpu_index_array optimizedIndices( cpuMergedIndices );

	gpu_normal_array sum_normals( optimizedVertices.extent );
	array_view<int> num_vert_refs( optimizedVertices.extent );
	gpu_normal_array index_normals( optimizedIndices.extent );

	std::cout << clock( ) - s;

	parallel_for_each(
		sum_normals.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		sum_normals[idx] = float_3( 0 );
	} );

	parallel_for_each(
		optimizedIndices.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		int tri_index = idx[0] / 3;
		//	norm_a = norm_b = norm_c at this point
		index_normals[idx] = rmesh[tri_index].norm_a;
		atomic_fetch_inc( &num_vert_refs[optimizedIndices[idx]] );
	} );

	for( int i = 0; i < index_normals.extent[0]; i++ )
	{
		int point_index = optimizedIndices[i];
		sum_normals[point_index] += index_normals[i];
	}

	//	Not correct average

	for( int i = 0; i < sum_normals.extent[0]; i++ )
		sum_normals[i] /= num_vert_refs[i];

	parallel_for_each(
		mesh->extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto tri = rmesh[idx];
		tri.norm_a = sum_normals[optimizedIndices[idx[0] * 3 + 0]];
		tri.norm_b = sum_normals[optimizedIndices[idx[0] * 3 + 1]];
		tri.norm_c = sum_normals[optimizedIndices[idx[0] * 3 + 2]];

		rmesh[idx] = tri;
	} );
}