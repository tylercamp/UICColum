#pragma once

#include "Types.h"
#include "Utilities.h"
#include "Workflows/GatherMeshBoundsWorkflow.h"
#include "Workflows/PartitioningWorkflow.h"
#include "Workflows/MeshChunkingWorkflow.h"
//#include "Workflows/ImportFbxWorkflow.h"
#include "Workflows/TriangulationWorkflow.h"
#include "Workflows/NormalGenerationWorkflow.h"
#include "Workflows/ImportMshWorkflow.h"
#include "Workflows/VolumeTaggingWorkflow.h"
#include "Workflows/VolumeNormalGenerationWorkflow.h"
#include "Workflows/MeshRecenteringWorkflow.h"
#include "Workflows/ImportMshHeaderWorkflow.h"
//#include "Workflows/ChunkExportFbxWorkflow.h"
#include "Workflows/ChunkExportBinaryWorkflow.h"
#include "Workflows/ImportAssimpWorkflow.h"
#include "Workflows/RenderWorkflow.h"
#include "Workflows/SmoothNormalsWorkflow.h"
#include "Workflows/PartitionSchemeExportWorkflow.h"

enum ExportMode {
	EXPORT_BINARY
};




//	MAIN PROCESSING FUNCTIONS

void generate_chunks( gpu_triangle_array * tris, cpu_chunk_array ** out_chunks )
{
	float_3 min, max;
	workflow_gather_mesh_bounds( tris, &min, &max );

	gpu_index_array * tags;
	gpu_index_array * partitionCounts;

	gpu_partition_descriptor_array * partitions;
	workflow_generate_partitions( tris, min, max, &partitions, &tags, &partitionCounts );
	workflow_chunk_from_partitions( tris, partitions, partitionCounts, out_chunks );
}

float_3 collection_center;
bool should_generate_center = false;
bool did_generate_center = false;



void process_mesh( const std::string & file, ExportMode exportMode, bool forceInvertNormals, bool shouldGenerateCenter = false )
{
	auto start = clock( );

	std::cout << "\n\n OPERATING ON " << file << std::endl;

	gpu_triangle_array * tris = nullptr, * volumeTris = nullptr;

	bool invertNormals = false;

	/*
	//	This list is inverted (no pun intended), need to reverse cross-product
	//		order for correct normal generation, confirm data quality after change
	auto invertList = { "bs/bs", "ch/ch", "nn/nn", "1_mc", "2_mc" };
	for( auto i : invertList )
		if( contains( file, i ) )
			invertNormals = true;
			*/

	if( forceInvertNormals )
		invertNormals = true;

	/*** LOAD MESH DATA ***/

	//	Determine loading method
	std::string fileExt = toLower( getFileExtension( file ) );
	if( fileExt != "msh" )
	{
		gpu_vertex_array * points;
		gpu_index_array * indices;

		//workflow_import_fbx( file, &points, &indices );
		workflow_import_assimp( file, &points, &indices );
		workflow_gen_tris( points, indices, &tris );
		workflow_gen_normals( tris, invertNormals );
		//workflow_smooth_normals( tris, points, indices );

		delete points;
		delete indices;
	}
	else
	{
		gpu_vertex_array * points;
		//	Tags are the real "volume indices"
		gpu_index_array * surfaceIndices, * surfaceTags, * volumeIndices, * volumeTags;

		int numVolumes = workflow_import_msh( file, &points, &surfaceIndices, &surfaceTags, &volumeIndices, &volumeTags );
		workflow_gen_tris( points, surfaceIndices, &tris );
		workflow_gen_tris( points, volumeIndices, &volumeTris );
		workflow_tag_mesh_volumes( volumeTris, volumeTags );
		workflow_tag_mesh_volumes( tris, surfaceTags );
		workflow_gen_normals( tris );
		//workflow_gen_normals( volumeTris );
		workflow_gen_volume_normals( volumeTris, numVolumes );

		delete points;
		delete surfaceIndices, surfaceTags, volumeIndices, volumeTags;
	}
	
	std::cout << "Surface mesh has " << tris->extent.size( ) << " tris" << std::endl;
	if( volumeTris ) std::cout << "Volume mesh has " << volumeTris->extent.size( ) << " tris" << std::endl;

	//workflow_render_mesh( tris );
	//if( volumeTris ) workflow_render_mesh( volumeTris );



	/*** RECENTER MESHES (optional) ***/

	if( shouldGenerateCenter )
	{
		if( !shouldGenerateCenter ) {
			collection_center = workflow_get_mesh_center( tris );
			did_generate_center = true;
		}

		workflow_recenter_mesh( tris, collection_center );
		if( volumeTris ) workflow_recenter_mesh( volumeTris, collection_center );
	}




	/*** PARTITION DATA ***/

	cpu_chunk_array * chunks, * volumeChunks = nullptr;
	std::cout << "Processing surface data" << std::endl;
	generate_chunks( tris, &chunks );
	std::cout << "Processing surface data DONE" << std::endl;
	if( volumeTris )
	{
		std::cout << "Processing volume data" << std::endl;
		generate_chunks( volumeTris, &volumeChunks );
		std::cout << "Processing volume data DONE" << std::endl;
	}

	//workflow_render_mesh( chunks );



	/*** SAVE TO DISK ***/

	CreateDirectoryA( getStoragePath( file ).c_str( ), nullptr );

	workflow_chunk_export_binary( getStoragePath( file ) + "/surfaces", chunks );
	if( volumeChunks ) workflow_chunk_export_binary( getStoragePath( file ) + "/volumes", volumeChunks );

	/*
	std::string partitionSchemeFile = "partitions.binmeshscheme";
	workflow_export_partition_scheme( getStoragePath( file ) + "/surfaces/" + partitionSchemeFile, chunks );
	if( volumeChunks ) workflow_export_partition_scheme( getStoragePath( file ) + "/volumes/" + partitionSchemeFile, volumeChunks );
	*/



	/*** CLEANUP ***/
	delete chunks;
	delete tris;
	if( volumeChunks ) delete volumeChunks;
	if( volumeTris ) delete volumeTris;
}