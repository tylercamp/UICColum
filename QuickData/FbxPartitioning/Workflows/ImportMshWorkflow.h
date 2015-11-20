
#pragma once

#include <string>

#include "../MshReader.h"
#include "../Types.h"



void workflow_import_msh( const std::string & fileName, gpu_vertex_array ** out_points, gpu_index_array ** out_indices, gpu_index_array ** out_inner_indices, gpu_index_array ** out_volume_indices, gpu_index_array ** out_inner_volume_indices )
{
	cpu_vertex_array cpu_vertices;
	cpu_index_array cpu_indices;
	cpu_index_array cpu_inner_indices;
	cpu_index_array cpu_volume_tags;
	cpu_index_array cpu_inner_volume_tags;

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

		cpu_index_array * target_indices;
		cpu_index_array * target_volume_indices;

		//	Ignore inner faces
		if( faceData.data[0].is_inner )
		{
			target_indices = &cpu_inner_indices;
			target_volume_indices = &cpu_inner_volume_tags;
		}
		else
		{
			target_indices = &cpu_indices;
			target_volume_indices = &cpu_volume_tags;
		}

		for( std::size_t j = 0; j < faceData.data.size( ); j++ )
		{
			auto & face = faceData.data[j];

			switch( face.type )
			{
			case( MSH_TETRAHEDRON ):
			{
				target_indices->push_back( face.point_indices[0] - 1 );
				target_indices->push_back( face.point_indices[1] - 1 );
				target_indices->push_back( face.point_indices[2] - 1 );

				target_volume_indices->push_back( face.point_indices[3] );

				if( face.point_indices[4] != 0 )
				{
					//	Generate another triangle, generate another volume index (with [4])
				}
				break;
			}

			case( MSH_HEXAHEDRON ):
			{
				target_indices->push_back( face.point_indices[0] - 1 );
				target_indices->push_back( face.point_indices[1] - 1 );
				target_indices->push_back( face.point_indices[2] - 1 );

				target_indices->push_back( face.point_indices[0] - 1 );
				target_indices->push_back( face.point_indices[2] - 1 );
				target_indices->push_back( face.point_indices[3] - 1 );

				target_volume_indices->push_back( face.point_indices[4] );

				if( face.point_indices[5] != 0 )
				{
					//	Generate another triangle, generate another volume index (with [5])
				}
				break;
			}
			}
		}
	}

	*out_points = bindless_copy( cpu_vertices );
	*out_indices = bindless_copy( cpu_indices );
	*out_inner_indices = bindless_copy( cpu_inner_indices );
	*out_volume_indices = bindless_copy( cpu_volume_tags );
	*out_inner_volume_indices = bindless_copy( cpu_inner_volume_tags );
}
