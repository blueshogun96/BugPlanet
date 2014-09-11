#pragma once

//--------------------------------------------------------------------------------------
// OpenGL includes
//--------------------------------------------------------------------------------------
/*#include <SDL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>*/


struct texture_t
{
    unsigned int handle;
    int width, height;
    int bpp;
};

struct render_target_t
{
    unsigned int fbo;
    unsigned int rbo;
    struct texture_t tex;
};

//--------------------------------------------------------------------------------------
// Functions for dealing with OpenGL
//--------------------------------------------------------------------------------------
int  EnableOpenGL();	// Initialization
void DisableOpenGL();	// Uninitialization
int toggle_fullscreen();
int glh_extension_supported( const char *extension );
void resize_viewport( GLsizei width, GLsizei height );
void clear_buffers();
void swap();

void glEnable2D();
void glDisable2D();

void draw_quad( GLuint texture, float x, float y, float w, float h );
void draw_quad2( GLuint texture, float* tex, float x, float y, float w, float h );
void draw_quad_batched( float x, float y, float w, float h );
void draw_quad2_batched( float* tex, float x, float y, float w, float h );
void flush_batch( GLuint texture );
struct texture_t* create_texture( char* filename );
struct texture_t* create_texture_from_file( void* filebuf, char* filename );
GLuint create_texture_rectangle( int width, int height, int bpp );
void copy_scene_to_texture( GLuint *tex, int width, int height );
void delete_texture( struct texture_t* t );
void draw_line( float x1, float y1, float x2, float y2 );
void set_colour( float r, float g, float b, float a );
void set_colourv( float* c );
void transparent_blend( int enable );
void flash_white( int enable );
void shadow( int enable );
void enable_vsync( int enable );
void translate( float x, float y );
void rotate( float angle );
void wireframe( int enable );
void block_until_vertical_blank();
void set_z_depth( float z );

int create_render_target( int width, int height, unsigned int colour_format, unsigned int depth_format, unsigned int stencil_format, struct render_target_t* rt );
void delete_render_target( struct render_target_t* render_target );
void set_render_target( struct render_target_t* render_target );

GLuint open_shader( char* shader_file, GLenum shader_type );
GLuint link_program( GLuint vs, GLuint fs );
void delete_program( GLuint program );

static int get_spf() { extern int spf; return spf; }

extern void (*enable_2d)();
extern void (*disable_2d)();
extern void (*push_pos)();
extern void (*pop_pos)();