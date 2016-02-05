#pragma once

#include <string>
#include "../Types.h"

#include "../VolumeMeshTimeline.h"

void workflow_volume_data_export( const std::string & targetFile, gpu_data_sequence_array * sourceData, double * timestamps )
{
	VolumeMeshTimeline outputTimeline;
	
	for( int i = 0; i < sourceData->size( ); i++ )
	{
		auto frame = sourceData->at( i );

		VolumeMeshState * newState = new VolumeMeshState( );
		newState->dataStore = frame->data( );
		newState->numDimensions = 1;
		newState->numVolumes = frame->extent.size( );
		newState->timestamp = timestamps[i];
		
		//TODO: GENERATE MINMAX
		//newState->min = 
		//newState->max = 

		outputTimeline.states.push_back( newState );
	}

	outputTimeline.SaveTo( targetFile );

	for( auto state : outputTimeline.states )
		delete state;
}