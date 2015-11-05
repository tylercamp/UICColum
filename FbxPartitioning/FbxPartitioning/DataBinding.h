#pragma once

#include "Types.h"



template <typename T>
class ResolvedDataBinding;

template <typename T>
class DataBindingContract
{
	ResolvedDataBinding<T> * m_ResolvedContract;

public:
	enum AccessType
	{
		CPU, Mixed, GPU
	} AccessType;

	DataBindingContract( ) : m_ResolvedContract( nullptr )
	{
	}

	~DataBindingContract( )
	{
		if( IsResolved )
			Nullify( );
	}

	get( bool, IsResolved ) { return m_ResolvedContract != nullptr; }
	get( ResolvedDataBinding<T> &, ResolvedBinding )
	{
		if( !IsResolved )
			NOT_YET_IMPLEMENTED( );

		return *m_ResolvedContract;
	}

	void Nullify( )
	{
		if( m_ResolvedContract == nullptr )
			return;

		NOT_YET_IMPLEMENTED( );
	}

	virtual ~DataBindingContract( ) { }
};

template <typename T>
class ResolvedDataBinding
{
public:

	ResolvedDataBinding( const T & value )
	{
		m_Storage = new StorageUnion( );
		m_Storage->asSingleValue = new T( value );

		CurrentStorageType = SingleValue;
	}

	ResolvedDataBinding( const ResolvedDataBinding<T> & ref )
	{
		NOT_YET_IMPLEMENTED( );
	}

	ResolvedDataBinding( std::vector<T> & data )
	{
		m_Storage = new StorageUnion( );
		m_Storage->asVector = new std::vector<T>( data );
		CurrentStorageType = StandardVector;
	}

	ResolvedDataBinding( concurrency::array_view<T> & data )
	{
		m_Storage = new StorageUnion( );
		m_Storage->asArrayView = new concurrency::array_view<T>( data );
		CurrentStorageType = ConcurrencyArrayView;
	}

	virtual ~ResolvedDataBinding( )
	{
		if( m_Storage == nullptr )
			NOT_YET_IMPLEMENTED( ); // should never happen

		switch( CurrentStorageType )
		{
		case(SingleValue) :
			delete m_Storage->asSingleValue;
			break;
		case( StandardVector ):
			delete m_Storage->asVector;
			break;
		case( ConcurrencyArrayView ):
			//delete m_Storage->asArrayView;
			break;
		}

		free( m_Storage );
		m_Storage = nullptr;
	}

	vget( bool, IsResolved ) override { return true; }



	vget( std::vector<T> &, AsStandardVector )
	{
		if( CurrentStorageType != StandardVector )
			__debugbreak( );

		return *(m_Storage->asVector);
	}

	vget( concurrency::array_view<T> &, AsArrayView )
	{
		if( CurrentStorageType != ConcurrencyArrayView )
			__debugbreak( );

		return *(m_Storage->asArrayView);
	}

	vget( T &, AsSingleValue )
	{
		if( CurrentStorageType != SingleValue )
			__debugbreak( );

		return *(m_Storage->asSingleValue);
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
		T * asSingleValue;
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

	~DataBinding( )
	{
		NOT_YET_IMPLEMENTED( );
		if( m_Contract != nullptr )
			delete m_Contract;
		m_Contract = nullptr;
	}

	template <typename container>
	container & Resolve( );

	template <typename container, typename param>
	container & Resolve( const param & );
	
	template <>
	inline std::vector<T> & Resolve<std::vector<T>>( )
	{
		if( !ContractIsInitialized ) {
			//	Cannot resolve when data has not been assigned
			NOT_YET_IMPLEMENTED( );

			/*
			if( m_Contract != nullptr )
			delete m_Contract;
			m_Contract = nullptr;
			//	Init new resolved contract
			*/
		}

		return ResolvedContract.AsStandardVector;
	}

	template <>
	inline concurrency::array_view<T, 1> & Resolve<concurrency::array_view<T, 1>>( )
	{
		if( !ContractIsInitialized )
		{
			//	Cannot resolve when data has not been assigned
			NOT_YET_IMPLEMENTED( );

			/*
			if( m_Contract != nullptr )
			delete m_Contract;
			m_Contract = nullptr;
			//	Init new resolved contract
			*/
		}

		return ResolvedContract.AsArrayView;
	}

	//	Provide a version for when new extent size is relevant
	template <>
	inline concurrency::array_view<T, 1> & Resolve<concurrency::array_view<T, 1>>( const extent<1> & extent )
	{
		if( !ContractIsInitialized ) {
			if( m_Contract != nullptr )
			{
				//	Not sure why this would happen
				NOT_YET_IMPLEMENTED( );
				delete m_Contract;
			}

			m_Contract = nullptr;
			//	Init new resolved contract
			NOT_YET_IMPLEMENTED( );
			//m_Contract = new ResolvedDataBindingContract<T>( con )
		}

		return ResolvedContract.AsArrayView;
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
	get( ResolvedDataBinding<T> &, ResolvedContract ) { return ContractIsInitialized ? m_Contract->ResolvedBinding : nullptr; }
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
		if( !ContractIsInitialized ) {
			//	Cannot resolve when data has not been assigned
			__debugbreak( );

			/*
			if( m_Contract != nullptr )
				delete m_Contract;
			m_Contract = nullptr;
			m_Contract = new ResolvedDataBindingContract<T>( T( ) );
			*/
		}

		return ResolvedContract.AsSingleValue;
	}


	void Assign( const T & value )
	{
		if( ContractIsInitialized )
		{
			if( m_Contract != nullptr ) {
				delete m_Contract;
				m_Contract = nullptr;
			}

			m_Contract = new ResolvedDataBindingContract<T>( T( ) );
		}

		auto & valueStorage = ResolvedContract.AsSingleValue;
		valueStorage = value;
	}

protected:
	DataBindingContract<T> * m_Contract;

	template <typename U>
	friend class IngressDatumBinding;
	template <typename U>
	friend class OutgressDatumBinding;

private:

	get( bool, ContractIsInitialized ) { return m_Contract == nullptr ? false : m_Contract->IsResolved; }
	get( ResolvedDataBinding<T> &, ResolvedContract ) { return ContractIsInitialized ? m_Contract-ResolvedBinding : nullptr; }
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
		this->m_Contract = source.m_Contract;
	}
};

template <typename T>
class OutgressDatumBinding : public DatumBinding<T>
{
public:
	OutgressDatumBinding( DatumBinding * source )
	{
		this->m_Contract = source->m_Contract;
	}
};