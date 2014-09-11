#include "platform.h"
#include "ogldrv.h"
#include "texfont.h"


/* Dynamic strings */
struct dynamicstring_t strings[4];


#if 1
BOOL init_font( struct font_t* font )
{
	/* Sanity checks */
	if( !font ) return FALSE;
	if( !font->texname ) return FALSE;
	if( !font->width ) return FALSE;
	if( !font->height ) return FALSE;
	if( !font->letter_width ) return FALSE;
	if( !font->letter_height ) return FALSE;
//	if( !font->letters_per_row ) return FALSE;

	/* Open the texture file */
    /* Read in that texture */
    
    font->tex = create_texture( font->texname );

	/* Get the number of letters per row */
	font->letters_per_row = font->width / font->letter_width;

	return TRUE;
}

void uninit_font( struct font_t* font )
{
	delete_texture( font->tex );

	ZeroMemory( font, sizeof( struct font_t ) );
}

void draw_char( struct font_t* font, int x, int y, char c )
{
	div_t result;
	int offsetx, offsety;
	float tex[8];
	RECT rect;

	if( !font )
		return;

	c -= 32 + font->char_offset;

	result = div( c, font->letters_per_row );

	offsetx = result.rem * font->letter_width;
	offsety = result.quot * font->letter_height;

	/*SetRect( &rect, offsetx, offsety,
			 offsetx + font->letter_width,
			 offsety + font->letter_height );*/
    
    rect.left = offsetx;
    rect.top = offsety;
    rect.right = offsetx + font->letter_width;
    rect.bottom = offsety + font->letter_height;
    
    tex[0] = rect.left / font->width;
    tex[1] = 1.0f - (rect.top / font->height);
    tex[2] = rect.right / font->width;
    tex[3] = 1.0f - (rect.top / font->height);
    tex[4] = rect.right / font->width;
    tex[5] = 1.0f - (rect.bottom / font->height);
    tex[6] = rect.left / font->width;
    tex[7] = 1.0f - (rect.bottom / font->height);

//	if( font->has_colour_key ) flags |= DDBLTFAST_SRCCOLORKEY;

//	lpDDSBack->BltFast( x, y, font->dds_bmp, &rect, flags );
	draw_quad2_batched( tex, x, y, font->letter_width, font->letter_height );
}

void draw_scaled_char( struct font_t* font, int x, int y, char c, float scale )
{
	div_t result;
	int offsetx, offsety;
	float tex[8];
	float s, t, w, h;
	RECT rect;

	if( !font )
		return;

	/* TODO: Find out why it's necessary to subtract 16 instead of 32 for OpenGL */
	c -= 16 + font->char_offset;

/*	result = div( c, font->letters_per_row );

	offsetx = result.rem * font->letter_width;
	offsety = result.quot * font->letter_height;

	SetRect( &rect, offsetx, offsety, 
			 offsetx + font->letter_width,
			 offsety + font->letter_height );*/

	result = div( c, (int) font->letters_per_row );
	s = (float) result.rem * ( font->letter_width / font->width );
	t = (float) 1.0f - ( result.quot * ( font->letter_height / font->height ) );

	tex[0] = s; tex[1] = t + font->letter_height / font->height;
	tex[2] = s + font->letter_width / font->width; tex[3] = t + font->letter_height / font->height;
	tex[4] = s + font->letter_width / font->width; tex[5] = t;
	tex[6] = s; tex[7] = t;

	w = font->letter_width*scale;
	h = font->letter_height*scale;

	draw_quad2( font->tex->handle, tex, x-(w/2.0f), y-(h/2.0f), w, h );
}

void draw_text( struct font_t* font, int x, int y, char* string )
{
	int i, offset = 0;

	if( !font->tex )
		return;

//	enable_2d();
//	transparent_blend( TRUE );
/*	set_colour( 1.0f, 1.0f, 1.0f, 1.0f ); */
/*	glTranslatef( 160.0f, 0.0f, 0.0f ); */

	for( i = 0; i < strlen( string ); i++ )
	{
		if( string[i] == '\n' )
		{
			y += font->letter_height;
			offset -= i;
			continue;
		}

		draw_char( font, ((x+offset)+(font->letter_width*(i+offset)))-(font->letter_offset*(i+offset)), y, string[i] );
	}

    flush_batch( font->tex->handle );
    
//	transparent_blend( FALSE );
//	disable_2d();
}

void draw_shadowed_text( struct font_t* font, int x, int y, char* string )
{
    set_colour( 0.0f, 0.0f, 0.0f, 1.0f );
    draw_text( font, x+2, y+2, string );
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
    draw_text( font, x, y, string );
}

void draw_dynamic_text( struct font_t* font, struct dynamicstring_t* ds )
{
	int i;

	/* Sanity checks */

	if( !font->tex )
		return;

	if( !ds )
		return;

	ds->font = &(*font);
	memset( ds->active, 0, 4*16 );
	memset( ds->scale, 10.0f, 4*16 );
	ds->alpha = 1.0f;
	ds->fade_delay = 60*3;
	ds->length = strlen( ds->str );
	ds->active_chars = 0;
}

void update_dynamic_texts()
{
#if 0
	int i, j = 0;
	static int delay = 0;

	if( !font->tex )
		return;

	enable_2d();
	transparent_blend( TRUE );
/*	set_colour( 1.0f, 1.0f, 1.0f, 1.0f ); */
/*	glTranslatef( 160.0f, 0.0f, 0.0f ); */

	while( j < 4 )
	{
		if( strings[j].alpha > 0.0f )
		{
			for( i = 0; i < lstrlen( string ); i++ )
			{
				draw_scaled_char( strings[j].font, 
					(strings[j].x+(strings[j].font->letter_width*i))-(strings[j].font->letter_offset*i), strings[j].y, 
					strings[j].str[i] );
			}
		}

		j++;

		delay++;

		/* Activate the next char */
		if( delay >= 20 )
		{
			if( strings[j].active_chars < 16 )
			{
				strings[j].active[strings[j].active_chars++] = Yes;
			}

			delay = 0;
		}
	}

	transparent_blend( FALSE );
	disable_2d();
#endif
}

#endif