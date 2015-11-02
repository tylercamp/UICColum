#pragma once

#include <string>

#include "../Workflow.h"
#include "../Types.h"

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
				//	How is this thing triangulated?
			}
		}

		m_Vertices.Assign( cpu_vertices );
		m_OuterIndices.Assign( cpu_indices );
	}
};