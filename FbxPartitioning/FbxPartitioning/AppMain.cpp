
#define NOMINMAX

#include <amp.h>
#include <amp_math.h>
#include <amp_graphics.h>
#include <fbxsdk.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <cmath>
#include <thread>

#include <sstream>
#include <fstream>

#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include "include\SDL.h"

#include "MshReader.h"

#ifdef main
# undef main
#endif

#pragma comment( lib, "lib/x64/SDL2.lib" )
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )

/*
Algorithm: Multi-pass spatial partition

INITIALIZATION
1. Generate a list of triangles and a list of partition descriptors (partition bounds)
2. Allocate an integer co-array for the list of triangles (same extent as triangles)

DETERMINE EFFECTIVENESS OF "CURRENT" PARTITIONS LIST
1. For each triangle, tag it with the index of the containing partition descriptor (store in co-array)
2. (Tiled) Generate the amount of triangles tagged to each partition list
3. If any partition exceeds MAX_CHUNK_VERTICES, re-assess overflowing partitions (subdivide), repeat



ONCE EFFECTIVE PARTITION LIST IS DISCOVERED
1. In-place sort triangle list by partition index
2. Copy back to CPU, copy into partition buffers, write to disk
*/

#define MAX_CHUNK_TRIANGLES 0xFFFF

#define PREVIEW_MESH
#define PREVIEW_MESH_HIDPI

using std::uint32_t;
using concurrency::index;
using concurrency::tiled_index;
using concurrency::extent;
using concurrency::parallel_for_each;
using concurrency::array_view;
using concurrency::graphics::float_3;

#pragma comment( lib, "libfbxsdk.lib" )

#pragma warning( disable: 4018 ) // signed/unsigned mismatch
#pragma warning( disable: 4244 ) // floating-point conversion warnings
#pragma warning( disable: 4267 ) // integer conversion warnings

template <typename l, typename r>
inline l max( l left, r right )
{
	return left > right ? left : right;
}

template <typename l, typename r>
inline l min( l left, r right )
{
	return left < right ? left : right;
}



//	FBX Sample:
//	http://download.autodesk.com/us/fbx/20102/FBX_SDK_Help/index.html?url=WS8e4c2438b09b7f9c-50e6e6531197ccd93c5-7ffa.htm,topicNumber=d0e2083
//	http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html?url=WS73099cc142f487551fea285e1221e4f9ff8-7f56.htm,topicNumber=d0e4642
//	http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html?url=WS73099cc142f487551fea285e1221e4f9ff8-7f5b.htm,topicNumber=d0e4543

//	Info about licensing
//	http://forums.autodesk.com/t5/fbx-sdk/about-fbx-sdk-license/td-p/4032339



//	See: http://gamedev.stackexchange.com/questions/46802/importing-and-displaying-fbx-files
//	For FBX SDK example

//	FBX SDK Path: C:\Program Files\Autodesk\FBX\FBX SDK\2015.1


FbxMesh * SearchNode( FbxNode* pNode )
{
	auto mesh = pNode->GetMesh( );
	if( mesh )
		return mesh;

	for( int j = 0; j < pNode->GetChildCount( ); j++ )
	{
		mesh = SearchNode( pNode->GetChild( j ) );
		if( mesh )
			return mesh;
	}

	return nullptr;
}

FbxMesh * findMeshData( FbxScene * scene )
{
	auto root = scene->GetRootNode( );

	return SearchNode( root );
}

struct triangle
{
	float_3 a, b, c;

	__declspec(property(get = get_center)) float_3 center;
	float_3 get_center( ) const restrict( amp, cpu )
	{
		return (a + b + c) * 0.33333f;
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

	mesh_chunk( )
	{
	}

	mesh_chunk( mesh_partition_descriptor desc, const std::vector<triangle> & source )
	{
		bounds = desc;

		for( auto tri : source )
		if( bounds.contains_point( tri.center ) )
			tris.push_back( tri );

		num_tris = tris.size( );
	}
};

typedef concurrency::array_view<triangle> gpu_triangle_array;
typedef concurrency::array_view<float_3> gpu_vertex_array;
typedef concurrency::array_view<int> gpu_index_array;

typedef concurrency::array_view<mesh_partition_descriptor> gpu_partition_descriptor_array;


typedef std::vector<triangle> cpu_triangle_array;
typedef std::vector<float_3> cpu_vertex_array;
typedef std::vector<int> cpu_index_array;

typedef std::vector<mesh_partition_descriptor> cpu_partition_descriptor_array;
typedef std::vector<mesh_chunk> cpu_chunk_array;

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



namespace gpu
{
	//	Assumes the extent of dev_target was properly assigned to reflect the size of dev_indices
	void generate_triangle_list( gpu_triangle_array & dev_target, gpu_vertex_array & dev_verts, gpu_index_array & dev_indices )
	{
		segmented_parallel_for_each(
			dev_target,
			[=]( index<1> idx ) restrict( amp )
		{
			auto & tri = dev_target[idx];

			tri.a = dev_verts[dev_indices[idx[0] * 3 + 0]];
			tri.b = dev_verts[dev_indices[idx[0] * 3 + 1]];
			tri.c = dev_verts[dev_indices[idx[0] * 3 + 2]];
		} );
	}

	void tag_mesh_partitions( cpu_partition_descriptor_array & chunks, gpu_triangle_array & mesh, gpu_index_array & out_tags )
	{
		gpu_partition_descriptor_array dev_partitions( chunks );

		int num_partitions = (int)chunks.size( );

		segmented_parallel_for_each(
			mesh,
			[=]( index<1> idx ) restrict( amp )
		{
			float_3 tri_center = mesh[idx].center;

			int owner_chunk = -1;

			for( int i = 0; i < num_partitions; i++ )
			{
				mesh_partition_descriptor chunk = dev_partitions[i];
				if( chunk.contains_point( tri_center ) )
				{
					owner_chunk = i;
					break;
				}
			}

			out_tags[idx] = owner_chunk;
		} );

		dev_partitions.synchronize( );
	}

	void mesh_get_range( gpu_triangle_array & dev_tris, float_3 * out_min, float_3 * out_max )
	{
		using namespace concurrency::fast_math;
		//	TODO: Look into tile operations
		const int target_size = 100;

		auto base_extent = dev_tris.extent;

#define TILE_SIZE 128

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

		parallel_for_each(
			base_extent.tile<TILE_SIZE>( ).pad( ),
			[=]( tiled_index<TILE_SIZE> t_idx ) restrict( amp )
		{
			tile_static float_3 tile_ranges[TILE_SIZE];

			if( t_idx.global[0] < dev_tris.extent[0] )
				tile_ranges[t_idx.local[0]] = dev_tris[t_idx].center;

			t_idx.barrier.wait( );

			if( t_idx.local[0] == 0 )
			{
				float_3 min = tile_ranges[0], max = tile_ranges[0];

				for( int i = 1; i < TILE_SIZE; i++ )
				{
					if( t_idx.global[0] + i >= dev_tris.extent[0] )
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
}

namespace cpu
{
	void generate_triangle_list( cpu_triangle_array & dev_target, cpu_vertex_array & dev_verts, cpu_index_array & dev_indices )
	{
		for( int tri = 0; tri < dev_indices.size( ) / 3; tri++ )
		{
			triangle new_tri;
			new_tri.a = dev_verts[dev_indices[tri * 3 + 0]];
			new_tri.b = dev_verts[dev_indices[tri * 3 + 1]];
			new_tri.c = dev_verts[dev_indices[tri * 3 + 2]];

			dev_target.emplace_back( new_tri );
		}
	}

	void mesh_get_range( gpu_triangle_array & dev_tris, float_3 * out_min, float_3 * out_max )
	{
		float_3 min = dev_tris[0].center;
		float_3 max = dev_tris[0].center;

		for( int i = 0; i < dev_tris.extent.size( ); i++ )
		{
			float_3 tri = dev_tris[i].center;
			min.x = fminf( min.x, tri.x );
			min.y = fminf( min.y, tri.y );
			min.z = fminf( min.z, tri.z );

			max.x = fmaxf( max.x, tri.x );
			max.y = fmaxf( max.y, tri.y );
			max.z = fmaxf( max.z, tri.z );
		}

		*out_min = min;
		*out_max = max;
	}

	void tag_mesh_partitions( const cpu_partition_descriptor_array & chunks, const cpu_triangle_array & mesh, cpu_index_array & out_tags )
	{
		int num_partitions = chunks.size( );

		for( int t = 0; t < mesh.size( ); t++ )
		{
			float_3 tri_center = mesh[t].center;

			int owner_chunk = -1;

			for( int i = 0; i < num_partitions; i++ )
			{
				mesh_partition_descriptor chunk = chunks[i];
				if( chunk.contains_point( tri_center ) )
				{
					owner_chunk = i;
					break;
				}
			}

			out_tags[t] = owner_chunk;
		}
	}

	//	Splits on 3D space
	cpu_partition_descriptor_array split_partition( mesh_partition_descriptor & partition, int num_splits )
	{
		//	Todo - would be more effective if I sorted the partitions per axis and placed partitions based on percentiles

		cpu_partition_descriptor_array result;
		result.reserve( pow( num_splits + 1, 3 ) );

		auto range_min = partition.bounds_start;
		auto range_max = partition.bounds_end;

		float x_base = range_min.x;
		float y_base = range_min.y;
		float z_base = range_min.z;


		float x_step = (range_max.x - range_min.x) / num_splits;
		float y_step = (range_max.y - range_min.y) / num_splits;
		float z_step = (range_max.z - range_min.z) / num_splits;
		for( int z = 0; z < num_splits + 1; z++ )
		{
			for( int y = 0; y < num_splits + 1; y++ )
			{
				for( int x = 0; x < num_splits + 1; x++ )
				{
					mesh_partition_descriptor desc;
					desc.bounds_start.x = x_base + x * x_step;
					desc.bounds_start.y = y_base + y * y_step;
					desc.bounds_start.z = z_base + z * z_step;

					desc.bounds_end.x = x_base + (x + 1) * x_step;
					desc.bounds_end.y = y_base + (y + 1) * y_step;
					desc.bounds_end.z = z_base + (z + 1) * z_step;

					result.push_back( desc );
				}
			}
		}

		return result;
	}

	void chunk_mesh( gpu_triangle_array mesh, cpu_chunk_array * out_chunks )
	{
		//	TODO - should be multithreaded copy
		auto & chunks = *out_chunks;

		for( int i = 0; i < mesh.extent.size( ); i++ )
		{
			if( i % (mesh.extent.size( ) / 10) == 0 )
				std::cout << ceilf( i * 100.0f / mesh.extent.size( ) ) << "%.. ";

			for( int p = 0; p < chunks.size( ); p++ )
			{
				if( chunks[p].num_tris == chunks[p].tris.size( ) )
					continue;

				auto & desc = chunks[p].bounds;

				if( desc.contains_point( mesh[i].center ) )
				{
					chunks[p].tris.push_back( mesh[i] );
					break;
				}
			}
		}
	}
}

void draw_mesh( const cpu_triangle_array & tris )
{
	//glBegin( GL_TRIANGLES );
	glBegin( GL_POINTS );
	glColor3f( 1.0f, 1.0f, 1.0f );
	for( const auto & tri : tris )
	{
		const auto & a = tri.a;
		const auto & b = tri.b;
		const auto & c = tri.c;
		glVertex3f( a.x, a.y, a.z );
		glVertex3f( b.x, b.y, b.z );
		glVertex3f( c.x, c.y, c.z );
	}
	glEnd( );
}

void draw_mesh( const gpu_triangle_array & tris )
{
	//glBegin( GL_TRIANGLES );
	glBegin( GL_POINTS );
	glColor3f( 1.0f, 1.0f, 1.0f );
	for( int i = 0; i < tris.extent.size( ); i++ )
	{
		auto & tri = tris[i];

		const auto & a = tri.a;
		const auto & b = tri.b;
		const auto & c = tri.c;
		glVertex3f( a.x, a.y, a.z );
		glVertex3f( b.x, b.y, b.z );
		glVertex3f( c.x, c.y, c.z );
	}
	glEnd( );
}

void draw_points( const cpu_vertex_array & verts )
{
	//glBegin( GL_TRIANGLES );
	glBegin( GL_POINTS );
	glColor3f( 1.0f, 1.0f, 1.0f );
	for( const auto & vert : verts )
	{
		glVertex3f( vert.x, vert.y, vert.z );
	}
	glEnd( );
}

void draw_partition( const mesh_partition_descriptor & partition )
{
	glBegin( GL_LINE_LOOP );
	//	Fixed bug where GPU mesh wouldn't fully show, now need to fix weird partitioning schemes
	//throw;
	glEnd( );
}

void draw_partitions( const cpu_partition_descriptor_array & partitions )
{
	for( auto & partition : partitions )
		draw_partition( partition );
}





void run_window( cpu_triangle_array & dev_tris )
{
	int width = 1920;
	int height = 1080;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.5;
	height *= 1.5;
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 10000.0 );



	float ox = 0.0f, oy = 0.0f, oz = 0.0f;

	glMatrixMode( GL_MODELVIEW );
	//glTranslatef( -ox, -oy, -oz );
	//glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
	//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	bool run = true;
	while( run )
	{
		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
		glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		//draw_mesh( cpu_tris );
		draw_mesh( dev_tris );
		//draw_points( cpu_vertices );

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
		if( e.type == SDL_QUIT )
			run = false;

		if( GetAsyncKeyState( 'A' ) )
			ox -= 2.0f;
		if( GetAsyncKeyState( 'D' ) )
			ox += 2.0f;
		if( GetAsyncKeyState( 'W' ) )
			oz -= 2.0f;
		if( GetAsyncKeyState( 'S' ) )
			oz += 2.0f;

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

void run_window( gpu_triangle_array & dev_tris )
{
	cpu_triangle_array tris;
	tris.resize( dev_tris.extent.size( ) );

	for( int i = 0; i < tris.size( ); i++ )
		tris[i] = dev_tris[i];

	run_window( tris );
}

void run_window( cpu_vertex_array & verts )
{
	int width = 1920;
	int height = 1080;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.5;
	height *= 1.5;
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 10000.0 );



	float ox = 0.0f, oy = 0.0f, oz = 0.0f;

	glMatrixMode( GL_MODELVIEW );
	//glTranslatef( -ox, -oy, -oz );
	//glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
	//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	bool run = true;
	while( run )
	{
		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
		glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		//draw_mesh( cpu_tris );
		//draw_mesh( dev_tris );
		draw_points( verts );

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
		if( e.type == SDL_QUIT )
			run = false;

		if( GetAsyncKeyState( 'A' ) )
			ox -= 2.0f;
		if( GetAsyncKeyState( 'D' ) )
			ox += 2.0f;
		if( GetAsyncKeyState( 'W' ) )
			oz -= 2.0f;
		if( GetAsyncKeyState( 'S' ) )
			oz += 2.0f;

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

void partition_mesh( gpu_triangle_array & mesh, mesh_partition_descriptor & basePartition, cpu_chunk_array * out_chunks )
{
	auto & dev_mesh = mesh;
	cpu_triangle_array cpu_tris;
	for( int i = 0; i < mesh.extent.size( ); i++ )
		cpu_tris.push_back( mesh[i] );

	//	options
	int axisSplits = 2;

	cpu_partition_descriptor_array partitions = cpu::split_partition( basePartition, axisSplits );
	std::vector<int> partitionSizes;

	for( ;; )
	{
		std::cout << "Attempting mesh partitioning using " << partitions.size( ) << " partitions" << std::endl;

		std::cout << "\t(GPU partition tagging... ";
		gpu_index_array dev_meshtags( dev_mesh.extent );
		dev_meshtags.discard_data( );

		gpu::tag_mesh_partitions( partitions, dev_mesh, dev_meshtags );
		//cpu::tag_mesh_partitions( partitions, cpu_tris, meshtags );

		dev_meshtags.synchronize( );

		partitionSizes.clear( );
		partitionSizes.assign( partitions.size( ), 0 );

		std::cout << "done)" << std::endl;


		std::cout << "\t(CPU counting partition membership... ";
		for( int i = 0; i < cpu_tris.size( ); i++ )
		{
			int partitionIndex = dev_meshtags[i];
#ifdef _DEBUG
			triangle tri = dev_mesh[i];
			if( partitionIndex < 0 )
				__debugbreak( );
#endif
			++partitionSizes[partitionIndex];
		}

		//	Check if the current partitioning scheme is valid (satisfies max chunk size)

		cpu_partition_descriptor_array newPartitions;

		std::cout << "done)" << std::endl;

		std::cout << "\t(CPU discovering new partitions... ";
		bool schemeWasValid = true;
		int largestViolation = 0;
		int numRemovedPartitions = 0;
		for( int i = 0; i < partitions.size( ); i++ )
		{
			int violation = fmax( 0, partitionSizes[i] - MAX_CHUNK_TRIANGLES );
			largestViolation = fmax( largestViolation, violation );

			if( violation > 0 )
			{
				//	Break partition
				auto missingPartitions = cpu::split_partition( partitions[i], axisSplits );
				for( auto p : missingPartitions )
					newPartitions.emplace_back( p );

				partitions.erase( partitions.begin( ) + i );
				partitionSizes.erase( partitionSizes.begin( ) + i );

				--i;

				schemeWasValid = false;
			}
			else if( partitionSizes[i] == 0 )
			{
				//	Remove partitions that aren't useful
				partitions.erase( partitions.begin( ) + i );
				partitionSizes.erase( partitionSizes.begin( ) + i );
				--i;
				numRemovedPartitions++;
			}
		}

		std::cout << "done)" << std::endl;

		std::cout << "Discovered " << newPartitions.size( ) << " new partitions, removed " << numRemovedPartitions << " unused partitions, largest violation was " << largestViolation << " tris" << std::endl;

		for( auto newPartition : newPartitions )
			partitions.emplace_back( newPartition );

		if( schemeWasValid )
			break;

		std::cout << std::endl;
	}

	std::cout << "\nCPU chunking mesh using partitions... ";

	//	TODO - Could parallelize (sort data by partition?)
	cpu_chunk_array & chunks = *out_chunks;
	chunks.resize( partitions.size( ) );

	for( int c = 0; c < chunks.size( ); c++ )
	{
		chunks[c].num_tris = partitionSizes[c];
		chunks[c].bounds = partitions[c];
		chunks[c].tris.reserve( partitionSizes[c] );
	}

	cpu::chunk_mesh( mesh, &chunks );

	std::cout << "done)" << std::endl;
}

void get_mesh_data_fbx( std::string filename, cpu_vertex_array * out_vertices, cpu_index_array * out_indices )
{
	FbxManager * fbxManager = FbxManager::Create( );

	FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	fbxManager->SetIOSettings( ios );

	FbxImporter * importer = FbxImporter::Create( fbxManager, "" );
	if( !importer->Initialize( filename.c_str( ), -1, fbxManager->GetIOSettings( ) ) )
		__debugbreak( );

	FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
	importer->Import( scene );
	importer->Destroy( );

	FbxMesh * mesh = findMeshData( scene );

	int numVerts = mesh->GetControlPointsCount( );

	int numIndices = mesh->GetPolygonVertexCount( );
	int * indices = mesh->GetPolygonVertices( );

	cpu_vertex_array & cpu_vertices = *out_vertices;
	cpu_vertices.resize( numVerts );
	cpu_index_array & cpu_indices = *out_indices;
	cpu_indices.resize( numIndices );

	//	Begin copy process
	for( int i = 0; i < numVerts; i++ )
	{
		FbxVector4 vert = mesh->GetControlPointAt( i );
		cpu_vertices[i].x = (float)vert.mData[0];
		cpu_vertices[i].y = (float)vert.mData[1];
		cpu_vertices[i].z = (float)vert.mData[2];
	}

	for( int i = 0; i < numIndices; i++ )
	{
		cpu_indices[i] = indices[i];
	}

	fbxManager->Destroy( );
}

void get_mesh_data_msh( std::string filename, cpu_vertex_array * out_vertices, cpu_index_array * out_surface_indices, cpu_index_array * out_inner_indices )
{
	cpu_vertex_array & cpu_vertices = *out_vertices;
	cpu_index_array & cpu_indices = *out_surface_indices;

	MshReader * reader = new MshReader( filename );
	//	Generally no more than one set of point data
	if( reader->PointData.size( ) > 1 )
		__debugbreak( );

	auto & pointData = reader->PointData[0];
	for( std::size_t i = 0; i < pointData.count; i++ )
	{
		auto & currentPoint = pointData.data[i];
		cpu_vertices.push_back( float_3( currentPoint.data[0], currentPoint.data[1], currentPoint.data[2] ) );
	}

	for( std::size_t i = 0; i < reader->FaceData.size( ); i++ )
	{
		auto & faceData = reader->FaceData[i];
		//	Ignore inner faces
		if( faceData.data[0].is_inner )
			continue;

		for( std::size_t j = 0; j < faceData.data.size( ); j++ )
		{
			//	How is this thing triangulated?
		}
	}
}

void save_chunk( std::string filename, const mesh_chunk & chunk )
{
	FbxManager * fbxManager = FbxManager::Create( );

	FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	fbxManager->SetIOSettings( ios );

	FbxExporter * exporter = FbxExporter::Create( fbxManager, "" );
	if( !exporter->Initialize( filename.c_str( ), -1, fbxManager->GetIOSettings( ) ) )
		__debugbreak( );

	FbxScene * scene = FbxScene::Create( fbxManager, "scene" );
	FbxNode * meshNode = FbxNode::Create( scene, "MeshNode" );
	FbxMesh * outputMesh = FbxMesh::Create( scene, "Mesh" );
	meshNode->SetNodeAttribute( outputMesh );
	scene->GetRootNode( )->AddChild( meshNode );

	outputMesh->InitControlPoints( chunk.num_tris * 3 );
	auto * controlPoints = outputMesh->GetControlPoints( );

	for( int i = 0; i < chunk.num_tris; i++ )
	{
		auto & tri = chunk.tris[i];

		controlPoints[i * 3 + 0].Set( tri.a.x, tri.a.y, tri.a.z );
		controlPoints[i * 3 + 1].Set( tri.b.x, tri.b.y, tri.b.z );
		controlPoints[i * 3 + 2].Set( tri.c.x, tri.c.y, tri.c.z );

		outputMesh->BeginPolygon( );
		outputMesh->AddPolygon( i * 3 + 0 );
		outputMesh->AddPolygon( i * 3 + 1 );
		outputMesh->AddPolygon( i * 3 + 2 );
		outputMesh->EndPolygon( );
	}

	exporter->Export( scene );

	exporter->Destroy( );
	fbxManager->Destroy( );
}

void save_chunkset_metadata( std::string filename, const cpu_chunk_array & chunkset )
{
	std::ofstream file( filename );

	//file << "FbxPartitioning"

	file.close( );
}

std::string formatDataSize( long long numBytes )
{
	const int KB = 1024;
	const int MB = 1024 * KB;
	const int GB = 1024 * MB;

	std::string label;
	float count;

	if( numBytes > GB )
	{
		label = "GB";
		count = numBytes / (float)GB;
	}
	else if( numBytes > MB )
	{
		label = "MB";
		count = numBytes / (float)MB;
	}
	else if( numBytes > KB )
	{
		label = "KB";
		count = numBytes / (float)KB;
	}
	else
	{
		label = "B";
		count = (float)numBytes;
	}

	int count_spec = (int)(count * 100);

	std::ostringstream result;
	result << (count_spec / 100.0f) << label;
	return result.str( );
}

std::string formatTime( long ms )
{
	const int duration_seconds = 1000;
	const int duration_minutes = duration_seconds * 60;
	const int duration_hours = duration_minutes * 60;

	std::string label;
	float count;

	if( ms > duration_hours )
	{
		label = "hrs";
		count = ms / (float)duration_hours;
	}
	else if( ms > duration_minutes )
	{
		label = "mins";
		count = ms / (float)duration_minutes;
	}
	else if( ms > duration_seconds )
	{
		label = "s";
		count = ms / (float)duration_seconds;
	}
	else
	{
		label = "ms";
		count = (float)ms;
	}

	int count_spec = (int)(count * 100);

	std::ostringstream result;
	result << (count_spec / 100.0f) << label;
	return result.str( );
}

void process( std::string file )
{
	auto start = clock( );

	std::cout << "\n\n OPERATING ON " << file << std::endl;

	cpu_vertex_array cpu_vertices;
	cpu_index_array cpu_indices;
	auto lt = clock( );
	std::cout << "Loading mesh data... ";
	//get_mesh_data_fbx( file, &cpu_vertices, &cpu_indices );
	get_mesh_data_msh( file, &cpu_vertices, &cpu_indices, nullptr );

	run_window( cpu_vertices );

	int numTris = cpu_indices.size( ) / 3; // assume model is triangulated 
	concurrency::extent<1> meshExtent( numTris );

	gpu_triangle_array dev_tris( meshExtent );

	std::cout << "copying... ";

	//	generate tris list
	{
		gpu_index_array dev_indices( cpu_indices );
		gpu_vertex_array dev_verts( cpu_vertices );

		dev_tris.discard_data( );

		gpu::generate_triangle_list( dev_tris, dev_verts, dev_indices );

		dev_tris.synchronize( );
	}

	//cpu_triangle_array cpu_tris;
	//cpu::generate_triangle_list( cpu_tris, cpu_vertices, cpu_indices );

	std::cout << "done." << std::endl;

#ifdef PREVIEW_MESH
	//run_window( cpu_vertices );
	run_window( dev_tris );
#endif

	std::cout << "Loading mesh took " << formatTime( clock( ) - lt ) << std::endl;
	std::cout << "Tris array size: " << formatDataSize( dev_tris.extent.size( ) * sizeof(triangle) ) << std::endl;

	float_3 range_min, range_max;
	gpu::mesh_get_range( dev_tris, &range_min, &range_max );
	//cpu::mesh_get_range( dev_tris, &range_min, &range_max );

	mesh_partition_descriptor basePartition;
	basePartition.bounds_start = range_min;
	basePartition.bounds_end = range_max;

	cpu_chunk_array chunks;

	auto pt = clock( );
	partition_mesh( dev_tris, basePartition, &chunks );
	std::cout << "Partitioning took " << formatTime( clock( ) - pt ) << std::endl;

	std::cout << "Saving chunks... ";
	CreateDirectoryA( (file + "_chunks/").c_str( ), nullptr );
	for( int i = 0; i < chunks.size( ); i++ )
	{
		std::ostringstream name;
		name << file << "_chunks/";
		name << file << "_" << i << ".fbx";
		auto & chunk = chunks[i];
		save_chunk( name.str( ), chunk );
	}

	save_chunkset_metadata( file + "_chunks/meta", chunks );

	std::cout << "\nDONE" << std::endl;
	std::cout << "Total operation took " << formatTime( clock( ) - start ) << std::endl;
}

void pause( )
{
	std::string str;
	std::getline( std::cin, str );
}

int main( )
{
	auto start = clock( );

	process( "IanArteries5.GAMBIT.msh" );

	//process( "bsArteries.fbx" );
	//process( "bsCSF.fbx" );
	//process( "bsGray.fbx" );
	//process( "bsScalp.fbx" );
	//process( "bsSkull.fbx" );
	//process( "bsVeins.fbx" );
	//process( "bsWhite.fbx" );

	auto end = clock( );

	std::cout << "\n\nTOTAL OPERATION TOOK: " << (end - start) / 1000 << "s" << std::endl;

	pause( );
	return 0;
}

/*
//	(I think this already has a name?)
ALGORITHM FOR GENERATING STATISTICAL PARTITION SPLITS

With vertex data on the GPU, sort vertex data on sign and magnitude per dimension (spatial_sort)
Copy vertex data to the CPU, sample at quartiles


*/