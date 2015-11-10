#pragma once

#include "../Workflow.h"
#include "../Types.h"

#include <Windows.h>
#include <SDL.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#define PREVIEW_MESH_HIDPI



//	Code to avoid immediate-mode, TODO: Integratea
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

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		//draw_partitions( partitions );

		glBegin( GL_TRIANGLES );
		//glBegin( GL_POINTS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & tri : tris )
		{
			const auto & a = tri.a;
			const auto & b = tri.b;
			const auto & c = tri.c;
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

class RenderWorkflow : public Workflow
{

	/* Drawing utilities */

	void draw_mesh( const cpu_triangle_array & tris )
	{
		//glBegin( GL_TRIANGLES );
		glBegin( GL_POINTS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & tri : tris )
		{
			const auto & a = tri.a;
			const auto & b = tri.b;
			const auto & c = tri.c;
			glVertex3f( a.x, a.y, a.z );
			glVertex3f( b.x, b.y, b.z );
			glVertex3f( c.x, c.y, c.z );
		}
		glEnd( );
	}

	void draw_mesh( const gpu_triangle_array & tris )
	{
		//glBegin( GL_TRIANGLES );
		glBegin( GL_POINTS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( int i = 0; i < tris.extent.size( ); i++ )
		{
			auto & tri = tris[i];

			const auto & a = tri.a;
			const auto & b = tri.b;
			const auto & c = tri.c;
			glVertex3f( a.x, a.y, a.z );
			glVertex3f( b.x, b.y, b.z );
			glVertex3f( c.x, c.y, c.z );
		}
		glEnd( );
	}

	void draw_points( const cpu_vertex_array & verts )
	{
		//glBegin( GL_TRIANGLES );
		glBegin( GL_POINTS );
		glColor3f( 1.0f, 1.0f, 1.0f );
		for( const auto & vert : verts )
		{
			glVertex3f( vert.x, vert.y, vert.z );
		}
		glEnd( );
	}


	/* Windowing functions */
	//	Window/rendering only for debugging, final application should just be a processing tool.

	//	Multiple variations on run_window, where you can pass in a list of triangles or points, on the CPU or GPU

	void run_window( cpu_triangle_array & dev_tris )
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

			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			//draw_partitions( partitions );

			//draw_mesh( cpu_tris );
			draw_mesh( dev_tris );
			//draw_points( cpu_vertices );

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

	void run_window( gpu_triangle_array & dev_tris )
	{
		cpu_triangle_array tris;
		tris.resize( dev_tris.extent.size( ) );

		for( int i = 0; i < tris.size( ); i++ )
			tris[i] = dev_tris[i];

		run_window( tris );
	}

	void run_window( cpu_vertex_array & verts )
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

			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			//draw_partitions( partitions );

			//draw_mesh( cpu_tris );
			//draw_mesh( dev_tris );
			draw_points( verts );

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

public:
	RenderWorkflow( IngressDataBinding<triangle> mesh );
	RenderWorkflow( IngressDataBinding<float_3> points );

	void Run( )
	{
		NOT_YET_IMPLEMENTED( );
	}
};