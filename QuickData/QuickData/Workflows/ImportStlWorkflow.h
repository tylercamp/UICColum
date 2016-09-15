
#include "Types.h"
#include "Utilities.h"

void workflow_import_stl( const std::string & file, gpu_triangle_array ** out_tris )
{
	std::size_t size = getFileSize( file );
	char * data = new char[size];
	FILE * f = fopen( file.c_str( ), "rb" );
	fread( data, 1, size, f );
	fclose( f );


	char * pos = data + 80; // skip header
	std::size_t numTris = *((unsigned int *)pos);
	pos += 4;

	if( 84 + numTris * 50 != size )
	{
		std::cout << "Non-binary STL files not yet implemented!";
		NOT_YET_IMPLEMENTED( );
	}

	std::vector<float_3> norms;
	norms.reserve( numTris );
	std::vector<float_3> verts;
	verts.reserve( numTris * 3 );

	cpu_triangle_array tris;
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

	*out_tris = bindless_copy( tris );
}