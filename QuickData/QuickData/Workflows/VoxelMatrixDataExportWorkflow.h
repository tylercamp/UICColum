#pragma once

#include "../BinaryVoxelMatrix.h"

void workflow_export_voxel_matrix_data( const std::string & file, voxel_matrix * vm )
{
	BinaryVoxelMatrix bvm;
	bvm.resolution = int_3( vm->width, vm->height, vm->depth );
	bvm.spatial_start = vm->start;
	bvm.spatial_end = vm->end;

	//	TODO: Full export of voxel_matrix structure
	//	TODO - IMPORTANT - ENFORCE SOME STANDARD OF DATA ORDER

	double * cpu_data = new double[bvm.resolution.x * bvm.resolution.y * bvm.resolution.z];
	int idx = 0;
	for( int x = 0; x < bvm.resolution.x; x++ )
		for( int y = 0; y < bvm.resolution.y; y++ )
			for( int z = 0; z < bvm.resolution.z; z++, idx++ )
			{
				int val = vm->dev_voxel_tag_data[0]->operator()( x, y, z );
				cpu_data[idx] = val;
			}

	bvm.voxel_data.push_back( cpu_data );

	bvm.SaveTo( file );
}