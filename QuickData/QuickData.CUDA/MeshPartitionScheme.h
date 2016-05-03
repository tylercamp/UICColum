
#pragma once

#include "Utilities.h"

const int PARTITION_FILE_START_TAG = 0x8823a;

class MeshPartitionScheme
{
public:
	//	descriptor index is 1-1 with mesh chunk collection (ie descriptor 0 for chunk 0, etc.)
	std::vector<mesh_partition_descriptor> descriptors;

	MeshPartitionScheme( ) {}

	void LoadFrom( const std::string & filename )
	{
		FILE * file = fopen( filename.c_str( ), "rb" );

		int headerTag;
		fread( &headerTag, sizeof(int), 1, file );
		if( headerTag != PARTITION_FILE_START_TAG )
			NOT_YET_IMPLEMENTED( );

		int numChunks;
		fread( &numChunks, sizeof(int), 1, file );

		std::vector<float3> floats;
		floats.resize( numChunks * 2 );

		fread( floats.data( ), sizeof(float3), numChunks * 2, file );

		for( int i = 0; i < numChunks; i++ )
		{
			mesh_partition_descriptor desc;

			desc.bounds_start = floats[i * 2];
			desc.bounds_end = floats[i * 2 + 1];

			descriptors.emplace_back( std::move( desc ) );
		}

		//delete[] buffer;

		fclose( file );
	}
};

