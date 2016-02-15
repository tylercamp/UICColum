#pragma once

#include "../BinaryVoxelMatrix.h"

//	Encodes the tag type as voxel data by using floor(val) as the tag index, and the
//		extra precision to indicate relative strength in that tag (between [floor(val) floor(val)+1))
void workflow_export_voxel_matrix_data( const std::string & file, voxel_matrix * vm, bool includeRawTagData = false )
{
	BinaryVoxelMatrix bvm;
	bvm.resolution = int_3( vm->width, vm->height, vm->depth );
	bvm.spatial_start = vm->start;
	bvm.spatial_end = vm->end;

	//	TODO: Full export of voxel_matrix structure
	//	TODO - IMPORTANT - ENFORCE SOME STANDARD OF DATA ORDER

	double * cpu_data = new double[bvm.resolution.x * bvm.resolution.y * bvm.resolution.z];

	std::vector<int> tagBounds;
	tagBounds.resize( vm->dev_voxel_tag_data.size( ) );
	for( int x = 0; x < bvm.resolution.x; x++ )
	{
		for( int y = 0; y < bvm.resolution.y; y++ )
		{
			for( int z = 0; z < bvm.resolution.z; z++ )
			{
				for( int t = 0; t < tagBounds.size( ); t++ )
				{
					auto tags = vm->dev_voxel_tag_data[t];
					tagBounds[t] = max(tagBounds[t], (*tags)( x, y, z ));
				}
			}
		}
	}


	int idx = 0;
	std::vector<int> tagCounts( vm->dev_voxel_tag_data.size( ) );
	for( int x = 0; x < bvm.resolution.x; x++ )
		for( int y = 0; y < bvm.resolution.y; y++ )
			for( int z = 0; z < bvm.resolution.z; z++, idx++ )
			{ 
				int maxTags = 0, maxIndex = -1;
				for( int tagIndex = 0; tagIndex < vm->dev_voxel_tag_data.size( ); tagIndex++ )
				{
					int currentTags = vm->dev_voxel_tag_data[tagIndex]->operator()( x, y, z );
					tagCounts[tagIndex] = currentTags;

					if( currentTags > maxTags )
					{
						maxTags = currentTags;
						maxIndex = tagIndex;
					}
				}

				if( maxIndex < 0 )
				{
					cpu_data[idx] = -1;
				}
				else
				{
					int largestVal = vm->dev_voxel_tag_data[maxIndex]->operator()( x, y, z );
					float ref = tagBounds[maxIndex] * 0.5f;

					float membership = ( largestVal > ref ? 1.0 : largestVal / ref );
					//std::cout << "Membership: " << membership << "\t";

					cpu_data[idx] = maxIndex + membership;

					//	Take the most significant tag, and its membership to its significant tag with respect to
					//		all associated tags becomes the membership function
					//cpu_data[idx] = largestVal / ( sqrt( sumTags2 ) + 1e-3f ) + maxIndex;
				}
			}

	bvm.voxel_data.push_back( cpu_data );

	bvm.SaveTo( file );
}