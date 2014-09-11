#include "platform.h"
#include "ogldrv.h"
#include "tgaLoader.h"


//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
int win_w, win_h;
extern int fullscreen;
GLuint GL_tex_rect = GL_TEXTURE_2D;

//SDL_Surface* surface = NULL;

float current_colour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

int spf = 0; // Sprites per frame
int batched_spf = 0;
int unbatched_spf = 0;
float z_depth = 0.0f;	// Z-depth for sprites

int sprites_enqueued = 0;
int max_sprite_queue = 1024;
float* vertex_queue = NULL;
float* texcoord_queue = NULL;
//float* colour_queue = NULL;

void (*enable_2d)();
void (*disable_2d)();
void (*push_pos)();
void (*pop_pos)();



//-----------------------------------------------------------------------------
// Name: glh_extension_supported
// Desc: Checks the list of OpenGL extensions supported by this videocard/driver
//		 to see if it's supported.  This function was borrowed from the NVIDIA
//		 SDK (v9.5).  I was too lazy to write my own extension checking function :)
//		 See glh_extensions.h
//-----------------------------------------------------------------------------
int glh_extension_supported( const char *extension )
{
    static const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;
    
    // Extension names should not have spaces. 
    where = (GLubyte *) strchr( extension, ' ');
    if ( where || *extension == '\0')
      return 0;
    
    if ( !extensions )
      extensions = glGetString( GL_EXTENSIONS );

    // It takes a bit of care to be fool-proof about parsing the
    // OpenGL extensions string.  Don't be fooled by sub-strings,
    // etc.
    start = extensions;
    for (;;) 
    {
        where = (GLubyte *) strstr( (const char *) start, extension );
        if ( !where )
            break;
        terminator = where + strlen( extension );
        if ( where == start || *(where - 1) == ' ' ) 
        {
            if ( *terminator == ' ' || *terminator == '\0' ) 
            {
                return 1;
            }
        }
        start = terminator;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Name: resize_viewport
// Desc: Changes the viewport and projection matrices.
//-----------------------------------------------------------------------------
void resize_viewport( GLsizei width, GLsizei height )
{
	// Prevent divide by zero exception
	if( !height ) height = 1;

	// Reset viewport
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();

	// Reset perspective
	gluPerspective( 45.0f, (double) width/height, 0.1f, 5000.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	win_w = width;
	win_h = height;
}


//--------------------------------------------------------------------------------------
// Name: EnableOpenGL
// Desc: Initializes OpenGL and sets default render states.
//--------------------------------------------------------------------------------------
int EnableOpenGL()
{
#if 0
    const SDL_VideoInfo* info = NULL;
    int bpp = 0;
    int flags;
    
    /* Initialize SDL */
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        return 0;
    }
    
    /* Get the current video mode */
    info = SDL_GetVideoInfo();
    if( !info )
    {
        SDL_Quit();
        return 0;
    }
    
    /* Intialize OpenGL */
    win_w = 640;
    win_h = 480;
    bpp = info->vfmt->BitsPerPixel;
	
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    
    flags = SDL_OPENGL | SDL_FULLSCREEN;
    
    surface = SDL_SetVideoMode( win_w, win_h, bpp, flags );
    if( surface == NULL )
    {
        SDL_Quit();
        return 0;
    }
#endif
    
    // Set default OpenGL render states
//	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glHint( GL_FOG_HINT, GL_NICEST );
//	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

	glDisable( GL_LIGHTING );
	glDisable( GL_BLEND );
	glEnable( GL_DITHER );
	glDisable( GL_CULL_FACE );
	glEnable( GL_TEXTURE_2D );

	// Reset viewport
	resize_viewport( 320, 480 );

	enable_2d = glEnable2D;
	disable_2d = glDisable2D;
	push_pos = glPushMatrix;
	pop_pos = glPopMatrix;
    
    // Allocate sprite queue
    vertex_queue = malloc( sizeof( float ) * 12 * max_sprite_queue );
    texcoord_queue = malloc( sizeof( float ) * 12 * max_sprite_queue );
//    colour_queue = malloc( sizeof( float ) * 12 * max_sprite_queue );

	return TRUE;
}

//--------------------------------------------------------------------------------------
// Name: DisableOpenGL
// Desc: Uninitializes OpenGL
//--------------------------------------------------------------------------------------
void DisableOpenGL()
{
    // Free up sprite queue
    free( vertex_queue );
    free( texcoord_queue );
//    free( colour_queue );
}

int toggle_fullscreen()
{
    return 0;
}

//--------------------------------------------------------------------------------------
// Name: clear_buffers
// Desc: Clears the colour, depth and stencil buffer bits and resets the modelview matrix.
//--------------------------------------------------------------------------------------
void clear_buffers()
{
//    glClearColor( 0.5f, 0.5f, 0.5f, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

//--------------------------------------------------------------------------------------
// Name: swap
// Desc: Swaps the double buffer contents
//--------------------------------------------------------------------------------------
void swap()
{
    //SDL_GL_SwapBuffers();

	spf = 0;
    batched_spf = 0;
    unbatched_spf = 0;
}

//--------------------------------------------------------------------------------------
// Name: glEnable2D
// Desc: Enables 2D rendering via Ortho projection.
//--------------------------------------------------------------------------------------
GLvoid glEnable2D( GLvoid )
{
	GLint iViewport[4];

	// Get a copy of the viewport
	glGetIntegerv( GL_VIEWPORT, iViewport );

	// Save a copy of the projection matrix so that we can restore it 
	// when it's time to do 3D rendering again.
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();

	// Set up the orthographic projection
	glOrthof( iViewport[0], iViewport[0]+iViewport[2],
			 iViewport[1]+iViewport[3], iViewport[1], -1, 1 );
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();

	// Make sure depth testing and lighting are disabled for 2D rendering until
	// we are finished rendering in 2D
	/*glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );*/
}


//--------------------------------------------------------------------------------------
// Name: glDisable2D
// Desc: Disables the ortho projection and returns to the 3D projection.
//--------------------------------------------------------------------------------------
void glDisable2D( GLvoid )
{
//	glPopAttrib();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
}

//--------------------------------------------------------------------------------------
// Name: draw_quad
// Desc: Renders a 2D quad to the screen (using a triangle fan).
//--------------------------------------------------------------------------------------
void draw_quad( GLuint texture, float x, float y, float w, float h )
{
	float vertices[] = { x, y, x+w, y, x+w, y+h, x, y+h };
	float tex[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

	glBindTexture( GL_TEXTURE_2D, texture );

//	glEnable2D();
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glVertexPointer( 2, GL_FLOAT, sizeof(float)*2, vertices );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
//	glDisable2D();

	spf++;
    unbatched_spf++;
}

//--------------------------------------------------------------------------------------
// Name: draw_quad2
// Desc: Renders a 2D quad to the screen (using a triangle fan), but allows you to specify
//		 the texture coordinates yourself.
//--------------------------------------------------------------------------------------
void draw_quad2( GLuint texture, float* tex, float x, float y, float w, float h )
{
	float vertices[] = { x, y, x+w, y, x+w, y+h, x, y+h };
//	float tex[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

	glBindTexture( GL_TEXTURE_2D, texture );

//	glEnable2D();
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glVertexPointer( 2, GL_FLOAT, sizeof(float)*2, vertices );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
//	glDisable2D();

	spf++;
    unbatched_spf++;
}

//--------------------------------------------------------------------------------------
// Name: draw_quad3
// Desc: Renders a 2D quad to the screen (using a triangle fan).
//--------------------------------------------------------------------------------------
void draw_quad3( GLuint texture, float x, float y, float w, float h )
{
	float vertices[] = { x, y, x+w, y, x+w, y+h, x, y+h };
	float tex[] = { 0.0f, 480.0f, 640.0f, 480.0f, 640.0f, 0.0f, 0.0f, 0.0f };

	glEnable( GL_tex_rect );
	glBindTexture( GL_tex_rect, texture );

//	glEnable2D();
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glVertexPointer( 2, GL_FLOAT, sizeof(float)*2, vertices );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
//	glDisable2D();

	glDisable( GL_tex_rect );

	spf++;
}

//--------------------------------------------------------------------------------------
// Name: draw_line
// Desc: Renders a 2D line to the screen.
//--------------------------------------------------------------------------------------
void draw_line( float x1, float y1, float x2, float y2 )
{
	float vertices[] = { x1, y1, x2, y2 };
	float colour[] = { current_colour[0], current_colour[1], current_colour[2], current_colour[3],
		current_colour[0], current_colour[1], current_colour[2], current_colour[3] };

	glDisable( GL_TEXTURE_2D );

//	glEnable2D();
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );
	glVertexPointer( 2, GL_FLOAT, 0, vertices );
	glColorPointer( 4, GL_FLOAT, 0, colour );
	glDrawArrays( GL_LINES, 0, 2 );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
//	glDisable2D();

	glEnable( GL_TEXTURE_2D );
}

void draw_quad_batched( float x, float y, float w, float h )
{
    float vertices[] = { x, y, x+w, y, x+w, y+h, x+w, y+h, x, y+h, x, y };
	float tex[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    float colours[] = { current_colour[0], current_colour[1], current_colour[2], current_colour[3],
		current_colour[0], current_colour[1], current_colour[2], current_colour[3],
        current_colour[0], current_colour[1], current_colour[2], current_colour[3],
		current_colour[0], current_colour[1], current_colour[2], current_colour[3] };
    
    memmove( &vertex_queue[sprites_enqueued*12], vertices, sizeof( float ) * 12 );
    memmove( &texcoord_queue[sprites_enqueued*12], tex, sizeof( float ) * 12 );
//    memmove( &colour_queue[sprites_enqueued*12], colours, sizeof( float ) * 12 );
    
    sprites_enqueued++;
    batched_spf++;
}

void draw_quad2_batched( float* tex, float x, float y, float w, float h )
{
    float vertices[] = { x, y, x+w, y, x+w, y+h, x+w, y+h, x, y+h, x, y };
	float tex2[12];
    float colours[] = { current_colour[0], current_colour[1], current_colour[2], current_colour[3],
		current_colour[0], current_colour[1], current_colour[2], current_colour[3],
        current_colour[0], current_colour[1], current_colour[2], current_colour[3],
		current_colour[0], current_colour[1], current_colour[2], current_colour[3] };
    
    memmove( tex2, tex, sizeof( float ) * 6 );
    memmove( &tex2[6], &tex[4], sizeof( float ) * 4 );
    memmove( &tex2[10], &tex[0], sizeof( float ) * 2 );
    
    memmove( &vertex_queue[sprites_enqueued*12], vertices, sizeof( float ) * 12 );
    memmove( &texcoord_queue[sprites_enqueued*12], tex2, sizeof( float ) * 12 );
//    memmove( &colour_queue[sprites_enqueued*12], colours, sizeof( float ) * 12 );
    
    sprites_enqueued++;
    batched_spf++;
}

void flush_batch( GLuint texture )
{
    if( sprites_enqueued == 0 )
        return;
    
    glBindTexture( GL_TEXTURE_2D, texture );
    
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
//    glEnableClientState( GL_COLOR_ARRAY );
	glTexCoordPointer( 2, GL_FLOAT, 0, texcoord_queue );
	glVertexPointer( 2, GL_FLOAT, 0, vertex_queue );
//    glColorPointer( 4, GL_FLOAT, 0, colour_queue );
	glDrawArrays( GL_TRIANGLES, 0, sprites_enqueued*6 );
//    glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableClientState( GL_VERTEX_ARRAY );
    
	spf += sprites_enqueued;
    sprites_enqueued = 0;
}

//--------------------------------------------------------------------------------------
// Name: set_colour
// Desc: Sets the current diffuse colour (and saves it to the global RGBA colour array).
//--------------------------------------------------------------------------------------
void set_colour( float r, float g, float b, float a )
{
	current_colour[0] = r;
	current_colour[1] = g;
	current_colour[2] = b;
	current_colour[3] = a;

	glColor4f( r, g, b, a );
}

void set_colourv( float* c )
{
	current_colour[0] = c[0];
	current_colour[1] = c[1];
	current_colour[2] = c[2];
	current_colour[3] = c[3];

	glColor4f( c[0], c[1], c[2], c[3] );
}

//--------------------------------------------------------------------------------------
// Name: transparent_blend
// Desc: Toggles alpha blending for transparency.
//--------------------------------------------------------------------------------------
void transparent_blend( int enable )
{
	if( enable )
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else
	{
		glDisable( GL_BLEND );
	}
}

//--------------------------------------------------------------------------------------
// Name: flash_white
// Desc: Causes a textured sprite to flash white
//--------------------------------------------------------------------------------------
void flash_white( int enable )
{
	float colour[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float colour2[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if( enable )
	{
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND );
		glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour );
	}
	else
    {
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour2 );
    }
}

//--------------------------------------------------------------------------------------
// Name: shadow
// Desc: Causes a textured sprite to appear black and slightly transparent
//--------------------------------------------------------------------------------------
void shadow( int enable )
{
	float colour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float colour2[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    
	if( enable )
	{
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE );
		glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour );
	}
	else
    {
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
        glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, colour2 );
    }
}

//--------------------------------------------------------------------------------------
// Name: enable_vsync
// Desc: Controls vertical sync
//--------------------------------------------------------------------------------------
void enable_vsync( int enable )
{
//	wglSwapIntervalEXT( enable );
}

//--------------------------------------------------------------------------------------
// Name: create_texture
// Desc: Opens a .tga texture file from disk.
//--------------------------------------------------------------------------------------
struct texture_t* create_texture( char* filename )
{
	GLubyte* pixels;
	GLuint w, h, bpp;
	GLuint handle;
	GLuint pixel_format = GL_RGB;
	char string[256];
    struct texture_t* t = NULL;

    t = (struct texture_t*) malloc( sizeof( struct texture_t ) );
    if( !t )
        return NULL;
    
	glGenTextures( 1, &handle );

	if( !LoadTgaImage( filename, &pixels, &w, &h, &bpp ) )
	{
		sprintf( string, "[ERROR]: Could not load (%s)\n", filename );
//		MessageBox( NULL, string, "Pale Dragon", MB_OK );
		return 0;
	}

	if( bpp == 4 )
		pixel_format = GL_RGBA;

	glBindTexture( GL_TEXTURE_2D, handle );
	glTexImage2D( GL_TEXTURE_2D, 0, pixel_format, w, h, 0, pixel_format, GL_UNSIGNED_BYTE, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	free( pixels );

    t->handle = handle;
    t->width = w;
    t->height = h;
    t->bpp = bpp;
    
	return t;
}

//--------------------------------------------------------------------------------------
// Name: create_texture_from_file
// Desc: Opens a .tga texture file from a file pointer.
//--------------------------------------------------------------------------------------
struct texture_t* create_texture_from_file( void* filebuf, char* filename )
{
	GLubyte* pixels;
	GLuint w, h, bpp;
	GLuint handle;
	GLuint pixel_format = GL_RGB;
	char string[256];
    struct texture_t* t = NULL;

    t = (struct texture_t*) malloc( sizeof( struct texture_t ) );
    if( !t )
        return NULL;
    
	glGenTextures( 1, &handle );

	if( !LoadTgaImageFromMemory( filebuf, &pixels, &w, &h, &bpp ) )
	{
		sprintf( string, "[ERROR]: Could not load (%s)\n", filename );
//		MessageBox( NULL, string, "Pale Dragon", MB_OK );
		return 0;
	}

	if( bpp == 4 )
		pixel_format = GL_RGBA;

	glBindTexture( GL_TEXTURE_2D, handle );
	glTexImage2D( GL_TEXTURE_2D, 0, pixel_format, w, h, 0, pixel_format, GL_UNSIGNED_BYTE, pixels );
//	gluBuild2DMipmaps( GL_TEXTURE_2D, pixel_format, w, h, pixel_format, GL_UNSIGNED_BYTE, pixels );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	free( pixels );

    t->handle = handle;
    t->width = w;
    t->height = h;
    t->bpp = bpp;
    
	return t;
}

//--------------------------------------------------------------------------------------
// Name: create_texture_rectangle
// Desc: Creates a blank texture rectangle
//--------------------------------------------------------------------------------------
GLuint create_texture_rectangle( int width, int height, int bpp )
{
	GLuint handle;
	GLuint pixel_format = ( bpp == 32 ) ? GL_RGBA : GL_RGB ;

	glEnable( GL_tex_rect );

	glGenTextures( 1, &handle );
	glBindTexture( GL_tex_rect, handle );
	glTexImage2D( GL_tex_rect, 0, pixel_format, width, height, 0, pixel_format, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_tex_rect, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_tex_rect, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	return handle;
}

//--------------------------------------------------------------------------------------
// Name: copy_scene_to_texture
// Desc: 
//--------------------------------------------------------------------------------------
void copy_scene_to_texture( GLuint *tex, int width, int height )
{
	GLenum error;

//	glEnable( GL_tex_rect );
	glBindTexture( GL_tex_rect, *tex );
	error = glGetError();

	glCopyTexSubImage2D( GL_tex_rect, 0, 0, 0, 0, 0, width, height );
//	glDisable( GL_tex_rect );

	error = glGetError();

	if( error != GL_NO_ERROR )
	{
		// Do whatever...
	}
}

//--------------------------------------------------------------------------------------
// Name: delete_texture
// Desc: Deletes textures.
//--------------------------------------------------------------------------------------
void delete_texture( struct texture_t* t )
{
    if( t )
    {
        glDeleteTextures( 1, &t->handle );
        free(t);
        t = NULL;
    }
}

//--------------------------------------------------------------------------------------
// Name: translate
// Desc: 
//--------------------------------------------------------------------------------------
void translate( float x, float y )
{
	glTranslatef( x, y, z_depth );
}

//--------------------------------------------------------------------------------------
// Name: rotate
// Desc: 
//--------------------------------------------------------------------------------------
void rotate( float angle )
{
	glRotatef( angle, 0.0f, 0.0f, 1.0f );
}

//--------------------------------------------------------------------------------------
// Name: wireframe
// Desc: 
//--------------------------------------------------------------------------------------
void wireframe( int enable )
{
	/*if( enable )
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	else
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );*/
}

//--------------------------------------------------------------------------------------
// Name: block_until_vertical_blank
// Desc: Stalls the current thread until vblank occurs.
//--------------------------------------------------------------------------------------
void block_until_vertical_blank()
{
	int refresh_rate = 60;

//	SDL_Delay(1000/refresh_rate);
}

void set_z_depth( float z )
{
	z_depth = z;
}

int create_render_target( int width, int height, unsigned int colour_format, unsigned int depth_format, unsigned int stencil_format, struct render_target_t* rt )
{
    return 0;
}

void delete_render_target( struct render_target_t* render_target )
{
    
}

void set_render_target( struct render_target_t* render_target )
{
    
}

#if 0
GLuint open_shader( char* shader_file, GLenum shader_type )
{
    FILE* fp = fopen( shader_file, "r" );
    GLuint shader = 0;
    char* str = NULL;
    long strl = 0;
    GLint status;

    if(!fp)
        return shader;
    
    fseek( fp, 0, SEEK_END );
    strl = ftell(fp);
    fseek( fp, 0, SEEK_SET );
    
    str = malloc(strl);
    fread( str, 1, strl, fp );
    
    shader = glCreateShader(shader_type);
    glShaderSource( shader, 1, &str, NULL );
    glCompileShader( shader );
    
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    if( status == 0 )
    {
        glDeleteShader( shader );
        return 0;
    }
    
    return shader;
}

GLuint link_program( GLuint vs, GLuint fs )
{
    return 0;
}

void delete_program( GLuint program )
{
    glDeleteProgram( program );
}
#endif