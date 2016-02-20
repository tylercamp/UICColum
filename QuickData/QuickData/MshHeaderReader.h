#pragma once

#include <vector>
#include <string>

//	Minimal reader for parsing data out of a MSH header .mm file

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

		std::stringstream ss( fileData );

		std::vector<std::string> & result = *output;
		result.reserve( length / 3 ); // heuristic

		std::string line;
		while( std::getline( ss, line, '\n' ) )
			result.push_back( line );

		result.reserve( result.size( ) );
	}

	//	Returns offset into array
	int ParseNextList( const std::vector<std::string> & fileLines, int baseIndex )
	{
		//	# of volumes (declared in-file)
		int numVolumes = -1;

		//	Find volume data start
		int dataStartIndex;
		for( dataStartIndex = baseIndex; dataStartIndex < fileLines.size( ); dataStartIndex++ )
		{
			int i = dataStartIndex;

			//	Looking for something like '(12(...)('
			if( fileLines[i].find( "(12" ) == 0 && fileLines[i].back( ) == '(' )
			{
				//	Extract data bounds from data list declaration (get numVolumes)

				std::string line = fileLines[i];
				//	# of volumes held in inner parentheses, after '(12('
				int innerParenthesesStart = 4;
				int innerParenthesesEnd = line.find_first_of( ')' );

				std::vector<std::string> dataDef;
				SplitSpaces( &dataDef, line.substr( innerParenthesesStart, innerParenthesesEnd - innerParenthesesStart ) );

				int firstVolumeIndex = ParseHexInt( dataDef[1] );
				int lastVolumeIndex = ParseHexInt( dataDef[2] );

				numVolumes = lastVolumeIndex - firstVolumeIndex + 1;

				dataStartIndex = i + 1;
				break;
			}
		}

		if( dataStartIndex < 0 )
			//	Invalid file (or just new format?)
			return 0;

		//	No more data
		if( numVolumes < 0 )
			return fileLines.size( ) - baseIndex;

		std::vector<double> result;
		result.resize( numVolumes );

		int lastLine = dataStartIndex;
		//	Gather data until we reach a closing parenthesis
		for( int i = dataStartIndex; i < fileLines.size( ); i++ )
		{
			const auto & line = fileLines[i];
			if( line[0] == ')' )
			{
				lastLine = i;
				break;
			}

			result[i - dataStartIndex] = ParseFloat( line );
		}

		if( result.size( ) > 0 )
		{
			data.emplace_back( std::move( result ) );
			std::cout << "Loaded store " << data.size( ) << std::endl;
		}

		return lastLine - baseIndex;
	}

public:
	std::vector<std::vector<double>> data;

	MshHeaderReader( const std::string & filepath )
	{
		Load( filepath );
	}

	void Load( const std::string & filepath )
	{
		auto filesize = getFileSize( filepath );;

		FILE * file;
		fopen_s( &file, filepath.c_str( ), "r" );
		
		std::cout << "Loading file... ";
		char * fileData = new char[filesize];
		fread( fileData, 1, filesize, file );
		fclose( file );
		std::cout << "Done." << std::endl;

		std::cout << "Preprocessing data... ";
		std::vector<std::string> lines;
		SplitLines( &lines, fileData, filesize );
		std::cout << "Done." << std::endl;

		int offset = 0;
		while( offset < lines.size( ) )
		{
			//	Ignore empty lines
			if( removeWhitespace( lines[offset] ).size( ) == 0 )
			{
				++offset;
				continue;
			}

			offset += ParseNextList( lines, offset );
		}
	}
};