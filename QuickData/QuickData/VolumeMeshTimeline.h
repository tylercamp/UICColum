#pragma once

#include <algorithm>
#include "VolumeMeshState.h"

const int TIMELINE_FILE_START_TAG = 0x928154;

const int VOLUME_DATA_START_TAG = 0xfe22ac;


class VolumeMeshTimeline
{
public:
	std::vector<VolumeMeshState *> states;

	bool validate( )
	{
		bool mismatchDimensions = false;
		bool mismatchVolumes = false;

		int standardDim = states.front( )->numDimensions;
		int standardVolumes = states.front( )->numVolumes;
		for( auto state : states )
		{
			if( state->numDimensions != standardDim )
				mismatchDimensions = true;
			if( state->numVolumes != standardVolumes )
				mismatchVolumes = true;
		}

		return !mismatchDimensions && !mismatchVolumes;
	}

	void sort( )
	{
		std::sort( states.begin( ), states.end( ), []( VolumeMeshState * l, VolumeMeshState * r ) { return r->timestamp < l->timestamp; } );
	}

	void SaveTo( const std::string & filePath )
	{
		if( !validate( ) )
			NOT_YET_IMPLEMENTED( );

		sort( );

		FILE * file = fopen( filePath.c_str( ), "wb" );
		if( !file )
			NOT_YET_IMPLEMENTED( );

		fwrite( &TIMELINE_FILE_START_TAG, sizeof( int ), 1, file );

		for( auto state : states )
		{
			state->SaveTo( file );
		}

		fclose( file );
	}

	void LoadFrom( const std::string & filePath )
	{
		FILE * file = fopen( filePath.c_str( ), "rb" );
		if( !file )
			NOT_YET_IMPLEMENTED( );

		int readStartTag;
		fread( &readStartTag, sizeof( int ), 1, file );
		if( readStartTag != TIMELINE_FILE_START_TAG )
			NOT_YET_IMPLEMENTED( );

		while( !feof( file ) )
		{
			auto loadedState = new VolumeMeshState( );
			loadedState->LoadFrom( file );
			states.push_back( loadedState );
		}

		fclose( file );
	}
};

