
//#include <fbxsdk.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "QuickData.Unity.h"

#include "../QuickData/BinaryMesh.h"
#include "../QuickData/VolumeMeshTimeline.h"



ParsedMeshStructure * LoadBinaryMesh( const char * targetPath )
{
	BinaryMesh mesh( targetPath );

	ParsedMeshStructure * result = new ParsedMeshStructure( );
	result->path = targetPath;
	result->numTris = mesh.numTris;
	//	float_3 data should just be contiguous floats
	result->colorData = reinterpret_cast<float *>( mesh.colors );
	result->normalData = reinterpret_cast<float *>( mesh.normals );
	result->vertexData = reinterpret_cast<float *>( mesh.vertices );
	result->volumeData = mesh.volumes;

	//	Take ownership of these arrays, avoid copy
	mesh.colors = nullptr;
	mesh.normals = nullptr;
	mesh.vertices = nullptr;
	mesh.volumes = nullptr;


	return result;
}


void CompareTimelines( const VolumeMeshTimeline & ref, const VolumeMeshTimeline & timeline )
{
	if( timeline.states.size( ) != ref.states.size( ) )
	{
		std::cout << "Timeline does not match ref! Ref has " << ref.states.size( ) << " states, timeline has " << timeline.states.size( ) << std::endl;
		//return;
	}

	for( int s = 0; s < ref.states.size( ) && s < timeline.states.size( ); s++ )
	{
		auto refState = ref.states[s];
		auto state = timeline.states[s];

		for( int i = 0; i < refState->numVolumes; i++ )
		{
			for( int d = 0; d < refState->numDimensions; d++ )
			{
				auto refVal = refState->dataStore[i * refState->numDimensions + d];
				auto val = state->dataStore[i * refState->numDimensions + d];

				if( refVal != val )
				{
					std::cout << "Inconsistency! Data element " << i << " on dimension " << d << " in state " << s << " mismatched! expected: " << refVal << ", got: " << val << std::endl;
					//return;
				}
			}
		}
	}
	
	std::cout << "Timelines match!" << std::endl;
}





bool FileExists( const std::string & path )
{
	std::ifstream f( path );
	bool exists = f.is_open( );
	f.close( );
	return exists;
}

//#ifdef _DEBUG
int main( )
{
	auto start = clock( );
	//auto meshSet = LoadMeshSet( "E:\\Dropbox\\HotRepos\\UICColum\\QuickData\\FbxPartitioning\\bsCSF\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "C:\\Users\\algor\\Dropbox\\hotrepos\\UICColum\\QuickData\\FbxPartitioning\\IanArteries5.GAMBIT\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "E:\\Dropbox\\HotRepos\\UICColum\\QuickData\\FbxPartitioning\\IanArteries5.GAMBIT\\volumes\\", "*.binmesh" );
	//LoadMeshSet( "MESHES\\IanArteries5.GAMBIT\\surfaces\\", "*.binmesh" );
	//LoadMeshSet( "../FbxPartitioning/patients/bs/bsCSF/surfaces/", "*.binmesh" );

	VolumeMeshTimeline timeline;
	//timeline.LoadFrom( "FullCNS_Drug_CellCenters_T1.dynamic.binvolumes" );
	//timeline.LoadFrom( "CNSTest-1-00051.dat.binvolumes" );
	timeline.LoadFrom( "../QuickData/patients/KevinBestV7_ForGrant-11-2.0501.dat.binvolumes" );

	VolumeMeshTimeline timeline2;
	//timeline2.LoadFrom( "CNSTest-1-00051.dat - Copy.binvolumes" );
	timeline2.LoadFrom( "../QuickData/patients/CORRECT REF KevinBestV7_ForGrant-11-2.0501.dat.binvolumes" );

	CompareTimelines( timeline2, timeline );

	//std::cout << "Loaded " << meshSet->meshes.size( ) << " meshes" << std::endl;

	std::getchar( );


	return 0;
}

//#endif
