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

	std::int64_t fileSize = getFileSize( sourceMeshHeader );

	/*
	//	Simple implementation
	{
		cpu_data_sequence_array * data;
		double * timestamps;

		workflow_import_msh_header_cpu( sourceMeshHeader, &data, &timestamps );
		workflow_volume_data_export( getStoragePath( sourceMeshHeader ) + ".binvolumes", data, timestamps );
	}
	*/


	
	//	Iterative processing for large file
	{
		cpu_data_sequence_array * data;
		double * timestamps;

		FILE * outputFile = nullptr;

		std::string targetFile = getStoragePath( sourceMeshHeader ) + ".binvolumes";

		std::int64_t offset = 0;
		std::int64_t chunkSize;
		int maxSize = 128 * 1024 * 1024; // 128MB, the max size of data read at a time - actual runtime mem usage much larger (ie 2GB)
		int i = 0;
		do
		{
			std::cout << "Large file .mm iteration " << ++i << std::endl;

			chunkSize = workflow_import_msh_header_cpu_part( sourceMeshHeader, maxSize, offset, &data, &timestamps );
			//	If no data was loaded (max data size not large enough) try again with larger chunk size
			while( chunkSize == 0 )
			{
				maxSize += (int) ( maxSize * 0.5 );
				chunkSize = workflow_import_msh_header_cpu_part( sourceMeshHeader, maxSize, offset, &data, &timestamps );
			}

			outputFile = workflow_volume_data_export_part( targetFile, outputFile, data, timestamps );
			offset += chunkSize;

			for( auto dataLine : *data )
				delete dataLine;
			delete data;
			delete timestamps;


		} while( offset + 1 < fileSize );

		fclose( outputFile );
	}
	
	
	std::cout << "Done." << std::endl;
}
