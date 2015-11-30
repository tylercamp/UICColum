
#include "QuickData.Unity.h"
#include <Windows.h>

static MeshDataset * g_LoadedDataset = nullptr;

bool LoadMeshSet( const char * searchPath, const char * searchString )
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
		return false;

	// List all the files in the directory with some info about them.

	do
	{
		if( !( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			std::string foundPath = std::string( searchPath ) + ffd.cFileName;
			ParsedMeshStructure * mesh;
			if( foundPath.substr( foundPath.find_last_of( '.' ) + 1 ) == "fbx" )
				mesh = LoadFbxMesh( foundPath.c_str( ) );
			else
				mesh = LoadBinaryMesh( foundPath.c_str( ) );

#ifdef _DEBUG
			if( mesh == nullptr )
				__debugbreak( );
#endif
			result->meshes.push_back( mesh );

			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			printf( "  %s   %ld bytes\n", ffd.cFileName, filesize.QuadPart );
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	dwError = GetLastError( );
	if( dwError != ERROR_NO_MORE_FILES )
		return false;

	FindClose( hFind );
	
	g_LoadedDataset = result;
	return true;
}

void FreeMesh( ParsedMeshStructure * mesh )
{
	if( mesh->colorData ) delete[] mesh->colorData;
	if( mesh->normalData ) delete[] mesh->normalData;
	if( mesh->vertexData ) delete[] mesh->vertexData;

	delete mesh;
}

UNITY_NATIVE_EXPORT void FreeMeshSet( )
{
	if( !g_LoadedDataset )
		return;

	for( auto meshData : g_LoadedDataset->meshes )
	{
		FreeMesh( meshData );
	}

	delete g_LoadedDataset;
	g_LoadedDataset = nullptr;
}

UNITY_NATIVE_EXPORT int GetMeshSetCount( )
{
	return g_LoadedDataset->meshes.size( );
}

UNITY_NATIVE_EXPORT int MeshTrisCount( int meshIndex )
{
	auto mesh = g_LoadedDataset->meshes[meshIndex];
	return mesh->numTris;
}

UNITY_NATIVE_EXPORT void MeshGetVertices( int meshIndex, float * targetBuffer, int bufferSize )
{
	const int VERTEX_STRIDE = sizeof( float ) * 3;
	auto mesh = g_LoadedDataset->meshes[meshIndex];
	for( int i = 0; i < bufferSize && i < mesh->numTris * 3 * VERTEX_STRIDE; i++ )
		targetBuffer[i] = mesh->vertexData[i];
}

UNITY_NATIVE_EXPORT void MeshGetNormals( int meshIndex, float * targetBuffer, int bufferSize )
{
	const int NORMAL_STRIDE = sizeof( float ) * 3;
	auto mesh = g_LoadedDataset->meshes[meshIndex];
	for( int i = 0; i < bufferSize && i < mesh->numTris * 3 * NORMAL_STRIDE; i++ )
		targetBuffer[i] = mesh->normalData[i];
}

UNITY_NATIVE_EXPORT void MeshGetVolumes( int meshIndex, int * targetBuffer, int bufferSize )
{
	auto mesh = g_LoadedDataset->meshes[meshIndex];
	for( int i = 0; i < bufferSize && i < mesh->numTris * 3; i++ )
		targetBuffer[i] = mesh->volumeData[i];
}



UNITY_NATIVE_EXPORT bool MeshHasVertices( int meshIndex )
{
	return g_LoadedDataset->meshes[meshIndex]->vertexData != nullptr;
}

UNITY_NATIVE_EXPORT bool MeshHasNormals( int meshIndex )
{
	return g_LoadedDataset->meshes[meshIndex]->normalData != nullptr;
}

UNITY_NATIVE_EXPORT bool MeshHasVolumes( int meshIndex )
{
	return g_LoadedDataset->meshes[meshIndex]->volumeData != nullptr;
}

