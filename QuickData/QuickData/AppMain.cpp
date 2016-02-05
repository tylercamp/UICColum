
#define NOMINMAX

#include <iostream>

//	App Logic
#include "process_mesh.h"
#include "process_volume_states.h"
#include "process_voxels.h"
#include "process_cs31.h"



//	(SDL-specific)
#ifdef main
# undef main
#endif

//	Library references
#pragma comment( lib, "SDL2.lib" )
#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )
#pragma comment( lib, "winmm.lib" ) // For PlaySound() on task complete



#pragma comment( lib, "assimp.lib" )
#pragma comment( lib, "libfbxsdk.lib" )

#pragma warning( disable: 4018 ) // signed/unsigned mismatch
#pragma warning( disable: 4244 ) // floating-point conversion warnings
#pragma warning( disable: 4267 ) // integer conversion warnings







bool fileExists( const std::string & filename )
{
	FILE * f = fopen( filename.c_str( ), "r" );
	bool exists = (f != nullptr);
	if( exists )
		fclose( f );
	return exists;
}

void try_process_mesh( const std::string & filename, ExportMode exportMode, bool invertNormals )
{
	if( !fileExists( filename ) )
	{
		std::cout << "Could not find " << filename << std::endl;
		return;
	}

	process_mesh( filename, exportMode, invertNormals );

	concurrency::accelerator( concurrency::accelerator::default_accelerator ).default_view.flush( );
}

void process_mesh_set( const std::string & groupName, ExportMode exportMode, bool invertAll = false )
{
	try_process_mesh( groupName + "Arteries.stl", exportMode, invertAll );
	try_process_mesh( groupName + "CSF.stl", exportMode, invertAll );
	try_process_mesh( groupName + "Gray.stl", exportMode, invertAll );
	try_process_mesh( groupName + "Scalp.stl", exportMode, invertAll );
	try_process_mesh( groupName + "Veins.stl", exportMode, invertAll );
	try_process_mesh( groupName + "White.stl", exportMode, invertAll );
}


void debug_render_volume_output( const std::string & meshesPath, const std::string & volumeTimelineFile );

#include "Workflows\VoxelTagBoundariesWorkflow.h"
#include "Workflows\ImportBinaryMeshWorkflow.h"

//#ifdef _DEBUG
int main( int argc, char * argv[] )
//#else
//int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow )
//#endif
{
	//debug_render_volume_output( "msh-tetrahedral/FullCNSINJ_V1.GAMBIT/volumes/", "msh-tetrahedral/FullCNS_Drug_CellCenters_T1.dynamic.binvolumes" );
	//return 0;

	
	//std::string prefix = "C:/Users/algor/Desktop/UIC/";
	std::string prefix = "";

	auto meshes = {
		"patients/bs/bsArteries/surfaces/",
		//"patients/bs/bsCSF/surfaces/",
		//"patients/bs/bsGray/surfaces/",
		//"patients/bs/bsScalp/surfaces/",
		//"patients/bs/bsVeins/surfaces/",
		//"patients/bs/bsWhite/surfaces/"
	};

	auto allstart = clock( );
	for( auto meshpath : meshes )
	{
		std::cout << "Processing " << meshpath << std::endl;
		cpu_chunk_array * mesh_chunks;
		workflow_import_binary_mesh_set( prefix + meshpath, &mesh_chunks );

		int res = 512;

		auto startA = clock( );
		std::cout << "Generating matrix...";
		voxel_matrix vm( res, res, res, float_3( -20.0 ), float_3( 190.0 ) );
		std::cout << "done. Took " << ( clock( ) - startA ) << "ms" << std::endl;

		std::cout << "Generating tag data for matrix " << res << "x" << res << "x" << res << "..." << std::endl;
		startA = clock( );
		workflow_tag_voxels_with_mesh_boundary( &vm, mesh_chunks );
		//vm.generate_test_data( );
		std::cout << "Done. Took " << ( clock( ) - startA ) / 1000.0f << "s" << std::endl;

		
		int maxTags = 0;
		for( int x = 0; x < vm.width; x++ )
			for( int y = 0; y < vm.height; y++ )
				for( int z = 0; z < vm.depth; z++ )
					maxTags = max( maxTags, vm.dev_voxel_tag_data[0]->operator()( x, y, z ) );

		std::cout << "max(tags) = " << maxTags << std::endl;


		{
			gpu_voxel_data & voxel_data = *( new gpu_voxel_data( vm.dev_voxels->extent ) );
			auto & tags = *( vm.dev_voxel_tag_data[0] );

			for( int x = 0; x < vm.width; x++ )
				for( int y = 0; y < vm.height; y++ )
					for( int z = 0; z < vm.depth; z++ )
						voxel_data( x, y, z ) = tags( x, y, z ) / (double) maxTags;

			vm.dev_voxel_data = &voxel_data;
		}
		

		run_window( vm );

		delete mesh_chunks;
	}

	std::cout << "Took " << ( clock( ) - allstart ) / 1000.0f << "s" << std::endl;

	pause( );

	return 0;
	
	
	

	std::vector<std::string> jobList;
	bool invertAll = false;

	for( int i = 1; i < argc; i++ )
	{
		if( toLower( argv[i] ) == "invertall" )
			invertAll = true;
		else
			jobList.push_back( argv[i] );
	}

	auto start = clock( );

	if( invertAll )
		std::cout << "Will invert normals on all supplied mesh jobs" << std::endl;
	
	if( jobList.size( ) == 0 )
	{
		jobList = {
			"patients/bs/bs",
			//"patients/ch/ch",
			//"patients/ig/ig",
			//"patients/kt/kt",
			//"patients/mg/mg",
			//"patients/nn/nn"

			//"patients/MMPOST1_mc.stl",
			//"patients/MMPost2_mc.stl"

			//"IanArteries5.GAMBIT.msh",

			//"msh-tetrahedral/FullCNSINJ_V1.GAMBIT.msh",
			//"msh-tetrahedral/FullCNS_Drug_CellCenters_T1.dynamic.mm",
		};
	}

	for( auto job : jobList )
	{
		std::cout << "Starting job: " << job << std::endl;

		auto type = getFileExtension( job );
		if( type == "" )
			process_mesh_set( job, EXPORT_BINARY );
		if( type == "stl" )
			process_mesh( job, EXPORT_BINARY, invertAll );
		if( type == "mm" )
			process_volume_states( job );
		if( type == "msh" )
			process_mesh( job, EXPORT_BINARY, invertAll );
	}


	auto end = clock( );

	std::cout << "\n\nTOTAL OPERATION TOOK: ~" << (end - start) / 1000 << "s" << std::endl;

	PlaySound( TEXT( "SystemStart" ), nullptr, SND_ALIAS );
	SetForegroundWindow( GetConsoleWindow( ) );

	pause( );
	return 0;
}












void debug_render_output( const std::string & targetPath )
{
	cpu_chunk_array chunks;

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	std::string inputPath( targetPath );
	if( inputPath.back( ) != '\\' && inputPath.back( ) != '\\' )
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
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	dwError = GetLastError( );
	if( dwError != ERROR_NO_MORE_FILES )
		return;

	FindClose( hFind );

	//workflow_render_mesh( &chunks );
}



void debug_render_volume_output( const std::string & meshesPath, const std::string & volumeTimelineFile )
{
	cpu_chunk_array chunks;

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;

	std::string inputPath( meshesPath );
	if( inputPath.back( ) != '\\' && inputPath.back( ) != '/' )
		inputPath += '\\';
	inputPath += "*.binmesh";


	hFind = FindFirstFileA( inputPath.c_str( ), &ffd );

	//ASSERT( INVALID_HANDLE_VALUE != hFind );

	// List all the files in the directory with some info about them.

	do
	{
		if( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			std::string foundPath = std::string( meshesPath ) + ffd.cFileName;
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

				tri.norm_a = mesh.normals[i * 3 + 0];
				tri.norm_b = mesh.normals[i * 3 + 1];
				tri.norm_c = mesh.normals[i * 3 + 2];

				tri.volumeIndex = mesh.volumes[i * 3 + 0];
				tri.volumeIndex = mesh.volumes[i * 3 + 1];
				tri.volumeIndex = mesh.volumes[i * 3 + 2];
			}

#ifdef _DEBUG
			if( mesh.numTris < 0 )
				__debugbreak( );
#endif
			chunks.emplace_back( std::move( chunk ) );
		}
	} while( FindNextFileA( hFind, &ffd ) != 0 );

	dwError = GetLastError( );
	if( dwError != ERROR_NO_MORE_FILES )
		return;

	FindClose( hFind );

	
	
	VolumeMeshTimeline timeline;
	timeline.LoadFrom( volumeTimelineFile );
	cpu_data_sequence_array sequence;
	for( auto state : timeline.states )
	{
		//	Only 1-dimensional data for now, need to confirm import support for vector data
		ASSERT( state->numDimensions == 1 );

		auto cpuState = new cpu_data_array( );
		cpuState->assign( state->dataStore, state->dataStore + state->numDimensions * state->numVolumes );
		sequence.push_back( cpuState );
	}
	
	
	workflow_render_volume_mesh( &chunks, &sequence );
}
