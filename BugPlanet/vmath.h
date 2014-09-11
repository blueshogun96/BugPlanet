#pragma once

#include <math.h>

//-----------------------------------------------------------------------------
// 2D vector
//-----------------------------------------------------------------------------
struct Vector2
{
	float x, y;
};

//-----------------------------------------------------------------------------
// Rectangle
//-----------------------------------------------------------------------------
struct Rect
{
    float x1, y1;
    float x2, y2;
};

//-----------------------------------------------------------------------------
// Interesting math functions
//-----------------------------------------------------------------------------
float distance2d( float x1, float y1, float x2, float y2 );
float distance2dv( struct Vector2* v1, struct Vector2* v2 );
float angle( float sx, float sy, float dx, float dy );
float anglev( struct Vector2* s, struct Vector2* d );

void set_rect( struct Rect* r, float x1, float y1, float x2, float y2 );
int point_in_rect( struct Rect* r, float x, float y );