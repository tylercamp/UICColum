#pragma once

#include "../Types.h"

void workflow_tag_voxels_with_mesh_boundary( voxel_matrix * voxelmatrix, cpu_chunk_array * chunked_mesh )
{
	if( !voxelmatrix->dev_voxels )
		NOT_YET_IMPLEMENTED( ); // Logic error

	using concurrency::graphics::int_3;

	gpu_chunk_tris_array chunked_mesh_tris;

	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		gpu_triangle_array * chunk_tris = new gpu_triangle_array( chunked_mesh->at( i ).tris );
		chunked_mesh_tris.push_back( chunk_tris );
	}

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
			std::cout << "VOXEL MATRIX DIMS CANNOT CONTAIN MESH" << std::endl;
			NOT_YET_IMPLEMENTED( );
		}

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}
	
	//	Overlap count = (end - start).size( ) -> x * y * z


	gpu_voxel_tag_data & voxel_tag_data = *( new gpu_voxel_tag_data( voxels.extent ) );
	parallel_for_each(
		voxels.extent,
		[=]( index<3> idx ) restrict( amp )
	{
		voxel_tag_data[idx] = 0;
	} );

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

	std::vector<extent<2>> work_extents;
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		auto range = chunk_voxel_overlap_end[i] - chunk_voxel_overlap_start[i];
		//	Dim0: # of voxels
		//	Dim1: # of tris
		work_extents.push_back( extent<2>( range.x * range.y * range.z, chunked_mesh_tris[i]->extent.size( ) ) );
	}

	gpu_array<int_3> dev_chunk_voxel_start( chunk_voxel_overlap_start );
	gpu_array<int_3> dev_chunk_voxel_end( chunk_voxel_overlap_end );

	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		int_3 current_chunk_range = dev_chunk_voxel_end[i] - dev_chunk_voxel_start[i];
		auto current_work_extent = work_extents[i];

		auto & current_chunk_tris = *chunked_mesh_tris[i];
		int maxVoxelsPerIteration = 16;
		int maxTrisPerIteration = 20000;

		int numVoxelIterations = current_work_extent[0] / maxVoxelsPerIteration;
		int numTriIterations = current_work_extent[1] / maxTrisPerIteration;

		//	TODO - OPTIMIZE FOR TILED USE

		//	Do sub-iterations to manage compute extents from blowing up, and to provide responsiveness while processing
		for( int s_voxel = 0; s_voxel < numVoxelIterations; s_voxel++ )
		{
			auto divided_work_extent( current_work_extent );
			divided_work_extent[0] = min( maxVoxelsPerIteration, current_work_extent[0] - s_voxel * maxVoxelsPerIteration );
			for( int s_tri = 0; s_tri < numTriIterations; s_tri++ )
			{
				divided_work_extent[1] = min( maxTrisPerIteration, current_work_extent[1] - s_tri * maxTrisPerIteration );

				auto voxelsStart = dev_chunk_voxel_start[i];

				int voxelsOffset = s_voxel * maxVoxelsPerIteration;
				int trisOffset = s_tri * maxTrisPerIteration;

				parallel_for_each(
					divided_work_extent,
					[=]( index<2> idx ) restrict( amp )
				{
					int voxel_flat_index = idx[0] + voxelsOffset;
					int tri_index = idx[1] + trisOffset;

					int_3 voxel_index;

					voxel_index.z = voxel_flat_index / ( current_chunk_range.x * current_chunk_range.y );
					voxel_flat_index -= voxel_index.z * current_chunk_range.x * current_chunk_range.y;
					voxel_index.y = voxel_flat_index / current_chunk_range.x;
					voxel_flat_index -= voxel_index.y * current_chunk_range.x;
					voxel_index.x = voxel_flat_index;

					voxel_index += voxelsStart;

					auto idx_voxel = index<3>( voxel_index.x, voxel_index.y, voxel_index.z );
					voxel v = voxels[idx_voxel];
					triangle tri = current_chunk_tris[tri_index];

					auto center = tri.center;
					if( v.contains_point( center ) )
					{
						auto & tagData = voxel_tag_data[idx_voxel];
						concurrency::atomic_fetch_inc( &tagData );
					}
				} );

				//voxel_tag_data.source_accelerator_view.;

				concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );
			}
		}

		//if( i % ( chunked_mesh->size( ) / 15 ) == 0 )
		{
			int completed = i + 1;
			int total = chunked_mesh->size( );
			std::cout << "Completed chunk " << completed << " of " << total << " (" << completed * 100 / total << "%)\n";
		}
	}


	//std::cout << "0: " << voxel_tag_data(0, 0, 0);

	voxel_tag_data.synchronize_async( );

	voxelmatrix->dev_voxel_tag_data.push_back( &voxel_tag_data );

	//	Cleanup
	for( auto tris : chunked_mesh_tris )
		delete tris;
}