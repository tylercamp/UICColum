
#define NOMINMAX

#include <iostream>

#include "Types.h"
#include "Utilities.h"

#include "Workflows/ImportFbxWorkflow.h"
#include "Workflows/ImportMshWorkflow.h"

#include "Workflows/TriangulationWorkflow.h"
#include "Workflows/NormalGenerationWorkflow.h"
#include "Workflows/VolumeTaggingWorkflow.h"
#include "Workflows/MeshRecenteringWorkflow.h"
#include "Workflows/RenderWorkflow.h"

#include "Workflows/GatherMeshBoundsWorkflow.h"
#include "Workflows/PartitioningWorkflow.h"
#include "Workflows/MeshChunkingWorkflow.h"

#include "Workflows/ChunkExportFbxWorkflow.h"
#include "Workflows/ChunkExportBinaryWorkflow.h"

#include "MshHeaderReader.h"

//	(SDL-specific)
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


float_3 collection_center;
bool should_generate_center = false;
bool did_generate_center = false;

enum ExportMode {
	EXPORT_FBX,
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
	workflow_chunk_from_partitions( tris, partitions, tags, partitionCounts, out_chunks );

	delete partitions;
	delete tags, partitionCounts;
}

void process( const std::string & file, ExportMode exportMode )
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
		workflow_gen_normals( tris );

		delete points;
		delete indices;
	}
	else if( fileExt == "msh" )
	{
		gpu_vertex_array * volumeData = nullptr;

		gpu_vertex_array * points;
		gpu_index_array * surfaceIndices, * volumeIndices, * volumeTags;

		workflow_import_msh( file, &points, &surfaceIndices, &volumeIndices, &volumeTags );
		int x = surfaceIndices->data()[0];
		int y = volumeIndices->data( )[0];
		workflow_gen_tris( points, surfaceIndices, &tris );
		workflow_gen_tris( points, volumeIndices, &volumeTris );
		workflow_gen_normals( tris );
		workflow_gen_normals( volumeTris );
		workflow_tag_mesh_volumes( volumeTris, volumeTags );
		
		if( volumeData )
			delete volumeData;
		delete points;
		delete surfaceIndices, volumeIndices;
	}
	else
		NOT_YET_IMPLEMENTED( );



	//workflow_render_mesh( tris );
	//if( volumeTris ) workflow_render_mesh( volumeTris );

	if( should_generate_center )
	{
		if( !did_generate_center ) {
			collection_center = workflow_get_mesh_center( tris );
			did_generate_center = true;
		}

		workflow_recenter_mesh( tris, collection_center );
		if( volumeTris ) workflow_recenter_mesh( volumeTris, collection_center );
	}


	cpu_chunk_array * chunks, * volumeChunks = nullptr;
	generate_chunks( tris, &chunks );
	if( volumeTris ) generate_chunks( volumeTris, &volumeChunks );

	//workflow_render_mesh( chunks );

	CreateDirectoryA( getFileName( file ).c_str( ), nullptr );

	typedef void( *ChunkExportWorkflow )(const std::string &, cpu_chunk_array *);
	//	Strategy pattern
	ChunkExportWorkflow exportWorkflow = nullptr;
	switch( exportMode )
	{
	case(EXPORT_FBX) :
		exportWorkflow = workflow_chunk_export_fbx;
		break;
	case(EXPORT_BINARY) :
		exportWorkflow = workflow_chunk_export_binary;
		break;
	}

	exportWorkflow( getFileName( file ) + "/surfaces", chunks );
	if( volumeChunks ) exportWorkflow( getFileName( file ) + "/volumes", volumeChunks );


	delete tris;
	if( volumeTris )
		delete volumeTris;
	delete chunks;
	if( volumeChunks )
		delete volumeChunks;

	std::cout << "\nDONE" << std::endl;
	std::cout << "Total operation took " << formatTime( clock( ) - start ) << std::endl;
}



void debug_render_output( const std::string & targetPath )
{
	cpu_chunk_array chunks;

	WIN32_FIND_DATAA ffd;
	LARGE_INTEGER filesize;
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	std::string inputPath( targetPath );
	if( inputPath.back( ) != '\\' && inputPath.back( ) != '/' )
		inputPath += '\\';
	inputPath += "*.binmesh";


	hFind = FindFirstFileA( inputPath.c_str( ), &ffd );

	if( INVALID_HANDLE_VALUE == hFind )
		return;

	// List all the files in the directory with some info about them.

	do
	{
		if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			std::string foundPath = std::string( targetPath ) + ffd.cFileName;
			BinaryMesh mesh( foundPath );
			mesh_chunk chunk;
			chunk.num_tris = mesh.numTris;
			chunk.tris.resize( chunk.num_tris );
			for( int i = 0; i < chunk.num_tris; i++ )
			{
				auto & tri = chunk.tris[i];
				tri.a = mesh.vertices[i * 3 + 0];
				tri.b = mesh.vertices[i * 3 + 1];
				tri.c = mesh.vertices[i * 3 + 2];

				tri.norm_a = mesh.normals[i * 3 + 2];
				tri.norm_b = mesh.normals[i * 3 + 2];
				tri.norm_c = mesh.normals[i * 3 + 2];
			}

#ifdef _DEBUG
			if( mesh.numTris < 0 )
				__debugbreak( );
#endif
			chunks.emplace_back( std::move( chunk ) );

			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			printf( "  %s   %l bytes\n", ffd.cFileName, filesize.QuadPart );
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	dwError = GetLastError( );
	if( dwError != ERROR_NO_MORE_FILES )
		return;

	FindClose( hFind );

	//workflow_render_mesh( &chunks );
}



//#ifdef _DEBUG
int main( )
//#else
//int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow )
//#endif
{
	//debug_render_output( "IanArteries5.GAMBIT/surfaces/" );

	MshHeaderReader reader( "IanArteries5-1.mm" );

	return 0;

	auto start = clock( );

	process( "bsArteries.fbx", EXPORT_BINARY );
	process( "bsCSF.fbx", EXPORT_BINARY );
	process( "bsGray.fbx", EXPORT_BINARY );
	process( "bsSkull.fbx", EXPORT_BINARY );
	process( "bsVeins.fbx", EXPORT_BINARY );
	process( "bsWhite.fbx", EXPORT_BINARY );

	//processMsh( "IanArteries5.GAMBIT.msh", EXPORT_BINARY, { "IanArteries5-1.mm" } );
	//process( "IanArteries5.GAMBIT.msh", EXPORT_BINARY );

	auto end = clock( );

	std::cout << "\n\nTOTAL OPERATION TOOK: " << ( end - start ) / 1000 << "s" << std::endl;

	pause( );
	return 0;
}