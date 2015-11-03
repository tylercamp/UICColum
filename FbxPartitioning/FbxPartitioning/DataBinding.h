#pragma once

#include "Types.h"

template <typename T>
class DataBindingContract
{
public:
	enum AccessType
	{
		CPU, Mixed, GPU
	} AccessType;

	vget( bool, IsResolved ) { return false; }

	virtual ~DataBindingContract( ) { }
};

template <typename T>
class ResolvedDataBindingContract : public DataBindingContract<T>
{
public:

	ResolvedDataBindingContract( const T & value )
	{
		throw nullptr;
	}

	ResolvedDataBindingContract( const DataBindingContract<T> & ref )
	{
		throw nullptr;
	}

	ResolvedDataBindingContract( std::vector<T> & data )
	{
		//m_Storage
		throw nullptr;
	}

	ResolvedDataBindingContract( concurrency::array_view<T> & data )
	{
		throw nullptr;
	}

	virtual ~ResolvedDataBindingContract( ) { }

	vget( bool, IsResolved ) override { return true; }



	std::vector<T> & AsStandardVector( )
	{
		return *(m_Storage->asVector);
	}

	concurrency::array_view<T, 1> & AsArrayView( )
	{
		return *(m_Storage->asArrayView);
	}

	enum StorageType
	{
		Unknown,
		SingleValue,
		StandardVector,
		ConcurrencyArrayView,
		ConcurrencyArray,
		ManuallyStagedArray
	} CurrentStorageType;

private:
#pragma warning( push )
#pragma warning( disable: 4624 ) // destructor was implicitly defined as deleted because a base class destructor is inaccessible or deleted
	union StorageUnion {
		T asSingleValue;

		std::vector<T> * asVector;
		concurrency::array_view<T, 1> * asArrayView;
		concurrency::array<T, 1> * asArray;
	} * m_Storage;
#pragma warning( pop )
};



template <typename T>
class DataBinding
{
public:
	DataBinding( ) : m_Contract( nullptr )
	{
	}

	template <typename container>
	container & Resolve( );
	
	template <>
	std::vector<T> & Resolve<std::vector<T>>( )
	{
		return ResolvedContract->AsStandardVector( );
	}

	template <>
	concurrency::array_view<T, 1> & Resolve<concurrency::array_view<T, 1>>( )
	{
		return ResolvedContract->AsArrayView( );
	}



	void Assign( std::vector<T> & data )
	{
		if( m_Contract != nullptr ) {
			delete m_Contract;
			m_Contract = nullptr;
		}

		m_Contract = new ResolvedDataBindingContract<T>( data );
	}

	void Assign( concurrency::array_view<T> & data )
	{
		if( m_Contract != nullptr ) {
			delete m_Contract;
			m_Contract = nullptr;
		}

		m_Contract = new ResolvedDataBindingContract<T>( data );
	}

protected:
	DataBindingContract<T> * m_Contract;

	template <typename U>
	friend class IngressDataBinding;
	template <typename U>
	friend class OutgressDataBinding;

private:

	get( bool, ContractIsInitialized ) { return m_Contract == nullptr ? false : m_Contract->IsResolved; }
	get( ResolvedDataBindingContract<T> *, ResolvedContract ) { return ContractIsInitialized ? (ResolvedDataBindingContract<T> *)m_Contract : nullptr; }
};


template <typename T>
class DatumBinding
{
public:
	DatumBinding( ) : m_Contract( nullptr )
	{
	}

	T & Resolve( )
	{
		throw nullptr;
	}


	void Assign( const T & value )
	{
		if( m_Contract != nullptr ) {
			delete m_Contract;
			m_Contract = nullptr;
		}

		m_Contract = new ResolvedDataBindingContract<T>( value );
	}

protected:
	DataBindingContract<T> * m_Contract;

	template <typename U>
	friend class IngressDatumBinding;
	template <typename U>
	friend class OutgressDatumBinding;

private:

	get( bool, ContractIsInitialized ) { return m_Contract == nullptr ? false : m_Contract->IsResolved; }
	get( ResolvedDataBindingContract<T> *, ResolvedContract ) { return ContractIsInitialized ? (ResolvedDataBindingContract<T> *)m_Contract : nullptr; }
};



template <typename T>
class IngressDataBinding : public DataBinding<T>
{
public:
	IngressDataBinding( const DataBinding & source )
	{
		this->m_Contract = source.m_Contract;
	}
};

template <typename T>
class OutgressDataBinding : public DataBinding<T>
{
public:
	OutgressDataBinding( DataBinding<T> * source )
	{
		this->m_Contract = source->m_Contract;
	}
};

template <typename T>
class IngressDatumBinding : public DatumBinding<T>
{
public:
	IngressDatumBinding( const DatumBinding & source )
	{
		//this->m_Contract = source.m_Contract;
	}
};

template <typename T>
class OutgressDatumBinding : public DatumBinding<T>
{
public:
	OutgressDatumBinding( DatumBinding * source )
	{
		//this->m_Contract = source->m_Contract;
	}
};