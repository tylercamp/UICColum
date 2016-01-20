#pragma once

#include <fstream>
#include "Types.h"



const int MESH_FILE_START_TAG = 0x2345; // Verifies file format
const int TRIS_COUNT_TAG = 0x0001; // Abbreviation of triangle list into a single tris count (indexing is then linear)
const int DATASTORES_COUNT_TAG = 0x0002; // # of data stores attached to this mesh

//	Data buffer start tags
const int COLORS_START_TAG = 0x0010;
const int VERTICES_START_TAG = 0x0020;
const int NORMALS_START_TAG = 0x0030;
const int VOLUMES_START_TAG = 0x0040;



//	Represents an unoptimized mesh that can be loaded from/saved to disk.
class BinaryMesh
{
	void LoadBufferFrom( std::istream & source, float_3 ** targetBuffer, std::size_t count )
	{
		*targetBuffer = new float_3[count];

		for( int i = 0; i < count; i++ )
		{
			float x, y, z;
			source >> x;
			source >> y;
			source >> z;

			(*targetBuffer)[i] = float_3( x, y, z );
		}
	}

	void WriteBufferTo( std::ostream & target, float_3 * buffer )
	{
		for( int i = 0; i < numTris * 3; i++ )
		{
			const auto & v = buffer[i];
			target << v.x;
			target << v.y;
			target << v.z;
		}
	}

	void LoadBufferFrom( FILE * source, float_3 ** targetBuffer, std::size_t count )
	{
		*targetBuffer = new float_3[count];
		
		fread( *targetBuffer, sizeof( float_3 ), count, source );

		/*
		for( int i = 0; i < count; i++ )
		{
			float x, y, z;
			fread( &x, 4, 1, source );
			fread( &y, 4, 1, source );
			fread( &z, 4, 1, source );

			( *targetBuffer )[i] = float_3( x, y, z );
		}
		*/
	}

	void LoadBufferFrom( FILE * source, int ** targetBuffer, std::size_t count )
	{
		*targetBuffer = new int[count];

		fread( *targetBuffer, sizeof( int ), count, source );
	}

	void LoadBufferFrom( FILE * source, float ** targetBuffer, std::size_t count )
	{
		*targetBuffer = new float[count];

		fread( *targetBuffer, sizeof( float ), count, source );
	}

	void WriteBufferTo( FILE * target, float_3 * buffer )
	{
		fwrite( buffer, sizeof( float_3 ), numTris * 3, target );
	}

	void WriteBufferTo( FILE * target, int * buffer )
	{
		fwrite( buffer, sizeof( int ), numTris * 3, target );
	}

	void WriteBufferTo( FILE * target, float * buffer )
	{
		fwrite( buffer, sizeof( float ), numTris * 3, target );
	}

public:
	float_3 * colors, * vertices, * normals;
	int * volumes;
	std::size_t numTris;

	BinaryMesh( )
	{
		numTris = -1;

		vertices = normals = colors = nullptr;
		volumes = nullptr;
	}

	BinaryMesh( const std::string & filePath )
	{
		LoadFrom( filePath );
	}

	~BinaryMesh( )
	{
		if( colors ) delete[] colors;
		if( vertices ) delete[] vertices;
		if( normals ) delete[] normals;
		if( volumes ) delete[] volumes;
	}



	void LoadFrom( const std::string & filePath )
	{
		FILE * file;
		if( fopen_s( &file, filePath.c_str( ), "rb" ) != 0 )
			return;

		//if( !in.is_open( ) || in.bad( ) )
		//	throw std::exception( "Could not find file" );
		fseek( file, 0, SEEK_END );
		int fileSize = ftell( file );
		fseek( file, 0, SEEK_SET );

		int startTag;
		fread( &startTag, 4, 1, file );
		if( startTag != MESH_FILE_START_TAG )
			throw std::exception( "Invalid binary mesh file." );

		//	FileStartTag should be immediately followed by TrisCountTag
		int trisCountTag;
		fread( &trisCountTag, 4, 1, file );
		if( trisCountTag != TRIS_COUNT_TAG )
			throw std::exception( "Invalidly-formatted binary mesh file." );

		fread( &numTris, 4, 1, file );

		//	And then DataStoresCountTag (Deprecated)
		int dataStoresCountTag;
		fread( &dataStoresCountTag, 4, 1, file );
		if( dataStoresCountTag == DATASTORES_COUNT_TAG )
			fread( &dataStoresCountTag, 4, 1, file ); // stores tag followed by count
		else
			fseek( file, -4, SEEK_CUR );


		
		while( ftell( file ) != fileSize )
		{
			int currentDataTag;
			fread( &currentDataTag, sizeof( int ), 1, file );
			switch( currentDataTag )
			{
			case(VERTICES_START_TAG) :
				LoadBufferFrom( file, &vertices, numTris * 3 );
				break;
			case(NORMALS_START_TAG) :
				LoadBufferFrom( file, &normals, numTris * 3 );
				break;
			case(COLORS_START_TAG) :
				LoadBufferFrom( file, &colors, numTris * 3 );
				break;
			case(VOLUMES_START_TAG) :
				LoadBufferFrom( file, &volumes, numTris * 3 );
				break;

			default:
				NOT_YET_IMPLEMENTED( );
				break;
			}
		}

		fclose( file );
	}
	
	void SaveTo( const std::string & filePath )
	{
		/*
		//	Validate data consistency
		if( colors.size( ) > 0 && numTris * 3 != colors.size( ) )
			throw std::exception( "" );
		if( vertices.size( ) > 0 && numTris * 3 != vertices.size( ) )
			throw std::exception( "" );
		if( normals.size( ) > 0 && numTris * 3 != normals.size( ) )
			throw std::exception( "" );
			*/

		//static char * bigbuffer = new char[MB(4)];
		FILE * file;
		file = fopen( filePath.c_str( ), "wb" );

		//out.rdbuf( )->pubsetbuf( bigbuffer, MB(4) );
		fwrite( &MESH_FILE_START_TAG, sizeof( int ), 1, file );


		fwrite( &TRIS_COUNT_TAG, sizeof( int ), 1, file );
		fwrite( &numTris, sizeof( int ), 1, file );

		if( colors ) {
			fwrite( &COLORS_START_TAG, sizeof( int ), 1, file );
			WriteBufferTo( file, colors );
		}

		if( vertices ) {
			fwrite( &VERTICES_START_TAG, sizeof( int ), 1, file );
			WriteBufferTo( file, vertices );
		}

		if( normals ) {
			fwrite( &NORMALS_START_TAG, sizeof( int ), 1, file );
			WriteBufferTo( file, normals );
		}

		if( volumes ) {
			fwrite( &VOLUMES_START_TAG, sizeof( int ), 1, file );
			WriteBufferTo( file, volumes );
		}

		fclose( file );
	}
};