
#include "Types.h"
#include "Utilities.h"

void fill_tris_binary( char * data, std::size_t length, cpu_triangle_array * out_tris )
{
	char * pos = data + 80; // skip header
	std::size_t numTris = *((unsigned int *)pos);
	pos += 4;

	auto & tris = *out_tris;
	tris.resize( numTris );

	for( std::size_t i = 0; i < numTris; i++ )
	{
		float * norm = (float *)pos;
		float * v1 = (float *)(pos + 1 * 12);
		float * v2 = (float *)(pos + 2 * 12);
		float * v3 = (float *)(pos + 3 * 12);

		auto & tri = tris[i];

		auto vnorm = float_3( norm[0], norm[1], norm[2] );
		auto vv1 = float_3( v1[0], v1[1], v1[2] );
		auto vv2 = float_3( v2[0], v2[1], v2[2] );
		auto vv3 = float_3( v3[0], v3[1], v3[2] );

		tri.norm_a = vnorm;
		tri.norm_b = vnorm;
		tri.norm_c = vnorm;

		tri.a = vv1;
		tri.b = vv2;
		tri.c = vv3;

		pos += 4 * 12 + 2;
	}
}

void fill_tris_text( char * data, std::size_t length, cpu_triangle_array * out_tris )
{
	char * lineStart = data, * lineEnd;

	auto & tris = *out_tris;
	tris.reserve( length / 50 );

	int numVerts = 0;
	float_3 verts[3], norm;
	bool triStarted = false;

	std::string as_str;

	while( lineEnd = strchr( lineStart, '\n' ) )
	{
		as_str.assign( lineStart, lineEnd - lineStart );
		if( triStarted )
		{
			if( as_str.find( "vertex" ) != as_str.npos )
			{
				float x, y, z;
				sscanf( as_str.c_str( ) + as_str.find_first_not_of( ' ' ), "vertex %f %f %f", &x, &y, &z );
				verts[numVerts++] = float_3( x, y, z );
			}
			else if( as_str.find( "endloop" ) != as_str.npos )
			{
				numVerts = 0;
				triStarted = false;

				triangle tri;
				tri.norm_a = norm;
				tri.norm_b = norm;
				tri.norm_c = norm;
				tri.a = verts[0];
				tri.b = verts[1];
				tri.c = verts[2];

				tris.emplace_back( std::move( tri ) );
			}
		}
		else
		{
			if( as_str.find( "facet normal" ) != as_str.npos )
			{
				triStarted = true;
				float x, y, z;
				sscanf( as_str.c_str( ) + as_str.find_first_not_of( ' ' ), "facet normal %f %f %f", &x, &y, &z );
				norm = float_3( x, y, z );
			}
		}

		lineStart = lineEnd + 1;
	}
}

void workflow_import_stl( const std::string & file, gpu_triangle_array ** out_tris )
{
	std::size_t size = getFileSize( file );
	char * data = new char[size];
	FILE * f = fopen( file.c_str( ), "rb" );
	fread( data, 1, size, f );
	fclose( f );

	std::size_t numTris = *((unsigned int *)(data + 80));

	cpu_triangle_array tris;
	if( 84 + numTris * 50 == size )
	{
		fill_tris_binary( data, size, &tris );
		delete data;
	}
	else
	{
		//	Re-load as text
		delete data;
		size = getTextFileSize( file );
		data = new char[size + 1];
		f = fopen( file.c_str( ), "r" );
		fread( data, 1, size, f );
		fclose( f );
		data[size] = 0;
		fill_tris_text( data, size, &tris );
		delete data;
	}

	*out_tris = bindless_copy( tris );
}