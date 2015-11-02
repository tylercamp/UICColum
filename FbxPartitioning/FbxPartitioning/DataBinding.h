#pragma once

#include "Types.h"

class DataBindingContract
{
public:

	enum AccessType
	{
		CPU, Mixed, GPU
	} AccessType;




};

template <typename T>
class DataBinding
{
public:
	template <typename container>
	const container & Resolve( );
};

template <typename T>
class DatumBinding
{
public:
	const T & Resolve( );
};



template <typename T>
class IngressDataBinding : public DataBinding<T>
{
public:
	IngressDataBinding( const DataBinding & source );
};

template <typename T>
class OutgressDataBinding : public DataBinding<T>
{
public:
	OutgressDataBinding( DataBinding * source );
};

template <typename T>
class IngressDatumBinding : public DatumBinding<T>
{
public:
	IngressDatumBinding( const DatumBinding & source );
};

template <typename T>
class OutgressDatumBinding : public DatumBinding<T>
{
public:
	OutgressDatumBinding( DatumBinding * source );
};