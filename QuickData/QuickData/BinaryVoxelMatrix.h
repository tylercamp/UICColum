#pragma once

#include "Types.h"

const int MATRIX_FILE_START_TAG = 0x8273859;

class BinaryVoxelMatrix
{
public:
	//	
	std::vector<double *> voxel_data;
	float_3 spatial_start;
	float_3 spatial_end;
	int_3 resolution; 


	BinaryVoxelMatrix( )
	{
		
	}

	~BinaryVoxelMatrix( )
	{
		for( auto arr : voxel_data )
			delete[] arr;
	}

	void SaveTo( const std::string & target )
	{
		auto f = fopen( target.c_str( ), "w" );
		fwrite( &MATRIX_FILE_START_TAG, sizeof( int ), 1, f );
		fwrite( &resolution, sizeof( int ) * 3, 1, f );
		fwrite( &spatial_start, sizeof( float ) * 3, 1, f );
		fwrite( &spatial_end, sizeof( float ) * 3, 1, f );
		int numDims = voxel_data.size( );
		fwrite( &numDims, sizeof( float ), 1, f );

		for( auto data : voxel_data )
		{
			int numData = resolution.x * resolution.y * resolution.z;
			fwrite( data, sizeof( double ), numData, f );
		}

		fclose( f );
	}

	void LoadFrom( const std::string & target )
	{
		auto f = fopen( target.c_str( ), "w" );
		int headerTag;
		fread( &headerTag, sizeof( int ), 1, f );
		if( headerTag != MATRIX_FILE_START_TAG )
			NOT_YET_IMPLEMENTED( ); // Not matrix data file

		fread( &resolution, sizeof( int ) * 3, 1, f );
		fread( &spatial_start, sizeof( float ) * 3, 1, f );
		fread( &spatial_end, sizeof( float ) * 3, 1, f );
		int numDims = voxel_data.size( );
		fread( &numDims, sizeof( float ), 1, f );

		for( int i = 0; i < numDims; i++ )
		{
			int numData = resolution.x * resolution.y * resolution.z;
			double * data = new double[numData];
			fread( data, sizeof( double ), numData, f );
			voxel_data.push_back( data );
		}

		fclose( f );
	}
};