#pragma once

#include <string>

#include "../Types.h"
#include "../Utilities.h"

#include "../MshHeaderReader.h"

void workflow_import_msh_header( const std::string & fileName, gpu_data_array ** out_data )
{
	MshHeaderReader reader( fileName );
	*out_data = bindless_copy( reader.data );
}