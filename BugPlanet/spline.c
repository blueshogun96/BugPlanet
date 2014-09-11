#include "platform.h"
#include "vmath.h"
#include "spline.h"

/* NOTES: The actual speed value is really divided by 100 to get a percentage.
		  This dictates how far the position moves between one point to the next
		  in the spline.  As the tesselation for the spline grows, the slower the
		  entity will move as there are more and shorter line segments to traverse
		  through.
*/

void tesselate( struct spline_t** s )
{
	struct Vector2 first, last;
	struct Vector2* temp;
	int new_size = 1;
	int i = 0, v = 1;

	first.x = (*s)->points[0].x;
	first.y = (*s)->points[0].y;
	last.x = (*s)->points[(*s)->num_points-1].x;
	last.y = (*s)->points[(*s)->num_points-1].y;

	for( i = 0; i < (*s)->num_points-1; ++i )
		new_size += 2;

	new_size++;

	temp = (struct Vector2*) malloc( sizeof( struct Vector2 ) * new_size );

	temp[0].x = first.x;
	temp[0].y = first.y;

	for( i = 0; i < (*s)->num_points-1; ++i )
	{
		struct Vector2* p0 = &(*s)->points[i];
		struct Vector2* p1 = &(*s)->points[i+1];
		struct Vector2 Q, R;

		Q.x = 0.75f*p0->x + 0.25f*p1->x;
		Q.y = 0.75f*p0->y + 0.25f*p1->y;

		R.x = 0.25f*p0->x + 0.75f*p1->x;
		R.y = 0.25f*p0->y + 0.75f*p1->y;

		temp[v].x = Q.x;
		temp[v].y = Q.y;
		v++;
		temp[v].x = R.x;
		temp[v].y = R.y;
		v++;
	}

	temp[new_size-1].x = last.x;
	temp[new_size-1].y = last.y;

	free((*s)->points);
	(*s)->points = (struct Vector2*) malloc( sizeof( struct Vector2 ) * new_size );
	memcpy( (*s)->points, temp, sizeof( struct Vector2 ) * new_size );
	free(temp);

	(*s)->num_points = new_size;
}

int create_spline( struct spline_t* s, struct Vector2* bp, int bp_count, int tess, float speed )
{
	/* Sanity checks */
	/* For a straight line, just a regular line segment to avoid wasting memory */
	if( !s || !bp || bp_count < 3 )
		return 0;


	/* Copy the list of base points */
	/* The copy of spline_t::base_points will be modfied during tesselation */
	s->base_points = (struct Vector2*) malloc( sizeof( struct Vector2 ) * bp_count );
	if( !s->base_points )
		return 0;

	s->points = (struct Vector2*) malloc( sizeof( struct Vector2 ) * bp_count );
	if( !s->points )
		return 0;

	memcpy( s->base_points, bp, sizeof( struct Vector2 ) * bp_count );
	memcpy( s->points, bp, sizeof( struct Vector2 ) * bp_count );

	/* Set other important fields */
	s->num_base_points = bp_count;
	s->num_points = bp_count;
	s->tesselation = tess;
	s->speed = speed * 0.01f;
	s->start_point = 0;
	s->end_point = 1;
	s->delta = 0.0f;

	/* Tesselate the spline (if desired) */
	while( tess > 0 )
	{
		tesselate( &s );
		tess--;
	}

	return 1;
}

void delete_spline( struct spline_t* s )
{
	/* Delete allocated memory */
	if(s->base_points)
	{
		free(s->base_points);
		s->base_points = NULL;
	}
	if(s->points)
	{
		free(s->points);
		s->points = NULL;
	}
}

void draw_spline( struct spline_t* s, float* colour )
{
#if 0
	/* Draw the spline (useful for debugging) */
	glDisable( GL_TEXTURE_2D );
	glColor4fv( colour );
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_FLOAT, 0, (float*) s->points );
	glDrawArrays( GL_LINE_STRIP, 0, s->num_points );
#if 1
	glPointSize(5);
	glVertexPointer( 2, GL_FLOAT, 0, (float*) s->base_points );
	glDrawArrays( GL_POINTS, 0, s->num_base_points );
	glPointSize(1);
#endif
	glDisableClientState( GL_VERTEX_ARRAY );
	glEnable( GL_TEXTURE_2D );
#endif
}

int move_position_on_spline( struct spline_t* s )
{
	/* Move the entity's position along this spline using the given speed
	   settings.  When the entity reaches the end of the spline, this function
	   will return a non-zero value. */

	struct Vector2 pos;
	struct Vector2 p1;
	struct Vector2 p2;
	int end_of_line = 0;

	if( !s )
		return 0;

	memcpy( &p1, &s->points[s->start_point], sizeof( struct Vector2 ) );
	memcpy( &p2, &s->points[s->end_point], sizeof( struct Vector2 ) );

	pos.x = p1.x + s->delta * ( p2.x - p1.x );
	pos.y = p1.y + s->delta * ( p2.y - p1.y );

	s->delta += s->speed;

	if( s->delta > 1.0f )
	{
		if( !end_of_line )
			s->start_point++;

		if( ++s->end_point > s->num_points-1 )
		{
		//	s->start_point = 0;
		//	s->end_point = 1;

			end_of_line = 1;
		}

		s->delta = 0.0f;
	}

	return end_of_line;
}

void get_current_position_on_spline( struct spline_t* s, struct Vector2* vout )
{
	/* Return the current position of the entity's movement on this spline */
	struct Vector2 p1;
	struct Vector2 p2;

	if( !s )
		return;

	if( !vout )
		return;

	memcpy( &p1, &s->points[s->start_point], sizeof( struct Vector2 ) );
	memcpy( &p2, &s->points[s->end_point], sizeof( struct Vector2 ) );

	vout->x = p1.x + s->delta * ( p2.x - p1.x );
	vout->y = p1.y + s->delta * ( p2.y - p1.y );
}

void get_next_position_on_spline( struct spline_t* s, struct Vector2* vout )
{
	/* Return the next position of the entity's movement on this spline */
    /* Predict movement by one increment. */
    
	struct Vector2 p1;
	struct Vector2 p2;
    float d = s->delta + s->speed;
    
	if( !s )
		return;
    
	if( !vout )
		return;
    
	memcpy( &p1, &s->points[s->start_point], sizeof( struct Vector2 ) );
	memcpy( &p2, &s->points[s->end_point], sizeof( struct Vector2 ) );
    
	vout->x = p1.x + d * ( p2.x - p1.x );
	vout->y = p1.y + d * ( p2.y - p1.y );
}

int copy_spline( struct spline_t* dst, struct spline_t* src )
{
	/* This function simply makes a copy of an existing spline and attempts to copy
	   the data over to the new spline being created based on what data has already
	   been provided. The caller is responsible for deallocating the copied spline. */

	/* Before copying a darn thing, we need to verify that we have valid pointers */
	if( !dst || !src )
		return 0; /* fail */

	/* We have valid spline pointers to work with. Create a new spline based on the
	   original one. */
	return create_spline( dst, src->base_points, src->num_base_points, src->tesselation, src->speed );
}
