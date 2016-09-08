

/*
	Validates input data against normal deviations
*/



//	Possibility - continuance check?

#include <iostream>
#include <ctime>

#include <Windows.h>
#include "../QuickData/MshReader.h"
#include "../QuickData/Workflows/ImportMshWorkflow.h"

using namespace concurrency::graphics;

struct MshSample
{
	float_3 min, max, avg;
};



//	Modified from GatherMeshBoundsWorkflow
void gather_point_bounds( gpu_vertex_array * pts, float_3 * out_min, float_3 * out_max )
{
	using namespace concurrency::fast_math;
	//	TODO: Look into tile operations
	const int target_size = 100;

	auto & dev_pts = *pts;
	auto base_extent = dev_pts.extent;

#define TILE_SIZE 256

	array_view<float_3> mins( base_extent / TILE_SIZE + 1 );
	mins.discard_data( );
	array_view<float_3> maxs( base_extent / TILE_SIZE + 1 );
	maxs.discard_data( );

	//	Repeat while size > target_size


	int tris_per_tile = base_extent[0] / TILE_SIZE;
	if( tris_per_tile > 0xFFFF )
	{
		throw "Mesh too large, tiling size must be increased for processing";
	}


	//	FUTURE TODO: USE 2D TILE EXTENTS
	parallel_for_each(
		base_extent.tile<TILE_SIZE>( ).pad( ),
		[dev_pts, mins, maxs]( tiled_index<TILE_SIZE> t_idx ) restrict( amp )
	{
		tile_static float_3 tile_ranges[TILE_SIZE];

		if( t_idx.global[0] < dev_pts.extent[0] )
			tile_ranges[t_idx.local[0]] = dev_pts[t_idx];

		t_idx.barrier.wait( );

		if( t_idx.local[0] == 0 )
		{
			float_3 min = tile_ranges[0], max = tile_ranges[0];

			for( int i = 1; i < TILE_SIZE; i++ )
			{
				if( t_idx.global[0] + i >= dev_pts.extent[0] )
					break;

				float_3 & current = tile_ranges[i];
				min.x = fminf( min.x, current.x );
				min.y = fminf( min.y, current.y );
				min.z = fminf( min.z, current.z );

				max.x = fmaxf( max.x, current.x );
				max.y = fmaxf( max.y, current.y );
				max.z = fmaxf( max.z, current.z );
			}

			mins[t_idx.tile[0]] = min;
			maxs[t_idx.tile[0]] = max;
		}
	} );

#undef TILE_SIZE

	mins.synchronize( );
	maxs.synchronize( );

	float_3 min, max;
	min = mins[0];
	max = maxs[0];

	for( int i = 0; i < mins.extent[0]; i++ )
	{
		float_3 current = mins[i];
		min.x = fminf( min.x, current.x );
		min.y = fminf( min.y, current.y );
		min.z = fminf( min.z, current.z );
	}

	for( int i = 0; i < mins.extent[0]; i++ )
	{
		float_3 current = maxs[i];
		max.x = fmaxf( max.x, current.x );
		max.y = fmaxf( max.y, current.y );
		max.z = fmaxf( max.z, current.z );
	}

	float_3 range = max - min;
	//	Extend working bounds to offset error
	min -= range * 0.01f;
	max += range * 0.01f;

	*out_min = min;
	*out_max = max;
}


MshSample generate_sample( const std::string & mshFile )
{
	gpu_vertex_array * verts;
	VolumeType type;
	
	MshReader reader( mshFile );
	std::vector<float_3> cpu_verts;
	cpu_verts.reserve( reader.PointData[0].count );
	for( std::size_t i = 0; i < reader.PointData[0].count; i++ )
	{
		auto pt = reader.PointData[0].data[i];
		cpu_verts.push_back( float_3( pt.data[0], pt.data[1], pt.data[2] ) );
	}

	verts = new gpu_vertex_array( cpu_verts );
	

	MshSample result;

	float_3 min, max;
	gather_point_bounds( verts, &min, &max );

	result.min = min;
	result.max = max;
	result.avg = float_3(-1);

	delete verts;

	return result;
}

std::ostream& operator<<(std::ostream & strm, float_3 & f)
{
	strm << f.x << ", " << f.y << ", " << f.z;
	return strm;
}

#ifndef BUILD_DLL
int main( )
#else
int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show )
#endif
{
	auto file = "../QuickData/patients/KevinBestV7.GAMBIT.msh";

	//	todo - find proper values
	MshSample reference;

	MshSample sample = generate_sample( file );
	std::cout << "min: " << sample.min << std::endl;
	std::cout << "max: " << sample.max << std::endl;

	std::getline( std::cin, std::string(), '\n' );

	return 0;
}