
#include "QuickData.Unity.h"
#include "../FbxPartitioning/VolumeMeshTimeline.h"

static VolumeMeshTimeline * g_LoadedVolumes = nullptr;

UNITY_NATIVE_EXPORT void LoadVolumeSet( const char * storePath )
{
	if( g_LoadedVolumes != nullptr )
		delete g_LoadedVolumes;

	g_LoadedVolumes = new VolumeMeshTimeline( );
	g_LoadedVolumes->LoadFrom( storePath );
}

UNITY_NATIVE_EXPORT void FreeVolumeSet( )
{
	if( g_LoadedVolumes != nullptr )
		delete g_LoadedVolumes;

	g_LoadedVolumes = nullptr;
}



UNITY_NATIVE_EXPORT int VolumeGetNumDimensions( )
{
	return g_LoadedVolumes->states.front( )->numDimensions;
}

UNITY_NATIVE_EXPORT int VolumeGetNumStates( )
{
	return g_LoadedVolumes->states.size( );
}

UNITY_NATIVE_EXPORT int VolumeGetNumVolumes( )
{
	return g_LoadedVolumes->states.front( )->numVolumes;
}

UNITY_NATIVE_EXPORT void VolumeGetStateData( int frameIndex, double * target, int numDimensions, int count )
{
	auto state = g_LoadedVolumes->states[frameIndex];

	if( numDimensions != state->numDimensions )
		NOT_YET_IMPLEMENTED( );

	if( count > state->numVolumes )
		NOT_YET_IMPLEMENTED( );

	for( int i = 0; i < count * numDimensions; i++ )
		target[i] = state->dataStore[i];
}