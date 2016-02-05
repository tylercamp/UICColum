#pragma once

#include <string>
#include <iostream>
#include <sstream>

/* Utility functions */

template <typename l, typename r>
inline l max( l left, r right )
{
	return left > right ? left : right;
}

template <typename l, typename r>
inline l min( l left, r right )
{
	return left < right ? left : right;
}

std::string formatDataSize( long long numBytes )
{
	const int KB = 1024;
	const int MB = 1024 * KB;
	const int GB = 1024 * MB;

	std::string label;
	float count;

	if( numBytes > GB )
	{
		label = "GB";
		count = numBytes / (float) GB;
	}
	else if( numBytes > MB )
	{
		label = "MB";
		count = numBytes / (float) MB;
	}
	else if( numBytes > KB )
	{
		label = "KB";
		count = numBytes / (float) KB;
	}
	else
	{
		label = "B";
		count = (float) numBytes;
	}

	int count_spec = (int) ( count * 100 );

	std::ostringstream result;
	result << ( count_spec / 100.0f ) << label;
	return result.str( );
}

std::string formatTime( long ms )
{
	const int duration_seconds = 1000;
	const int duration_minutes = duration_seconds * 60;
	const int duration_hours = duration_minutes * 60;

	std::string label;
	float count;

	if( ms > duration_hours )
	{
		label = "hrs";
		count = ms / (float) duration_hours;
	}
	else if( ms > duration_minutes )
	{
		label = "mins";
		count = ms / (float) duration_minutes;
	}
	else if( ms > duration_seconds )
	{
		label = "s";
		count = ms / (float) duration_seconds;
	}
	else
	{
		label = "ms";
		count = (float) ms;
	}

	int count_spec = (int) ( count * 100 );

	std::ostringstream result;
	result << ( count_spec / 100.0f ) << label;
	return result.str( );
}

std::string getStoragePath( const std::string & referencePath )
{
	std::string fpath = referencePath;
	while( fpath.find( '\\' ) != fpath.npos )
		fpath = fpath.replace( fpath.find( '\\' ), 1, "/" );

	int extPos = referencePath.find_last_of( '.' );
	if( extPos < 0 )
		extPos = fpath.size( );

	return fpath.substr( 0, extPos );
}

std::string toLower( const std::string & s )
{
	std::string result;
	for( int i = 0; i < s.size( ); i++ )
		result += tolower( s[i] );
	return result;
}

std::string getFileExtension( const std::string & filepath )
{
	std::string fpath = filepath;
	while( fpath.find( '\\' ) != fpath.npos )
		fpath = fpath.replace( fpath.find( '\\' ), 1, "/" );

	int extPos = fpath.find_last_of( '.' );
	if( extPos < 0 )
		return "";

	return fpath.substr( extPos + 1 );
}

std::string getFileName( const std::string & filepath )
{
	std::string fpath = filepath;
	while( fpath.find( '\\' ) != fpath.npos )
		fpath = fpath.replace( fpath.find( '\\' ), 1, "/" );

	int lastSlashPos = fpath.find_last_of( '/' );
	if( lastSlashPos >= 0 )
		fpath = fpath.substr( lastSlashPos + 1 );

	int extPos = fpath.find_last_of( '.' );
	if( extPos >= 0 )
		fpath = fpath.substr( 0, extPos );

	return fpath;
}

std::string removeWhitespace( const std::string & s )
{
	std::string result;

	for( int i = 0; i < s.size( ); i++ )
	{
		char c = s[i];
		if( c == ' ' || c == '\n' || c == '\r' || c == '\t' )
			continue;

		result += c;
	}
	return result;
}

bool contains( const std::string & str, const std::string & text )
{
	return str.find( text, 0 ) != str.npos;
}

std::vector<std::string> findFiles( const std::string & fileSearchPath, const std::string & searchString = "" )
{
	std::vector<std::string> result;

	std::string searchPath = fileSearchPath;

	WIN32_FIND_DATAA ffd;
	LARGE_INTEGER filesize;
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	std::string inputPath( searchPath );
	if( inputPath.back( ) != '\\' && inputPath.back( ) != '/' )
	{
		inputPath += '\\';
		searchPath += '\\';
	}
	inputPath += searchString;


	hFind = FindFirstFileA( inputPath.c_str( ), &ffd );

	if( INVALID_HANDLE_VALUE == hFind )
		NOT_YET_IMPLEMENTED( );

	// List all the files in the directory with some info about them.

	do
	{
		if( !( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			std::string foundPath = std::string( searchPath ) + ffd.cFileName;
			result.push_back( foundPath );
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	return result;
}

void pause( )
{
	std::string str;
	std::getline( std::cin, str );
}

float_3 cross( float_3 a, float_3 b ) restrict( amp )
{
	return float_3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
		);
}

float dot( float_3 a, float_3 b ) restrict( amp )
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float mag2( float_3 v ) restrict( amp )
{
	return dot( v, v );
}

float_3 mag( float_3 v ) restrict( amp )
{
	return concurrency::fast_math::sqrtf( mag2( v ) );
}

float_3 norm( float_3 v ) restrict( amp )
{
	return v / mag( v );
}
