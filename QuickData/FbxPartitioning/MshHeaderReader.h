#pragma once

#include <vector>
#include <string>

class MshHeaderReader
{
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




	void SplitSpaces( std::vector<std::string> * output, const std::string & text )
	{
		auto start = clock( );

		std::vector<std::string> & result = *output;

		int startIndex = 0;
		for( std::size_t i = 0; i < text.length( ); i++ )
		{
			if( text[i] != ' ' )
				continue;

			std::string line;
			line.assign( text.data( ) + startIndex, text.data( ) + i );
			result.emplace_back( line );
			startIndex = i + 1;
		}

		result.reserve( result.size( ) );
	}

	void SplitLines( std::vector<std::string> * output, const char * fileData, int length )
	{
		auto start = clock( );

		std::vector<std::string> & result = *output;
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
	}

	void Parse( const char * fileData, std::size_t length )
	{
		std::vector<std::string> lines;
		SplitLines( &lines, fileData, length );

		//	# of data to load (declared in-file)
		int numData = -1;

		//	Find volume data start
		int dataStartIndex = -1;
		for( int i = 0; i < lines.size( ); i++ )
		{
			//	Looking for something like '(12(...)('
			if( lines[i].find( "(12" ) == 0 && lines[i].back( ) == '(' )
			{
				//	Extract data bounds from data list declaration (load numData)

				std::string line = lines[i];
				//	# of volumes held in inner parentheses, after '(12('
				int innerParenthesesStart = 4;
				int innerParenthesesEnd = line.find_first_of( ')' );

				std::vector<std::string> dataDef;
				SplitSpaces( &dataDef, line.substr( innerParenthesesStart, innerParenthesesEnd - innerParenthesesStart ) );

				int firstVolumeIndex = ParseHexInt( dataDef[1] );
				int lastVolumeIndex = ParseHexInt( dataDef[2] );

				numData = lastVolumeIndex - firstVolumeIndex + 1;

				dataStartIndex = i + 1;
				break;
			}
		}

		if( dataStartIndex < 0 )
			//	Invalid file (or just new format?)
			return;

		data.resize( numData );

		//	Gather data until we reach a closing parenthesis
		for( int i = dataStartIndex; i < lines.size( ); i++ )
		{
			const auto & line = lines[i];
			if( line[0] == ')' )
				break;

			data[i - dataStartIndex] = ParseFloat( line );
		}
	}

public:
	std::vector<float> data;

	MshHeaderReader( const std::string & filepath )
	{
		Load( filepath );
	}

	void Load( const std::string & filepath )
	{
		FILE * file;
		fopen_s( &file, filepath.c_str( ), "r" );
		std::size_t filesize;
		fseek( file, 0, SEEK_END );
		filesize = ftell( file );
		fseek( file, 0, SEEK_SET );

		char * fileData = new char[filesize];
		fread( fileData, 1, filesize, file );
		fclose( file );

		Parse( fileData, filesize );
	}
};