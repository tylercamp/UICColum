
#define NOMINMAX

#include <iostream>

#include "Types.h"
#include "Utilities.h"
#include "WorkflowScheduler.h"

#include "Workflows/FbxChunkExportWorkflow.h"
#include "Workflows/FbxImportWorkflow.h"
#include "Workflows/MshImportWorkflow.h"
#include "Workflows/PartitioningWorkflow.h"
#include "Workflows/TriangulationWorkflow.h"
#include "Workflows/RenderWorkflow.h"
#include "Workflows/MeshChunkingWorkflow.h"
#include "Workflows/GatherMeshBoundsWorkflow.h"
#include "Workflows/NormalGenerationWorkflow.h"

#ifdef main
# undef main
#endif

//	Library references
#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )



//	Toggles
#define PREVIEW_MESH



#pragma comment( lib, "libfbxsdk.lib" )

#pragma warning( disable: 4018 ) // signed/unsigned mismatch
#pragma warning( disable: 4244 ) // floating-point conversion warnings
#pragma warning( disable: 4267 ) // integer conversion warnings




//	MAIN PROCESSING FUNCTIONS

void generate_chunks( gpu_triangle_array * tris, cpu_chunk_array ** out_chunks )
{
	float_3 min, max;
	workflow_gather_mesh_bounds( tris, &min, &max );

	gpu_index_array * tags;
	gpu_index_array * partitionCounts;

	gpu_partition_descriptor_array * partitions;
	workflow_generate_partitions( tris, min, max, &partitions, &tags, &partitionCounts );
	workflow_chunk_from_partitions( tris, partitions, tags, partitionCounts, out_chunks );

	delete partitions;
	delete tags, partitionCounts;
}

void process( const std::string & file )
{
	auto start = clock( );

	std::cout << "\n\n OPERATING ON " << file << std::endl;
	
	gpu_triangle_array * tris = nullptr, * volumeTris = nullptr;


	//	Determine loading method
	std::string fileExt = getFileExtension( file );
	if( fileExt == "fbx" )
	{
		gpu_vertex_array * points;
		gpu_index_array * indices;

		workflow_import_fbx( file, &points, &indices );
		workflow_gen_tris( points, indices, &tris );

		delete points;
		delete indices;
	}
	else if( fileExt == "msh" )
	{
		gpu_vertex_array * volumeData = nullptr;

		gpu_vertex_array * points;
		gpu_index_array * indices, * innerIndices;

		workflow_import_msh( file, &points, &indices, &innerIndices );
		int x = indices->data()[0];
		int y = innerIndices->data( )[0];
		workflow_gen_tris( points, indices, &tris );
		workflow_gen_tris( points, innerIndices, &volumeTris );
		
		if( volumeData )
			delete volumeData;
		delete points;
		delete indices, innerIndices;
	}
	else
		NOT_YET_IMPLEMENTED( );



	workflow_render_mesh( tris );
	if( volumeTris )
		workflow_render_mesh( volumeTris );


	cpu_chunk_array * chunks, * volumeChunks = nullptr;
	generate_chunks( tris, &chunks );
	if( volumeTris )
		generate_chunks( volumeTris, &volumeChunks );

	CreateDirectoryA( getFileName( file ).c_str( ), nullptr );
	workflow_chunk_export_fbx( getFileName( file ) + "/surfaces", chunks );
	if( volumeChunks )
		workflow_chunk_export_fbx( getFileName( file ) + "/volumes", volumeChunks );



	delete tris;
	if( volumeTris )
		delete volumeTris;
	delete chunks;
	if( volumeChunks )
		delete volumeChunks;

	std::cout << "\nDONE" << std::endl;
	std::cout << "Total operation took " << formatTime( clock( ) - start ) << std::endl;
}



//#ifdef _DEBUG
int main( )
//#else
//int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow )
//#endif
{
	auto start = clock( );

	//MshReader mshReader( "IanArteries5.GAMBIT.msh" );
	//return 0;

	//process( "bsArteries.fbx" );
	//process( "bsCSF.fbx" );
	process( "IanArteries5.GAMBIT.msh" );
	/*process( "bsGray.fbx" );
	process( "bsScalp.fbx" );
	process( "bsSkull.fbx" );
	process( "bsVeins.fbx" );
	process( "bsWhite.fbx" );*/

	auto end = clock( );

	std::cout << "\n\nTOTAL OPERATION TOOK: " << ( end - start ) / 1000 << "s" << std::endl;

	pause( );
	return 0;
}