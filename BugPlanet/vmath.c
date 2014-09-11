#include "platform.h"
#include "vmath.h"


//-----------------------------------------------------------------------------
// Name: distance2d
// Desc: Returns the distance between two 2D points.
//-----------------------------------------------------------------------------
float distance2d( float x1, float y1, float x2, float y2 )
{
	float d = 0.0f;
	float x = x2 - x1;
	float y = y2 - y1;

	x *= x;
	y *= y;

	d = (float) sqrt(x+y);

	return d;
}

float distance2dv( struct Vector2* v1, struct Vector2* v2 )
{
	float d = 0.0f;
	float x = v2->x - v1->x;
	float y = v2->y - v1->y;

	x *= x;
	y *= y;

	d = (float) sqrt(x+y);

	return d;
}

float angle( float sx, float sy, float dx, float dy )
{
	return (float) atan2( dy-sy, dx-sx );
}

float anglev( struct Vector2* s, struct Vector2* d )
{
	return (float) atan2( d->y-s->y, d->x-s->x );
}

void set_rect( struct Rect* r, float x1, float y1, float x2, float y2 )
{
    r->x1 = x1;
    r->y1 = y1;
    r->x2 = x2;
    r->y2 = y2;
}

int point_in_rect( struct Rect* r, float x, float y )
{
    if( x >= r->x1 && x <= r->x2 &&
        y >= r->y1 && y <= r->y2 )
    {
        return Yes;
    }
    
    return No;
}