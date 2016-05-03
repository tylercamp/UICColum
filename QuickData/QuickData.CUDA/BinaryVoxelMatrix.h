#pragma once

#include <iostream>
#include <map>
#include "Utilities.h"

const int MATRIX_FILE_START_TAG = 0x8273859;

class BinaryVoxelMatrix
{
private:
	template <typename T>
	std::size_t encode_rle( T * buffer, std::size_t bufferLength, std::int8_t * out_encoded, std::size_t outputBufferSize )
	{
		auto start = clock( );
		std::cout << "Beginning run-length encoding... ";
		std::size_t offset = 0;
		std::size_t currentRunStart = -1;
		T currentRunValue = T( );
		int numEncoded = 0;
		for( std::size_t i = 0; i < bufferLength; i++ )
		{
			if( currentRunStart == -1 )
			{
				currentRunStart = i;
				currentRunValue = buffer[i];
			}
			else
			{
				if( buffer[i] != currentRunValue )
				{
					std::size_t numInLastRun = i - currentRunStart;
					//std::cout << "Run length: " << numInLastRun << std::endl;
					*((std::size_t *)(out_encoded + offset)) = numInLastRun;
					offset += sizeof(std::size_t);

					*((T *)(out_encoded + offset)) = currentRunValue;
					offset += sizeof(T);

					//std::cout << "Encoded " << currentRunValue << "\n";

					numEncoded += numInLastRun;

					currentRunStart = i;
					currentRunValue = buffer[i];
				}
			}

			if( i == bufferLength - 1 )
			{
				if( currentRunStart == i )
				{
					*((std::size_t *)(out_encoded + offset)) = 1;
					offset += sizeof(std::size_t);

					*((T *)(out_encoded + offset)) = currentRunValue;
					offset += sizeof(T);

					numEncoded += 1;
				}
				else
				{
					*((std::size_t *)(out_encoded + offset)) = i - currentRunStart + 1;
					offset += sizeof(std::size_t);

					*((T *)(out_encoded + offset)) = currentRunValue;
					offset += sizeof(T);

					numEncoded += i - currentRunStart + 1;
				}
			}
		}

		if( numEncoded != bufferLength )
			__debugbreak( );

		std::cout << "Done. Took " << clock( ) - start << "ms" << std::endl;

		return offset;
	}

	template <typename T>
	std::size_t decode_rle( std::int8_t * encodedBuffer, std::size_t encodedBufferSize, T * out_decoded, std::size_t numDecodableElements )
	{
		std::size_t offset = 0;
		std::size_t numDecodedElements = 0;

		while( offset < encodedBufferSize )
		{
			std::size_t currentRunLength = *((std::size_t *)(encodedBuffer + offset));
			//std::cout << currentRunLength << std::endl;
			offset += sizeof(std::size_t);
			T value = *((T *)(encodedBuffer + offset));
			offset += sizeof(T);

			for( std::size_t v = 0; v < currentRunLength; v++ )
				out_decoded[numDecodedElements++] = value;
		}

		if( numDecodedElements != numDecodableElements )
			NOT_YET_IMPLEMENTED( );

		return offset;
	}


public:
	//	
	std::vector<double *> voxel_data;
	float3 spatial_start;
	float3 spatial_end;
	
	int resolution_x, resolution_y, resolution_z;

	//	Not yet implemented
	bool has_semantic_encoding;
	std::map<std::string, std::string> semantic_properties;


	BinaryVoxelMatrix( )
	{

	}

	~BinaryVoxelMatrix( )
	{
		for( auto arr : voxel_data )
			delete[] arr;
	}

	void SaveTo( const std::string & target )
	{
		auto f = fopen( target.c_str( ), "wb" );
		fwrite( &MATRIX_FILE_START_TAG, sizeof(int), 1, f );
		fwrite( &resolution_x, sizeof(int), 1, f );
		fwrite( &resolution_y, sizeof(int), 1, f );
		fwrite( &resolution_z, sizeof(int), 1, f );
		fwrite( &spatial_start, sizeof(float)* 3, 1, f );
		fwrite( &spatial_end, sizeof(float)* 3, 1, f );
		int numDims = voxel_data.size( );
		fwrite( &numDims, sizeof(float), 1, f );

		for( auto data : voxel_data )
		{
			int numData = resolution_x * resolution_y * resolution_z;
			int encodedBufferSize = numData * sizeof(double) / 2;
			std::int8_t * encodedData = new std::int8_t[encodedBufferSize];
			std::size_t encodedSize = encode_rle( data, numData, encodedData, encodedBufferSize );
			fwrite( &encodedSize, sizeof(std::size_t), 1, f );
			fwrite( encodedData, 1, encodedSize, f );
			delete[] encodedData;
		}

		fclose( f );
	}
};