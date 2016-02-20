#pragma once

#include <string>

#include "../Types.h"
#include "../Utilities.h"

#include "../MshHeaderReader.h"

void workflow_import_msh_header( const std::string & fileName, gpu_data_sequence_array ** out_data, double ** out_timeStamps )
{
	MshHeaderReader reader( fileName );

	auto resultData = new gpu_data_sequence_array( );
	auto resultTimestamps = new double[resultData->size( )];

	for( int i = 0; i < reader.data.size( ); i++ )
	{
		resultData->push_back( bindless_copy( reader.data[i] ) );
		resultTimestamps[i] = 0.0;
	}

	*out_data = resultData;
	*out_timeStamps = resultTimestamps;
}

void workflow_import_msh_header_cpu( const std::string & fileName, cpu_data_sequence_array ** out_data, double ** out_timeStamps )
{
	MshHeaderReader reader( fileName );

	auto resultData = new cpu_data_sequence_array( );
	auto resultTimestamps = new double[resultData->size( )];

	for( int i = 0; i < reader.data.size( ); i++ )
	{
		resultData->push_back( new cpu_data_array( reader.data[i] ) );
		resultTimestamps[i] = 0.0;
	}

	*out_data = resultData;
	*out_timeStamps = resultTimestamps;
}