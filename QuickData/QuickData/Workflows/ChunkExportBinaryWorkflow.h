#pragma once

#include "../Types.h"
#include "../BinaryMesh.h"

void save_chunk_binary( std::string filename, const mesh_chunk & chunk )
{
	int numTris = chunk.num_tris;

	BinaryMesh mesh;
	mesh.numTris = numTris;

	mesh.vertices = new float_3[numTris * 3];
	mesh.normals = new float_3[numTris * 3];
	mesh.volumes = new int[numTris * 3];

	ASSERT( chunk.tris.size( ) == chunk.num_tris );

	std::size_t dataIndex = 0;
	for( std::size_t i = 0; i < numTris; i++ )
	{
		const auto & tri = chunk.tris[i];

		mesh.vertices[dataIndex] = tri.a;
		mesh.normals[dataIndex] = tri.norm_a;
		mesh.volumes[dataIndex] = tri.volumeIndex;
		++dataIndex;

		mesh.vertices[dataIndex] = tri.b;
		mesh.normals[dataIndex] = tri.norm_b;
		mesh.volumes[dataIndex] = tri.volumeIndex;
		++dataIndex;

		mesh.vertices[dataIndex] = tri.c;
		mesh.normals[dataIndex] = tri.norm_c;
		mesh.volumes[dataIndex] = tri.volumeIndex;
		++dataIndex;
	}


	mesh.SaveTo( filename );
}

void workflow_chunk_export_binary( const std::string & targetFolder, cpu_chunk_array * chunks )
{
	std::cout << "Saving chunks... ";
	CreateDirectoryA( targetFolder.c_str( ), nullptr );
	//	http://stackoverflow.com/questions/1530760/how-do-i-recursively-create-a-folder-in-win32
	//system( ("mkdir " + targetFolder).c_str( ) );
	//SHCreateDirectoryExA( NULL, targetFolder.c_str( ), NULL );
	for( int i = 0; i < chunks->size( ); i++ )
	{
		std::ostringstream pathName;
		pathName << targetFolder << "/" << i << ".binmesh";
		auto & chunk = chunks->at( i );
		save_chunk_binary( pathName.str( ), chunk );
	}

	std::cout << "Done." << std::endl;
}
