#pragma once

#ifdef _DEBUG

#include "../Types.h"

#include <Windows.h>
#include <SDL.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#define PREVIEW_MESH_HIDPI



//	Code to avoid immediate-mode, TODO: Integrate
/*

GLuint vertArrayId;

bool didInit = false;
bool didLoadData = false;
const int maxBufferCount = 0xFFFF / (sizeof(triangle)/8);
std::vector<GLuint> vertBuffers;
void initDraw( int numTris )
{
	glewInit( );

	glGenVertexArrays( 1, &vertArrayId );
	glBindVertexArray( vertArrayId );

	int numBuffers = ceil( static_cast<float>(numTris) / maxBufferCount );
	vertBuffers.resize( numBuffers );
	glGenBuffers( numBuffers, vertBuffers.data( ) );

	didInit = true;
}

void draw_mesh( const cpu_triangle_array & tris )
{
	if( !didInit )
		initDraw( tris.size( ) );

	if( !didLoadData )
	{
		for( int b = 0; b < vertBuffers.size( ); b++ )
		{
			int buffer = vertBuffers[b];

			int offset = b * maxBufferCount;
			int count = min( maxBufferCount, tris.size( ) - offset );

			auto data = tris.data( ) + offset;

			glBindBuffer( GL_ARRAY_BUFFER, buffer );
			glBufferData( GL_ARRAY_BUFFER, count * sizeof( triangle ), data, GL_STATIC_DRAW );
			glFlush( );
		}

		didLoadData = true;
	}

	glEnableVertexAttribArray( 0 );
	for( int i = 0; i < vertBuffers.size( ); i++ )
	{
		int count = min( maxBufferCount, tris.size( ) - i * maxBufferCount );

		glBindBuffer( GL_ARRAY_BUFFER, vertBuffers[i] );
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, nullptr );
		glDrawArrays( GL_TRIANGLES, 0, 3 * count );
	}
	glDisableVertexAttribArray( 0 );
}

void draw_mesh_safe( const cpu_triangle_array & tris )
{
	glBegin( GL_TRIANGLES );
	for( int i = 0; i < tris.size( ); i++ )
	{
		glVertex3f( tris[i].a.x, tris[i].a.y, tris[i].a.z );
		glVertex3f( tris[i].b.x, tris[i].b.y, tris[i].b.z );
		glVertex3f( tris[i].c.x, tris[i].c.y, tris[i].c.z );
	}
	glEnd( );
}

*/






/* Windowing functions */
//	Window/rendering only for debugging, final application should just be a processing tool.

//	Multiple variations on run_window, where you can pass in a list of triangles or points, on the CPU or GPU

void run_window( const cpu_triangle_array & tris )
{
	int width = 1920;
	int height = 1080;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.5;
	height *= 1.5;
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 10000.0 );

	glEnable( GL_DEPTH_TEST );
	//glDepthFunc( GL_LEQUAL );
	glEnable( GL_CULL_FACE );

	float ox = 0.0f, oy = 0.0f, oz = 0.0f;

	glMatrixMode( GL_MODELVIEW );
	//glTranslatef( -ox, -oy, -oz );
	//glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
	//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	bool run = true;
	while( run )
	{
		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
		glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );
		glScalef( 50.0f, 50.0f, 50.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		//const float scale = 100.0f;
		const float scale = 1.0f;

		glBegin( GL_TRIANGLES );
		//glBegin( GL_POINTS );
		unsigned long i = 0;
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & tri : tris )
		{
			++i;
			int c_r = (i & 0xFF);
			int c_g = (i & 0xFF00) >> 8;
			int c_b = ((i * 3) & 0xFF0000) >> 16;
			const auto & a = tri.a * scale;
			const auto & b = tri.b * scale;
			const auto & c = tri.c * scale;
			glColor3f( c_r / 255.0f, c_g / 255.0f, c_b / 255.0f );
			glVertex3f( a.x, a.y, a.z );
			glVertex3f( b.x, b.y, b.z );
			glVertex3f( c.x, c.y, c.z );
		}
		glEnd( );

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
			if( e.type == SDL_QUIT )
				run = false;

		if( GetAsyncKeyState( 'A' ) )
			ox -= 2.0f;
		if( GetAsyncKeyState( 'D' ) )
			ox += 2.0f;
		if( GetAsyncKeyState( 'W' ) )
			oz -= 2.0f;
		if( GetAsyncKeyState( 'S' ) )
			oz += 2.0f;

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

void run_window( const cpu_chunk_array & chunks )
{
	int width = 1920;
	int height = 1080;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.5;
	height *= 1.5;
#endif

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 10000.0 );

	glEnable( GL_DEPTH_TEST );
	//glDepthFunc( GL_LEQUAL );
	glEnable( GL_CULL_FACE );
	glDisable( GL_CULL_FACE );

	float ox = 0.0f, oy = 0.0f, oz = 0.0f;

	glMatrixMode( GL_MODELVIEW );
	//glTranslatef( -ox, -oy, -oz );
	//glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
	//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

	//glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClearColor( 1.0, 1.0, 1.0, 1.0 );
	bool run = true;
	while( run )
	{
		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
		glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		//glBegin( GL_POINTS );
		unsigned long i = 0;
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & chunk : chunks )
		{
			glBegin( GL_TRIANGLES );
			for( auto & tri : chunk.tris )
			{
				++i;
				int c_r = (i & 0xFF);
				int c_g = (i & 0xFF00) >> 8;
				int c_b = ((i * 3) & 0xFF0000) >> 16;
				const auto & a = tri.a;
				const auto & b = tri.b;
				const auto & c = tri.c;
				glColor3f( c_r / 255.0f, c_g / 255.0f, c_b / 255.0f );
				glVertex3f( a.x, a.y, a.z );
				glVertex3f( b.x, b.y, b.z );
				glVertex3f( c.x, c.y, c.z );
			}
			glEnd( );
		}

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
			if( e.type == SDL_QUIT )
				run = false;

		if( GetAsyncKeyState( 'A' ) )
			ox -= 2.0f;
		if( GetAsyncKeyState( 'D' ) )
			ox += 2.0f;
		if( GetAsyncKeyState( 'W' ) )
			oz -= 2.0f;
		if( GetAsyncKeyState( 'S' ) )
			oz += 2.0f;

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

void run_window( const cpu_chunk_array & chunks, const cpu_data_sequence_array & volumeStates )
{
	int width = 1920;
	int height = 1080;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.85;
	height *= 1.85;
#endif


	int currentFrameIndex = 0;
	int numStates = volumeStates.size( );

	double volMin = 1e10;
	double volMax = 1e-10;
	for( auto state : volumeStates )
	{
		for( int v = 0; v < state->size( ); v++ )
		{
			volMin = min( state->at( v ), volMin );
			volMax = max( state->at( v ), volMax );
		}
	}

	//	Normalize within range for rendering
	for( auto state : volumeStates )
	{
		for( int v = 0; v < state->size( ); v++ )
		{
			double & val = state->at( v );
			val = (val - volMin) / (volMax - volMin);
		}
	}


	float_3 minColor = float_3( 0.0f, 0.0f, 0.0f );
	float_3 maxColor = float_3( 0.0f, 0.0f, 1.0f );


	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 10000.0 );

	glEnable( GL_DEPTH_TEST );
	//glDepthFunc( GL_LEQUAL );
	//glEnable( GL_CULL_FACE );
	
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );

	auto glBlendEquation = (void( *)(GLenum))wglGetProcAddress( "glBlendEquation" );
	const GLenum GL_MAX = 0x8008;
	//glBlendEquation( GL_MAX );

	//glBlendFunc( GL_ONE, GL_ONE );
		
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE );

	float ox = 0.0f, oy = 0.0f, oz = 0.0f;
	float ry = 0.0f;

	float frameAdvanceTime = 3.5f;
	auto animStart = clock( );

	//glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	//glClearColor( 0.0, 1.0, 0.0, 1.0 );
	glClearColor( minColor.r, minColor.g, minColor.b, 1.0f );
	bool run = true;
	while( run )
	{
		if( clock( ) - animStart > frameAdvanceTime * 1000 )
		{
			if( ++currentFrameIndex == volumeStates.size( ) )
				currentFrameIndex = 0;
			animStart = clock( );
			//std::cout << "Frame " << currentFrameIndex << std::endl;
		}

		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 00.0f + ry, 0.0f, 1.0f, 0.0f );
		//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		const double scale = 100.0;

		//glBegin( GL_POINTS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & chunk : chunks )
		{
			unsigned long i = 0;
			glBegin( GL_TRIANGLES );
			for( auto & tri : chunk.tris )
			{
				double val = volumeStates[currentFrameIndex]->at(tri.volumeIndex);
				//float_3 color = maxColor * val + minColor * (1 - val);
				//float_3 color = 0.01f * float_3( 1.0f ) + maxColor * pow( val, 1.25f );
				float_3 color = float_3( tri.volumeIndex % 2048, tri.volumeIndex / 2048, 0.0f ) / 2048.0f;
				glColor3f( color.r, color.g, color.b );
				const auto & a = tri.a * scale;
				const auto & b = tri.b * scale;
				const auto & c = tri.c * scale;
				glVertex3f( a.x, a.y, a.z );
				glVertex3f( b.x, b.y, b.z );
				glVertex3f( c.x, c.y, c.z );
				++i;
			}
			glEnd( );
		}

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
			if( e.type == SDL_QUIT )
				run = false;

		if( GetAsyncKeyState( 'A' ) ) ox -= 2.0f;
		if( GetAsyncKeyState( 'D' ) ) ox += 2.0f;
		if( GetAsyncKeyState( 'W' ) ) oz -= 2.0f;
		if( GetAsyncKeyState( 'S' ) ) oz += 2.0f;
		if( GetAsyncKeyState( 'Z' ) ) oy -= 2.0f;
		if( GetAsyncKeyState( 'X' ) ) oy += 2.0f;

		if( GetAsyncKeyState( VK_UP ) ) { currentFrameIndex++; animStart = clock( ); }
		if( GetAsyncKeyState( VK_DOWN ) ) { currentFrameIndex--; animStart = clock( ); }

		if( currentFrameIndex < 0 ) currentFrameIndex = numStates - 1;
		if( currentFrameIndex == numStates ) currentFrameIndex = 0; animStart = clock( );

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

void draw_voxel( const voxel & v, double value )
{
	glColor3f( value, value, value );

	//glColor3f( 1.0f, 1.0f, 1.0f );
	//glColor3f( 0.5f, 0.5f, 0.5f );

	//glColor3f( 1.0f, 0.0f, 0.0f );
	glVertex3f( v.start.x, v.end.y, v.start.z );
	glVertex3f( v.end.x, v.end.y, v.start.z );
	glVertex3f( v.end.x, v.start.y, v.start.z );
	glVertex3f( v.start.x, v.start.y, v.start.z );

	//glColor3f( 0.0f, 1.0f, 0.0f );
	glVertex3f( v.start.x, v.end.y, v.start.z );
	glVertex3f( v.end.x, v.end.y, v.start.z );
	glVertex3f( v.end.x, v.end.y, v.end.z );
	glVertex3f( v.start.x, v.end.y, v.end.z );

	//glColor3f( 0.0f, 0.0f, 1.0f );
	glVertex3f( v.start.x, v.start.y, v.start.z );
	glVertex3f( v.start.x, v.end.y, v.start.z );
	glVertex3f( v.start.x, v.end.y, v.end.z );
	glVertex3f( v.start.x, v.start.y, v.end.z );
}

void run_window( const voxel_matrix & vm )
{
	if( !vm.dev_voxel_data )
		NOT_YET_IMPLEMENTED( );

	int width = 1680;
	int height = 900;

#ifdef PREVIEW_MESH_HIDPI
	SetProcessDPIAware( );
	width *= 1.85;
	height *= 1.85;
#endif

	std::vector<float_3> idColors = {
		float_3( 1.0f, 0.0f, 0.0f ),
		float_3( 0.0f, 1.0f, 0.0f ),
		float_3( 0.0f, 0.0f, 1.0f ),
		float_3( 1.0f, 1.0f, 0.0f ),
		float_3( 0.0f, 1.0f, 1.0f ),
		float_3( 0.0f ),
		float_3( 0.0f )
	};

	int numVoxels = vm.width * vm.height * vm.depth;

	double valMin = 1e10;
	double valMax = 1e-10;
	std::cout << "Generating voxel value bounds... ";
	for( int x = 0; x < vm.width; x++ )
		for( int y = 0; y < vm.height; y++ )
			for( int z = 0; z < vm.depth; z++ )
			{
				const auto & data = (*vm.dev_voxel_data)( x, y, z );
				valMin = min( data, valMin );
				valMax = max( data, valMax );
			}

	std::cout << "Done." << std::endl;


	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_Window * window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL );
	SDL_GL_CreateContext( window );
	glMatrixMode( GL_PROJECTION );
	gluPerspective( 90.0, 1920.0 / 1080.0, 1.0, 1000.0 );

	bool useMIP = false; // Otherwise additive

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glEnable( GL_BLEND );

	if( true )
	{
		auto glBlendEquation = ( void( *)( GLenum ) )wglGetProcAddress( "glBlendEquation" );
		const GLenum GL_MAX = 0x8008;
		glBlendEquation( GL_MAX );
	}

	glDisable( GL_DEPTH_TEST );

	//glBlendFunc( GL_ONE, GL_ONE );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	glBlendFunc( GL_ONE, GL_ONE );

	float ox = 100.0f, oy = 100.0f, oz = 170.0f;
	float ry = 0.0f;

	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	//glClearColor( 0.0, 1.0, 0.0, 1.0 );
	//glClearColor( minColor.r, minColor.g, minColor.b, 1.0f );
	bool run = true;
	while( run )
	{
		auto start = clock( );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		glTranslatef( -ox, -oy, -oz );
		glRotatef( 0.0f + ry, 0.0f, 1.0f, 0.0f );
		//glRotatef( 45.0f, 1.0f, 0.0f, 0.0f );

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		const double scale = 100.0;

		glBegin( GL_QUADS );
		for( int x = 0; x < vm.width; x++ )
			for( int y = 0; y < vm.height; y++ )
				for( int z = 0; z < vm.depth; z++ )
				{
					auto & voxel = (*vm.dev_voxels)( x, y, z );
					auto & value = (*vm.dev_voxel_data )( x, y, z );
					int tagId = floorf( value );
					if( tagId == -1 ) continue;
					auto & color = idColors[tagId];
					float membership = value - tagId;
					//glColor3f( color.r * membership, color.g * membership, color.b * membership );
					draw_voxel( voxel, value );
				}

		glEnd( );

		SDL_GL_SwapWindow( window );

		SDL_Event e;
		SDL_PumpEvents( );
		while( SDL_PollEvent( &e ) )
			if( e.type == SDL_QUIT )
				run = false;

		float spd = 10.0f;
		if( GetAsyncKeyState( 'A' ) ) ox -= spd;
		if( GetAsyncKeyState( 'D' ) ) ox += spd;
		if( GetAsyncKeyState( 'W' ) ) oz -= spd;
		if( GetAsyncKeyState( 'S' ) ) oz += spd;
		if( GetAsyncKeyState( 'Z' ) ) oy -= spd;
		if( GetAsyncKeyState( 'X' ) ) oy += spd;

		std::cout << ox << ",\t" << oy << ",\t" << oz << "\n";

		auto time = clock( ) - start;

		Sleep( max( 0, 30 - time ) );
	}

	SDL_Quit( );
}

enum RenderType
{
	Tris,
	Points
};

void workflow_render_mesh( gpu_triangle_array * mesh )
{
	auto & dev_tris = *mesh;
	cpu_triangle_array tris;
	tris.resize( dev_tris.extent.size( ) );

	for( int i = 0; i < tris.size( ); i++ )
		tris[i] = dev_tris[i];

	run_window( tris );
}

void workflow_render_mesh( cpu_chunk_array * chunks )
{
	run_window( *chunks );
}

void workflow_render_volume_mesh( cpu_chunk_array * chunks, cpu_data_sequence_array * sequence )
{
	run_window( *chunks, *sequence );
}

#endif
