#pragma once

#include "../Workflow.h"
#include "../Types.h"

class TriangulationWorkflow : public Workflow
{
	IngressDataBinding<float_3> m_Vertices;
	IngressDataBinding<int> m_Indices;
	OutgressDataBinding<triangle> m_Mesh;

	void cpu_generate_triangle_list( cpu_triangle_array & dev_target, cpu_vertex_array & dev_verts, cpu_index_array & dev_indices )
	{
		for( int tri = 0; tri < dev_indices.size( ) / 3; tri++ )
		{
			triangle new_tri;
			new_tri.a = dev_verts[dev_indices[tri * 3 + 0]];
			new_tri.b = dev_verts[dev_indices[tri * 3 + 1]];
			new_tri.c = dev_verts[dev_indices[tri * 3 + 2]];

			dev_target.emplace_back( new_tri );
		}
	}

public:
	TriangulationWorkflow( IngressDataBinding<float_3> vertices, IngressDataBinding<int> indices, OutgressDataBinding<triangle> mesh ) :
		m_Vertices( vertices ), m_Indices( indices ),
		m_Mesh( mesh )
	{

	}

	void Run( )
	{
		auto & verts = m_Vertices.Resolve<gpu_vertex_array>( );
		auto & indices = m_Indices.Resolve<gpu_index_array>( );
		auto & mesh = m_Mesh.Resolve<gpu_triangle_array>( );

		//	Assumes the extent of dev_target was properly assigned to reflect the size of dev_indices
		segmented_parallel_for_each(
			mesh,
			[=]( index<1> idx ) restrict( amp )
		{
			auto & tri = mesh[idx];

			tri.a = verts[indices[idx[0] * 3 + 0]];
			tri.b = verts[indices[idx[0] * 3 + 1]];
			tri.c = verts[indices[idx[0] * 3 + 2]];
		} );
	}
};