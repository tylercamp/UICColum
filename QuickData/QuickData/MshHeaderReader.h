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

	//	Returns offset into array
	int ParseNextList( const std::vector<const char *> & fileLines, int baseIndex )
	{
		//	# of volumes (declared in-file)
		int numVolumes = -1;

		std::string as_str;
		as_str.reserve( 20 );

		//	Find volume data start
		int dataStartIndex;
		for( dataStartIndex = baseIndex; dataStartIndex < fileLines.size( ) - 1; dataStartIndex++ )
		{
			int i = dataStartIndex;

			as_str.assign( fileLines[i], fileLines[i + 1] - fileLines[i] - 1 );

			//	Looking for something like '(12(...)('
			if( as_str.find( "(12" ) == 0 && as_str.back( ) == '(' )
			{
				//	Extract data bounds from data list declaration (get numVolumes)

				//	# of volumes held in inner parentheses, after '(12('
				int innerParenthesesStart = 4;
				int innerParenthesesEnd = as_str.find_first_of( ')' );

				std::vector<std::string> dataDef;
				SplitSpaces( &dataDef, as_str.substr( innerParenthesesStart, innerParenthesesEnd - innerParenthesesStart ) );

				int firstVolumeIndex = ParseHexInt( dataDef[1] );
				int lastVolumeIndex = ParseHexInt( dataDef[2] );

				numVolumes = lastVolumeIndex - firstVolumeIndex + 1;

				dataStartIndex = i + 1;
				break;
			}
		}

		if( dataStartIndex < 0 || numVolumes < 0 )
			//	Couldn't find any start point, no lines read
			return 0;

		std::vector<double> result;
		result.reserve( numVolumes );

		int lastLine = dataStartIndex;
		//	Gather data until we reach a closing parenthesis
		for( int i = dataStartIndex; i < fileLines.size( ) - 1; i++ )
		{
			if( *(fileLines[i]) == ')' )
			{
				lastLine = i;
				break;
			}

			result.push_back( strtod( fileLines[i], nullptr ) );
		}

		//	If we completed a full list
		if( lastLine != dataStartIndex )
		{
			if( result.size( ) != numVolumes )
				std::cout << "Warning: Volume count mismatch in frame " << data.size( ) + 1 << ", expected " << numVolumes << ", got " << result.size( ) << std::endl;
			data.emplace_back( std::move( result ) );
			std::cout << "Loaded store " << data.size( ) << std::endl;
		}
		else
		{
			//	Should only happen if we reach the end of a file before finishing a list
			return 0;
		}

		return lastLine - baseIndex;
	}

public:
	std::vector<std::vector<double>> data;

	MshHeaderReader( )
	{

	}

	MshHeaderReader( const std::string & filepath )
	{
		Load( filepath );
	}

	//	Returns num bytes read
	std::int64_t LoadPart( const std::string & filepath, std::int64_t readOffset, std::int64_t maxBuffer )
	{
		auto filesize = getTextFileSize( filepath );
		auto fileavail = min( filesize - readOffset, maxBuffer );

		FILE * file;
		fopen_s( &file, filepath.c_str( ), "r" );
		_fseeki64( file, readOffset, SEEK_SET );

		std::cout << "Loading file... ";
		char * fileData = new char[fileavail + 1];
		fread( fileData, 1, fileavail, file );
		fclose( file );
		fileData[fileavail] = 0;
		std::cout << "Done." << std::endl;

		std::cout << "Preprocessing data... ";
		std::vector<const char *> lines;
		SplitLines( &lines, fileData, fileavail );
		std::cout << "Done." << std::endl;

		int lineOffset = 0;
		while( lineOffset < lines.size( ) - 2 )
		{
			std::string as_str( lines[lineOffset], lines[lineOffset + 1] - lines[lineOffset] - 1 );
			//	Ignore empty lines
			if( removeWhitespace( as_str ).size( ) == 0 )
			{
				++lineOffset;
				continue;
			}

			std::size_t lastOffset = lineOffset;
			bool success = false;
			try
			{
				int numLines = ParseNextList( lines, lineOffset );
				if( numLines == 0 )
					break;

				lineOffset += numLines;
				success = true;
			}
			catch( ... )
			{
				break;
			}

			if( success )
			{
				++lineOffset; // finished last line, move to next
			}
		}

		std::size_t byteOffset = lines[lineOffset + 1] - fileData + lineOffset + 1; // add lineOffset to account for extra carriage return characters

		std::cout << "Done loading stores." << std::endl;
		std::cout << "Freeing memory... ";

		delete[] fileData;
		lines.clear( );
		lines.reserve( 1 );

		std::cout << "Done." << std::endl;

		return byteOffset;
	}

	void Load( const std::string & filepath )
	{
		auto filesize = getTextFileSize( filepath );

		FILE * file;
		fopen_s( &file, filepath.c_str( ), "r" );
		
		std::cout << "Loading file... ";
		char * fileData = new char[filesize];
		fread( fileData, 1, filesize, file );
		fclose( file );
		std::cout << "Done." << std::endl;

		std::cout << "Preprocessing data... ";
		std::vector<const char *> lines;
		SplitLines( &lines, fileData, filesize );
		std::cout << "Done." << std::endl;

		int offset = 0;
		while( offset < lines.size( ) - 1 )
		{
			std::string as_str( lines[offset], lines[offset + 1] - lines[offset] - 1 );

			//	Ignore empty lines
			if( removeWhitespace( as_str ).size( ) == 0 )
			{
				++offset;
				continue;
			}

			offset += ParseNextList( lines, offset );
		}

		delete[] fileData;

		fclose( file );
	}
};