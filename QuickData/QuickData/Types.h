#pragma once

#include <amp.h>
#include <amp_math.h>
#include <amp_graphics.h>

#include <cstdint>
#include <memory>

#define ASSERT(x) if( !x ) __debugbreak( )
#define NOT_YET_IMPLEMENTED( ) __debugbreak( )

#define KB(s) ((s)*1024)
#define MB(s) ((s)*KB(1024))


using std::uint32_t;
using concurrency::index;
using concurrency::tiled_index;
using concurrency::extent;
using concurrency::parallel_for_each;
using concurrency::array_view;
using concurrency::graphics::float_3;

using std::unique_ptr;
using std::weak_ptr;
using std::shared_ptr;


/* Types */

//	Convenience getter function
#define get(type, name)		\
	__declspec(property(get = get_##name)) type name;	\
	inline type get_##name( ) const

#define vget(type, name)		\
	__declspec(property(get = get_##name)) type name;	\
	virtual inline type get_##name( ) const




struct triangle
{
	//	a, b, c, are the 3 vertices defining the triangle

	//	Use 3-dimensional floats to represent each point as X, Y, Z
	float_3 a, b, c;

	//	For each vertex, also associate a normal perpendicular to the face (value is currently the same for all 3)
	float_3 norm_a, norm_b, norm_c;

	//	Index of the volume associated with this triangle. If there is no volume associated, this is -1.
	int volumeIndex;


	triangle( ) : volumeIndex( -1 )
	{
	}

	__declspec( property( get = get_center ) ) float_3 center;
	float_3 get_center( ) const restrict( amp, cpu )
	{
		return ( a + b + c ) * 0.33333f;
	}
};

struct mesh_partition_descriptor
{
	float_3 bounds_start, bounds_end;

	bool contains_point( float_3 p ) const restrict( cpu, amp )
	{
		return
			bounds_start.x <= p.x &&
			bounds_start.y <= p.y &&
			bounds_start.z <= p.z &&
			bounds_end.x >= p.x &&
			bounds_end.y >= p.y &&
			bounds_end.z >= p.z;
	}
};

struct mesh_chunk
{
	int num_tris;
	std::vector<triangle> tris;
	mesh_partition_descriptor bounds;
};

struct voxel
{
	float_3 start, end;

	inline bool contains_point( float_3 point ) restrict( cpu, amp )
	{
		return
			start.x <= point.x && start.y <= point.y && start.z <= point.z &&
			end.x >= point.x && end.y >= point.y && end.z >= point.z;
	}
};



typedef array_view<voxel, 3> gpu_voxels;
typedef array_view<double, 3> gpu_voxel_data;
typedef array_view<int, 3> gpu_voxel_tag_data;

struct voxel_matrix
{
	gpu_voxels * dev_voxels;
	std::vector<gpu_voxel_tag_data *> dev_voxel_tag_data;
	gpu_voxel_data * dev_voxel_data;
	
	int width, height, depth;
	float_3 start, end;

	voxel_matrix( ) : dev_voxels( nullptr ), dev_voxel_data( nullptr ) { }
	voxel_matrix( int res_width, int res_height, int res_depth, float_3 bounds_start, float_3 bounds_end ) :
		dev_voxel_data( nullptr )
	{
		width = res_width;
		height = res_height;
		depth = res_depth;

		start = bounds_start;
		end = bounds_end;

		float_3 spatial_stride = bounds_end - bounds_start;
		spatial_stride.x /= width;
		spatial_stride.y /= height;
		spatial_stride.z /= depth;

		dev_voxels = new array_view<voxel, 3>( res_width, res_height, res_depth );
		auto & voxels = *dev_voxels;
		voxels.discard_data( );

		extent<3> matrixExtents( width, height, depth );

		int maxExtent = 8;
		int numIterations = width / maxExtent;

		for( int i = 0; i < numIterations; i++ )
		{
			auto currentExtent = matrixExtents;
			int full = maxExtent;
			int remaining = width - i * maxExtent;
			currentExtent[0] = remaining < full ? remaining : full;
			parallel_for_each(
				currentExtent,
				[=]( index<3> idx ) restrict( amp )
			{
				idx[0] += maxExtent * i;
				auto voxel = voxels[idx];

				float_3 fidx = float_3( idx[0], idx[1], idx[2] );
				float_3 fidx_1 = fidx + float_3( 1.0f );

				voxel.start = bounds_start + spatial_stride * fidx;
				voxel.end = bounds_start + spatial_stride * fidx_1;

				voxels[idx] = voxel;
			} );

			//voxels.synchronize( );
			concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );
			
			//voxels.source_accelerator_view.flush( );

			//voxels[0]; // Force sync occasionally for responsiveness
		}
	}

	~voxel_matrix( )
	{
		if( dev_voxels )
			delete dev_voxels;
		if( dev_voxel_data )
			delete dev_voxel_data;
		
		for( auto tags : dev_voxel_tag_data )
			delete tags;
	}

	void generate_test_data( )
	{
		using namespace concurrency::fast_math;
		dev_voxel_data = new array_view<double, 3>( dev_voxels->extent );
		auto & voxel_data = *dev_voxel_data;
		float scale = 0.2f;
		parallel_for_each(
			dev_voxels->extent,
			[=]( index<3> idx ) restrict( amp )
		{
			voxel_data[idx] = cos( idx[0] * scale ) * cos( idx[1] * scale ) * cos( idx[2] * scale );
		} );
	}
};

//	Describe CPU v GPU types
typedef concurrency::array_view<triangle> gpu_triangle_array;
typedef concurrency::array_view<float_3> gpu_vertex_array;
typedef concurrency::array_view<float_3> gpu_normal_array;
typedef concurrency::array_view<int> gpu_index_array;
typedef concurrency::array_view<double> gpu_data_array;
typedef std::vector<gpu_data_array *> gpu_data_sequence_array;

typedef concurrency::array_view<mesh_partition_descriptor> gpu_partition_descriptor_array;

typedef std::vector<gpu_triangle_array *> gpu_chunk_tris_array;



template <typename Type>
using gpu_array = concurrency::array_view<Type>;



typedef std::vector<triangle> cpu_triangle_array;
typedef std::vector<float_3> cpu_vertex_array;
typedef std::vector<int> cpu_index_array;
typedef std::vector<double> cpu_data_array;
typedef std::vector<cpu_data_array *> cpu_data_sequence_array;

typedef std::vector<mesh_partition_descriptor> cpu_partition_descriptor_array;

typedef std::vector<mesh_chunk> cpu_chunk_array;





//	This is a Utility, but its usage cases are better fitted to Types

//	Helper wrapper for concurrency::parallel_for_each
//	Breaks up a large data operation into multiple steps, to minimize GPU hang
template <typename T, typename Func>
void segmented_parallel_for_each( array_view<T> data, Func func )
{
	auto total_extent = data.extent;
	int max_extent_size = 0x7FFF;
	int num_steps = total_extent[0] / max_extent_size + 1;

	for( int step = 0; step < num_steps; step++ )
	{
		int current_offset = step * max_extent_size;
		if( current_offset >= total_extent[0] )
			break;

		//	Synchronize occasionally for responsiveness
		if( step % 10 == 0 )
		{
			data[0]; // access element to force synchronize ( calling data.synchronize() wouldn't work for some reason? )
					 //Sleep( 1 );
		}

		extent<1> current_extent( min( total_extent[0] - current_offset, max_extent_size ) );
		parallel_for_each(
			current_extent,
			[=]( index<1> idx ) restrict( amp )
		{
			func( idx + current_offset );
		} );
	}
}

template <typename T>
concurrency::array_view<T> * bindless_copy( const std::vector<T> & source )
{
	auto result = new concurrency::array_view<T>( (int)source.size( ) );
	result->discard_data( );

	for( std::size_t i = 0; i < source.size( ); i++ )
	{
		(*result)[i] = source[i];
	}

	return result;
}

template <typename T>
concurrency::array_view<T> * bindless_copy( const concurrency::array_view<T> & source )
{
	auto result = new concurrency::array_view<T>( source.extent );
	result->discard_data( );

	for( std::size_t i = 0; i < source.extent.size( ); i++ )
	{
		(*result)[i] = source[i];
	}

	return result;
}