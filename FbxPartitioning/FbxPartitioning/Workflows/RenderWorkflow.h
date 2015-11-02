#pragma once

#include "../Workflow.h"
#include "../Types.h"

#include <SDL.h>

#define PREVIEW_MESH_HIDPI

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

	}
};