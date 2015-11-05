#pragma once

#include "Types.h"
#include "DataBinding.h"

//	Will be useful if the scheduler gets more advanced features
struct Workspace
{
	template <typename FirstType, typename... Remaining>
	void AddData( FirstType & firstData, Remaining&... data )
	{
		//NOT_YET_IMPLEMENTED( );
		AddData( data... );
	}

	void AddData( ) { }
};



class Workflow
{
public:
	virtual ~Workflow( ) { }

	virtual void Run( ) = 0;

protected:

	//void AddIngress( DataBinding * dataIngress );
	//void AddOutgress( DataBinding * dataOutgress );
};