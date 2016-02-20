#pragma once
#include <string>
#include "Types.h"
#include "Utilities.h"
#include "Workflows/VolumeDataExportWorkflow.h"

void process_volume_states( const std::string & sourceMeshHeader )
{
	std::string fileExt = getFileExtension( sourceMeshHeader );

	//	Only supporting MSH Header .mm files
	if( fileExt != "mm" )
		NOT_YET_IMPLEMENTED( );

	std::cout << "OPERATING ON " << sourceMeshHeader << std::endl;

	cpu_data_sequence_array * data;
	double * timestamps;
	workflow_import_msh_header_cpu( sourceMeshHeader, &data, &timestamps );

	workflow_volume_data_export( getStoragePath( sourceMeshHeader ) + ".binvolumes", data, timestamps );

	delete data;
	delete timestamps;
	std::cout << "Done." << std::endl;
}
