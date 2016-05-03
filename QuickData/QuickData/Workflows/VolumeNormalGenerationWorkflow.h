#pragma once

#include "../Types.h"
#include "../Utilities.h"



#define HEXAHEDRAL_VOLUME_MEMBER_COUNT 12
#define TETRAHEDRAL_VOLUME_MEMBER_COUNT 4

struct volume_hexahedral
{
	//	numStored incremented upon generation, should reach VOLUME_MEMBER_COUNT
	volume_hexahedral( ) : numStored( 0 ), center( 0.0f ) { }

	volume_hexahedral & operator=( const volume_hexahedral & vt ) restrict( cpu, amp )
	{
		numStored = vt.numStored;
		center = vt.center;

		for( int i = 0; i < HEXAHEDRAL_VOLUME_MEMBER_COUNT; i++ )
			tris[i] = vt.tris[i];

		return *this;
	}

	//	6 faces to a hexahedron, 2 triangles per face
	triangle tris[HEXAHEDRAL_VOLUME_MEMBER_COUNT];
	int numStored;

	float_3 center;
};

struct volume_tetrahedral
{
	//	numStored incremented upon generation, should reach VOLUME_MEMBER_COUNT
	volume_tetrahedral( ) : numStored( 0 ), center( 0.0f )
	{
	}

	volume_tetrahedral & operator=( const volume_tetrahedral & vt ) restrict( cpu, amp )
	{
		numStored = vt.numStored;
		center = vt.center;

		for( int i = 0; i < TETRAHEDRAL_VOLUME_MEMBER_COUNT; i++ )
			tris[i] = vt.tris[i];

		return *this;
	}

	//	4 faces to a tetrahedron
	triangle tris[TETRAHEDRAL_VOLUME_MEMBER_COUNT];
	int numStored;

	float_3 center;
};



void gen_volume_normals_tetrahedral( gpu_triangle_array * tris, std::size_t numVolumes )
{

	auto & mesh = *tris;

	//	GENERATE VOLUME STRUCTURE

	std::vector<volume_tetrahedral> volumes;
	volumes.resize( numVolumes );

	//	Organize into volumes
	for( std::size_t i = 0; i < mesh.extent.size( ); i++ )
	{
		auto & tri = mesh[i];
		auto & volume = volumes[tri.volumeIndex];
		if( volume.numStored >= TETRAHEDRAL_VOLUME_MEMBER_COUNT )
			__debugbreak( );
		volume.tris[volume.numStored++] = tri;
	}

	array_view<volume_tetrahedral> dev_volumes( volumes );

	//	Calculate volume centers
	parallel_for_each(
		dev_volumes.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto volume = dev_volumes[idx];

		float_3 sum( 0.0f );
		for( int i = 0; i < TETRAHEDRAL_VOLUME_MEMBER_COUNT; i++ )
			sum += volume.tris[i].center;

		volume.center = sum / volume.numStored;

		dev_volumes[idx] = volume;
	} );

	parallel_for_each(
		mesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto tri = mesh[idx];
		const auto volume = dev_volumes[tri.volumeIndex];

		float_3 relativePosition = tri.center - volume.center;
		relativePosition = norm( relativePosition );

		float_3 a_b = tri.b - tri.a;
		float_3 a_c = tri.c - tri.a;
		float_3 norm = ::norm( cross( a_b, a_c ) );

		//	If our norm is facing the opposite direction...
		if( dot( relativePosition, norm ) < 0.0f )
		{
			//	Reverse the winding and normal
			float_3 c = tri.c;
			tri.c = tri.a;
			tri.a = c;

			norm = -norm;
		}

		tri.norm_a = norm;
		tri.norm_b = norm;
		tri.norm_c = norm;

		mesh[idx] = tri;
	} );
}

void gen_volume_normals_hexahedral( gpu_triangle_array * tris, std::size_t numVolumes )
{

	auto & mesh = *tris;

	//	GENERATE VOLUME STRUCTURE

	std::vector<volume_hexahedral> volumes;
	volumes.resize( numVolumes );

	

	//	Organize into volumes
	for( std::size_t i = 0; i < mesh.extent.size( ); i++ )
	{
		auto tri = mesh[i];
		auto & volume = volumes[tri.volumeIndex];
		if( volume.numStored >= HEXAHEDRAL_VOLUME_MEMBER_COUNT )
			__debugbreak( );
		volume.tris[volume.numStored++] = tri;
	}

	array_view<volume_hexahedral> dev_volumes( volumes );

	//	Calculate volume centers
	parallel_for_each(
		dev_volumes.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto volume = dev_volumes[idx];

		float_3 sum( 0.0f );
		for( int i = 0; i < HEXAHEDRAL_VOLUME_MEMBER_COUNT; i++ )
			sum += volume.tris[i].center;

		volume.center = sum / volume.numStored;

		dev_volumes[idx] = volume;
	} );

	parallel_for_each(
		mesh.extent,
		[=]( index<1> idx ) restrict( amp )
	{
		auto tri = mesh[idx];
		const auto volume = dev_volumes[tri.volumeIndex];

		float_3 relativePosition = tri.center - volume.center;
		relativePosition = norm( relativePosition );

		float_3 a_b = tri.b - tri.a;
		float_3 a_c = tri.c - tri.a;
		float_3 norm = ::norm( cross( a_b, a_c ) );

		//	If our norm is facing the opposite direction...
		if( dot( relativePosition, norm ) < 0.0f )
		{
			//	Reverse the winding and normal
			float_3 c = tri.c;
			tri.c = tri.a;
			tri.a = c;

			norm = -norm;
		}

		tri.norm_a = norm;
		tri.norm_b = norm;
		tri.norm_c = norm;

		mesh[idx] = tri;
	} );
}

//	Normals should be pointing out from center of volume, not guaranteed by direct normal from tri
void workflow_gen_volume_normals( gpu_triangle_array * tris, std::size_t numVolumes, VolumeType volumeType )
{
	switch( volumeType )
	{
		case( VolumeTypeTetrahedral ) :
			gen_volume_normals_tetrahedral( tris, numVolumes );
			break;

		case( VolumeTypeHexahedral ) :
			gen_volume_normals_hexahedral( tris, numVolumes );
			break;
	}
}