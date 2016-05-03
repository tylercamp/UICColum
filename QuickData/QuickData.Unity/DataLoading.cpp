
//#include <fbxsdk.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <gdcm\gdcmReader.h>
#include <gdcm\gdcmImageReader.h>
#include "QuickData.Unity.h"

#include "../QuickData/BinaryMesh.h"
#include "../QuickData/VolumeMeshTimeline.h"


#pragma comment( lib, "gdcmCommon.lib" )
#pragma comment( lib, "gdcmDSED.lib" )
#pragma comment( lib, "gdcmzlib.lib" )
#pragma comment( lib, "gdcmcharls.lib" )
#pragma comment( lib, "gdcmDICT.lib" )
#pragma comment( lib, "gdcmexpat.lib" )
#pragma comment( lib, "gdcmgetopt.lib" )
#pragma comment( lib, "gdcmIOD.lib" )
#pragma comment( lib, "gdcmjpeg8.lib" )
#pragma comment( lib, "gdcmjpeg12.lib" )
#pragma comment( lib, "gdcmjpeg16.lib" )
#pragma comment( lib, "gdcmMEXD.lib" )
#pragma comment( lib, "gdcmMSFF.lib" )
#pragma comment( lib, "gdcmopenjpeg.lib" )
#pragma comment( lib, "socketxx.lib" )

#pragma comment( lib, "Ws2_32.lib" )




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

	//VolumeMeshTimeline timeline;
	//timeline.LoadFrom( "FullCNS_Drug_CellCenters_T1.dynamic.binvolumes" );

	//std::cout << "Loaded " << meshSet->meshes.size( ) << " meshes" << std::endl;


	//	512x512, 361 slices

	gdcm::ImageReader reader;
	reader.SetFileName( "dicom.dcm" );
	std::cout << "canRead: " << reader.CanRead( ) << std::endl;
	std::cout << "read: " << reader.Read( ) << std::endl;

	auto & pixmap = reader.GetPixmap( );
	auto & img = reader.GetImage( );
	//img.

	
	
	
	auto & dataElement = img.GetDataElement( );
	
	int numDims = img.GetNumberOfDimensions( );
	auto bufferLength = img.GetBufferLength( );

	auto pixformat = img.GetPixelFormat( );
	auto scalarType = pixformat.GetScalarType( );

	auto numcolumns = img.GetColumns( );
	auto numrows = img.GetRows( );
	auto dims = img.GetDimensions( );
	auto dim1 = dims[0];
	auto dim2 = dims[1];
	auto dim3 = dims[2];
	
	auto & header = reader.GetFile( ).GetHeader( );
	auto & dataset = reader.GetFile( ).GetDataSet( );

	
	for( auto it = dataset.Begin( ); it != dataset.End( ); it++ )
	{
		const auto & val = *it;
		auto asSequence = val.GetValueAsSQ( );
		auto asValue = val.GetValue( );
		const auto & length = val.GetLength( );
		auto val = val.GetByteValue( );
		auto vl = val.GetVL( );
		auto vr = val.GetVR( );

		switch( vr )
		{
			case( gdcm::VR::IS ) :
				__debugbreak( );
				break;
			default:
				break;
		}
	}

	auto planeconfig = img.GetPlanarConfiguration( );

	

	auto spacings = img.GetSpacing( );
	auto spacing1 = spacings[0];
	auto spacing2 = spacings[1];
	auto spacing3 = spacings[2];

	std::cout << "Took " << (clock( ) - start) << "ms";

	std::string j;
	std::getline( std::cin, j );

	return 0;
}

//#endif
