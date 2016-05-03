
#include "../QuickData/MeshPartitionScheme.h"
#include "QuickData.Unity.h"

static MeshPartitionScheme * g_LoadedScheme = nullptr;

UNITY_NATIVE_EXPORT bool LoadMeshScheme( char * schemeFile )
{
	if( g_LoadedScheme != nullptr )
		delete g_LoadedScheme;

	MeshPartitionScheme * scheme = new MeshPartitionScheme( );
	scheme->LoadFrom( schemeFile );
	g_LoadedScheme = scheme;

	return true;
}

UNITY_NATIVE_EXPORT int MeshSchemeDescriptorsCount( )
{
	if( g_LoadedScheme == nullptr )
		return -1;

	return g_LoadedScheme->descriptors.size( );
}



UNITY_NATIVE_EXPORT float MeshSchemeDescriptorPositionXAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	return g_LoadedScheme->descriptors[index].bounds_start.x;
}

UNITY_NATIVE_EXPORT float MeshSchemeDescriptorPositionYAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	return g_LoadedScheme->descriptors[index].bounds_start.y;
}

UNITY_NATIVE_EXPORT float MeshSchemeDescriptorPositionZAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	return g_LoadedScheme->descriptors[index].bounds_start.z;
}

UNITY_NATIVE_EXPORT float MeshSchemeDescriptorSizeXAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	auto descriptor = g_LoadedScheme->descriptors[index];
	return ( descriptor.bounds_end - descriptor.bounds_start ).x;
}

UNITY_NATIVE_EXPORT float MeshSchemeDescriptorSizeYAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	auto descriptor = g_LoadedScheme->descriptors[index];
	return ( descriptor.bounds_end - descriptor.bounds_start ).y;
}

UNITY_NATIVE_EXPORT float MeshSchemeDescriptorSizeZAtIndex( int index )
{
	if( g_LoadedScheme == nullptr )
		return 0.0f;

	auto descriptor = g_LoadedScheme->descriptors[index];
	return ( descriptor.bounds_end - descriptor.bounds_start ).z;
}
