#pragma once

#include <memory>

#include "Workflow.h"

class WorkflowScheduler
{
	std::vector<Workflow *> m_WorkflowQueue;

public:
	WorkflowScheduler( ) { }
	WorkflowScheduler( std::shared_ptr<Workspace> workspace ) { SetWorkspace( workspace ); }

	void SetWorkspace( std::shared_ptr<Workspace> workspace )
	{
		//	TODO
		//NOT_YET_IMPLEMENTED( );
	}

	void AddWorkflow( Workflow * workflow )
	{
		m_WorkflowQueue.push_back( workflow );
	}

	void Run( )
	{
		for( auto & workflow : m_WorkflowQueue )
		{
			workflow->Run( );
		}
	}
};