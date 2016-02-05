
#pragma once

#include <string>

#include "../MshReader.h"
#include "../Types.h"


//	Returns number of volumes in the MSH file
int workflow_import_msh( const std::string & fileName, gpu_vertex_array ** out_points, gpu_index_array ** out_surface_indices, gpu_index_array ** out_surface_tags, gpu_index_array ** out_volume_indices, gpu_index_array ** out_volume_tags )
{
	cpu_vertex_array cpu_vertices;
	cpu_index_array cpu_surface_indices;
	cpu_index_array cpu_surface_tags;
	cpu_index_array cpu_volume_indices;
	cpu_index_array cpu_volume_tags;

	MshReader * reader = new MshReader( fileName );
	//	Generally no more than one set of point data
	if( reader->PointData.size( ) > 1 )
		__debugbreak( );

	auto & pointData = reader->PointData[0];
	for( std::size_t i = 0; i < pointData.count; i++ )
	{
		auto & currentPoint = pointData.data[i];
		cpu_vertices.push_back( float_3( currentPoint.data[0], currentPoint.data[1], currentPoint.data[2] ) );
	}

	for( std::size_t i = 0; i < reader->FaceData.size( ); i++ )
	{
		auto & faceData = reader->FaceData[i];

		for( std::size_t j = 0; j < faceData.data.size( ); j++ )
		{
			auto & face = faceData.data[j];

			switch( face.type )
			{
			case( MSH_TETRAHEDRON ):
			{
				if( face.is_inner )
				{
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[3] - 1 );

					//	Generate another face, tag it with the other volume index (with [4])
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[4] - 1 );
				}
				else
				{
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[3] - 1 );



					//	Generate a face for the surface data (with [3])
					cpu_surface_indices.push_back( face.point_indices[2] - 1 );
					cpu_surface_indices.push_back( face.point_indices[1] - 1 );
					cpu_surface_indices.push_back( face.point_indices[0] - 1 );

					//	The volume associated with this face
					cpu_surface_tags.push_back( face.point_indices[3] - 1 );
				}
				break;
			}

			case( MSH_HEXAHEDRON ):
			{
				if( face.is_inner )
				{
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_indices.push_back( face.point_indices[3] - 1 );
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[4] - 1 );
					cpu_volume_tags.push_back( face.point_indices[4] - 1 );



					//	Generate another face, tag it with the other volume index (with [5])
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_indices.push_back( face.point_indices[3] - 1 );
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[5] - 1 );
					cpu_volume_tags.push_back( face.point_indices[5] - 1 );
				}
				else
				{
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[1] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_indices.push_back( face.point_indices[3] - 1 );
					cpu_volume_indices.push_back( face.point_indices[2] - 1 );
					cpu_volume_indices.push_back( face.point_indices[0] - 1 );

					cpu_volume_tags.push_back( face.point_indices[4] - 1 );
					cpu_volume_tags.push_back( face.point_indices[4] - 1 );



					//	Generate a face for the surface data (with [5])
					cpu_surface_indices.push_back( face.point_indices[2] - 1 );
					cpu_surface_indices.push_back( face.point_indices[1] - 1 );
					cpu_surface_indices.push_back( face.point_indices[0] - 1 );

					cpu_surface_indices.push_back( face.point_indices[3] - 1 );
					cpu_surface_indices.push_back( face.point_indices[2] - 1 );
					cpu_surface_indices.push_back( face.point_indices[0] - 1 );

					//	The volume associated with this face
					cpu_surface_tags.push_back( face.point_indices[4] - 1 );
					cpu_surface_tags.push_back( face.point_indices[4] - 1 );
				}

				break;
			}
			}
		}
	}

	*out_points = bindless_copy( cpu_vertices );
	*out_surface_indices = bindless_copy( cpu_surface_indices );
	*out_surface_tags = bindless_copy( cpu_surface_tags );
	*out_volume_indices = bindless_copy( cpu_volume_indices );
	*out_volume_tags = bindless_copy( cpu_volume_tags );

	return reader->VolumeCount;
}