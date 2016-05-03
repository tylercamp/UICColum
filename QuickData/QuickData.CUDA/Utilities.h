
#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <vector>

#include <string>
#include <iostream>
#include <sstream>

#include "cuda_runtime.h"

#define NOT_YET_IMPLEMENTED() __debugbreak()




/* Types hacked in */

struct triangle
{
	//	a, b, c, are the 3 vertices defining the triangle

	//	Use 3-dimensional floats to represent each point as X, Y, Z
	float3 a, b, c;

	//	For each vertex, also associate a normal perpendicular to the face (value is currently the same for all 3)
	float3 norm_a, norm_b, norm_c;

	//	Index of the volume associated with this triangle. If there is no volume associated, this is -1.
	int volumeIndex;

	triangle( ) : volumeIndex( -1 )
	{
	}

	__declspec(property(get = get_center)) float3 center;
	float3 get_center( )
	{
		float3 result;
		result.x = a.x + b.x + c.x;
		result.y = a.y + b.y + c.y;
		result.z = a.z + b.z + c.z;
		result.x *= 0.33333f;
		result.y *= 0.33333f;
		result.z *= 0.33333f;
		return result;
	}

	triangle & operator=(const triangle & nt)
	{
		a = nt.a;
		b = nt.b;
		c = nt.c;
		norm_a = nt.norm_a;
		norm_b = nt.norm_b;
		norm_c = nt.norm_c;

		return *this;
	}
};

struct mesh_partition_descriptor
{
	float3 bounds_start, bounds_end;

	bool contains_point( float3 p ) const
	{
		return
			bounds_start.x <= p.x &&
			bounds_start.y <= p.y &&
			bounds_start.z <= p.z &&
			bounds_end.x >= p.x &&
			bounds_end.y >= p.y &&
			bounds_end.z >= p.z;
	}
};

struct mesh_chunk
{
	int num_tris;
	std::vector<triangle> tris;
	mesh_partition_descriptor bounds;
};

struct voxel
{
	float3 start, end;

	inline voxel & operator=(const voxel & o)
	{
		start = o.start;
		end = o.end;
		return *this;
	}

	inline bool contains_point( float3 point )
	{
		return
			start.x <= point.x && start.y <= point.y && start.z <= point.z &&
			end.x >= point.x && end.y >= point.y && end.z >= point.z;
	}
};










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

std::uint64_t getFileSize( std::string name )
{
	//	http://stackoverflow.com/questions/8991192/check-filesize-without-opening-file-in-c
	HANDLE hFile = CreateFileA( name.c_str( ), GENERIC_READ,
							   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return (std::uint64_t)-1; // error condition, could call GetLastError to find out more

	LARGE_INTEGER size;
	if( !GetFileSizeEx( hFile, &size ) )
	{
		CloseHandle( hFile );
		return (std::uint64_t) - 1; // error condition, could call GetLastError to find out more
	}

	CloseHandle( hFile );
	return size.QuadPart;
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
	for( std::size_t i = 0; i < s.size( ); i++ )
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

	for( std::size_t i = 0; i < s.size( ); i++ )
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
	HANDLE hFind = INVALID_HANDLE_VALUE;

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

float3 cross( float3 a, float3 b )
{
	return make_float3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
		);
}

float dot( float3 a, float3 b )
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float mag2( float3 v )
{
	return dot( v, v );
}

float mag( float3 v )
{
	return sqrt( mag2( v ) );
}

float3 norm( float3 v )
{
	float invMag = 1.0f / mag( v );
	return make_float3(
		invMag * v.x,
		invMag * v.y,
		invMag * v.z
		);
}




#define VECOP inline __host__ __device__

VECOP float3 operator-(const float3 & lhs, const float3 & rhs)
{
	return make_float3( lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z );
}

VECOP float3 operator+(const float3 & lhs, const float3 & rhs)
{
	return make_float3( lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z );
}

VECOP float3 operator/(const float3 & lhs, const float3 & rhs)
{
	return make_float3( lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z );
}

VECOP float3 operator*(const float3 & lhs, const float3 & rhs)
{
	return make_float3( lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z );
}



VECOP float3 operator*(const float3 & lhs, const int3 & rhs)
{
	return make_float3( lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z );
}

VECOP float3 operator/(const float3 & lhs, const int3 & rhs)
{
	return make_float3( lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z );
}



VECOP float3 operator*(const float3 & lhs, const int & rhs)
{
	return make_float3( lhs.x * rhs, lhs.y * rhs, lhs.z * rhs );
}

VECOP float3 operator*(const float3 & lhs, const float & rhs)
{
	return make_float3( lhs.x * rhs, lhs.y * rhs, lhs.z * rhs );
}





VECOP float3 make_float3( const float & f )
{
	return make_float3( f, f, f );
}