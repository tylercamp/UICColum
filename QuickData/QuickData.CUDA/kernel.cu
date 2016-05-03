
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include "Utilities.h"

#include "BinaryVoxelMatrix.h"
#include "BinaryMesh.h"
#include "ImportBinaryMeshWorkflow.h"

#include <iostream>
#include <string>
#include <fstream>






struct CUDA_VoxelMatrix
{
	voxel * raw_voxel_store;

	float3 * cuda_voxels_start;
	float3 * cuda_voxels_end;

	int * cuda_voxel_tags;

	//cudaExtent cuda_matrix_extent;
	//cudaChannelFormatDesc cuda_voxel_format_desc;

	int3 resolution;
	float3 start, end;

	CUDA_VoxelMatrix( int res_width, int res_height, int res_depth, float3 bounds_start, float3 bounds_end  )
	{
		resolution = make_int3( res_width, res_height, res_depth );

		//cuda_matrix_extent = make_cudaExtent( width, height, depth );
		//cuda_format_desc = cudaCreateChannelDesc(  )
		//cuda_voxel_format_desc = cudaCreateChannelDesc( 32, )

		start = bounds_start;
		end = bounds_end;

		float3 stride = (end - start) / resolution;



		int numVoxels = res_width * res_height * res_depth;
		cudaMalloc( &cuda_voxels_start, numVoxels * sizeof( float ) *  3 );
		cudaMalloc( &cuda_voxels_end, numVoxels * sizeof( float ) * 3 );
		cudaMalloc( &cuda_voxel_tags, numVoxels * sizeof( int ) );

		raw_voxel_store = new voxel[numVoxels];

		for( int z = 0, idx = 0; z < res_depth; z++ )
		{
			for( int y = 0; y < res_height; y++ )
			{
				for( int x = 0; x < res_width; x++ )
				{
					voxel current;
					current.start.x = start.x + stride.x * x;
					current.start.y = start.y + stride.y * y;
					current.start.z = start.z + stride.z * z;

					current.end.x = start.x + stride.x * (x + 1);
					current.end.y = start.y + stride.y * (y + 1);
					current.end.z = start.z + stride.z * (z + 1);

					raw_voxel_store[idx++] = current;
				}
			}
		}

		float * voxel_starts = new float[numVoxels * 3]; // 3 floats per voxel
		for( int i = 0; i < numVoxels; i++ )
		{
			const auto & voxel = raw_voxel_store[i];
			voxel_starts[i * 3 + 0] = voxel.start.x;
			voxel_starts[i * 3 + 1] = voxel.start.y;
			voxel_starts[i * 3 + 2] = voxel.start.z;
		}
		cudaMemcpy( cuda_voxels_start, voxel_starts, numVoxels * 3 * sizeof( float ), cudaMemcpyHostToDevice );
		delete[] voxel_starts;

		float * voxel_ends = new float[numVoxels * 3]; // 3 floats per voxel
		for( int i = 0; i < numVoxels; i++ )
		{
			const auto & voxel = raw_voxel_store[i];
			voxel_ends[i * 3 + 0] = voxel.end.x;
			voxel_ends[i * 3 + 1] = voxel.end.y;
			voxel_ends[i * 3 + 2] = voxel.end.z;
		}
		cudaMemcpy( cuda_voxels_end, voxel_ends, numVoxels * 3 * sizeof( float ), cudaMemcpyHostToDevice );
		delete[] voxel_ends;

		int * voxel_tags = new int[numVoxels];
		for( int i = 0; i < numVoxels; i++ )
			voxel_tags[i] = 0;
		cudaMemcpy( cuda_voxel_tags, voxel_tags, numVoxels * sizeof( int ), cudaMemcpyHostToDevice );
		delete[] voxel_tags;
	}

	~CUDA_VoxelMatrix( )
	{
		cudaFree( cuda_voxels_start );
		cudaFree( cuda_voxels_end );
		cudaFree( cuda_voxel_tags );
	}

	void CopyTo( voxel * voxels_store, int num_voxels )
	{
		NOT_YET_IMPLEMENTED( );
	}
};



#define TRIS_PER_TILE 128
#define TILE_SIZE 128

__global__ void tagVoxelsKernel(
	int * targetTags,
	const float3 * voxel_starts, const float3 * voxel_ends,
	const float3 * chunk_tris, int num_chunk_tris,
	int3 matrix_resolution,
	int3 voxel_offset // Offset of this span within the matrix
	)
{
	__shared__ float3 current_tile_tri_centers[TRIS_PER_TILE];

	int numTileTris = min( num_chunk_tris - blockIdx.x * TILE_SIZE, TRIS_PER_TILE );
	
	int threadStride = TILE_SIZE;
	int numIterations = (int)ceil( (float)TRIS_PER_TILE / (float)TILE_SIZE );
	int trisOffset = blockIdx.x * TRIS_PER_TILE;
	for( int i = 0; i < numIterations; i++ )
	{
		int bidx = i * threadStride + threadIdx.x;
		float3 a = chunk_tris[trisOffset + bidx + 0];
		float3 b = chunk_tris[trisOffset + bidx + 1];
		float3 c = chunk_tris[trisOffset + bidx + 2];

		float3 center;
		center.x = 0.333333f * ( a.x + b.x + c.x );
		center.y = 0.333333f * ( a.y + b.y + c.y );
		center.z = 0.333333f * ( a.z + b.z + c.z );

		current_tile_tri_centers[bidx] = center;
	}

	__syncthreads( );

	int vidx = threadIdx.x * blockDim.y * blockDim.z + threadIdx.y * blockDim.z + threadIdx.z;

	float3 vstart = voxel_starts[vidx];
	float3 vend = voxel_ends[vidx];

	int numTags = 0;
	for( int i = 0; i < numTileTris; i++ )
	{
		float3 c = current_tile_tri_centers[i];
		if( vstart.x < c.x &&
			vstart.y < c.y &&
			vstart.z < c.z &&
			vend.x > c.x &&
			vend.y > c.y &&
			vend.z > c.z )
			++numTags;
	}

	atomicAdd( targetTags + vidx, numTags );
}

int main()
{
	std::vector<mesh_chunk> * mesh_set;
	workflow_import_binary_mesh_set( "../QuickData/patients/bs/bsArteries/surfaces", &mesh_set );

	int numChunks = mesh_set->size( );
	std::vector<float3 *> mesh_set_tris;
	std::vector<int> mesh_set_tris_counts;
	for( int i = 0; i < numChunks; i++ )
	{
		const auto & chunk = mesh_set->at( i );
		float3 * cudaStore;
		cudaMalloc( &cudaStore, chunk.num_tris * sizeof(float3) * 3 );
		cudaMemcpy( cudaStore, chunk.tris.data( ), sizeof( float3 ) * 3, cudaMemcpyHostToDevice );

		mesh_set_tris.push_back( cudaStore );
		mesh_set_tris_counts.push_back( chunk.num_tris );
	}

	MeshPartitionScheme mesh_partition_scheme;
	mesh_partition_scheme.LoadFrom( "../QuickData/patients/bs/bsArteries/surfaces/partitions.binmeshscheme" );


	//	The overlapped regions between chunk and voxels will always be a box, definable by start and end points.
	std::vector<int3> chunk_voxel_overlap_start;
	std::vector<int3> chunk_voxel_overlap_end;

	std::cout << "Generating voxel-chunk bounds... ";
	CUDA_VoxelMatrix vm( 512, 512, 512, make_float3( -30.0f ), make_float3( 190.0f ) );
	int3 vm_dims = vm.resolution;

	auto vmStart = vm.start;
	auto stride = (vm.end - vm.start) / vm.resolution;
	auto invStride = make_float3( 1.0f ) / stride;

	std::cout << std::endl;
	//	For every chunk
	for( int i = 0; i < mesh_set->size( ); i++ )
	{
		auto & chunk = mesh_set->at( i );
		auto desc = chunk.bounds;


		float3 transformed_min = (desc.bounds_start - vmStart) * invStride;
		float3 transformed_max = (desc.bounds_end - vmStart) * invStride;

		int3 overlap_start = make_int3(
			floorf( transformed_min.x ),
			floorf( transformed_min.y ),
			floorf( transformed_min.z )
			);

		int3 overlap_end = make_int3(
			ceilf( transformed_max.x ),
			ceilf( transformed_max.y ),
			ceilf( transformed_max.z )
			);

		if( overlap_start.x < 0 || overlap_start.y < 0 || overlap_start.z < 0 ||
			overlap_end.x < 0 || overlap_end.y < 0 || overlap_end.z < 0 ||
			overlap_start.x >= vm_dims.x || overlap_start.y >= vm_dims.y || overlap_start.z >= vm_dims.z ||
			overlap_end.x >= vm_dims.x || overlap_end.y > vm_dims.y || vm_dims.z > vm_dims.z )
		{
			std::cout << "VOXEL MATRIX SPACE CANNOT CONTAIN MESH" << std::endl;
			NOT_YET_IMPLEMENTED( );
		}

		chunk_voxel_overlap_start.push_back( overlap_start );
		chunk_voxel_overlap_end.push_back( overlap_end );
	}

	
	
	for( int c = 0; c < numChunks; c++ )
	{
		std::cout << "c: " << c << std::endl;
		auto voxels_start = chunk_voxel_overlap_start[c];
		auto voxels_end = chunk_voxel_overlap_end[c];

		const auto & chunk = mesh_set->at( c );
		int numBlocks = (int)ceil( (float)chunk.num_tris / (float)TRIS_PER_TILE );
		dim3 voxelExtents( voxels_end.x - voxels_start.x, voxels_end.y - voxels_start.y, voxels_end.z - voxels_start.z );

		tagVoxelsKernel<<<numBlocks, voxelExtents>>>(
					vm.cuda_voxel_tags,
					vm.cuda_voxels_start, vm.cuda_voxels_end,
					mesh_set_tris[c],
					chunk.num_tris,
					vm.resolution,
					voxels_start
					);

		auto result = cudaDeviceSynchronize( );
		std::cout << "sync: " << result << std::endl;
		Sleep( 1 );
	}

	int * result_tags = new int[vm.resolution.x * vm.resolution.y * vm.resolution.z];
	cudaMemcpy( result_tags, vm.cuda_voxel_tags, sizeof( int ) * vm.resolution.x * vm.resolution.y * vm.resolution.z, cudaMemcpyDeviceToHost );
	for( int i = 0; i < vm.resolution.x * vm.resolution.y * vm.resolution.z; i++ )
		std::cout << result_tags[i] << ", ";

	std::string j;
	std::getline( std::cin, j );

    // Add vectors in parallel.
	/*
    cudaError_t cudaStatus = addWithCuda_Syncronous(c, a, b, arraySize);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addWithCuda failed!");
        return 1;
    }
	

    printf("{1,2,3,4,5} + {10,20,30,40,50} = {%d,%d,%d,%d,%d}\n",
        c[0], c[1], c[2], c[3], c[4]);

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }
	*/

    return 0;
}
