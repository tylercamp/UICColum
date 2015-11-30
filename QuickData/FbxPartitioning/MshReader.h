#pragma once

#include <string>
#include <fstream>
#include <vector>

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
	int point_indices[MAX_FACE_ELEMENTS];
	bool is_inner;
};

struct MshPointData
{
	std::size_t count;
	std::vector<MshPoint> data;
};

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



	int ParseInt( const std::string & str )
	{
		return std::stoul( str );
	}

	int ParseHexInt( const std::string & str )
	{
		return std::stoul( str, nullptr, 16 );
	}

	float ParseFloat( const std::string & str )
	{
		double result;
		sscanf_s( str.c_str( ), "%lf", &result );
		return result;
	}



	std::vector<std::string> GenerateLines( const char * fileData, int length )
	{
		auto start = clock( );

		std::vector<std::string> result;
		result.reserve( length / 3 ); // heuristic

		int startIndex = 0;
		for( std::size_t i = 0; i < length; i++ )
		{
			if( fileData[i] != '\r' && fileData[i] != '\n' )
				continue;

			std::string line;
			line.assign( fileData + startIndex, fileData + i );
			result.emplace_back( line );
			startIndex = i + 1;
		}

		result.reserve( result.size( ) );
		return result;
	}

public:

	std::vector<MshPointData> PointData;
	std::vector<MshFaceData> FaceData;

	MshReader( const std::string & filename )
	{
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

	void Parse( const char * fileData, int length )
	{
		if( IsLoaded( ) )
			__debugbreak( );

		parseMode = None;
		std::cout << "Preprocessing data... ";
		auto lines = GenerateLines( fileData, length );
		std::cout << "done." << std::endl;

		for( std::size_t line = 0; line < lines.size( ); line++ )
		{
			//auto lineStart = lines[line];
			auto lineText = lines[line];

			switch( parseMode )
			{

			case(Points) :
			{
				if( lineText[0] == ')' )
				{
					std::cout << "done." << std::endl;
					PointData.back( ).count = PointData.back( ).data.size( );
					parseMode = None;
					continue;
				}

				int startIndex = -1;
				int numData = 0;
				MshPoint pt;
				for( int i = 0; i < lineText.size( ); i++ )
				{
					if( startIndex > 0 )
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
				PointData.back( ).data.emplace_back( pt );
				break;
			}

			case(Faces) :
			{
				if( lineText[0] == ')' )
				{
					std::cout << "done." << std::endl;
					FaceData.back( ).count = FaceData.back( ).data.size( );
					parseMode = None;
					continue;
				}

				int startIndex = -1;
				int numData = 0;
				MshFace face;
				for( int i = 0; i < lineText.size( ); i++ )
				{
					if( startIndex > -1 )
					{
						if( lineText[i] == ' ' )
						{
							std::string dataChunk = lineText.substr( startIndex, i - startIndex );
							if( numData == 0 )
							{
								face.type = (MshFaceType)ParseInt( dataChunk );
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

				face.is_inner = (face.point_indices[numData - 2] != 0);
				FaceData.back( ).data.emplace_back( face );

				break;
			}

			case(None) :
			{
				if( lineText[0] != '(' )
					continue;

				if( lineText[lineText.size( ) - 1] != '(' )
					continue;

				int listType = ParseInt( lineText.substr( 1, lineText.find( ' ' ) ) );
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

				default:
					__debugbreak( );
				}

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