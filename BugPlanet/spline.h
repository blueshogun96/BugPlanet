#pragma once

/* Spline structure */
struct spline_t
{
	struct Vector2* base_points;	/* The base points used to calculate this spline */
	struct Vector2* points;			/* The actual points forming this spline (after tesselation) */
	int num_base_points;			/* The number of base points */
	int num_points;					/* The number of points (after tesselation) */
	int tesselation;				/* Number of times this spline is to be tesselated */
	int start_point;				/* The point in which the entity is moving from (in a given segment) */
	int end_point;					/* The point in which the entity is moving to (in a given segment) */
	float delta;					/* Used in movement calculation (from one segment to the next) */
	float speed;					/* Speed of entity moving along this line (max 100) */
};


/* Spline calulation and rendering functions */
int create_spline( struct spline_t* s, struct Vector2* bp, int bp_count, int tess, float speed );
void delete_spline( struct spline_t* s );
void draw_spline( struct spline_t* s, float* colour );
int move_position_on_spline( struct spline_t* s );
void get_current_position_on_spline( struct spline_t* s, struct Vector2* vout );
void get_next_position_on_spline( struct spline_t* s, struct Vector2* vout );
int copy_spline( struct spline_t* dst, struct spline_t* src );