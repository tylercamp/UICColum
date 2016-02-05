#pragma once

#include "../Types.h"
#include "../MeshPartitionScheme.h"

void workflow_export_partition_scheme( const std::string & targetFile, cpu_chunk_array * partitionedChunks )
{
	MeshPartitionScheme scheme;
	for( auto & chunk : *partitionedChunks )
		scheme.descriptors.push_back( chunk.bounds );

	scheme.SaveTo( targetFile );
}