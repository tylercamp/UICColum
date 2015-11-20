
#include "QuickData.Unity.h"
#include <Windows.h>

MeshDataset * LoadMeshSet( const char * searchPath, const char * searchString )
{
	MeshDataset * result = new MeshDataset( );

	WIN32_FIND_DATAA ffd;
	LARGE_INTEGER filesize;
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	std::string inputPath( searchPath );
	if( inputPath.back( ) != '\\' && inputPath.back( ) != '/' )
		inputPath += '\\';
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
			ParsedMeshStructure * mesh;
			mesh = LoadFbxMesh( foundPath.c_str( ) );
			result->meshes.push_back( mesh );

			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			printf( "  %s   %ld bytes\n", ffd.cFileName, filesize.QuadPart );
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	dwError = GetLastError( );
	if( dwError != ERROR_NO_MORE_FILES )
		NOT_YET_IMPLEMENTED( );

	FindClose( hFind );
	
	return result;
}

void FreeMesh( ParsedMeshStructure * mesh )
{
	if( mesh->colorData ) delete[] mesh->colorData;
	if( mesh->normalData ) delete[] mesh->normalData;
	if( mesh->vertexData ) delete[] mesh->vertexData;

	delete mesh;
}

UNITY_NATIVE_EXPORT void FreeMeshSet( MeshDataset * dataset )
{
	NOT_YET_IMPLEMENTED( );
}

UNITY_NATIVE_EXPORT int MeshTrisCount( ParsedMeshStructure * mesh )
{
	return mesh->numTris;
}

UNITY_NATIVE_EXPORT void MeshGetVertices( ParsedMeshStructure * mesh, float * targetBuffer, int bufferSize )
{
	for( int i = 0; i < bufferSize && i < mesh->numTris * 3; i++ )
		targetBuffer[i] = mesh->vertexData[i];
}

UNITY_NATIVE_EXPORT void MeshGetNormals( ParsedMeshStructure * mesh, float * targetBuffer, int bufferSize )
{
	for( int i = 0; i < bufferSize && i < mesh->numTris * 3; i++ )
		targetBuffer[i] = mesh->normalData[i];
}
