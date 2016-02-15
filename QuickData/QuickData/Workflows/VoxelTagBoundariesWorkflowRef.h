#pragma once

#include "../Types.h"

//	CPU implementation of voxel tagging for correctness
void workflow_tag_voxels_with_mesh_boundary_ref( voxel_matrix * voxelmatrix, cpu_chunk_array * chunked_mesh )
{
	auto & voxels = *voxelmatrix->dev_voxels;
	int_3 voxelstack_dims = int_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );


	//	The overlapped regions between chunk and voxels will always be a box, definable by start and end points.
	std::vector<int_3> chunk_voxel_overlap_start;
	std::vector<int_3> chunk_voxel_overlap_end;

	auto vmStart = voxelmatrix->start;
	auto stride = ( voxelmatrix->end - voxelmatrix->start ) / float_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );
	auto invStride = float_3( 1.0f ) / stride;

	//	For every chunk
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		auto & chunk = chunked_mesh->at( i );
		auto desc = chunk.bounds;

		float_3 transformed_min = ( desc.bounds_start - vmStart ) * invStride;
		float_3 transformed_max = ( desc.bounds_end - vmStart ) * invStride;

		int_3 overlap_start = int_3(
			floorf( transformed_min.x ),
			floorf( transformed_min.y ),
			floorf( transformed_min.z )
			);

		int_3 overlap_end = int_3(
			ceilf( transformed_max.x ),
			ceilf( transformed_max.y ),
			ceilf( transformed_max.z )
			);

		if( overlap_start.x < 0 || overlap_start.y < 0 || overlap_start.z < 0 ||
			overlap_end.x < 0 || overlap_end.y < 0 || overlap_end.z < 0 ||
			overlap_start.x >= voxelstack_dims.x || overlap_start.y >= voxelstack_dims.y || overlap_start.z >= voxelstack_dims.z ||
			overlap_end.x >= voxelstack_dims.x || overlap_end.y > voxelstack_dims.y || overlap_end.z > voxelstack_dims.z )
		{
			std::cout << "(Warn) Voxel space does not contain entire mesh" << std::endl;
		}

		overlap_start = max( overlap_start, int_3( 0 ) );
		overlap_end = min( overlap_end, voxelstack_dims - int_3( 1 ) );

		if( overlap_start != int_3( 0 ) || overlap_end != voxelstack_dims - int_3( 1 ) )
			std::cout << "Found data to tag!" << std::endl;

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}

	gpu_voxel_tag_data & tagdata = *( new gpu_voxel_tag_data( voxelmatrix->dev_voxels->extent ) );
	for( int x = 0; x < tagdata.extent[0]; x++ )
		for( int y = 0; y < tagdata.extent[1]; y++ )
			for( int z = 0; z < tagdata.extent[2]; z++ )
				tagdata( x, y, z ) = 0;

	for( int c = 0; c < chunked_mesh->size( ); c++ )
	{
		auto voxel_range = chunk_voxel_overlap_end[c] - chunk_voxel_overlap_start[c] + 1;
		if( voxel_range == int_3( 0 ) )
			continue;

		const auto & current_chunk = chunked_mesh->at( c );
		for( int x = 0; x < voxel_range.x; x++ )
		{
			for( int y = 0; y < voxel_range.y; y++ )
			{
				for( int z = 0; z < voxel_range.z; z++ )
				{
					auto voxelIndex = int_3( x, y, z ) + chunk_voxel_overlap_start[c];
					//auto voxelIndex = chunk_voxel_overlap_start[c] + int_3( x, y, z );
					auto idx_voxel = index<3>( voxelIndex.x, voxelIndex.y, voxelIndex.z );

					auto voxel = voxelmatrix->dev_voxels->operator()( idx_voxel );
					int numTags = 0;

					for( int t = 0; t < current_chunk.num_tris; t++ )
					{
						auto tri = current_chunk.tris[t];
						if( voxel.contains_point( tri.center ) )
							++numTags;
					}

					tagdata( idx_voxel ) += numTags;
				}
			}
		}

		std::cout << "Completed chunk " << c + 1 << "/" << chunked_mesh->size( ) << std::endl;
	}

	voxelmatrix->dev_voxel_tag_data.push_back( &tagdata );
}