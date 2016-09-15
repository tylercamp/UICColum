#pragma once
#include <string>
#include "Types.h"
#include "Utilities.h"
#include "Workflows/VolumeDataExportWorkflow.h"


void process_volume_states_largefile( const std::string & sourceMeshHeader )
{
	std::string fileExt = getFileExtension( sourceMeshHeader );

	//	Only supporting MSH Header .mm files
	if( fileExt != "mm" )
		NOT_YET_IMPLEMENTED( );

	std::cout << "OPERATING ON " << sourceMeshHeader << std::endl;

	std::int64_t fileSize = getTextFileSize( sourceMeshHeader );

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

		if( fileExists( targetFile ) )
			DeleteFileA( targetFile.c_str( ) );

		std::int64_t offset = 0;
		std::int64_t chunkSize;
		size_t maxSize = 1024 * 1024 * 1024; // 1GB, the max size of data to read and process at a time
		int i = 0;
		do
		{
			std::cout << "Large file .mm iteration " << ++i << std::endl;

			chunkSize = workflow_import_msh_header_cpu_part( sourceMeshHeader, maxSize, offset, &data, &timestamps );
			//	If no data was loaded (max data size not large enough) try again with larger chunk size
			while( chunkSize == 0 )
			{
				maxSize += (size_t) ( maxSize * 0.5 );
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

void process_volume_states_simple( const std::string & sourceMeshHeader )
{
	std::string fileExt = getFileExtension( sourceMeshHeader );

	//	Only supporting MSH Header .mm files
	if( fileExt != "mm" )
		NOT_YET_IMPLEMENTED( );

	std::cout << "OPERATING ON " << sourceMeshHeader << std::endl;

	//	Simple implementation
	{
		cpu_data_sequence_array * data;
		double * timestamps;

		workflow_import_msh_header_cpu( sourceMeshHeader, &data, &timestamps );
		workflow_volume_data_export( getStoragePath( sourceMeshHeader ) + ".binvolumes", data, timestamps );
	}

	std::cout << "Done." << std::endl;
}



void process_volume_states( const std::string & sourceMeshHeader )
{
	process_volume_states_largefile( sourceMeshHeader );
	//process_volume_states_simple( sourceMeshHeader );
}
