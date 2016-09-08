#pragma once

#include <amp_graphics.h>

#include <string>
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <functional>
#include <cctype>
#include <cstdint>

#include "Types.h"

#ifdef max
# undef max
#endif
#ifdef min
# undef min
#endif

/* Utility functions */

template <typename L, typename R>
inline L max( L left, R right ) restrict( cpu, amp )
{
	return left > right ? left : right;
}

using concurrency::graphics::float_3;
template <>
inline float_3 max<float_3, float_3>( float_3 left, float_3 right ) restrict( cpu, amp )
{
	return float_3(
		max( left.x, right.x ),
		max( left.y, right.y ),
		max( left.z, right.z )
	);
}

using concurrency::graphics::int_3;
template <>
inline int_3 max<int_3, int_3>( int_3 left, int_3 right ) restrict( cpu, amp )
{
	return int_3(
		max( left.x, right.x ),
		max( left.y, right.y ),
		max( left.z, right.z )
		);
}

template <typename L, typename R>
inline L min( L left, R right ) restrict( amp )
{
	return left < right ? left : right;
}

template <typename L, typename R>
inline L min( L left, R right )
{
	return left < right ? left : right;
}

template <>
inline float_3 min<float_3, float_3>( float_3 left, float_3 right )
{
	return float_3(
		min( left.x, right.x ),
		min( left.y, right.y ),
		min( left.z, right.z )
		);
}

template <>
inline int_3 min<int_3, int_3>( int_3 left, int_3 right )
{
	return int_3(
		min( left.x, right.x ),
		min( left.y, right.y ),
		min( left.z, right.z )
		);
}

template <>
inline float_3 min<float_3, float_3>( float_3 left, float_3 right ) restrict( amp )
{
	return float_3(
		min( left.x, right.x ),
		min( left.y, right.y ),
		min( left.z, right.z )
		);
}

template <>
inline int_3 min<int_3, int_3>( int_3 left, int_3 right ) restrict( amp )
{
	return int_3(
		min( left.x, right.x ),
		min( left.y, right.y ),
		min( left.z, right.z )
		);
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

bool fileExists( const std::string & filename )
{
	FILE * f = fopen( filename.c_str( ), "rb" );
	bool exists = (f != nullptr);
	if( exists )
		fclose( f );
	return exists;
}

std::uint64_t getFileSize( std::string name )
{
	//	http://stackoverflow.com/questions/8991192/check-filesize-without-opening-file-in-c
	HANDLE hFile = CreateFileA( name.c_str( ), GENERIC_READ,
							   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return -1; // error condition, could call GetLastError to find out more

	LARGE_INTEGER size;
	if( !GetFileSizeEx( hFile, &size ) )
	{
		CloseHandle( hFile );
		return -1; // error condition, could call GetLastError to find out more
	}

	CloseHandle( hFile );
	return size.QuadPart;
}

std::uint64_t getTextFileSize( std::string name )
{
	//	Getting the size of the text buffer for the file
	FILE * f = fopen( name.c_str( ), "r" );
	_fseeki64( f, 0, SEEK_END );
	std::uint64_t size = _ftelli64( f );
	fclose( f );

	return size;
}

bool directoryExists( const std::string & path )
{
	auto cpath = path.back( ) == '/' || path.back( ) == '\\' ? path : path + '/';
	FILE * testfile = fopen( ( cpath + "__testfile" ).c_str( ), "w" );

	bool exists = ( testfile != nullptr );
	if( testfile )
		fclose( testfile );

	DeleteFileA( (cpath + "__testfile").c_str( ) );

	return exists;
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

template <typename T>
std::string toString( const T & v )
{
	std::ostringstream os;
	os << v;
	return os.str( );
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



//	http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start
static inline std::string &ltrim( std::string &s ) {
	s.erase( s.begin( ), std::find_if( s.begin( ), s.end( ), std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );
	return s;
}

// trim from end
static inline std::string &rtrim( std::string &s ) {
	s.erase( std::find_if( s.rbegin( ), s.rend( ), std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), s.end( ) );
	return s;
}

// trim from both ends
static inline std::string &trim( std::string &s ) {
	return ltrim( rtrim( s ) );
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
