#pragma once

#include "Types.h"




const int STATE_FILE_START_TAG = 0x492800;







class VolumeMeshState
{
public:

	//int associatedMeshHash;
	double timestamp;
	int numDimensions;
	int numVolumes;
	double * dataStore;
	Concurrency::graphics::double_4 max, min;

	bool validate( )
	{
		bool validDimensions = numDimensions > 0;
		bool validDataStore = dataStore != nullptr;
		return validDimensions && validDataStore;
	}

	VolumeMeshState( ) : numDimensions( -1 ), numVolumes( -1 ), timestamp( -1 ), dataStore( nullptr )
	{
		min = 1e10;
		max = -1e10;
	}

	void LoadFrom( FILE * file )
	{
		int startTag;
		fread( &startTag, sizeof( int ), 1, file );
		if( startTag != STATE_FILE_START_TAG )
			__debugbreak( );

		int newNumVolumes;
		int newNumDims;
		Concurrency::graphics::double_4 newMin, newMax;

		fread( &newNumVolumes, sizeof( int ), 1, file );
		fread( &newNumDims, sizeof( int ), 1, file );
		fread( &newMin, sizeof( newMin ), 1, file );
		fread( &newMax, sizeof( newMax ), 1, file );

		double * data = new double[newNumVolumes];
		fread( data, sizeof( double ), newNumVolumes * newNumDims, file );

		numDimensions = newNumDims;
		numVolumes = newNumVolumes;
		max = newMax;
		min = newMin;
		dataStore = data;
	}

	void LoadFrom( const std::string & source )
	{
		FILE * file = fopen( source.c_str( ), "rb" ); 
		if( !file )
			NOT_YET_IMPLEMENTED( );
		LoadFrom( file );
		fclose( file );
	}

	void SaveTo( FILE * file )
	{
		fwrite( &STATE_FILE_START_TAG, sizeof( int ), 1, file );

		fwrite( &numVolumes, sizeof( int ), 1, file );
		fwrite( &numDimensions, sizeof( int ), 1, file );
		fwrite( &min, sizeof( min ), 1, file );
		fwrite( &max, sizeof( max ), 1, file );

		fwrite( dataStore, sizeof( double ), numVolumes * numDimensions, file );
	}

	void SaveTo( const std::string & target )
	{
		FILE * file = fopen( target.c_str( ), "wb" );
		if( !file )
			NOT_YET_IMPLEMENTED( );
		SaveTo( file );
		fclose( file );
	}
};