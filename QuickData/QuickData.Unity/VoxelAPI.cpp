
#include "QuickData.Unity.h"
#include "../QuickData/BinaryVoxelMatrix.h"

static BinaryVoxelMatrix * g_LoadedMatrix = nullptr;

UNITY_NATIVE_EXPORT bool LoadVoxelMatrix( const char * storePath )
{
	if( g_LoadedMatrix != nullptr )
		delete g_LoadedMatrix;

	try
	{
		g_LoadedMatrix = new BinaryVoxelMatrix( storePath );
	}
	catch( ... )
	{
		g_LoadedMatrix = nullptr;
		return false;
	}
	return true;
}

UNITY_NATIVE_EXPORT void FreeVoxelMatrix( )
{
	if( g_LoadedMatrix )
		delete g_LoadedMatrix;

	g_LoadedMatrix = nullptr;
}

UNITY_NATIVE_EXPORT bool MatrixIsLoaded( )
{
	return g_LoadedMatrix != nullptr;
}



UNITY_NATIVE_EXPORT void MatrixGetData( int dimension, double * targetStore, int storeLength )
{
	if( !g_LoadedMatrix )
		return;

	int numData = g_LoadedMatrix->resolution.x * g_LoadedMatrix->resolution.y * g_LoadedMatrix->resolution.z;
	for( int i = 0; i < numData && i < storeLength; i++ )
		targetStore[i] = g_LoadedMatrix->voxel_data[dimension][i];
}




UNITY_NATIVE_EXPORT int MatrixResolutionX( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->resolution.x;
}

UNITY_NATIVE_EXPORT int MatrixResolutionY( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->resolution.y;
}

UNITY_NATIVE_EXPORT int MatrixResolutionZ( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->resolution.z;
}

UNITY_NATIVE_EXPORT float MatrixSpatialStartX( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_start.x;
}

UNITY_NATIVE_EXPORT float MatrixSpatialStartY( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_start.y;
}

UNITY_NATIVE_EXPORT float MatrixSpatialStartZ( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_start.z;
}

UNITY_NATIVE_EXPORT float MatrixSpatialEndX( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_end.x;
}

UNITY_NATIVE_EXPORT float MatrixSpatialEndY( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_end.y;
}

UNITY_NATIVE_EXPORT float MatrixSpatialEndZ( )
{
	if( !g_LoadedMatrix )
		return 0;

	return g_LoadedMatrix->spatial_end.z;
}

