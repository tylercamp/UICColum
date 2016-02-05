#pragma once

#include <fstream>
#include "Types.h"

const int PARTITION_FILE_START_TAG = 0x8823a;

class MeshPartitionScheme
{
public:
	//	descriptor index is 1-1 with mesh chunk collection (ie descriptor 0 for chunk 0, etc.)
	std::vector<mesh_partition_descriptor> descriptors;

	MeshPartitionScheme( ) { }

	MeshPartitionScheme( const cpu_chunk_array * source )
	{
		for( int i = 0; i < source->size( ); i++ )
			descriptors.push_back( source->at( i ).bounds );
	}

	void LoadFrom( const std::string & filename )
	{
		FILE * file = fopen( filename.c_str( ), "r" );

		int headerTag;
		fread( &headerTag, sizeof( int ), 1, file );
		if( headerTag != PARTITION_FILE_START_TAG )
			NOT_YET_IMPLEMENTED( );

		int numChunks;
		fread( &numChunks, sizeof( int ), 1, file );

		for( int i = 0; i < numChunks; i++ )
		{
			mesh_partition_descriptor desc;
			fread( &desc.bounds_start, sizeof( float ) * 3, 1, file );
			fread( &desc.bounds_end, sizeof( float ) * 3, 1, file );
			descriptors.emplace_back( std::move( desc ) );
		}

		fclose( file );
	}

	void SaveTo( const std::string & filename )
	{
		FILE * file = fopen( filename.c_str( ), "w" );
		fwrite( &PARTITION_FILE_START_TAG, sizeof( int ), 1, file );
		int numChunks = descriptors.size( );
		fwrite( &numChunks, sizeof( int ), 1, file );

		for( int i = 0; i < numChunks; i++ )
		{
			auto & desc = descriptors[i];
			fwrite( &desc.bounds_start, sizeof( float ) * 3, 1, file );
			fwrite( &desc.bounds_end, sizeof( float ) * 3, 1, file );
		}

		fclose( file );
	}
};