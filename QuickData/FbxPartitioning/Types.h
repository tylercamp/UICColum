#pragma once

#include <amp.h>
#include <amp_math.h>
#include <amp_graphics.h>

#include <cstdint>

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


/* Types */

//	Convenience getter function
#define get(type, name)		\
	__declspec(property(get = get_##name)) type name;	\
	inline type get_##name( ) const

#define vget(type, name)		\
	__declspec(property(get = get_##name)) type name;	\
	virtual inline type get_##name( ) const



#define TRI_NUM_DATASTORES 1

struct triangle
{
	float_3 a, b, c;
	float_3 norm_a, norm_b, norm_c;

	int volumeIndex;
	float volumeValues[TRI_NUM_DATASTORES];

	triangle( ) : volumeIndex( -1 )
	{
		for( int i = 0; i < TRI_NUM_DATASTORES; i++ )
			volumeValues[i] = 0.0f;
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

	bool contains_point( float_3 p ) restrict( cpu, amp )
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

//	Describe CPU v GPU types
typedef concurrency::array_view<triangle> gpu_triangle_array;
typedef concurrency::array_view<float_3> gpu_vertex_array;
typedef concurrency::array_view<float_3> gpu_normal_array;
typedef concurrency::array_view<int> gpu_index_array;
typedef concurrency::array_view<float> gpu_data_array;

typedef concurrency::array_view<mesh_partition_descriptor> gpu_partition_descriptor_array;


typedef std::vector<triangle> cpu_triangle_array;
typedef std::vector<float_3> cpu_vertex_array;
typedef std::vector<int> cpu_index_array;
typedef std::vector<float> cpu_data_array;

typedef std::vector<mesh_partition_descriptor> cpu_partition_descriptor_array;

typedef std::vector<mesh_chunk> cpu_chunk_array;




//	Alternative to concurrency::parallel_for_each
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
concurrency::array_view<T> * bindless_copy( const std::vector<T> source )
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