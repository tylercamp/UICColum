


/*

	OUTLINE

	Main application logic is handled via the process_mesh, process_voxel_states functions. These functions are invoked
		from main() function after determining the file type.

	main() function operates on a job queue, which can be passed as command-line arguments.

	The process_* functions contain domain logic regarding the processing of assets. The actual work is deferred to
		workflow_* functions.




*/




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
	FILE * f = fopen( filename.c_str( ), "rb" );
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
#include "Workflows\VoxelTagBoundariesWorkflow2.h"
#include "Workflows\VoxelTagBoundariesWorkflowRef.h"

#include "Workflows\ImportBinaryMeshWorkflow.h"
#include "Workflows\VoxelMatrixDataExportWorkflow.h"

//#ifdef _DEBUG
int main( int argc, char * argv[] )
//#else
//int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow )
//#endif
{
	//debug_render_volume_output( "msh-tetrahedral/FullCNSINJ_V1.GAMBIT/volumes/", "msh-tetrahedral/FullCNS_Drug_CellCenters_T1.dynamic.binvolumes" );
	//return 0;
	
	/*
	{
		BinaryVoxelMatrix m( "voxeloutput/Arteries_CSF_Gray_Veins_White_400.binmatrix" );
		//BinaryVoxelMatrix m( "voxeloutput/Arteries_Gray_White_400.binmatrix" );
		voxel_matrix vm( 400, 400, 400, float_3( -20.0 ), float_3( 190.0 ) );
		vm.dev_voxel_data = new gpu_voxel_data( vm.width, vm.height, vm.depth, m.voxel_data[0] );

		run_window( vm );

		return 0;
	}
	*/
	
	
	
	//	VOXEL MATRIX TAGGING (NOT USED FOR CONVERSION OPS)
#if false
	
	//std::string prefix = "C:/Users/algor/Desktop/UIC/";
	std::string prefix = "";

	std::vector<std::string> meshes = {
		"patients/bs/bsArteries/surfaces/",
		"patients/bs/bsCSF/surfaces/",
		"patients/bs/bsGray/surfaces/",
		"patients/bs/bsVeins/surfaces/",
		"patients/bs/bsWhite/surfaces/"
	};

	std::string outputPath = "voxeloutput/";
	if( !directoryExists( outputPath ) )
		CreateDirectoryA( outputPath.c_str( ), nullptr );

	std::vector<std::string> meshNames = {
		"Arteries",
		"CSF",
		"Gray",
		"Veins",
		"White"
	};

	

	int res = 512;
	auto startA = clock( );
	std::cout << "Generating matrix...";
	voxel_matrix vm( res, res, res, float_3( -20.0f ), float_3( 190.0f ) );
	std::cout << "done. Took " << ( clock( ) - startA ) << "ms" << std::endl;

	auto allstart = clock( );
	for( int i = 0; i < meshes.size( ); i++ )
	{
		auto meshpath = meshes[i];

		std::cout << "Processing " << meshpath << std::endl;
		cpu_chunk_array * mesh_chunks;
		workflow_import_binary_mesh_set( prefix + meshpath, &mesh_chunks );

		float_3 bounds_min = float_3( FLT_MAX );
		float_3 bounds_max = float_3( -FLT_MAX );
		for( auto & chunk : *mesh_chunks )
		{
			bounds_min = min( bounds_min, chunk.bounds.bounds_start );
			bounds_max = max( bounds_max, chunk.bounds.bounds_end );

			//	Quality check
			float_3 range = chunk.bounds.bounds_end - chunk.bounds.bounds_start;
			ASSERT( range.x > 0 && range.y > 0 && range.z > 0 );
		}

		std::cout << "Generating tag data for matrix " << res << "x" << res << "x" << res << "..." << std::endl;
		startA = clock( );
		workflow_tag_voxels_with_mesh_boundary( &vm, mesh_chunks );
		//workflow_tag_voxels_with_mesh_boundary2( &vm, mesh_chunks );
		//workflow_tag_voxels_with_mesh_boundary_ref( &vm, mesh_chunks );
		//vm.generate_test_data( );
		std::cout << "Done. Took " << ( clock( ) - startA ) / 1000.0f << "s" << std::endl;


		int maxTags = 0;
		for( int x = 0; x < vm.width; x++ )
			for( int y = 0; y < vm.height; y++ )
				for( int z = 0; z < vm.depth; z++ )
					maxTags = max( maxTags, vm.dev_voxel_tag_data[i]->operator()( x, y, z ) );

		std::cout << "max(tags) = " << maxTags << std::endl;

		//	Generate data 
		{
			gpu_voxel_data & voxel_data = *( new gpu_voxel_data( vm.dev_voxels->extent ) );
			auto & tags = *( vm.dev_voxel_tag_data[i] );

			for( int x = 0; x < vm.width; x++ )
				for( int y = 0; y < vm.height; y++ )
					for( int z = 0; z < vm.depth; z++ )
						voxel_data( x, y, z ) = sqrtf( tags( x, y, z ) / (double) maxTags ) * 0.3f;

			vm.dev_voxel_data = &voxel_data;
		}


		run_window( vm );

		delete vm.dev_voxel_data;
		vm.dev_voxel_data = nullptr;


		//	Cleanup
		delete mesh_chunks;
	}

	std::string all_meshes;
	for( auto names : meshNames )
		all_meshes += names + "_";

	workflow_export_voxel_matrix_data( outputPath + all_meshes + toString( res ) + ".binmatrix", &vm );

	std::cout << "Took " << ( clock( ) - allstart ) / 1000.0f << "s" << std::endl;

	pause( );

	return 0;
#endif

	std::vector<std::string> jobList;
	bool invertAll = false;
	
	//	Allow output stream listeners to attach
	Sleep( 100 );
	
	if( argc > 1 && std::string( argv[1] ).find( "-stringcommand" ) != std::string::npos )
	{
		//	Run in streaming mode - This process has been launched and is being polled
		//	by an external application for progress and result information.

		//	Interpret requested command parameters
		using std::string;
		string collapsedCommand;
		{
			string separator;
			for( int i = 1; i < argc; i++ ) {
				collapsedCommand += separator;
				separator = " ";
			}
		}

		string command = collapsedCommand;
		std::cout << "Received command string:\n" << command << std::endl;
		return 0;
	}





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
			//
			//"patients/for-tyler/GAMBIT.CY.ch.msh", // success!
			//"patients/for-tyler/GAMBIT.CY.ig.msh", // success!
			////"patients/for-tyler/GAMBIT.CY.kt.msh", // fail! (formatting!)
			////"patients/for-tyler/GAMBIT.CY.mg2.msh", // fail! (formatting!)
			//"patients/for-tyler/GAMBIT.CY.nn.msh", // success! (previous failure likely due to incorrect point parsing)
			"patients/for-tyler/GAMBIT.CY.ben.msh", // fail gen_volume_normals!

			////	success!
			//"patients/for-tyler/bsSurf.msh",
			//"patients/for-tyler/chSurf.msh",
			//"patients/for-tyler/igSurf.msh",
			//"patients/for-tyler/ktSurf.msh",
			//"patients/for-tyler/mgSurf.msh",
			//"patients/for-tyler/nnSurf.msh",
			

			/*
			"patients/ch.msh",
			"patients/mg.msh",
			"patients/mg2.msh"
			*/

			/*
			"patients/GAMBIT.CY.ch.msh",
			"patients/GAMBIT.CY.mg2.msh",
			"patients/GAMBIT.CY.nn.msh"
			*/

			//"patients/KevinBestV7.GAMBIT.msh",
			//"patients/CNSTest-1-00051.dat.mm",
			////"patients/KevinBestV7_ForGrant-11-2.0501.dat.mm",
			//"msh-hexahedral/Grant2-10-00001.dat.mm",
			//"msh-hexahedral/CYcutBAv3.GAMBIT.msh",
			//"patients/bs/bs",
			//"patients/ch/ch",
			//"patients/ig/ig",
			//"patients/kt/kt",
			//"patients/mg/mg",
			//"patients/nn/nn",

			//"patients/MMPOST1_mc.stl",
			//"patients/MMPost2_mc.stl",

			//"msh-hexahedral/IanArteries5.GAMBIT.msh",

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

	//PlaySound( TEXT( "SystemStart" ), nullptr, SND_ALIAS );
	//SetForegroundWindow( GetConsoleWindow( ) );

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
			for(int i = 0; i < chunk.num_tris; i++ )
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

