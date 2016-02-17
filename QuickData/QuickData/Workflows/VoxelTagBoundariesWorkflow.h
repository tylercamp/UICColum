#pragma once

#include "../Types.h"

//	V3
/*
void workflow_tag_voxels_with_mesh_boundary( voxel_matrix * voxelmatrix, cpu_chunk_array * chunked_mesh )
{
	if( !voxelmatrix->dev_voxels )
		NOT_YET_IMPLEMENTED( ); // Logic error

	using concurrency::graphics::int_3;

	std::cout << "Loading data... ";
	gpu_chunk_tris_array chunked_mesh_tris;

	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		gpu_triangle_array * chunk_tris = new gpu_triangle_array( chunked_mesh->at( i ).tris );
		chunked_mesh_tris.push_back( chunk_tris );

		chunk_tris->synchronize( concurrency::access_type_read_write );

	#ifdef _DEBUG
		const auto & src = chunked_mesh->at( i );

		//	Confirm that copy went okay...
		for( int t = 0; t < src.num_tris; t++ )
		{
			auto a = src.tris[t];
			auto b = chunk_tris->operator()( t );
			if( a.a != b.a || a.b != b.b || a.c != b.c || a.center != b.center )
				__debugbreak( );
		}
	#endif
	}

	std::cout << "Done.\n";

	auto & voxels = *voxelmatrix->dev_voxels;
	int_3 voxelstack_dims = int_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );


	//	The overlapped regions between chunk and voxels will always be a box, definable by start and end points.
	std::vector<int_3> chunk_voxel_overlap_start;
	std::vector<int_3> chunk_voxel_overlap_end;

	std::cout << "Generating voxel-chunk bounds... ";


	auto vmStart = voxelmatrix->start;
	auto stride = ( voxelmatrix->end - voxelmatrix->start ) / float_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );
	auto invStride = float_3( 1.0f ) / stride;

	std::cout << std::endl;
	//	For every chunk
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		auto & chunk = chunked_mesh->at( i );
		auto desc = chunk.bounds;

		//	HAVE FOUND STRANGE BOUNDS VALUES, NEED TO RE-CHECK CHUNK EXPORT/IMPORT

		//std::cout << desc.bounds_start.x << ", " << desc.bounds_start.y << ", " << desc.bounds_start.z << " | ";
		//std::cout << desc.bounds_end.x << ", " << desc.bounds_end.y << ", " << desc.bounds_end.z << "\n";

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
			std::cout << "VOXEL MATRIX SPACE CANNOT CONTAIN MESH" << std::endl;
			NOT_YET_IMPLEMENTED( );
		}

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}

	//	Overlap count = (end - start).size( ) -> x * y * z

	std::cout << "Done.\n";

	std::cout << "Initializing tag data... ";
	gpu_voxel_tag_data & voxel_tag_data = *( new gpu_voxel_tag_data( voxels.extent ) );
	parallel_for_each(
		voxels.extent,
		[=]( index<3> idx ) restrict( amp )
	{
		voxel_tag_data[idx] = 0;
	} );
	std::cout << "Done.\n";

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );



	gpu_array<int_3> dev_chunk_voxel_start( chunk_voxel_overlap_start );
	gpu_array<int_3> dev_chunk_voxel_end( chunk_voxel_overlap_end );

	std::cout << "BEGINNING VOXEL-MESH TAGGING" << std::endl;
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		const auto & chunk = chunked_mesh->at( i );
		int numTris = chunk.num_tris;
		auto & tris = *chunked_mesh_tris[i];

		auto voxel_range = dev_chunk_voxel_end[i] - dev_chunk_voxel_start[i];
		auto voxels_start = dev_chunk_voxel_start[i];

		auto idx_voxels_start = index<3>( voxels_start.x, voxels_start.y, voxels_start.z );
		auto work_extent = extent<3>( voxel_range.x, voxel_range.y, voxel_range.z );

		parallel_for_each(
			work_extent,
			[=]( index<3> idx ) restrict( amp )
		{
			index<3> voxel_idx = idx + idx_voxels_start;
			auto voxel = voxels[voxel_idx];

			for( int t = 0; t < numTris; t++ )
			{
				if( voxel.contains_point( tris[t].center ) )
					voxel_tag_data[voxel_idx]++;
			}
		} );

		concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

		if( i % (int) ( chunked_mesh->size( ) * 0.04 ) == 0 ) // Update in ~4% increments
		{
			int completed = i + 1;
			int total = chunked_mesh->size( );
			std::cout << "Completed chunk " << completed << " of " << total << " (" << completed * 100 / total << "%)\n";
		}
	}

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

	voxelmatrix->dev_voxel_tag_data.push_back( &voxel_tag_data );

	//	Cleanup
	for( auto tris : chunked_mesh_tris )
		delete tris;
}
*/

//	V2



void workflow_tag_voxels_with_mesh_boundary( voxel_matrix * voxelmatrix, cpu_chunk_array * chunked_mesh )
{
	if( !voxelmatrix->dev_voxels )
		NOT_YET_IMPLEMENTED( ); // Logic error

	using concurrency::graphics::int_3;

	std::cout << "Loading data... ";
	gpu_chunk_tris_array chunked_mesh_tris;

	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		gpu_triangle_array * chunk_tris = new gpu_triangle_array( chunked_mesh->at( i ).tris );
		chunked_mesh_tris.push_back( chunk_tris );
	}

	std::cout << "Done.\n";

	auto & voxels = *voxelmatrix->dev_voxels;
	int_3 voxelstack_dims = int_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );


	//	The overlapped regions between chunk and voxels will always be a box, definable by start and end points.
	std::vector<int_3> chunk_voxel_overlap_start;
	std::vector<int_3> chunk_voxel_overlap_end;

	std::cout << "Generating voxel-chunk bounds... ";


	auto vmStart = voxelmatrix->start;
	auto stride = ( voxelmatrix->end - voxelmatrix->start ) / float_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );
	auto invStride = float_3( 1.0f ) / stride;

	std::cout << std::endl;
	//	For every chunk
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		auto & chunk = chunked_mesh->at( i );
		auto desc = chunk.bounds;

		//	HAVE FOUND STRANGE BOUNDS VALUES, NEED TO RE-CHECK CHUNK EXPORT/IMPORT

		//std::cout << desc.bounds_start.x << ", " << desc.bounds_start.y << ", " << desc.bounds_start.z << " | ";
		//std::cout << desc.bounds_end.x << ", " << desc.bounds_end.y << ", " << desc.bounds_end.z << "\n";

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
			std::cout << "VOXEL MATRIX SPACE CANNOT CONTAIN MESH" << std::endl;
			NOT_YET_IMPLEMENTED( );
		}

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}

	//	Overlap count = (end - start).size( ) -> x * y * z

	std::cout << "Done.\n";

	std::cout << "Initializing tag data... ";
	gpu_voxel_tag_data & voxel_tag_data = *( new gpu_voxel_tag_data( voxels.extent ) );
	parallel_for_each(
		voxels.extent,
		[=]( index<3> idx ) restrict( amp )
	{
		voxel_tag_data[idx] = 0;
	} );
	std::cout << "Done.\n";

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

	std::vector<extent<3>> work_extents;
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		auto range = chunk_voxel_overlap_end[i] - chunk_voxel_overlap_start[i];
		work_extents.push_back( extent<3>( range.x, range.y, range.z ) );
	}

	gpu_array<int_3> dev_chunk_voxel_start( chunk_voxel_overlap_start );
	gpu_array<int_3> dev_chunk_voxel_end( chunk_voxel_overlap_end );

	//	Attempt minimal full-voxel solution for GPU reference implementation
	
	std::cout << "BEGINNING VOXEL-MESH TAGGING" << std::endl;
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		//	Should switch to TRIS-major grouping, more numerically stable than voxels-major

#define NUM_TRIS_PER_JOB 256
#define NUM_VOXELS_PER_DIM 8
#define NUM_VOXELS_PER_TILE (NUM_VOXELS_PER_DIM * NUM_VOXELS_PER_DIM * NUM_VOXELS_PER_DIM)

		int_3 current_chunk_range = dev_chunk_voxel_end[i] - dev_chunk_voxel_start[i];
		auto current_work_extent = work_extents[i];

		auto & current_chunk_tris = *chunked_mesh_tris[i];

		int numTriPasses = ceilf( current_chunk_tris.extent.size( ) / (float) NUM_TRIS_PER_JOB );

		auto start_voxels = index<3>( dev_chunk_voxel_start[i].x, dev_chunk_voxel_start[i].y, dev_chunk_voxel_start[i].z );

		for( int p = 0; p < numTriPasses; p++ )
		{
			int triOffset = p * NUM_TRIS_PER_JOB;
			int numCurrentTris = min( NUM_TRIS_PER_JOB, current_chunk_tris.extent.size( ) - triOffset );

			const int numLoadsPerThread = (int)ceilf( NUM_TRIS_PER_JOB / (float) NUM_VOXELS_PER_TILE );

			parallel_for_each(
				current_work_extent.tile<NUM_VOXELS_PER_DIM, NUM_VOXELS_PER_DIM, NUM_VOXELS_PER_DIM>().pad( ),
				[=]( tiled_index<NUM_VOXELS_PER_DIM, NUM_VOXELS_PER_DIM, NUM_VOXELS_PER_DIM> idx ) restrict( amp )
			{
				tile_static float_3 tri_centers[NUM_TRIS_PER_JOB];

				int triIndexOffset =
					idx.local[0] * NUM_VOXELS_PER_DIM * NUM_VOXELS_PER_DIM +
					idx.local[1] * NUM_VOXELS_PER_DIM +
					idx.local[2];


				int stride = NUM_VOXELS_PER_TILE;

				for( int t = 0; t < numLoadsPerThread; t++ )
				{
					int storeIndex = triIndexOffset + t * stride;
					int loadIndex = triOffset + triIndexOffset + t * stride;
					tri_centers[storeIndex] = current_chunk_tris[loadIndex].center;
				}

				idx.barrier.wait_with_tile_static_memory_fence( );

				if( idx.global[0] >= current_chunk_range.x ||
					idx.global[1] >= current_chunk_range.y ||
					idx.global[2] >= current_chunk_range.z )
					return;

				index<3> voxel_idx = idx.global + start_voxels;

				int numTags = 0;
				voxel v = voxels[voxel_idx];
				for( int t = 0; t < numCurrentTris; t++ )
				{
					if( v.contains_point( tri_centers[t] ) )
						++numTags;
				}

				concurrency::atomic_fetch_add( &voxel_tag_data[voxel_idx], numTags );
			} );

			concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

			//std::cout << "Pass " << p << " of " << numTriPasses << std::endl;
		}

		if( i % (int) ( chunked_mesh->size( ) * 0.04 ) == 0 ) // Update in ~4% increments
		{
			int completed = i + 1;
			int total = chunked_mesh->size( );
			std::cout << "Completed chunk " << completed << " of " << total << " (" << completed * 100 / total << "%)\n";
		}
	}

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );

	voxelmatrix->dev_voxel_tag_data.push_back( &voxel_tag_data );

	//	Cleanup
	for( auto tris : chunked_mesh_tris )
		delete tris;
}





//	V1

/*
void workflow_tag_voxels_with_mesh_boundary( voxel_matrix * voxelmatrix, cpu_chunk_array * chunked_mesh )
{
	if( !voxelmatrix->dev_voxels )
		NOT_YET_IMPLEMENTED( ); // Logic error

	using concurrency::graphics::int_3;

	std::cout << "Loading data... ";
	gpu_chunk_tris_array chunked_mesh_tris;

	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		gpu_triangle_array * chunk_tris = new gpu_triangle_array( chunked_mesh->at( i ).tris );
		chunked_mesh_tris.push_back( chunk_tris );
	}

	std::cout << "Done.\n";

	auto & voxels = *voxelmatrix->dev_voxels;
	int_3 voxelstack_dims = int_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth );


	//	The overlapped regions between chunk and voxels will always be a box, definable by start and end points.
	std::vector<int_3> chunk_voxel_overlap_start;
	std::vector<int_3> chunk_voxel_overlap_end;

	std::cout << "Generating voxel-chunk bounds... ";


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

		//chunk_voxel_overlap_start.push_back( int_3( 0 ) );
		//chunk_voxel_overlap_end.push_back( int_3( voxelmatrix->width, voxelmatrix->height, voxelmatrix->depth ) );

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}

	//	Overlap count = (end - start).size( ) -> x * y * z

	std::cout << "Done.\n";

	std::cout << "Initializing tag data... ";
	gpu_voxel_tag_data & voxel_tag_data = *( new gpu_voxel_tag_data( voxels.extent ) );
	parallel_for_each(
		voxels.extent,
		[=]( index<3> idx ) restrict( amp )
	{
		voxel_tag_data[idx] = 0;
	} );
	std::cout << "Done.\n";

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

	//	Attempt minimal full-voxel solution for GPU reference implementation

	std::cout << "BEGINNING VOXEL-MESH TAGGING" << std::endl;
	for( int i = 0; i < chunked_mesh->size( ); i++ )
	{
		//	Should switch to TRIS-major grouping, more numerically stable than voxels-major

#define NUM_TRIS_PER_JOB 1024
#define NUM_VOXELS_PER_TILE 256

		int_3 current_chunk_range = dev_chunk_voxel_end[i] - dev_chunk_voxel_start[i];
		auto current_work_extent = work_extents[i];

		auto & current_chunk_tris = *chunked_mesh_tris[i];
		int maxVoxelsPerIteration = NUM_VOXELS_PER_TILE;

		int numVoxelIterations = ceilf( current_work_extent[0] / (float)maxVoxelsPerIteration );

		//	Do sub-iterations to manage compute extents from blowing up, and to provide responsiveness while processing
		for( int s_voxel = 0; s_voxel < numVoxelIterations; s_voxel++ )
		{
			auto divided_work_extent( current_work_extent );
			divided_work_extent[0] = min( maxVoxelsPerIteration, current_work_extent[0] - s_voxel * maxVoxelsPerIteration );
			divided_work_extent[1] = (int)ceilf(current_work_extent[1] / (float)NUM_TRIS_PER_JOB);

			auto voxelsStart = dev_chunk_voxel_start[i];

			int voxelsOffset = s_voxel * maxVoxelsPerIteration;

			if( NUM_TRIS_PER_JOB % NUM_VOXELS_PER_TILE != 0 )
				NOT_YET_IMPLEMENTED( );

			//	Tag between *these* voxels (from voxelsStart for maxVoxelsPerIteration) and idx[1]'th batch of triangles
			parallel_for_each(
				divided_work_extent.tile<NUM_VOXELS_PER_TILE, 1>().pad(),
				[=]( tiled_index<NUM_VOXELS_PER_TILE, 1> idx ) restrict( amp )
			{
				tile_static float_3 triangle_centers[NUM_TRIS_PER_JOB];

				int voxel_flat_index = idx.global[0] + voxelsOffset;
				int batch_index = idx.global[1];

				int numTrisInBatch = min( NUM_TRIS_PER_JOB, current_work_extent[0] - batch_index * NUM_TRIS_PER_JOB );

				//	Load tris into tile memory
				for( int x = 0; x < NUM_TRIS_PER_JOB / NUM_VOXELS_PER_TILE; x++ )
				{
					//	Could cause access violation error (or some variant) without bounding to numTrisInBatch
					triangle_centers[x * NUM_VOXELS_PER_TILE + idx.local[0]] = current_chunk_tris[batch_index * NUM_TRIS_PER_JOB + x * NUM_VOXELS_PER_TILE + idx.local[0]].center;
				}

				idx.barrier.wait_with_tile_static_memory_fence( );

				if( voxelsOffset + idx.local[0] >= current_work_extent[0] )
					return;

				//	Read: http://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
				//	Get current voxel (Not entirely correct, need to redo !!!)
				int_3 voxel_index;

				//	X-major (should change?)
				voxel_index.x = voxel_flat_index / ( current_chunk_range.y * current_chunk_range.z );
				voxel_flat_index -= voxel_index.x * current_chunk_range.y * current_chunk_range.z;
				voxel_index.y = voxel_flat_index / current_chunk_range.z;
				voxel_index.z = voxel_flat_index % current_chunk_range.z;

				voxel_index += voxelsStart;

				auto idx_voxel = index<3>( voxel_index.x, voxel_index.y, voxel_index.z );
				voxel v = voxels[idx_voxel];

				int numTags = 0;

				for( int i = 0; i < numTrisInBatch; i++ )
				{
					if( v.contains_point( triangle_centers[i] ) )
						++numTags;
				}

				auto & tagData = voxel_tag_data[idx_voxel];
				concurrency::atomic_fetch_add( &tagData, numTags );
			} );

			//voxel_tag_data.source_accelerator_view.;

			if( s_voxel % 4 == 0 )
				concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );
			
		}

		if( i % (int)( chunked_mesh->size( ) * 0.04 ) == 0 ) // Update in ~4% increments
		{
			int completed = i + 1;
			int total = chunked_mesh->size( );
			std::cout << "Completed chunk " << completed << " of " << total << " (" << completed * 100 / total << "%)\n";
		}
	}

	voxel_tag_data.synchronize_async( );

	std::cout << "(" << voxel_tag_data( 0, 0, 0 ) << ")" << std::endl;

	voxelmatrix->dev_voxel_tag_data.push_back( &voxel_tag_data );

	//	Cleanup
	for( auto tris : chunked_mesh_tris )
		delete tris;
}
*/