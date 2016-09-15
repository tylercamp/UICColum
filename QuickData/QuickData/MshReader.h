#pragma once

#include <string>
#include <fstream>
#include <vector>

#include "Utilities.h"

struct MshPoint
{
	float data[3];
};

enum MshFaceType
{
	MSH_TETRAHEDRON = 3,
	MSH_HEXAHEDRON = 4
};

#define MAX_FACE_ELEMENTS 8

struct MshFace
{
	MshFaceType type;
	std::size_t point_indices[MAX_FACE_ELEMENTS];
	bool is_inner;
};

//	A collection of MshPoint data
struct MshPointData
{
	std::size_t count;
	std::vector<MshPoint> data;
};

//	A collection of MshFace data
struct MshFaceData
{
	std::size_t count;
	std::vector<MshFace> data;
};


class MshReader
{
	//	Internally a state machine
	enum {
		None,
		Points,
		Faces
	} parseMode;



	std::size_t ParseInt( const std::string & str )
	{
		return std::stoull( str );
	}

	std::size_t ParseHexInt( const std::string & str )
	{
		return std::stoull( str, nullptr, 16 );
	}

	float ParseFloat( const std::string & str )
	{
		double result;
		sscanf_s( str.c_str( ), "%lf", &result );
		return result;
	}

	void SplitSpaces( std::vector<std::string> * output, const std::string & text )
	{
		auto start = clock( );

		std::string effectiveText = text;
		std::size_t msPos;
		//	Collapse multi-spaces to single spaces
		while( (msPos = effectiveText.find( "  " )) != effectiveText.npos )
			effectiveText.replace( msPos, 2, " " );

		std::vector<std::string> & result = *output;

		std::size_t startIndex = 0;
		for( std::size_t i = 0; i < effectiveText.length( ); i++ )
		{
			if( effectiveText[i] != ' ' )
				continue;

			std::string line;
			line.assign( effectiveText.data( ) + startIndex, effectiveText.data( ) + i );
			result.emplace_back( std::move( line ) );
			startIndex = i + 1;
		}

		if( startIndex < effectiveText.length( ) )
			result.push_back( effectiveText.substr( startIndex, effectiveText.size( ) - startIndex ) );

		result.reserve( result.size( ) );
	}

	void SplitLines( std::vector<const char *> * output, const char * text, std::size_t length )
	{
		const char * start = text;
		const char * end = strchr( text, '\n' );

		output->reserve( length / 8 );

		std::size_t offset = 0;

		while( end && (end - text < length) )
		{
			output->push_back( start );

			start = end + 1;
			end = strchr( start, '\n' );
		}

		offset = (std::size_t)(start - text);

		output->push_back( start );

		output->reserve( output->size( ) );
	}

public:

	std::vector<MshPointData> PointData;
	std::vector<MshFaceData> FaceData;

	std::size_t VolumeCount;

	MshReader( const std::string & filename )
	{
		VolumeCount = 0;
		Parse( filename );
	}

	void Parse( const std::string & filename )
	{
		char * filedata;
		std::size_t size;

		std::cout << "Loading MSH file... ";
		{
			std::ifstream file( filename );
			if( !file.is_open( ) )
				__debugbreak( );

			file.seekg( 0, file.end );
			size = file.tellg( );
			file.seekg( 0, file.beg );

			filedata = new char[size];
			file.read( filedata, size );
		}
		std::cout << "done." << std::endl;

		Parse( filedata, size );

		std::cout << "Finished basic parse." << std::endl;

		delete filedata;
	}

	void Parse( const char * fileData, std::size_t length )
	{
		if( IsLoaded( ) )
			__debugbreak( );

		parseMode = None;
		std::cout << "Preprocessing data... ";
		std::vector<const char *> lines;
		SplitLines( &lines, fileData, length );
		std::cout << "done." << std::endl;

		std::string as_str;
		as_str.reserve( 20 );

		for( std::size_t line = 0; line < lines.size( ) - 1; line++ )
		{
			//auto lineStart = lines[line];
			as_str.assign( lines[line], lines[line + 1] - lines[line] - 1 );

			switch( parseMode )
			{

			case(Points) :
			{
				if( as_str[0] == ')' )
				{
					std::cout << "done." << std::endl;
					PointData.back( ).count = PointData.back( ).data.size( );
					parseMode = None;
					continue;
				}

				int numData = 0;
				MshPoint pt;

				for( int startIndex = as_str.find_first_not_of(' '); startIndex < as_str.size( ); )
				{
					int spaceIdx = as_str.find( ' ', startIndex );
					if( spaceIdx == as_str.npos )
					{
						pt.data[numData++] = ParseFloat( as_str.substr( startIndex ) );
						startIndex = as_str.size( );
					}
					else
					{
						auto val = as_str.substr( startIndex, spaceIdx - startIndex );
						pt.data[numData++] = ParseFloat( val );
						int nextStart = as_str.find_first_not_of( ' ', spaceIdx );
						startIndex = nextStart != as_str.npos ? nextStart : as_str.size( );
					}
				}

				/*
				for( int i = 0; i < lineText.size( ); i++ )
				{
					if( startIndex > -1 )
					{
						if( lineText[i] == ' ' )
						{
							std::string dataChunk = lineText.substr( startIndex, i - startIndex );
							pt.data[numData++] = ParseFloat( dataChunk );

							startIndex = -1;
						}
					}
					else
					{
						if( lineText[i] == ' ' )
							continue;

						startIndex = i;
					}
				}
				*/

				PointData.back( ).data.emplace_back( pt );
				break;
			}

			case(Faces) :
			{
				if( as_str[0] == ')' )
				{
					std::cout << "done." << std::endl;
					FaceData.back( ).count = FaceData.back( ).data.size( );
					parseMode = None;
					continue;
				}

				int startIndex = -1;
				int numData = 0;
				MshFace face;
				for( std::size_t startIndex = as_str.find_first_not_of( ' ' ); startIndex < as_str.size( ); )
				{
					int spaceIdx = as_str.find( ' ', startIndex );
					if( spaceIdx == as_str.npos )
					{
						auto val = as_str.substr( startIndex );
						face.point_indices[numData++ - 1] = ParseHexInt( val );
						startIndex = as_str.size( );
					}
					else
					{
						auto val = as_str.substr( startIndex, spaceIdx - startIndex );
						std::size_t idx = ParseHexInt( val );

						if( numData == 0 ) {
							face.type = (MshFaceType) idx;
							++numData;
						}
						else {
							face.point_indices[numData++ - 1] = idx;
						}


						int nextStart = as_str.find_first_not_of( ' ', spaceIdx );
						startIndex = nextStart != as_str.npos ? nextStart : as_str.size( );
					}
				}

				/*
				for( std::size_t i = 0; i < lineText.size( ); i++ )
				{
					if( startIndex > -1 )
					{
						if( lineText[i] == ' ' )
						{
							std::string dataChunk = lineText.substr( startIndex, i - startIndex );
							if( numData == 0 )
							{
								face.type = (MshFaceType) ParseInt( dataChunk );
								++numData;
							}
							else
							{
								face.point_indices[numData++ - 1] = ParseHexInt( dataChunk );
							}

							startIndex = -1;
						}
					}
					else
					{
						if( lineText[i] == ' ' )
							continue;

						startIndex = i;
					}
				}
				*/

				face.is_inner = (face.point_indices[numData - 2] != 0); // normally -1 for array size, but -2 for array size and taking into account face type (counted in numData)
				FaceData.back( ).data.emplace_back( face );

				break;
			}

			case(None) :
			{
				const auto nextLine = line + 2 == lines.size( ) ? "" : std::string( lines[line + 1], lines[line + 2] - lines[line + 1] - 1 );

				if( as_str[0] != '(' )
					continue;

				int listType = ParseInt( as_str.substr( 1, as_str.find( ' ' ) ) );
				if( listType == 12 ) // volumes declaration
				{
					std::cout << "Reading volume count...\n";
					std::vector<std::string> list;
					int listStart = as_str.find_last_of( '(' ) + 1;
					int listEnd = as_str.find_first_of( ')' );
					SplitSpaces( &list, as_str.substr( listStart, listEnd - listStart ) );
					VolumeCount = ParseHexInt( list[2] );
					continue;
				}


				int numOpenParen = std::count( as_str.begin( ), as_str.end( ), '(' );
				int numCloseParen = std::count( as_str.begin( ), as_str.end( ), ')' );

				int nextNumOpenParen = std::count( nextLine.begin( ), nextLine.end( ), '(' );
				int nextNumCloseParen = std::count( nextLine.begin( ), nextLine.end( ), ')' );

				if( numOpenParen == numCloseParen )
					continue;

				/*
				(12 (...)
				(
				*/
				bool isNoncontiguousLine = false;
				if( numOpenParen > numCloseParen && nextNumOpenParen > 0 )
				{
					isNoncontiguousLine = true;
				}

				



				switch( listType )
				{
				case(10) :
					std::cout << "Starting Points list... ";
					PointData.push_back( MshPointData( ) );
					parseMode = Points;
					break;

				case(13) :
					std::cout << "Starting Faces list... ";
					FaceData.push_back( MshFaceData( ) );
					parseMode = Faces;
					break;

				//default:
					//__debugbreak( );
				}

				if( isNoncontiguousLine )
					++line;

				break;
			}
			}
		}
	}

	bool IsLoaded( )
	{
		return PointData.size( ) > 0 && FaceData.size( );
	}
};