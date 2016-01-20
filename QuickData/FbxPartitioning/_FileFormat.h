#pragma once

//	Not currently being used, not fully implemented

#include "Types.h"

#include <map>
#include <vector>

class FileFormat
{
public:
	typedef int Tag;

	Tag headerTag;

	get( int, numStores ) const
	{
		return taggedData.size( );
	}

	std::vector<std::pair<Tag, int>> intProperties;

	std::vector<std::pair<Tag, double *>> taggedData;
	std::vector<int> dataDims;
	std::vector<int> dataCounts;

	
	bool validate( )
	{
		bool storeDimsAlign = taggedData.size( ) == dataDims.size( );
		bool dataCountsAlign = taggedData.size( ) == dataCounts.size( );
		return storeDimsAlign && dataCountsAlign;
	}



	void AddIntProperty( Tag tag, int value )
	{
		
	}

	void AddDataStore( Tag tag, double * data, int numDimensions, int numElements )
	{
		taggedData.push_back( { tag, data } );
		dataDims.push_back( numDimensions );
		dataCounts.push_back( numElements );
	}



	void LoadFrom( const std::string & filePath )
	{
		FILE * file;
		file = fopen( filePath.c_str( ), "rb" );
		if( !file )
			NOT_YET_IMPLEMENTED( );

		Tag readHeaderTag;
		fread( &readHeaderTag, sizeof( int ), 1, file );

		for( auto taggedIntProperty : intProperties )
		{
			Tag readPropertyTag;
			//	Write the tag
			fread( &readPropertyTag, sizeof( Tag ), 1, file );
			//	Write the property
			fwrite( &(taggedIntProperty.second), sizeof( int ), 1, file );
		}

		for( int i = 0; i < numStores; i++ )
		{
			//	Write the tag
			fwrite( &(taggedData[i].first), sizeof( Tag ), 1, file );
			auto data = taggedData[i].second;
			int numDims = dataDims[i];
			int numData = dataCounts[i];
			//	Write the data dimensions
			fwrite( &numDims, sizeof( int ), 1, file );
			//	Write the num data
			fwrite( &numData, sizeof( int ), 1, file );
			//	Write the data
			fwrite( data, sizeof( double ), numData * numDims, file );
		}

		fclose( file );
	}

	void SaveTo( const std::string & filePath )
	{
		if( !validate( ) )
			NOT_YET_IMPLEMENTED( );

		FILE * file;
		file = fopen( filePath.c_str( ), "wb" );
		if( !file )
			NOT_YET_IMPLEMENTED( );

		fwrite( &headerTag, sizeof( int ), 1, file );

		for( auto taggedIntProperty : intProperties )
		{
			//	Write the tag
			fwrite( &(taggedIntProperty.first), sizeof( Tag ), 1, file );
			//	Write the property
			fwrite( &(taggedIntProperty.second), sizeof( int ), 1, file );
		}

		for( int i = 0; i < numStores; i++ )
		{
			//	Write the tag
			fwrite( &(taggedData[i].first), sizeof( Tag ), 1, file );
			auto data = taggedData[i].second;
			int numDims = dataDims[i];
			int numData = dataCounts[i];
			//	Write the data dimensions
			fwrite( &numDims, sizeof( int ), 1, file );
			//	Write the num data
			fwrite( &numData, sizeof( int ), 1, file );
			//	Write the data
			fwrite( data, sizeof( double ), numData * numDims, file );
		}

		fclose( file );
	}
};
