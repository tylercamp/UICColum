#pragma once

#include <memory>

#include "Workflow.h"

class WorkflowScheduler
{
	std::vector<std::unique_ptr<Workflow>> m_WorkflowQueue;

public:
	WorkflowScheduler( );
	WorkflowScheduler( std::shared_ptr<Workspace> workspace );

	void SetWorkspace( std::shared_ptr<Workspace> workspace );

	void AddWorkflow( std::unique_ptr<Workflow> workflow )
	{
		m_WorkflowQueue.push_back( std::move( workflow ) );
	}

	void Run( )
	{
		for( auto & workflow : m_WorkflowQueue )
		{
			workflow->Run( );
		}
	}
};