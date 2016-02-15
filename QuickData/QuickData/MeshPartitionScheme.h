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
		FILE * file = fopen( filename.c_str( ), "rb" );

		int headerTag;
		fread( &headerTag, sizeof( int ), 1, file );
		if( headerTag != PARTITION_FILE_START_TAG )
			NOT_YET_IMPLEMENTED( );

		int numChunks;
		fread( &numChunks, sizeof( int ), 1, file );

		std::vector<float_3> floats;
		floats.resize( numChunks * 2 );

		fread( floats.data( ), sizeof( float_3 ), numChunks * 2, file );

		for( int i = 0; i < numChunks; i++ )
		{
			mesh_partition_descriptor desc;

			desc.bounds_start = floats[i * 2];
			desc.bounds_end = floats[i * 2 + 1];

		#ifdef _DEBUG
			if( desc.bounds_end == float_3( 0.0 ) || desc.bounds_start == float_3( 0.0 ) ) // Uninitialized bounds
				NOT_YET_IMPLEMENTED( );
		#endif

			descriptors.emplace_back( std::move( desc ) );
		}

		//delete[] buffer;

		fclose( file );
	}

	void SaveTo( const std::string & filename )
	{
		FILE * file = fopen( filename.c_str( ), "wb" );
		fwrite( &PARTITION_FILE_START_TAG, sizeof( int ), 1, file );
		int numChunks = descriptors.size( );
		fwrite( &numChunks, sizeof( int ), 1, file );

		float_3 * output_buffer = new float_3[numChunks * 2];

		for( int i = 0; i < numChunks; i++ )
		{
			auto & desc = descriptors[i];

			output_buffer[i * 2] = desc.bounds_start;
			output_buffer[i * 2 + 1] = desc.bounds_end;
		}

		fwrite( output_buffer, sizeof( float_3 ), numChunks * 2, file );
		delete output_buffer;

		fclose( file );
	}
};