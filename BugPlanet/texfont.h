#pragma once

/* Textured font structure */
struct font_t
{
	struct texture_t* tex;	/* Handle to texture */
	float width;			/* Width of the texture */
	float height;			/* Height of the texture */
	float letter_width;		/* Width of each letter */
	float letter_height;	/* Height of each letter */
	float letters_per_row;	/* Number of letters each row */
	float letter_offset;	/* Offset between each letter */
	DWORD colour_key;		/* Colour key colour (unused right now) */
	BOOL has_colour_key;	/* Does this texture have a colour key */
	char* texname;			/* Name of the texture file */
	int texsize;			/* Size of the texture file (in bytes) */
	int char_offset;		/* The number needed to subtract from each char to form words x_x */
};

/* Dynamic string structure */
struct dynamicstring_t
{
	char str[16];			/* The dynamic text's message */
	int length;				/* Number of chars actually used */
	int x, y;				/* Position on screen */
	float scale[16];		/* Scaling for each char */
	int active[16];			/* Which chars are active */
	int active_chars;		/* Number of active chars */
	int fade_delay;			/* The amount of time in frames (60*sec) until this text fades out */
	float alpha;			/* Alpha component (for transparency) */
	struct font_t* font;	/* The font used to render this string */
};

BOOL init_font( struct font_t* font );
void uninit_font( struct font_t* font );
void draw_text( struct font_t* font, int x, int y, char* string );
void draw_shadowed_text( struct font_t* font, int x, int y, char* string );
void draw_char( struct font_t* font, int x, int y, char c );
void draw_scaled_char( struct font_t* font, int x, int y, char c, float scale );
void draw_dynamic_text( struct font_t* font, struct dynamicstring_t* ds );