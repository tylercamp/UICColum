#pragma once

#include <string>

#include "../MshReader.h"

#include "../Workflow.h"
#include "../Types.h"

void workflow_import_msh( const std::string & fileName, gpu_vertex_array ** out_points, gpu_index_array ** out_indices, gpu_index_array ** out_inner_indices )
{
	cpu_vertex_array cpu_vertices;
	cpu_index_array cpu_indices;
	cpu_index_array cpu_inner_indices;

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

		//	Ignore inner faces
		if( faceData.data[0].is_inner )
			target_indices = &cpu_inner_indices;
		else
			target_indices = &cpu_indices;

		for( std::size_t j = 0; j < faceData.data.size( ); j++ )
		{
			auto & face = faceData.data[j];

			target_indices->push_back( face.point_indices[0] - 1 );
			target_indices->push_back( face.point_indices[1] - 1 );
			target_indices->push_back( face.point_indices[2] - 1 );

			target_indices->push_back( face.point_indices[0] - 1 );
			target_indices->push_back( face.point_indices[2] - 1 );
			target_indices->push_back( face.point_indices[3] - 1 );
		}
	}

	*out_points = bindless_copy( cpu_vertices );
	*out_indices = bindless_copy( cpu_indices );
	*out_inner_indices = bindless_copy( cpu_inner_indices );
}






class MshImportWorkflow : public Workflow
{
	std::string m_TargetFileName;
	OutgressDataBinding<float_3> m_Vertices;
	OutgressDataBinding<int> m_OuterIndices;
	OutgressDataBinding<int> m_InnerIndices;

public:
	MshImportWorkflow( const std::string & fileName, OutgressDataBinding<float_3> vertices, OutgressDataBinding<int> outerIndices, OutgressDataBinding<int> innerIndices ) :
		m_TargetFileName( fileName ),
		m_Vertices( vertices ), m_OuterIndices( outerIndices ), m_InnerIndices( innerIndices )
	{

	}

	void Run( )
	{
		cpu_vertex_array cpu_vertices;
		cpu_index_array cpu_indices;

		MshReader * reader = new MshReader( m_TargetFileName );
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
			//	Ignore inner faces
			if( faceData.data[0].is_inner )
				continue;

			for( std::size_t j = 0; j < faceData.data.size( ); j++ )
			{
				auto & face = faceData.data[j];

				cpu_indices.push_back( face.point_indices[0] );
				cpu_indices.push_back( face.point_indices[1] );
				cpu_indices.push_back( face.point_indices[2] );

				cpu_indices.push_back( face.point_indices[0] );
				cpu_indices.push_back( face.point_indices[3] );
				cpu_indices.push_back( face.point_indices[2] );
			}
		}

		m_Vertices.Assign( cpu_vertices );
		m_OuterIndices.Assign( cpu_indices );
	}
};