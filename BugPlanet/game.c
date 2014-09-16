//
//  game.c
//  BugPlanet
//
//  Created by admin on 12/1/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#include "platform.h"
#include "vmath.h"
#include "ogldrv.h"
#include "aldrv.h"
#include "linkedlist.h"
#include "texfont.h"
#include "wavstream.h"
#include "spline.h"
#include "ezxml.h"
#include "game.h"
#include "input.h"
#include "menu.h"
#include "osx_util.h"
#include "rng.h"

#include <pthread.h>


/*
 * Globals
 */
void draw_game();
void update_game();

const int max_foliage = 64;

struct render_target_t render_target;  /* Our render target */

struct input_t input;           /* Input status */
struct node_t* tex = NULL;      /* Textures */
struct node_t* sfx = NULL;      /* Sound Effects */
struct node_t* enemies = NULL;  /* Enemies */
struct node_t* u_shoots = NULL; /* User shoots */
struct node_t* e_shoots = NULL; /* Enemy shoots */
struct node_t* explosions = NULL;/* Explosions */
struct node_t* splines = NULL;  /* Splines for enemy movement */
struct node_t* bubbles = NULL;  /* Deadly bubbles! */
struct node_t* crystals = NULL; /* Crystals (sweet rewards) */
struct node_t* missiles = NULL; /* Missles (from user) */
struct node_t* smoke = NULL;    /* Smoke puffs */
struct node_t* shockwave = NULL;/* Shockwaves */
struct bomb_t bomb;             /* Bombs (from user) */
struct extra_bomb_t extra_bomb; /* A reward for every 20,000 points */
struct user_t user;             /* User details */
struct font_t font1;            /* Basic font #1 */
struct rng_t  rng;              /* Random number generator */
struct foliage_t foliage[max_foliage];   /* Foliage (i.e. plants, grass and mushrooms) */
struct game_settings_t settings;/* Various game settings */
struct game_stats_t game_stats; /* Current game stats */
struct star_t   stars[2];       /* Stars (for ship #1) */
struct diff_settings_t diff_settings;   /* Difficulty settings */
struct diff_settings_t const_diff_settings[3] =
{
    { 60, 25, 50, 50, 30, 2.5f, 5.0f, 30 },
    { 50, 20, 45, 45, 25, 2.5f, 5.0f, 30 },
    { 40, 15, 40, 40, 20, 3.0f, 7.0f, 20 },
};

struct wave_stream* bgm = NULL; /* Background music */

int spline_count = 0;           /* Spline count */
int finish_fade = No;
int reset_fade = No;
int ingame_fade = -1;
float fade_rect[4] = { 0, 0, 640.0f, 480.0f };
float fade_colour[3] = { 0, 0, 0 };

int bgm_fade = No;
char bgm_next_track[64] = "";
int bgm_volume = 10;	/* 10=loudest,0=silent */
int change_music = No;
volatile int exit_streaming_audio_thread = No;
pthread_t audio_stream_thread = NULL;
pthread_mutex_t audio_stream_mutex;

int fps_limit = 60;             /* Frame rate limit for this game */
uint64_t fps = 0;               /* Current frame rate */
int reduce_speed = No;          /* Reduce speed by half when the game has too many sprites */
float elapsed_time = 0;         /* The amount of time going towards the next level */
int bomb_score = 0;             /* The amount of points going towards a free bomb */
int paused = No;                /* Game pause flag */
int skip_frame = No;            /* Hacky time */
int intro = Yes;                /* Are we viewing the intro? */
int swidth, sheight;
int global_id = 0;              /* Global enemy id tracker */
int ingame = No;

void (*draw_func)(void);        /* Drawing function */
void (*update_func)(void);      /* Update function */

/* iOS/QNX specific */
extern int _init_func();
extern void (*_display_func)(void);
extern void (*_touch_func)(int, int, int, int);
extern void (*_move_func)(int, int);
extern void (*_exit_func)();

void add_explosion( float x, float y, int is_big );
void add_new_crystal( float x, float y, int is_large );
void use_bomb();
void enemy_delete_func( void* ptr );
void add_smoke( float x, float y );
void add_new_shockwave( float x, float y, float vx, float vy, float max_size );
void check_for_extra_bomb( float x, float y );
void nullify_shoots( int owner );
void crystalize_shoots( int owner );


void get_tex_coords( struct texture_t* t, struct Rect* r, float* tc )
{
    tc[0] = r->x1 / t->width;
    tc[1] = 1.0f - ( r->y1 / t->height );
    tc[2] = r->x2 / t->width;
    tc[3] = 1.0f - ( r->y1 / t->height );
    tc[4] = r->x2 / t->width;
    tc[5] = 1.0f - ( r->y2 / t->height );
    tc[6] = r->x1 / t->width;
    tc[7] = 1.0f - ( r->y2 / t->height );
}

void scale_tex_coords( float sx, float sy, float* tc )
{
    tc[0] *= sx;
    tc[2] *= sx;
    tc[4] *= sx;
    tc[6] *= sx;
    
    tc[1] *= sy;
    tc[3] *= sy;
    tc[5] *= sy;
    tc[7] *= sy;
}

float clamp_float( float val, float min, float max )
{
    if( val < min )
        val = min;
    if( val > max )
        val = max;
    
    return val;
}

float absf( float v )
{
    if( v < 0 )
        v *= -1;
    
    return v;
}

void dbg_brk()
{
    unsigned char code[2] = { 0xCC, 0xC2 };
    void (*f)() = (void(*)())code;
    
    f();
}

void play_snd( int sound, int loop )
{
    struct sound_t* snd = list_get_node_data( &sfx, sound );
    
    if(snd)
        play_sound_effect_static( snd, loop );
}

void stop_snd( int sound )
{
    struct sound_t* snd = list_get_node_data( &sfx, sound );
    
    if(snd)
        stop_sound_effect( snd );
}

/*
 * BGM related functions
 */

int fade_out_bgm()
{
	int finished = No;
	static int reset = Yes;
	static float volume = 1.0f;
    
    if( !bgm )
        return 0;
    
	if( reset )
	{
		volume = (float)(((float)bgm_volume)/10.0f);
		reset = No;
	}
    
	/* Continually decrease bgm volume until it reaches 0 */
    
	/* Are we done yet? */
	if( volume <= 0.0f )
	{
		/* Mark as finished */
		finished = Yes;
		volume = (float)(((float)bgm_volume)/10.0f);
        
		/* Close the current track */
		wavestream_close(&bgm[0]);
        
		/* Are we loading a new track? */
		if( strcmp( "", bgm_next_track ) )
		{
			/* If so, open it and start playing */
			wavestream_open(bgm_next_track, &bgm[0]);
			wavestream_play(&bgm[0]);
			wavestream_set_volume(&bgm[0], (float)(((float)bgm_volume)/10.0f));
		}
		else
		{
			/* If not, don't do a darn thing */
		}
	}
	else
	{
		wavestream_set_volume( &bgm[0], volume );
		volume -= 0.02f;
	}
    
	return finished;
}

void set_next_track( char* track )
{
    strcpy( bgm_next_track, track );
    change_music = Yes;
}

char* get_track_name()
{
    return bgm_next_track;
}

void handle_music_fade()
{
    if( change_music )
	{
		if( fade_out_bgm() )
			change_music = No;
	}
}

int msleep(unsigned long milisec)
{
    struct timespec req={0};
    time_t sec=(int)(milisec/1000);
    milisec=milisec-(sec*1000);
    req.tv_sec=sec;
    req.tv_nsec=milisec*1000000L;
    while(nanosleep(&req,&req)==-1)
        continue;
    return 1;
}

void* audio_stream_processing_thread_function( void* ptr )
{
    /* Load the selected track */
    strcpy( bgm_next_track, "transparence.wav" );
    bgm = malloc( sizeof( struct wave_stream ) );
    wavestream_open( bgm_next_track, &bgm[0] );
    set_next_track( "transparence.wav" );
    
    while( Yes )
    {
        /* Update streaming BGM */
        wavestream_update( &bgm[0] );
        handle_music_fade();
        
        /* If it's time to exit, then clean up and attempt to close this thread. */
        if( exit_streaming_audio_thread )
        {
            wavestream_close( &bgm[0] );
            free( bgm );
            
            pthread_exit( NULL );
        }
        
        /* Stall this thread for roughly one frame */
        //block_until_vertical_blank();
        msleep( 1000/60 );
    }
    
    /* Shouldn't get here */
    return NULL;
}

/*
 * Fading functions
 */

void fade_settings( float* frect, float* fcolour )
{
    memcpy( fade_rect, frect, sizeof(float)*4 );
    memcpy( fade_colour, fcolour, sizeof(float)*3 );
}

void fade_out()
{
    ingame_fade = 0;
}

void fade_in()
{
    ingame_fade = 1;
}

int do_fade( int fade_in )
{
	int finished = No;
	extern float current_colour[4];
    
	/* Enable 2D rendering */
//  enable_2d();
    
	if( fade_in )
	{
		static float a = 255.0f;
        
        if( reset_fade )
        {
            a = 255.0f;
            reset_fade = No;
        }
        
		transparent_blend(Yes);
		set_colour( fade_colour[0], fade_colour[1], fade_colour[2], a/255.0f );
		draw_quad( 0, fade_rect[0], fade_rect[1], fade_rect[2], fade_rect[3] );
		transparent_blend(No);
        
		a -= 5.0f;
        
		if( a < 0.0f )
		{
			a = 255.0f;
			finished = Yes;
		}
	}
	else
	{
		static float a = 0.0f;
        
        if( reset_fade )
        {
            a = 0;
            reset_fade = No;
        }
        
		transparent_blend(Yes);
		set_colour( fade_colour[0], fade_colour[1], fade_colour[2], a/255.0f );
		draw_quad( 0, fade_rect[0], fade_rect[1], fade_rect[2], fade_rect[3] );
		transparent_blend(No);
        
		a += 5.0f;
        
		if( a > 255.0f )
		{
			a = 0.0f;
			finished = Yes;
		}
	}
    
    set_colour( current_colour[0], current_colour[1], current_colour[2], current_colour[3] );
    
	/* Disable 2D rendering */
//	disable_2d();
    
	finish_fade = finished;
    
	return finished;
}

/*
 * Timing related stuff
 */

int calculate_framerate()
{
    fps = (uint64_t) get_frames_per_second();
    
    return 1;
}

void show_fps()
{
#if 0
    char string[16];
    char string2[16];
    char string3[16];
    char string4[16];
    
    extern int spf, batched_spf, unbatched_spf;
    
    sprintf( string, "FPS %d", (int)fps );
    sprintf( string2, "Sprites: %d", spf );
    sprintf( string3, "Batched: %d", batched_spf );
    sprintf( string4, "Unbatched: %d", unbatched_spf );
    
    set_colour( 1.0f, 1.0f, 0.0f, 1.0f );
    
    draw_text( &font1, 200, 30, string );
    draw_text( &font1, 200, 50, string2 );
    draw_text( &font1, 200, 70, string3 );
    draw_text( &font1, 200, 90, string4 );
    
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
#endif
}

/*
 * Font stuff
 */
int create_fonts()
{
    font1.width = 181;
    font1.height = 56;
    font1.letter_offset = 0;
    font1.letter_width = 180/20;
    font1.letter_height = 55/5;
    font1.letters_per_row = 20;
    font1.has_colour_key = Yes;
    font1.colour_key = 0xFF000000;
   // sprintf( font1.texname, "sfont2.tga" );
    font1.texname = "sfont2.tga";
    
    return init_font( &font1 );
}

void delete_fonts()
{
    uninit_font( &font1 );
}

/*
 * User function(s)
 */

void reset_user()
{
    /* Reset the user's status */
    
    ZeroMemory( &user, sizeof( struct user_t ) );
    user.x = 320.0f;
    user.y = 320.0f;
    user.score = 0;
    user.ships = 3;
    user.type = settings.ship_type;
    user.bombs = 3;
    user.speed = user.type == 0 ? 5.0f : 3.0f;
    user.collision_rect_size = (user.type == 0) ? 2.0f : 3.0f;
    bomb_score = 0;
    ZeroMemory( stars, sizeof( struct star_t ) * 2 );
}

void render_user()
{
    float tc[8];
    struct Rect r = { 17*(user.frame), 0, 17*(user.frame+1), 21 };
    struct texture_t* th = 0;
    float sprw = 17.0f*1.5f;
    float sprh = 21.0f*1.5f;
    
    if( user.ships < 1 )
        return;
    
    /* Set the texture list as the current head */
//    list_set_head( tex );
    
    /* Search for the correct sprite texture */
    th = list_get_node_data( &tex, 18+user.type );
    
    /* Now let's draw the sprite */
    if( !user.r_delay )
    {
        get_tex_coords( th, &r, tc );
//        scale_tex_coords( 17.0f/34.0f, 21.0f/32.0f, tc );
        draw_quad2( th->handle, tc,  user.x-(sprw/2.0f), user.y-(sprh/2.0f), sprw, sprh );
    }
}

void add_user_shoot( int is_double )
{
    /* Attempt to add a new user shoot */
    
    struct user_shoot_t* shoot = NULL;
    //ZeroMemory( &shoot, sizeof( struct user_shoot_t ) );
    shoot = malloc( sizeof( struct user_shoot_t ) );
    
    shoot->type = is_double;
    shoot->velocity = 7.0f;
    shoot->x = user.x;
    shoot->y = user.y;
    shoot->was_used = No;
    
    list_add_end( &u_shoots, shoot );
}

void add_small_user_shoot( float x, float y )
{
    /* Attempt to add a new user shoot */
    
    struct user_shoot_t* shoot = NULL;
    //ZeroMemory( &shoot, sizeof( struct user_shoot_t ) );
    shoot = malloc( sizeof( struct user_shoot_t ) );
    
    shoot->type = 0;
    shoot->velocity = 7.0f;
    shoot->x = x;
    shoot->y = y;
    shoot->was_used = No;
    
    list_add_end( &u_shoots, shoot );
}

void draw_user_shoots()
{
    struct node_t* n = u_shoots;
    struct texture_t* th = list_get_node_data( &tex, 5 );
    
    /* Go through the entire list of user shoots and render them all */
    
    set_colour( 1, 1, 1, 1 );
    while( n != NULL )
    {
        struct user_shoot_t* s = n->data;
        n = n->next;
        
        /* TODO: Textures and stuff */
        if(s)
        {
            if( s->type == 1 )
            {
                float tc[8] = { 0.0f, 1, 32.0f/264.0f, 1, 32.0f/264.0f, 0.66f, 0.0f, 0.66f };
                struct Rect r = { 0, 0, 33, 33 };
                get_tex_coords( th, &r, tc );
                draw_quad2_batched( tc, s->x-16.0f, s->y-16.0f, 32.0f, 32.0f );
            }
            else
            {
                float tc[8];// = { 0.0f, 1, 32.0f/264.0f, 1, 32.0f/264.0f, 0.66f, 0.0f, 0.66f };
                struct Rect r = { 33, 0, 66, 33 };
                get_tex_coords( th, &r, tc );
                draw_quad2_batched( tc, s->x-12.0f, s->y-12.0f, 24.0f, 24.0f );
            }
        }

        //n = n->next;
    }
    
    flush_batch( th->handle );
    
#if 0
    while( i < length )
    {
        struct user_shoot_t* s = list_get_node_data( &u_shoots, i+1 );
        
        /* TODO: Textures and stuff */
        if(s)
        {
            struct texture_t* th = list_get_node_data( &tex, 1 );
            float tc[8] = { 0.0f, 1, 32.0f/264.0f, 1, 32.0f/264.0f, 0.66f, 0.0f, 0.66f };
            draw_quad2( th->handle, tc, s->x-16.0f, s->y-16.0f, 32.0f, 32.0f );
        }
        
        i++;
    }
#endif
}

void user_shoot_delete_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void delete_user_shoots()
{
    set_deletion_callback( user_shoot_delete_func );
    list_clear( &u_shoots );
}

void update_user_shoots()
{
    struct node_t* n = u_shoots;
    
    /* Go through the entire list of user shoots and update them all */
    
    while( n != NULL )
    {
        struct user_shoot_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            s->y -= s->velocity;
            if( s->y < 0 )
            {
                set_deletion_callback( user_shoot_delete_func );
                list_delete( &u_shoots, s );
                s = NULL;
            }
            
            if( s != NULL )
            {
                if( s->was_used )
                {
                    set_deletion_callback( user_shoot_delete_func );
                    list_delete( &u_shoots, s );
                }
            }
        }
        
        //n = n->next;
    }
}

void update_user()
{
    int width, height;
    get_dimensions( &height, &width );
    
    /* Create a big explosion when the user is dead */
    if( user.r_delay > 0 )
    {
        int x = rng.random(17);
        int y = rng.random(21);
        
        float xx = ( user.x - 8.0f ) + x;
        float yy = ( user.y - 10.0f ) + y;
        
        add_explosion( xx, yy, No );
        
        user.r_delay--;
        
        if( user.r_delay <= 0 )
        {
            user.x = 320.0f;
            user.y = (float) sheight/2;
            user.i_delay = 90;
        }
        
        return;
    }
    
    /* Check for game over */
    if( user.ships < 1 )
    {
        if( finish_fade )
        {
            fade_in();
            drop_to_post_game_menu();
            skip_frame = Yes;
            ingame = No;
        }
        
        return;
    }
    
    if( user.ships < 1 )
        return;
    
    /* Animation */
    if( ++user.a_delay > 3 )
    {
        user.a_delay = 0;
        user.frame = !user.frame;
    }
    
    /* Update invincibility timer */
    if( user.i_delay > 0 )
        user.i_delay--;
    
    /* Move user */
    if( input.touch )
    {
        static int ox = 0, oy = 0;
        int dx = input.x - input.lx;
        int dy = input.y - input.ly;
        
        if( input.timestamp == 1 )
        {
            ox = input.x - user.x;
            oy = input.y - user.y;
        }
        
        /*if( reduce_speed )
        {
            dx *= 2;
            dy *= 2;
        }*/
        
        user.x = input.x - ox;
        user.y = input.y - oy;
    }
    
    /* Boundary check */
    if( user.x < 160.0f ) user.x = 160.0f;
    if( user.x > 480.0f ) user.x = 480.0f;
    if( user.y < 0.0f )   user.y = 0.0f;
    if( user.y > height ) user.y = height;
    
    if( input.firing  && user.s_delay <= 0 )
    {
        add_user_shoot(Yes);
        user.s_delay = 4;
        play_snd( 4, No );
    }
    else
    {
        user.s_delay--;
    }
    
    if( input.bomb && input.timestamp <= 3 )
    {
        use_bomb();
    }
}

void draw_user_stars()
{
    /* Only draw user stars if the ship is type #1 */
    if( user.type == 0 && user.r_delay == 0 )
    {
        struct texture_t* th = list_get_node_data( &tex, 22 );
        float tc[8];
        struct Rect r = { 12*(stars[0].frame), 0, 12*(stars[0].frame+1), 13 };
        
        get_tex_coords( th, &r, tc );
        
        draw_quad2( th->handle, tc, stars[0].x-9.0f, stars[0].y-9.0f, 18, 18 );
        draw_quad2( th->handle, tc, stars[1].x-9.0f, stars[1].y-9.0f, 18, 18 );
    }
}

void update_user_stars()
{
    if( user.type == 0 && user.r_delay == 0 )
    {
        stars[0].x = user.x - 20;
        stars[0].y = user.y + 10;
        
        stars[1].x = user.x + 20;
        stars[1].y = user.y + 10;
        
        stars[0].frame_timer++;
        if( stars[0].frame_timer > 5 )
        {
            stars[0].frame_timer = 0;
            stars[0].frame++;
            
            if( stars[0].frame > 2 )
                stars[0].frame = 0;
        }
        
        if( input.firing )
        {
            stars[0].shot_timer++;
            if( stars[0].shot_timer > 8 )
            {
                add_small_user_shoot( stars[0].x, stars[0].y );
                stars[0].shot_timer = 0;
            }
        
            stars[1].shot_timer++;
            if( stars[1].shot_timer > 8 )
            {
                add_small_user_shoot( stars[1].x, stars[1].y );
                stars[1].shot_timer = 0;
            }
        }
    }
}

/*
 * Missile functions
 */

int find_nearest_enemy_to_user()
{
    struct node_t* n = enemies;
    float d = 0, shortest_d;
    int current_enemy = 1;
    int nearest_enemy = 0;
    
    if( n == NULL )
        return 0;   /* No enemies on screen */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        
        if( e != NULL )
        {
            /* Get the distance between this enemy and the user */
            d = distance2d( user.x, user.y, e->x, e->y );
            
            /* If this is the first enemy, then go ahead and use this distance */
            if( current_enemy == 1 )
            {
                shortest_d = d;
                nearest_enemy = current_enemy;
            }
            else
            {
                /* Check the distance of this enemy */
                if( d < shortest_d )
                {
                    shortest_d = d;
                    nearest_enemy = current_enemy;
                }
            }
        }
        
        n = n->next;
        current_enemy++;
    }
    
    return nearest_enemy;
}

int find_nearest_enemy_to_missile( struct missle_t* m )
{
    struct node_t* n = enemies;
    float d = 0, shortest_d;
    int current_enemy = 1;
    int nearest_enemy = 0;
    
    if( n == NULL )
        return 0;   /* No enemies on screen */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        
        if( e != NULL )
        {
            /* Get the distance between this enemy and the user */
            d = distance2d( m->x, m->y, e->x, e->y );
            
            /* If this is the first enemy, then go ahead and use this distance */
            if( current_enemy == 1 )
            {
                shortest_d = d;
                nearest_enemy = current_enemy;
            }
            else
            {
                /* Check the distance of this enemy */
                if( d < shortest_d )
                {
                    shortest_d = d;
                    nearest_enemy = current_enemy;
                }
            }
        }
        
        n = n->next;
        current_enemy++;
    }
    
    return nearest_enemy;
}


void missile_deletion_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_new_missile( float x, float y )
{
    struct missle_t* m = malloc( sizeof( struct missle_t ) );
    int nearest_enemy = find_nearest_enemy_to_user();
    struct enemy_t* e = list_get_node_data( &enemies, nearest_enemy );
    float a;
    
    m->x = x;
    m->y = y;
    
    if( nearest_enemy != 0 )
        a = angle( user.x, user.y, e->x, e->y );
    else
        a = angle( user.x, user.y, user.x, user.y-2 );
    
    m->rot = a*(360.0f/(22.0f/3.5f));
     
    m->vx = cosf(a) * 7.0f;
    m->vy = sinf(a) * 7.0f;
     
    m->vx = clamp_float( m->vx, -10.0f, 10.0f );
    m->vy = clamp_float( m->vy, -10.0f, 10.0f );
    m->x += m->vx;
    m->y += m->vy;
    m->was_used = No;
    
    list_add_beginning( &missiles, m );
}

void delete_missiles()
{
    set_deletion_callback( missile_deletion_func );
    list_clear( &missiles );
}

void draw_missiles()
{
    struct node_t* n = missiles;
    struct texture_t* th = list_get_node_data( &tex, 20 );
    
    if( user.type != 1 )
        return;
    
    while( n != NULL )
    {
        struct missle_t* m = n->data;
        
        if( m != NULL )
        {
            push_pos();
            translate( m->x, m->y );
            rotate( m->rot-270.0f );
            draw_quad( th->handle, -16.0f, -16.0f, 32.0f, 32.0f );
            pop_pos();
        }
        
        n = n->next;
    }
}

void check_missiles_for_collisions()
{
    struct node_t* n = enemies;
    
    if( !enemies )
        return;
    
    if( !missiles )
        return;
    
    /* Go through the entire list of enemies and check for collisions with the missile */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        n = n->next;
        
        if( e != NULL )
        {
            float x1 = e->x-(e->w/2.0f);
            float y1 = e->y-(e->h/2.0f);
            float x2 = e->x+(e->w/2.0f);
            float y2 = e->y+(e->h/2.0f);
            
            struct node_t* mn = missiles;
            
            while( mn != NULL )
            {
                struct missle_t* m = mn->data;
                mn = mn->next;
                
                if( m != NULL )
                {
                    if( m->x > x1 && m->x < x2 &&
                       m->y > y1 && m->y < y2 )
                    {
                        /* Subtract the damage and check for a kill */
                        e->energy -= 5;
                        
                        if( e->energy < 1 )
                        {
                            /* If so, delete this enemy, it's spline, and add an explosion */
                            /* Also, add it's score value to the user's score */
                            user.score += e->score_value;
                            bomb_score += e->score_value;
                            user.bugs++;
                            add_explosion( e->x, e->y, No );
                            
                            /* Add a large crystal */
                            add_new_crystal( e->x, e->y, Yes );
                            
                            /* Nullify shots from small enemies, crystalize shots from large ones. */
                            if(!e->is_large)
                                nullify_shoots(e->id);
                            else
                                crystalize_shoots(e->id);
                            
                            set_deletion_callback( enemy_delete_func );
                            list_delete( &enemies, e );
                            e = NULL;
                        }
                        else
                            e->flashing = Yes;
                        
                        /* Go ahead and delete this missile too */
                        set_deletion_callback( missile_deletion_func );
                        list_delete( &missiles, m );
                        m = NULL;
                        
                        /* Play sound effect */
                        play_snd( 9, No );
             //           m->was_used = Yes;
                        
                        break;
                    }
                }
                
             //   mn = mn->next;
            }
        }
        
    //    n = n->next;
    }
}

void update_missiles()
{
    struct node_t* n = missiles;
    static int m_delay = 0;
    
    if( user.type != 1 )
        return;
    
    while( n != NULL )
    {
        struct missle_t* m = n->data;
        n = n->next;
        
        if( m != NULL )
        {
            int nearest_enemy = find_nearest_enemy_to_missile(m);
            struct enemy_t* e = list_get_node_data( &enemies, nearest_enemy );
            float a;
            
            /* Update the missile's position and rotation */
            
            /* If there's no enemy available to track, then just move the missle upward */
            if( nearest_enemy != 0 )
                a = angle( user.x, user.y, e->x, e->y );
            else
                a = angle( user.x, user.y, user.x, user.y-2 );
            
            m->rot = a*(360.0f/(22.0f/3.5f));
            m->vx = cosf(a) * 7.0f;
            m->vy = sinf(a) * 7.0f;
            
            m->vx = clamp_float( m->vx, -10.0f, 10.0f );
            m->vy = clamp_float( m->vy, -10.0f, 10.0f );
            m->x += m->vx;
            m->y += m->vy;
            
            /* Add smoke puffs */
            if( ++m->smoke_timer >= 5 )
            {
                m->smoke_timer = 0;
                add_smoke( m->x, m->y );
            }
            
            /* Check this missle for going out of bounds */
            if( m->x < 160.0f || m->x > 480.0f || m->y < 0 || m->y > sheight )
            {
                set_deletion_callback( missile_deletion_func );
                list_delete( &missiles, m );
                m = NULL;
            }
            
            /* Check for used missiles too */
            if( m != NULL )
            {
                if( m->was_used )
                {
                    set_deletion_callback( missile_deletion_func );
                    list_delete( &missiles, m );
                    m = NULL;
                }
            }
        }
        
    //    n = n->next;
    }
    
    m_delay++;
    
    if( input.firing && m_delay > 30 && user.r_delay <= 0 && user.ships > 0 )
    {
        add_new_missile( user.x-15, user.y );
        add_new_missile( user.x+15, user.y );
        m_delay = 0;
        play_snd( 3, No );
    }
    
    check_missiles_for_collisions();
}

/*
 * Spline handling functions
 */

void delete_spline_func( void* ptr )
{
    if( ptr )
    {
        delete_spline( ptr );
        free( ptr );
    }
}

int init_splines()
{
    FILE* fp = NULL;
    int index = 0;
    char string[64];
    
    /* Open every existing spline and get a count of how many exist */
    while( Yes )
    {
        sprintf( string, "spline%03d.dat", index );
        fp = fopen( string, "rb" );
        
        if(fp)
        {
            fclose(fp);
            index++;
        }
        else
            break;
    }
    
    spline_count = index;
    
    return 1;
}

void uninit_splines()
{
    set_deletion_callback( delete_spline_func );
    list_clear( &splines );
}

struct spline_t* open_spline( int spline_number )
{
    FILE* fp = NULL;
    struct Vector2 base_points[64];
    struct spline_t* spline = NULL;
    char string[64];
    int i = 0;
    
    /* Sanity check */
    if( spline_number > spline_count )
        return NULL;
    
    /* Attempt to open the spline of this number */
    sprintf( string, "spline%03d.dat", spline_number );
    fp = fopen( string, "rb" );
    if( !fp )
        return NULL;
    
    /* Read the spline header and create a new spline */
    spline = malloc( sizeof( struct spline_t ) );
    fread( &spline->num_base_points, 1, sizeof( int ), fp );
    fread( &spline->tesselation, 1, sizeof( int ), fp );
    fread( base_points, 1, sizeof( struct Vector2 ) * spline->num_base_points, fp );
    fclose(fp);
    
    if( !create_spline( spline, base_points, spline->num_base_points, spline->tesselation, diff_settings.e_speed ) )
    {
        delete_spline(spline);
        free(spline);
        return NULL;
    }
    
    /* Scale the coordinates to match the screen resolution */
    while( i < spline->num_points )
    {
        float ys = ((float)sheight)/480.0f;
        spline->points[i].y *= ys;
        i++;
    }
    
    return spline;
    
    /* Add the spline to list of splines */
    /*ret = list_length( &splines );
    list_add_end( &splines, spline );
    
    return ret+1;*/
}

void close_spline( struct spline_t** spline )
{
    if( *spline )
    {
        delete_spline( *spline );
        free( *spline );
        *spline = NULL;
    }
    
    /*set_deletion_callback( delete_spline_func );
    list_delete_loc( &splines,  spline_number );*/
}


/*
 * Enemy related functions
 */

void add_enemy_shoot( struct enemy_t* e, float speed )
{
    /* Attempt to add a new user shoot */
    
    struct enemy_shoot_t* shoot = NULL;
    float cx, cy;
    shoot = malloc( sizeof( struct enemy_shoot_t ) );
    
    shoot->x = e->x;
    shoot->y = e->y;
    shoot->angle = angle( e->x, e->y, user.x, user.y );
    if( !speed )
        shoot->speed = 3.0f;
    else
        shoot->speed = speed;
    
    shoot->vx = cosf( shoot->angle ) * shoot->speed;
    shoot->vy = sinf( shoot->angle ) * shoot->speed;
    shoot->owner = e->id;
    shoot->nullified = No;
    
#if 0   /* Normalize? */
    cx = cosf( shoot->angle );
    cy = sinf( shoot->angle );
    cx = abs(cx);
    cy = abs(cy);
    shoot->vx = clamp_float( cx, -shoot->speed, shoot->speed );
    shoot->vy = clamp_float( cy, -shoot->speed, shoot->speed );
#else
    shoot->vx = clamp_float( shoot->vx, -shoot->speed, shoot->speed );
    shoot->vy = clamp_float( shoot->vy, -shoot->speed, shoot->speed );
#endif
    
    list_add_end( &e_shoots, shoot );
}

void add_enemy_shoot2( float x, float y, float angle_rad, float speed, int owner )
{
    /* Attempt to add a new user shoot */
    
    struct enemy_shoot_t* shoot = NULL;
    float cx, cy;
    shoot = malloc( sizeof( struct enemy_shoot_t ) );
    
    shoot->x = x;
    shoot->y = y;
    shoot->angle = angle_rad;
    shoot->vx = cosf( shoot->angle ) * speed;
    shoot->vy = sinf( shoot->angle ) * speed;
    shoot->speed = speed;
    shoot->owner = owner;
    shoot->nullified = No;
    
#if 0   /* Normalize? */
    cx = cosf( shoot->angle );
    cy = sinf( shoot->angle );
    cx = abs(cx);
    cy = abs(cy);
    shoot->vx = clamp_float( cx, -shoot->speed, shoot->speed );
    shoot->vy = clamp_float( cy, -shoot->speed, shoot->speed );
#else
    shoot->vx = clamp_float( shoot->vx, -shoot->speed, shoot->speed );
    shoot->vy = clamp_float( shoot->vy, -shoot->speed, shoot->speed );
#endif
    
    list_add_end( &e_shoots, shoot );
}

void draw_enemy_shoots()
{
    struct node_t* n = e_shoots;
    float size = 48.0f;
    
    float c[4];
    float def[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float blue[] = { 0.0f, 0.3f, 1.0f, 1.0f };
    float red[] = { 1.0f, 0.3f, 0.3f, 1.0f };
    float green[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float pink[] = { 1.0f, 0.0f, 1.0f, 1.0f };
    float purple[] = { 0.5f, 0.0f, 1.0f, 1.0f };
    
    /* Shogun, you are a lazy BASTARD!!!!
       Fix this before I slap you, you crazy nigger! */
    switch( settings.shoot_colour )
    {
        case 1: memcpy( c, red, sizeof(float)*4 );  break;
        case 2: memcpy( c, green, sizeof(float)*4 );break;
        case 3: memcpy( c, blue, sizeof(float)*4 ); break;
        case 4: memcpy( c, pink, sizeof(float)*4 ); break;
        case 5: memcpy( c, purple, sizeof(float)*4 ); break;
        default: memcpy( c, def, sizeof(float)*4 ); break;
    }
    
    /* Go through the entire list of enemy shoots and render them all */
    
//    set_colour( 0.0f, 0.3f, 1.0f, 1.0f );
    struct texture_t* th = list_get_node_data( &tex, 5 );
    
    while( n != NULL )
    {
        struct enemy_shoot_t* s = n->data;
        
        /* TODO: Textures and stuff */
        if(s)
        {
            float tc[8];// = { 0.0f, 1, 32.0f/264.0f, 1, 32.0f/264.0f, 0.66f, 0.0f, 0.66f };
            struct Rect r = { 33.0f*2, 33.0f, 33.0f*3, 33.0f*2 };
            
            if(s->nullified)
                c[3] = 0.5f;
            else
                c[3] = 1.0f;
            
            set_colourv(c);
            get_tex_coords( th, &r, tc );
            //draw_quad2_batched( tc, s->x-(size/2.0f), s->y-(size/2.0f), size, size );
            draw_quad2( th->handle, tc, s->x-(size/2.0f), s->y-(size/2.0f), size, size );
        }
        
        n = n->next;
    }
    
    //flush_batch( th->handle );
    
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
}

void enemy_shoot_delete_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void delete_enemy_shoots()
{
    set_deletion_callback( enemy_shoot_delete_func );
    list_clear( &e_shoots );
}

void nullify_shoots( int owner )
{
    struct node_t* n = e_shoots;
    
    /* Go through the entire list of user shoots and nullify all that were fired by this owner */
    
    while( n != NULL )
    {
        struct enemy_shoot_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            if( s->owner == owner )
                s->nullified = Yes;
        }
    }
}

void crystalize_shoots( int owner )
{
    struct node_t* n = e_shoots;
    
    /* Go through the entire list of user shoots and crystalize all that were fired by this owner */
    
    while( n != NULL )
    {
        struct enemy_shoot_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            if( s->owner == owner )
            {
                /* Add a small crystal */
                add_new_crystal( s->x, s->y, No );
                /* Also add a small shockwave */
                add_new_shockwave( s->x, s->y, 0, 0, 32.0f );
                
                /* Delete this shoot */
                set_deletion_callback( enemy_shoot_delete_func );
                list_delete( &e_shoots, s );
            }
        }
    }
}

void update_enemy_shoots()
{
    struct node_t* n = e_shoots;
    
    /* Go through the entire list of user shoots and update them all */
    
    while( n != NULL )
    {
        struct enemy_shoot_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            struct Rect userRect = { user.x-user.collision_rect_size, user.y-user.collision_rect_size,
                user.x+user.collision_rect_size, user.y+user.collision_rect_size };
            
            /* Move the enemy shoot in it's direction */
            s->x += s->vx;
            s->y += s->vy;
            
            /* Check for screen boundaries */
            if( s->y < 0 || s->y > sheight || s->x < 160.0f || s->x > 480.0f )
            {
                set_deletion_callback( enemy_shoot_delete_func );
                list_delete( &e_shoots, s );
                s = NULL;
            }
            
            /* Check for collisions with player */
            if( s != NULL && user.r_delay == 0 && user.i_delay == 0 )
            {
                if( point_in_rect( &userRect, s->x, s->y ) && !s->nullified )
                {
                    set_deletion_callback( enemy_shoot_delete_func );
                    list_delete( &e_shoots, s );
                    
                    /* Katsu! */
                    if( user.ships > 0 )
                    {
                        user.ships--;
                        
                        if( settings.vibrate )
                            vibrate();
                        
                        /* Check for game over */
                        if( user.ships == 0 )
                        {
                            set_next_track( "silence.wav" );
                            fade_out();
                        }
                    }
                    
                    user.r_delay = 60;
                    play_snd( 7, No );
                
                }
            }
            
        }
        
    //    n = n->next;
    }
}

void enemy_delete_func( void* ptr )
{
    if( ptr )
    {
        /* Check for spline */
        struct spline_t* s = ((struct enemy_t*) ptr)->spline;
        close_spline(&s);
        
        free( ptr );
        ptr = NULL;
    }
}

void add_new_enemy()
{
    struct enemy_t* enemy;
    int large = rng.random(diff_settings.e_large_odds);
    
    /* TODO: Maybe have a 1/20 chance for a larger enemy to show up initially
       and maybe increase those odds as the user progresses. */
    
    enemy = malloc( sizeof( struct enemy_t ) );
    enemy->x = 0.0f;
    enemy->y = 0.0f;
    enemy->w = 17.0f*1.5f;
    enemy->h = 12.0f*1.5f;
    enemy->is_large = (large == 1) ? Yes : No;
    enemy->energy = enemy->is_large ? 40 : 1;
    enemy->type = enemy->is_large ? rng.random(3) : rng.random(5);
    enemy->s_delay = 10;
    enemy->a_delay = 5;
    enemy->tex = enemy->is_large ? 3 : 2;
    enemy->rot = 0.0f;
    enemy->score_value = enemy->is_large ? 1000 : 20;
    enemy->s_delay = rng.random(80);
    enemy->spline = open_spline( rng.random(spline_count-1) );
    enemy->flashing = 0;
    enemy->id = global_id++;
    memset( enemy->v, 0, sizeof( float ) * 10 );
    
    if( enemy->is_large )
    {
        enemy->v[8] = (float) rng.random(200)+100;
        enemy->x = (float) rng.random(300)+160;
    }
    
    if( enemy->type == 0 )
        dbg_brk();
    
    list_add_end( &enemies, enemy );
}

void draw_enemies()
{
    struct node_t* n = enemies;
    
    /* Go through the entire list of enemies and update them all */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        
        if( e != NULL )
        {
            struct texture_t* th = list_get_node_data( &tex, e->tex );
            float tc[8];
            
            /* Is this a large bug? */
            if( e->is_large )
            {
                /* Is it flashing? */
                if( e->flashing )
                    flash_white(Yes);
                
                if( e->type == 1 )
                {
                    struct Rect r = { 55*e->v[9], 0, 55*(e->v[9]+1), 38 };
            
                    e->w = 55.0f;
                    e->h = 38.0f;
                    
                    get_tex_coords( th, &r, tc );
            
                    push_pos();
                    translate( e->x, e->y );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }
                
                if( e->type == 2 )
                {
                    struct Rect r = { 68*e->v[9], 39, 68*(e->v[9]+1), 80 };
                    
                    e->w = 68.0f;
                    e->h = 42.0f;
                    
                    get_tex_coords( th, &r, tc );
                    
                    push_pos();
                    translate( e->x, e->y );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }

                if( e->type == 3 )
                {
                    struct Rect r = { 68*e->v[9], 81, 68*(e->v[9]+1), 118 };
                    
                    e->w = 64.0f;
                    e->h = 38.0f;
                    
                    get_tex_coords( th, &r, tc );
                    
                    push_pos();
                    translate( e->x, e->y );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }

                /* Stop flashing */
                if( e->flashing )
                {
                    flash_white(No);
                    e->flashing = No;
                }
            }
            else
            {
                struct Rect r = { 0+(17*e->v[9]), 0+(12*e->type), 17+(17*e->v[9]), 12+(12*e->type) };
                
                get_tex_coords( th, &r, tc );
                
                push_pos();
                translate( e->x, e->y );
                rotate( e->rot-90.0f );
                draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                //      wireframe(Yes);
                //    draw_quad2( 0, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                //  wireframe(No);
                pop_pos();
            }
        }
        n = n->next;
    }
}

void draw_enemy_shadows()
{
    struct node_t* n = enemies;
    float offset = 15.0f;
    
    /* Go through the entire list of enemies and update them all */
    
    /* Enable the shadow effect */
    //shadow( Yes );
    set_colour( 0, 0, 0, 0.6f );
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        
        if( e != NULL )
        {
            struct texture_t* th = list_get_node_data( &tex, e->tex );
            float tc[8];
            
            /* Is this a large bug? */
            if( e->is_large )
            {
                if( e->type == 1 )
                {
                    struct Rect r = { 55*e->v[9], 0, 55*(e->v[9]+1), 38 };
                    
                    e->w = 55.0f;
                    e->h = 38.0f;
                    
                    get_tex_coords( th, &r, tc );
                    
                    push_pos();
                    translate( e->x+offset, e->y+offset );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }
                
                if( e->type == 2 )
                {
                    struct Rect r = { 68*e->v[9], 39, 68*(e->v[9]+1), 80 };
                    
                    e->w = 68.0f;
                    e->h = 42.0f;
                    
                    get_tex_coords( th, &r, tc );
                    
                    push_pos();
                    translate( e->x+offset, e->y+offset );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }
                
                if( e->type == 3 )
                {
                    struct Rect r = { 68*e->v[9], 81, 68*(e->v[9]+1), 118 };
                    
                    e->w = 64.0f;
                    e->h = 38.0f;
                    
                    get_tex_coords( th, &r, tc );
                    
                    push_pos();
                    translate( e->x+offset, e->y+offset );
                    draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                    pop_pos();
                }
            }
            else
            {
                struct Rect r = { 0+(17*e->v[9]), 0+(12*e->type), 17+(17*e->v[9]), 12+(12*e->type) };
                
                get_tex_coords( th, &r, tc );
                
                push_pos();
                translate( e->x+offset, e->y+offset );
                rotate( e->rot-90.0f );
                draw_quad2( th->handle, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                //      wireframe(Yes);
                //    draw_quad2( 0, tc, -(e->w/2.0f), -(e->h/2.0f), e->w, e->h );
                //  wireframe(No);
                pop_pos();
            }
        }
        n = n->next;
    }
    
    /* Disable the shadow effect */
    //shadow( No );
    set_colour( 1, 1, 1, 1 );
}


void update_enemies()
{
    struct node_t* n = enemies;
    
    if( !enemies )
        return;
    
    /* Go through the entire list of enemies and update them all */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        n = n->next;
        
        if( e != NULL )
        {
            float x1 = e->x-(e->w/2.0f);
            float y1 = e->y-(e->h/2.0f);
            float x2 = e->x+(e->w/2.0f);
            float y2 = e->y+(e->h/2.0f);
            
            /* Update animation. V[9] is hereby reserved for animation. */
            if( ++e->a_delay > 30 )
            {
                e->v[9] = !e->v[9];
                e->a_delay = 0;
            }
            
            /* Check each user shoot to see if it hit any enemies */
            struct node_t* sn = u_shoots;
            
            while( sn != NULL )
            {
                struct user_shoot_t* s = sn->data;
                sn = sn->next;
                
                /* Do we have a hit? */
                if( s->x > x1 && s->x < x2 &&
                    s->y > y1 && s->y < y2 )
                {
                    /* Subtract the damage and check for a kill */
                    e->energy -= s->type+1;
                    
                    if( e->energy < 1 )
                    {
                        /* If so, delete this enemy, it's spline, and add an explosion */
                        /* Also, add it's score value to the user's score */
                        user.score += e->score_value;
                        bomb_score += e->score_value;
                        user.bugs++;
                        add_explosion( e->x, e->y, No );
                        play_snd( 9, No );
                        check_for_extra_bomb( e->x, e->y );
                        
                        /* Add a crystal */
                        add_new_crystal( e->x, e->y, e->is_large ? Yes : No );
                        
                        /* Nullify shoots for small enemies, crystalize for large ones. */
                        if(!e->is_large)
                            nullify_shoots(e->id);
                        else
                            crystalize_shoots(e->id);
                        
                        set_deletion_callback( enemy_delete_func );
                        list_delete( &enemies, e );
                    }
                    else
                        e->flashing = Yes;
                    
                    /* Also delete this user shoot */
                    /* NOTE: Deleting the shoot here and now causes a crash */
                    /* Maybe investigate this later, or if necessary */
                    set_deletion_callback( user_shoot_delete_func );
                    list_delete( &u_shoots, s );
                    
                    /* Mark this shoot for removal */
                  //  s->was_used = Yes;
                    
                    break;
                }
                
              //  sn = sn->next;
            }
        }
        
    //    n = n->next;
    }
    
    n = enemies;
    
    /* Move small emenies along their splines */
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        n = n->next;
        
        if( e != NULL && e->spline != NULL )
        {
            struct Vector2 vnew, vnext;
            
            /* Skip large enemies */
            if( e->is_large )
                continue;
            
            /* Get the next point and check for the end of the line */
            int end = move_position_on_spline( e->spline );
            get_current_position_on_spline( e->spline, &vnew );
            
            /* Get the next point on the spline, if it hasn't ended yet, and calculate the angle
               to the next point. */
            if( !end )
            {
                float rd;
                
                get_next_position_on_spline( e->spline, &vnext );
                rd = anglev( &vnew, &vnext );
                
                /* Convert from radians to degrees */
                /* It's not quite as accurate, but it works. */
                rd *= (360.0f/(22.0f/3.5f));
                e->rot = rd;
            }
            else
            {
                e->rot = 90.0f;
            }
            
            /* Update the position of this enemy */
            e->x = vnew.x;
            e->y = vnew.y;
            
            /* If we are at the end of this spline, go ahead and delete it! */
            if( end )
            {
                close_spline( &e->spline );
                
                /* Also, if the enemy reaches the end of the spline, and is somewhere offscreen,
                   delete the enemy, but give no reward for it, no explosion, etc. */
                if( e->x < 160.0f || e->x > 480.0f || e->y < 0 || e->y > 480.0f )
                {
                    set_deletion_callback( enemy_delete_func );
                    list_delete( &enemies, e );
                    e = NULL;
                }
            }
        }
        
        /* If this enemy is still alive, make it shoot. */
        if( e != NULL )
        {
            if( --e->s_delay <= 0 )
            {
                add_enemy_shoot( e, diff_settings.e_shoot_speed );
                e->s_delay = diff_settings.e_shoot_timer;
            }
        }
        
//        n = n->next;
    }
    
    n = enemies;
    
    /* Move big enemies towards their destinations */
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        n = n->next;
        
        if( e != NULL )
        {
            /* Skip small enemies */
            if( !e->is_large )
                continue;
            
            /* Move the enemy down to the destination */
            if( e->y < e->v[8] )
                e->y += 5;
            
            /* Make this enemy shoot based on it's type */
            if( --e->s_delay <= 0 )
            {
                if( e->type == 1 )
                {
                    const float rad = (22.0f/7.0f)*2.0f;
                    float a = angle( e->x, e->y, user.x, user.y );
                    add_enemy_shoot2( e->x, e->y, a, diff_settings.e_shoot_speed-1, e->id );
                    add_enemy_shoot2( e->x, e->y, a-(rad*0.08), diff_settings.e_shoot_speed-1, e->id );
                    add_enemy_shoot2( e->x, e->y, a+(rad*0.08), diff_settings.e_shoot_speed-1, e->id );
                    e->s_delay = diff_settings.e_shoot_timer;
                }
                
                if( e->type == 2 )
                {
                    /* v[0] = Number of shots fired before the delay */
                    /* v[1] = Add this to the enemy shoot's speed */
                    /* v[2] = Delay counter */
                    /* v[3] = Maximum delay */
                    
                    float a = angle( e->x, e->y, user.x, user.y );
                    float s = diff_settings.e_shoot_speed + e->v[1];
                    
                    e->v[3] = 3;
                    
                    if( ++e->v[2] > e->v[3] )
                    {
                        add_enemy_shoot2( e->x-5.0f, e->y, a, s, e->id );
                        add_enemy_shoot2( e->x+5.0f, e->y, a, s, e->id );
                        e->v[0]++;
                        e->v[1]+=0.5f;
                    
                        if( e->v[0] > 5 )
                        {
                            e->s_delay = diff_settings.e_shoot_timer+10;
                            e->v[0] = e->v[1] = 0;
                        }
                        
                        e->v[2] = 0;
                    }
                }
                
                if( e->type == 3 )
                {
                    const float rad = (22.0f/7.0f)*2.0f;
                    float a = 0;
                    e->v[0] += 0.2f;
                    a = e->v[0];
                    add_enemy_shoot2( e->x, e->y, a, diff_settings.e_shoot_speed-1, e->id );
                    add_enemy_shoot2( e->x, e->y, a-(rad*0.33), diff_settings.e_shoot_speed-1, e->id );
                    add_enemy_shoot2( e->x, e->y, a+(rad*0.33), diff_settings.e_shoot_speed-1, e->id );
                    e->s_delay = diff_settings.e_shoot_timer-25;
                }
            }
        }
    }

}

void kill_all_enemies()
{
    struct node_t* n = enemies;
    
    /* Kill all enemies */
    /* TODO: Handle large enemies */
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        n = n->next;
        
        if( e != NULL )
        {
            /* Add a new explosion */
            add_explosion( e->x, e->y, No );

            /* Also, add it's score value to the user's score */
            user.score += e->score_value;
            bomb_score += e->score_value;
            
            /* Increase the number of bugs killed */
            user.bugs++;
            
            /* Play explosion sound */
            play_snd( 9, No );
            
            /* Add a big crystal */
            add_new_crystal( e->x, e->y, Yes );
            check_for_extra_bomb( e->x, e->y );
        }
    }
    
    set_deletion_callback( enemy_delete_func );
    list_clear( &enemies );
}

void update_large_enemies()
{
    struct node_t* n = enemies;
    
    if( !enemies )
        return;
    
    /* Only update the big enemies */
    
    while( n != NULL )
    {
        struct enemy_t* e = n->data;
        
        if( e != NULL )
        {
            if( e->is_large )
            {
                
            }
        }
        
        n = n->next;
    }
}

void delete_enemies()
{
    set_deletion_callback( enemy_delete_func );
    list_clear( &enemies );
}

/*
 * Explosions
 */

void delete_explosion_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_explosion( float x, float y, int is_big )
{
    struct explosion_t* expl = NULL;
    
    /* Add a new explosion to the list */
    expl = malloc( sizeof( struct explosion_t ) );
    expl->x = x;
    expl->y = y;
    expl->big = is_big;
    expl->frame = 0;
    expl->timer = 0;
    expl->anim_speed = 3;
    expl->max_frame = 13;
    
    list_add_end( &explosions, expl );
}

void draw_explosions()
{
    struct node_t* n = explosions;
    
    if( !explosions )
        return;
    
    while( n != NULL )
    {
        struct explosion_t* ex = n->data;
        
        if( ex != NULL )
        {
            float tc[8];
            struct texture_t* th = list_get_node_data( &tex, 24 );
            struct Rect r = { 0, 224, 32, 256 };
            
            /* Get the current frame */
            r.x1 += ex->frame*32;
            r.x2 += ex->frame*32;
            
            /* Get the texture coordinates */
            get_tex_coords( th, &r, tc );
            
            /* Draw the explosion */
            draw_quad2( th->handle, tc, ex->x-16.0f, ex->y-16.0f, 32.0f, 32.0f );
        }
        
        n = n->next;
    }
}

void update_explosions()
{
    struct node_t* n = explosions;
    
    if( !explosions )
        return;
    
    while( n != NULL )
    {
        struct explosion_t* ex = n->data;
        n = n->next;
        
        if( ex != NULL )
        {
            /* */
            if( ++ex->timer > ex->anim_speed )
            {
                ex->frame++;
                ex->timer = 0;
                if( ex->frame >= ex->max_frame )
                {
                    set_deletion_callback( delete_explosion_func );
                    list_delete( &explosions, ex );
                    ex = NULL;
                }
            }
        }
        
    //    n = n->next;
    }
}

void delete_explosions()
{
    set_deletion_callback( delete_explosion_func );
    list_clear( &explosions );
}

/*
 * Smoke puffs
 */

void delete_smoke_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_smoke( float x, float y )
{
    struct smoke_t* s = NULL;
    
    /* Add a new smoke puff to the list */
    s = malloc( sizeof( struct smoke_t ) );
    s->x = x;
    s->y = y;
    s->frame = 0;
    s->timer = 0;
    s->anim_speed = 0;
    s->max_frame = 20;
    s->size = 32.0f;
    
    list_add_end( &smoke, s );
}

void draw_smoke()
{
    struct node_t* n = smoke;
    
    if( !smoke )
        return;
    
    while( n != NULL )
    {
        struct smoke_t* s = n->data;
        
        if( s != NULL )
        {
            float tc[8];
            struct texture_t* th = list_get_node_data( &tex, 24 );
            struct Rect r = { 0, 192, 32, 224 };
            float sz = s->size;
            
            /* Get the current frame */
            r.x1 += s->frame*32;
            r.x2 += s->frame*32;
            
            /* Get the texture coordinates */
            get_tex_coords( th, &r, tc );
            
            /* Draw the smoke */
            push_pos();
            translate( s->x, s->y );
            rotate( s->rot );
            draw_quad2( th->handle, tc, -(sz/2.0f), -(sz/2.0f), sz, sz );
            pop_pos();
        }
        
        n = n->next;
    }
}

void update_smoke()
{
    struct node_t* n = smoke;
    
    if( !smoke )
        return;
    
    while( n != NULL )
    {
        struct smoke_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            /* */
            s->size += 1.5f;
            s->rot += 3.0f;
            if( ++s->timer > s->anim_speed )
            {
                s->frame++;
                s->timer = 0;
                if( s->frame >= s->max_frame )
                {
                    set_deletion_callback( delete_smoke_func );
                    list_delete( &smoke, s );
                    s = NULL;
                }
            }
        }
        
//        n = n->next;
    }
}

void delete_smoke()
{
    set_deletion_callback( delete_smoke_func );
    list_clear( &smoke );
}

/*
 * Bubbles
 */

void bubble_deletion_callback( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_new_bubble()
{
    /* Add a new bubble starting at the top of the screen */
    
    struct bubble_t* bubble = malloc( sizeof( struct bubble_t ) );
    
    bubble->x = (float) (rng.random(320)+160);
    bubble->y = 1;
    bubble->anim_speed = 5;
    bubble->frame = 0;
    bubble->size = 24.0f;
    
    list_add_end( &bubbles, bubble );
}

void delete_bubbles()
{
    set_deletion_callback( bubble_deletion_callback );
    list_clear( &bubbles );
}

void draw_bubbles()
{
    struct node_t* n = bubbles;
    struct texture_t* t = list_get_node_data( &tex, 1 );
    float tc[8];
    struct Rect r = { 48, 0, 72, 24 };
    
    while( n != NULL )
    {
        struct bubble_t* b = n->data;
        
        if( b != NULL )
        {
            get_tex_coords( t, &r, tc );
            draw_quad2( t->handle, tc, b->x-(b->size/2.0f), b->y-(b->size/2.0f), b->size, b->size );
        }
        
        n = n->next;
    }
}

void update_bubbles()
{
    struct node_t* n = bubbles;
    
    while( n != NULL )
    {
        struct bubble_t* b = n->data;
        n = n->next;
        
        if( b != NULL )
        {
            /* Move each bubble further down the screen */
            b->y += 3.0f;
            
            /* If the bubble has reached the bottom of the screen, then delete it */
            if( b->y > sheight )
            {
                set_deletion_callback( bubble_deletion_callback );
                list_delete( &bubbles, b );
                b = NULL;
            }
            
            /* If the user gets too close to this bubble, make it pop! */
            if( b != NULL )
            {
                struct Vector2 v1 = { user.x, user.y };
                struct Vector2 v2 = { b->x, b->y };
                float d = distance2dv( &v1, &v2 );
                
                if( d <= 64.0f )
                {
                    /* Pop that bubble! Also add 12 enemy shoots to go along with it! */
                    float x = b->x;
                    float y = b->y;
                    int i = 0;
                    
                    /* Play the pop sound effect */
                    play_snd( 2, No );
                    
                    /* Shoot in all directions */
                    while( i < 12 )
                    {
                        float a = (22.0f/3.5f) * (i/12.0f);
                        add_enemy_shoot2( x, y, a, diff_settings.e_shoot_speed, -1 );
                        i++;
                    }
                    
                    /* Delete it */
                    set_deletion_callback( bubble_deletion_callback );
                    list_delete( &bubbles, b );
                    b = NULL;
                }
            }
        }
        
     //   n = n->next;
    }
}

/*
 * Crystal stuff
 */

void delete_crystal_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_new_crystal( float x, float y, int is_large )
{
    struct crystal_t* c = malloc( sizeof( struct crystal_t ) );
    
    c->x = x;
    c->y = y;
    c->delay = 30;
    c->size = is_large ? 32.0f : 24.0f;
    c->value = is_large ? 100 : 50;
    
    list_add_beginning( &crystals, c );
}

void draw_crystals()
{
    struct node_t* n = crystals;
    struct texture_t* t = list_get_node_data( &tex, 21 );
    float tc[8];
    struct Rect r = { 18, 0, 36, 18 };
    
    set_colour( 1, 1, 1, 1 );
    
    while( n != NULL )
    {
        struct crystal_t* c = n->data;
        
        if( c != NULL )
        {
            get_tex_coords( t, &r, tc );
            draw_quad2_batched( tc, c->x-(c->size/2.0f), c->y-(c->size/2.0f), c->size, c->size );
        }
        
        n = n->next;
    }
    
    flush_batch( t->handle );
}

void update_crystals()
{
    struct node_t* n = crystals;
    
    while( n != NULL )
    {
        struct crystal_t* c = n->data;
        n = n->next;
        
        if( c != NULL )
        {
            struct Rect r = { user.x-9, user.y-10, user.x+9, user.y+10 };
            
            if( c->delay > 0 )
                c->delay--;
            else
            {
/*
                shoot->x = e->x;
                shoot->y = e->y;
                shoot->angle = angle( e->x, e->y, user.x, user.y );
                shoot->vx = cosf( shoot->angle );
                shoot->vy = sinf( shoot->angle );
                if( !speed )
                    shoot->speed = 3.0f;
                else
                    shoot->speed = speed;
                
#if 0   
                cx = cosf( shoot->angle );
                cy = sinf( shoot->angle );
                cx = abs(cx);
                cy = abs(cy);
                shoot->vx = clamp_float( cx, -shoot->speed, shoot->speed );
                shoot->vy = clamp_float( cy, -shoot->speed, shoot->speed );
#else
                shoot->vx = clamp_float( shoot->vx, -shoot->speed, shoot->speed );
                shoot->vy = clamp_float( shoot->vy, -shoot->speed, shoot->speed );
#endif
*/
                float cx, cy;
                float a = angle( c->x, c->y, user.x, user.y );
                
                c->vx = cosf(a) * 6.0f;
                c->vy = sinf(a) * 6.0f;
               /* cx = cosf( a );
                cy = sinf( a );
                cx = absf(cx);
                cy = absf(cy);
                c->vx += clamp_float( cx, -7.0f, 7.0f );
                c->vy += clamp_float( cy, -7.0f, 7.0f );
                
                c->x += c->vx;
                c->y += c->vy;*/
                c->vx = clamp_float( c->vx, -10.0f, 10.0f );
                c->vy = clamp_float( c->vy, -10.0f, 10.0f );
                c->x += c->vx;
                c->y += c->vy;
            }
            
            /* Check for collision with user */
            if( point_in_rect( &r, c->x, c->y ) )
            {
                /* Add to the user's score */
                user.score += c->value;
                bomb_score += c->value;
                
                check_for_extra_bomb( (float)(rng.random(320)+160), (float)(rng.random(sheight)) );
                
                /* Play crystal sound effect */
               // play_snd( 6, No );
                
                /* Now delete this crystal */
                set_deletion_callback( delete_crystal_func );
                list_delete( &crystals, c );
                c = NULL;
            }
        
        }
        
        //n = n->next;
    }
}

void delete_crystals()
{
    set_deletion_callback( delete_crystal_func );
    list_clear( &crystals );
}

void turn_all_enemy_shoots_into_crystals()
{
    struct node_t* n = e_shoots;
    
    /* Go through the entire list of enemy shoots and turn them all into crystals */
    
    while( n != NULL )
    {
        struct enemy_shoot_t* s = n->data;
        
        if( s != NULL )
        {
            /* Add a small crystal */
            add_new_crystal( s->x, s->y, No );
            /* Also add a small shockwave */
            add_new_shockwave( s->x, s->y, 0, 0, 32.0f );
        }
        
        n = n->next;
    }
    
    /* Clear the entire list of ememy shoots */
    set_deletion_callback( enemy_shoot_delete_func );
    list_clear( &e_shoots );
}

/* 
 * Shockwave functions
 */

void shockwave_delete_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void add_new_shockwave( float x, float y, float vx, float vy, float max_size )
{
    /* Add a new shockwave */
    struct shockwave_t* s = malloc( sizeof( struct shockwave_t ) );
    
    s->x = x;
    s->y = y;
    s->vx = vx;
    s->vy = vy;
    s->size = 1.0f;
    s->max_size = max_size;
    s->alpha = 255.0f;
    
    list_add_end( &shockwave, s );
}

void draw_shockwaves()
{
    struct node_t* n = shockwave;
    struct texture_t* th = list_get_node_data( &tex, 23 );
    
    while( n != NULL )
    {
        struct shockwave_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            set_colour( 1.0f, 0.5f, 0.0f, s->alpha/255.0f );
            draw_quad( th->handle, s->x-(s->size/2.0f), s->y-(s->size/2.0f), s->size, s->size );
        }
    }
    
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
}

void update_shockwaves()
{
    struct node_t* n = shockwave;
    
    while( n != NULL )
    {
        struct shockwave_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            /* Update the shockwave's size. If it's reach the maximum size, fade it out. */
            if( s->size < s->max_size )
                s->size += 2.0f;
            else
            {
                s->alpha -= 5.0f;
                if( s->alpha <= 0.0f )
                {
                    set_deletion_callback( shockwave_delete_func );
                    list_delete( &shockwave, s );
                }
            }
        }
    }
}

void delete_shockwaves()
{
    set_deletion_callback( shockwave_delete_func );
    list_clear( &shockwave );
}

/*
 * Bomb stuff
 */

void use_bomb()
{
    if( user.bombs > 0 && !bomb.active )
    {
        /* Place the bomb */
        bomb.x = user.x;
        bomb.y = bomb.start_y = user.y;
        bomb.b_radius = 100;
        bomb.dist = 100;
        bomb.active = Yes;
        
        /* Subtract number of available bombs */
        user.bombs--;
    }
}

void draw_bomb()
{
    if( bomb.active )
    {
        struct texture_t* th = list_get_node_data( &tex, 5 );
        struct Rect br = { 99, 33, 132, 66 };
        float tc[8];
        
        get_tex_coords( th, &br, tc );
        draw_quad2( th->handle, tc, bomb.x-16.0f, bomb.y-16.0f, 33.0f, 33.0f );
    }
}

void update_bomb()
{
    if( bomb.active )
    {
        bomb.y -= 5.0f;
        
        /* Did it reach the max distabce before detonation? */
        if( bomb.start_y == bomb.y + (bomb.b_radius+(bomb.b_radius/2.0f)) )
        {
            /* Turn all enemy shoots into crystals */
            turn_all_enemy_shoots_into_crystals();
            
            /* Elliminate all enemies */
            kill_all_enemies();
            
            /* Deactivate bombs */
            bomb.active = No;
        }
    }
}

/* 
 * Extra bomb stuff
 */
void check_for_extra_bomb( float x, float y )
{
    /* Check to see if the user gets another bomb */
    if( !extra_bomb.active )
    {
        /* An extra bomb is given after every 20000 points */
        if( bomb_score >= 20000 )
        {
            extra_bomb.active = Yes;
            bomb_score -= 20000;
            
            /* We'll place the bomb where the last bug was killed */
            extra_bomb.x = x;
            extra_bomb.y = y;
            
            do{ extra_bomb.vx = (float) (rng.random(8)-4); } while( extra_bomb.vx == 0 );
            do{ extra_bomb.vy = (float) (rng.random(8)-4); } while( extra_bomb.vy == 0 );
        }
    }
}

void draw_extra_bomb()
{
    /* Draw a bomb with a letter B over it */
    if( extra_bomb.active )
    {
        struct texture_t* th = list_get_node_data( &tex, 5 );
        struct Rect br = { 99, 33, 132, 66 };
        float tc[8];
        
        get_tex_coords( th, &br, tc );
        draw_quad2( th->handle, tc, extra_bomb.x-16.0f, extra_bomb.y-16.0f, 33.0f, 33.0f );
        draw_text( &font1, extra_bomb.x-5.0f, extra_bomb.y-10.0f, "B" );
    }
}

void update_extra_bomb()
{
    if( extra_bomb.active )
    {
        struct Rect r = { extra_bomb.x-16.0f, extra_bomb.y-16.0f, extra_bomb.x+16.0f, extra_bomb.y+16.0f };
        
        /* Did we get it? */
        if( point_in_rect( &r, user.x, user.y ) )
        {
            user.bombs++;
            extra_bomb.active = No;
            play_snd( 10, No );
            
            return;
        }
        
        extra_bomb.x += extra_bomb.vx;
        extra_bomb.y += extra_bomb.vy;
        
        if( extra_bomb.x <= 160.0f || extra_bomb.x > swidth+160 )
            extra_bomb.vx *= -1.0f;
        if( extra_bomb.y <= 0.0f || extra_bomb.y > sheight )
            extra_bomb.vy *= -1.0f;
    }
}

/*
 * Misc rendering stuff
 */

int __cdecl tree_sort( const void* t1, const void* t2 )
{
	/* Z comparison: Less */
	/* TODO: Check for equal values? */
    
	float z_cmp_result = ((struct foliage_t*)t1)->y - ((struct foliage_t*)t2)->y;
    
    /* If we get an equal Z value, move the second one up a few units */
    if( z_cmp_result == 0 )
        ((struct foliage_t*)t2)->y -= 3.0f;
    
	return ((int) z_cmp_result);
}

void draw_background()
{
    struct texture_t* t = list_get_node_data( &tex, 26 );
    
    set_colour( 1, 1, 1, 1 );
    draw_quad( t->handle, 0, 0, 640, 480 );
}

void draw_environment()
{
	float tc[] = { 0.0f, 10.0f, 15.0f, 10.0f, 15.0f, 0.0f, 0.0f, 0.0f };
	static float h1 = 0;
	static float h2 = (float)-538.0f;
	static int setup_trees_n_stuff = 0; /* Temporary */
	int i = 0;
	struct texture_t* tile_tex = list_get_node_data( &tex, 13 );
	float w = 32.0f, h = 32.0f;
    int foliage_list[9] = { 7, 8, 9, 10, 11, 12, 15, 16, 17 };
    static float tmt = 0;
    
	float scroll_speed = 0.5f;
    
	if( !setup_trees_n_stuff )
		{
			i = 0;
            
			setup_trees_n_stuff = 1;
            
			while( i < max_foliage/4 )
			{
                int f = (rand()%9);
                struct texture_t* th = list_get_node_data( &tex, foliage_list[f] );

				foliage[i].x = (float) ( rand() % ( 320 - ((int)h) ) ) + 160;
				foliage[i].y = (float) ( rand() % sheight ); //-( rand() % 480 ) + h;
                foliage[i].tex = th->handle;
				i++;
			}
		}
	
    
	/* Draw the menu and ingame boundaries */
//  enable_2d();
#if 0
	/* First half of the scrolling effect */
	push_pos();
	translate( 640.0f/4.0f, h1 );
	//set_colour( 0.0f, 0.0f, 0.0f, 1.0f );
	//draw_quad( 0, 0.0f, 0.0f, 640.0f/2.0f, sheight );
	set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
	draw_quad2( tile_tex->handle, tc, 0.0f, 0.0f, 640.0f/2.0f, sheight );
	pop_pos();
    
	/* Second half of the scrolling effect */
	push_pos();
	translate( 640.0f/4.0f, h2 );
    //	set_colour( 0.0f, 0.0f, 0.0f, 1.0f );
    //	draw_quad( 0, 0.0f, 0.0f, 640.0f/2.0f, 480.0f );
	set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
	draw_quad2( tile_tex->handle, tc, 0.0f, 0.0f, 640.0f/2.0f, sheight );
	pop_pos();
#else
    tmt += scroll_speed;
    glMatrixMode( GL_TEXTURE );
    glTranslatef( 0, tmt/57.0f, 0 );
    glMatrixMode( GL_MODELVIEW );
    
    push_pos();
	translate( 640.0f/4.0f, 0.0f );
	//set_colour( 0.0f, 0.0f, 0.0f, 1.0f );
	//draw_quad( 0, 0.0f, 0.0f, 640.0f/2.0f, sheight );
	set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
	draw_quad2( tile_tex->handle, tc, 0.0f, 0.0f, 640.0f/2.0f, sheight );
	pop_pos();
    
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
#endif
    
	push_pos();
    //	translate( 640.0f/4.0f, 0.0f );
//	transparent_blend( TRUE );
    
	/* Sort trees according to their z-value */
	/* TODO: Sort trees once every time a tree is repositioned instead of every frame. */
	qsort( foliage, max_foliage, sizeof( struct foliage_t ), tree_sort );
    
	i = 0;
//	if( level != 5 && level != 0 && level != 6 && level != 7 )
//	{
		while( i < max_foliage )
		{
			if( foliage[i].x > 159 )
                draw_quad( foliage[i].tex, foliage[i].x, foliage[i].y, w, h );
			i++;
		}
//	}
    
//	transparent_blend( FALSE );
	pop_pos();
    
//	disable_2d();
    
	h1 += scroll_speed;
	h2 += scroll_speed;
    
	if( h1 >= sheight )
		h1 = 0.0f;
	if( h2 >= 0.0f )
		h2 = -480;
    
	/* Update trees 'n stuff */
	if( setup_trees_n_stuff )
	{
		i = 0;
        
		while( i < max_foliage )
		{
			foliage[i].y += scroll_speed;
            
			if( foliage[i].y >= sheight )
			{
                int f = (rand()%9);
                struct texture_t* th = list_get_node_data( &tex, foliage_list[f] );
                
				foliage[i].y = (float) -(( rand() % sheight ) + h);
				foliage[i].x = (float) (rand() % (320-((int)h)))+160;
                foliage[i].tex = th->handle;
                i++;
			}
            
			i++;
		}
	}
}

void draw_boundaries()
{
    
}

void render_hud()
{
    char string[64];
    struct texture_t* th = NULL;
    float w = 17.0f/34.0f;
    float h = 1.0f;
    float tc[] = { 0.0f, h, w, h, w, 0.0f, 0.0f, 0.0f };
    struct Rect r = { 0, 0, 17, 21 };
    float sprw = 17.0f;
    float sprh = 21.0f;
    struct Rect br = { 99, 33, 132, 66 };
    int i = 0;
    char* diff[3] = { "Easy", "Normal", "Hard" };
    
    /* Render the user's score */
    sprintf( string, "SCORE: %d", user.score );
    draw_shadowed_text( &font1, 340, 10, string );
    
    /* Show difficulty setting */
    draw_shadowed_text( &font1, 340, 30, diff[settings.difficulty-1] );
    
    /* Show the current stage */
    sprintf( string, "STAGE %d", game_stats.stage );
    draw_shadowed_text( &font1, 180, 10, string );
    
    /* Show the number of remaining ships */
    th = list_get_node_data( &tex, 18 );
    get_tex_coords( th, &r, tc );
    draw_quad2( th->handle, tc,  180.0f-(sprw/2.0f), (sheight-50)-(sprh/2.0f), sprw, sprh );
    sprintf( string, "x%d", user.ships );
    draw_text( &font1, 195, sheight-55, string );
    
    /* Show the number of bombs */
    th = list_get_node_data( &tex, 5 );
    get_tex_coords( th, &br, tc );
    
    while( i < user.bombs )
    {
        float x = (180.0f-16.0f)+(i*20.0f);
        float y = (sheight-30)-16.0f;
        
        draw_quad2( th->handle, tc, x, y, 33.0f, 33.0f );
        i++;
    }
    
    /* Bomb button on the bottom left corner */
    th = list_get_node_data( &tex, 28 );
    draw_quad( th->handle, 160, sheight-120, 50, 50 ); 
    
    /* Pause button in the top left corner */
    th = list_get_node_data( &tex, 27 );
    draw_quad( th->handle, (320-50)+160, 30, 50, 50 );
    
    /* TODO: User's bombs on the other side */
}

void draw_pause_screen()
{
    if( !paused )
        return;
    
    draw_ingame_pause_menu();
    
#if 0
    /* Darken the screen a bit */
    set_colour( 0.0f, 0.0f, 0.0f, 0.5f );
    draw_quad( 0, 160.0f, 0.0f, 320.0f, 480.0f );
    
    /* Put up a pause menu */
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
    draw_text( &font1, 290.0f, 250.0f, "Paused" );
#endif
}

void update_pause_screen()
{
    if( !paused )
        return;
    
    if( update_ingame_pause_menu() )
        paused = No;
    
    /* TODO: Allow user to quit and return to the menu. */
}

/*
 * Misc updating stuff
 */
void update_spawning()
{
    if( --diff_settings.e_spawn_timer <= 0 )
    {
        add_new_enemy();
        diff_settings.e_spawn_timer = diff_settings.e_spawn_timer_max;
    }
}

void check_for_new_level()
{
    /* Get the amount of time elapsed */
    update_time();
    elapsed_time = get_elapsed_time();
    
    /* If 30 seconds go by, increase the user's stage */
    if( elapsed_time > 30 )
    {
        game_stats.stage++;
        play_snd( 10, No );
        reset_time();
        elapsed_time = 0;
        
        /* Make enemies shoot and respawn more frequently */
        diff_settings.e_shoot_timer -= 5;
        if( diff_settings.e_shoot_timer < diff_settings.e_shoot_timer_min )
            diff_settings.e_shoot_timer = diff_settings.e_shoot_timer_min;
        
        diff_settings.e_spawn_timer_max -= 5;
        if( diff_settings.e_spawn_timer_max < diff_settings.e_spawn_timer_min )
            diff_settings.e_spawn_timer_max = diff_settings.e_spawn_timer_min;
    }
}

int get_relavent_sprite_count()
{
    int total_relavent_sprites = 0;
    
    total_relavent_sprites += list_length( &e_shoots );
    total_relavent_sprites += list_length( &crystals );
    total_relavent_sprites += list_length( &enemies );
    
    return total_relavent_sprites;
}

void set_pause_flags()
{
    extern int unpausing;
    extern float pause_screen_alpha;
    
    if( !ingame )
        return;
    
    paused = Yes;
    unpausing = No;
    pause_screen_alpha = 127.0f;
}

void unpause_now()
{
    extern int unpausing;
    
    unpausing = Yes;
}

void check_for_pause_request()
{
    /* Check for pause request */
    if( input.pause && input.timestamp <= 1 )
    {
        /* Swap pause flag */
        //paused = !paused;
        if( !paused )
        {
            set_pause_flags();
        }
        else
        {
            unpause_now();
        }
        
        reset_time();
        
        /* TODO: Pause sound effect? */
    }
}

void take_screenshot()
{
    /* Take a screenshot by reading the primary render target (using glReadPixels)
     and save the contents to an .bmp file. The expected output is WxHx24. */
    
    FILE* bmpfile;
	char filename[256];
	unsigned char header[0x36];
	long size;
	unsigned char line[1024*2*3];
	int w,h;
	short i,j;
	unsigned char empty[2] = {0,0};
	unsigned int color;
	unsigned int snapshotnr = 0;
    
	w = 640;
	h = 480;
	size = w * h * 3 + 0x38;
    
    unsigned int* buffer = malloc(w*h*4);
    memset(buffer,0,w*h*4);
    
	memset( header, 0, 0x36 );
	header[0] = 'B';
	header[1] = 'M';
	header[2] = size & 0xff;
	header[3] = ( size >> 8 ) & 0xff;
	header[4] = ( size >> 16 ) & 0xff;
	header[5] = ( size >> 24 ) & 0xff;
	header[0x0a] = 0x36;
	header[0x0e] = 0x28;
	header[0x12] = w % 256;
	header[0x13] = w / 256;
	header[0x16] = h % 256;
	header[0x17] = h / 256;
	header[0x1a] = 0x01;
	header[0x1c] = 0x18;
	header[0x26] = 0x12;
	header[0x27] = 0x0b;
	header[0x2a] = 0x12;
    
    glReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
    
	for(;;)
	{
		snapshotnr++;
        
		sprintf( filename, "screenshots/snapshot%03d.bmp", snapshotnr );
        
		bmpfile = fopen( filename, "rb" );
		if( bmpfile == NULL ) break;
		fclose( bmpfile );
	}
    
	if( ( bmpfile = fopen( filename, "wb" ) ) == NULL )
    {
        free(buffer);
		return;
    }
    
	fwrite( header, 0x36, 1, bmpfile );
	for( i = 0; i < h; i++ )
	{
		for( j = 0; j < w; j++ )
		{
			color = buffer[j+(i*w)];
			line[j*3+2] = ( color ) & 0xff;
			line[j*3+1] = ( color >> 8 ) & 0xff;
			line[j*3+0] = ( color >> 16 ) & 0xff;
		}
		fwrite( line, w * 3, 1, bmpfile );
	}
    
	fwrite( empty, 0x2, 1, bmpfile );
	fclose( bmpfile );
    free(buffer);
    
	return;
}

/*
 * Game initialization and stuff
 */
void texture_delete_func( void* tex )
{
    delete_texture( tex );
}

void sound_delete_func( void* snd )
{
    if( snd )
    {
        delete_sound( snd );
        free( snd );
        snd = NULL;
    }
}

int init_resources()
{
    ezxml_t xml = ezxml_parse_file( "res.xml" );
    ezxml_t sprites, soundfx, music, file;
    char string[128];
    const char* directory;
    int index = 0, ret;
    
    /* Sanity checks */
    if( !xml )
        return 0;
    
    /* Initialize menu stuff first */
    if( !init_menu_data() )
        return 0;
    
    /* Get the resource directory */
//    directory = ezxml_child( xml, "dir" )->txt;
    
    /* Get sprite information */
    sprites = ezxml_child( xml, "sprites" );
    index = atoi( ezxml_attr( sprites, "count" ) );
    
    /* Set sprite texture list */
//    list_set_head( &tex );
    
    /* Start loading sprites */
    file = ezxml_child( sprites, "file" );
    while( file != NULL )
    {
        struct texture_t* tex_h;
        
        /* Get the full path */
        sprintf( string, "%s", file->txt );
        
        /* Open the actual texture and add it to the list */
        tex_h = create_texture( string );
        
        if( !tex_h )
        {
            ezxml_free( xml );
            return 0;
        }
        
        list_add_end( &tex, tex_h );
        
        /* Next sprite please */
        file = file->next;
    }
    
    /* Get soundfx information */
    soundfx = ezxml_child( xml, "soundfx" );
    index = atoi( ezxml_attr( soundfx, "count" ) );
    
    /* Start loading sounds */
    file = ezxml_child( soundfx, "file" );
    while( file != NULL )
    {
        struct sound_t* snd = malloc( sizeof( struct sound_t ) );
        
        /* Get the full path */
        sprintf( string, "%s", file->txt );
        
        /* Open the actual sound and add it to the list */
        if( !create_sound_wav( string, snd ) )
        {
            ezxml_free( xml );
            return 0;
        }
        
        list_add_end( &sfx, snd );
        
        /* Next sound please */
        file = file->next;
    }
    
    /* Close the xml file */
    ezxml_free( xml );
    
    /*bgm = malloc( sizeof( struct wave_stream ) );
    wavestream_open( "transparence.wav", &bgm[0] );
    wavestream_play( &bgm[0] );*/
    
    /* Create thread and mutex */
    ret = pthread_mutex_init( &audio_stream_mutex, NULL );
    ret = pthread_create( &audio_stream_thread, NULL, audio_stream_processing_thread_function, "transparence.wav" );
    
    /* Fade in */
    fade_in();
    
    return 1;
}


void draw_intro()
{
    struct texture_t* th = list_get_node_data( &tex, 26 );
    int width, height;
    get_dimensions( &height, &width );
    
    set_colour( 0, 0, 0, 1 );
    draw_quad( 0, 0, 0, 640, 480 );
    set_colour( 1, 1, 1, 1 );
    draw_quad( th->handle, 320.0f-128.0f, ((float)height/2.0f)-64.0f, 256.0f, 128.0f );
}

void update_intro()
{
    static int timer = 120;
    int width, height;
    get_dimensions( &height, &width );
    
    if( --timer < 1 )
    {
        float frect[4] = { 0, 0, 640, height };
        float fcolour[3] = { 1, 1, 1 };
        
        fade_settings( frect, fcolour );
        fade_out();
    }
    
    if( timer < 0 )
    {
        if( finish_fade )
        {
            float frect[4] = { 160, 0, 320, height };
            float fcolour[3] = { 1, 1, 1 };
            
            fade_settings( frect, fcolour );
            
            draw_func = draw_menu;
            update_func = update_menu;
            fade_in();
            intro = No;
        }
    }
}

void uninit_resources()
{
    /*wavestream_close( &bgm[0] );
    free( bgm );
    bgm = NULL;*/
    
    /* Kill the audio stream thread and wait for it to finish */
    exit_streaming_audio_thread = Yes;
    pthread_join( audio_stream_thread, NULL );
    
    /* Kill the mutex */
    pthread_mutex_destroy( &audio_stream_mutex );
    
    /* Delete menu stuff */
    uninit_menu_data();
    
    /* Delete all sounds */
    set_deletion_callback( sound_delete_func );
    list_clear( &sfx );
    
    /* Delete all textures */
    set_deletion_callback( texture_delete_func );
    list_clear(&tex);
    
    /* Delete all user and enemy shoots just in case */
    delete_user_shoots();
    delete_enemy_shoots();
    
    /* Delete all enemies */
    set_deletion_callback( enemy_delete_func );
    list_clear( &enemies );
    
    delete_shockwaves();
    delete_bubbles();
    delete_crystals();
    delete_missiles();
    delete_smoke();
    delete_explosions();
    
 //   delete_explosion_func( delete_explosion_func );
 //   list_clear( &explosions );
}

void reset_game()
{
    /* Reset ingame stuff */
    reset_user();
    
    delete_enemies();
    delete_user_shoots();
    delete_enemy_shoots();
    delete_shockwaves();
    delete_bubbles();
    delete_crystals();
    delete_missiles();
    delete_smoke();
    delete_explosions();
    
    /* Reset game stats */
    game_stats.stage = 1;
    memcpy( &diff_settings, &const_diff_settings[settings.difficulty-1], sizeof( struct diff_settings_t ) );
    
    extra_bomb.active = No;
    
    /* Reset timer */
    reset_time();
}

int init_game()
{
    /* Get the dimensions of this device */
    get_dimensions( &sheight, &swidth );
    
    /* Change the current directory to the resource path */
    set_current_path_to_resource_directory();
    
    /* Reset the game */
    reset_game();
    
    /* Set default settings before actually loading saved settings. */
    memset( &settings, 0, sizeof( struct game_settings_t ) );
    settings.speed_drop = Yes;
    settings.vibrate = Yes;
    
    /* Select random number generator */
    rng_init( &rng, rand3 );
    rng.setseed(42);
    rng.reseed();
    
    /* Emergency uninitialization fail-safe */
    atexit( uninit_game );
    
    /* Initialize OpenAL */
    if( !EnableOpenAL() )
        return 0;
    
    /* Initialize OpenGL */
    if( !EnableOpenGL() )
        return 0;
    
    /* Create render target */
//    if( !create_render_target( 640, 480, 0, 0, 0, &render_target ) )
//        return 0;
    
    /* Initialize joystick */
    enable_joystick();
    
    /* Initialize resources */
    if( !init_resources() )
        return 0;
    
    if( !init_splines() )
        return 0;
    
    if( !create_fonts() )
        return 0;
    
    /* Load game settings */
    load_game_settings();
    
    /* Setup function pointers */
    draw_func = draw_intro;
    update_func = update_intro;
    
    
    return 1;
}

void uninit_game()
{
    /* Uninitialize game resources */
    delete_fonts();
    uninit_splines();
    uninit_resources();
//    delete_render_target( &render_target );
    
    /* Uninitialize APIs */
    disable_joystick();
    DisableOpenGL();
    DisableOpenAL();
}

void draw_game()
{
    draw_environment();
    draw_enemy_shadows();
    draw_explosions();
    render_user();
    draw_user_stars();
    draw_enemies();
    draw_shockwaves();
    draw_crystals();
    draw_extra_bomb();
    draw_enemy_shoots();
    draw_user_shoots();
    draw_missiles();
    draw_smoke();
    draw_bubbles();
    draw_bomb();
    render_hud();
    draw_pause_screen();
}

void update_game()
{
//    static int update = Yes;
    if( paused )
        goto skip_updates;
    
#if 0 /* TEST */
    static int timer = 60;
    static int max_timer = 60;
    
    if( --timer <= 0 )
    {
        add_new_enemy();
        timer = max_timer;
        
        max_timer--;
        if( max_timer < 25 )
            max_timer = 25;
    }
#endif
    
#if 1 /* TEST */
    static int timer2 = 200;
    
    if( --timer2 <= 0 )
    {
        add_new_bubble();
        timer2 = 200;
    }
#endif
    
    if( settings.speed_drop && fps > 40 )
    {
        if( get_relavent_sprite_count() > 30 )
            reduce_speed = !reduce_speed;
        else
            reduce_speed = No;
    }
    else
        reduce_speed = No;
    
    if( !reduce_speed )
    {
        update_spawning();
        update_user();
        update_user_stars();
        update_user_shoots();
        update_enemy_shoots();
        update_enemies();
        update_explosions();
        update_bubbles();
        update_crystals();
        update_bomb();
        update_missiles();
        update_smoke();
        update_shockwaves();
        update_extra_bomb();
    }
    
    /* TODO: Stop timing for pause mode */
    check_for_new_level();
    
skip_updates:
    
    /* Check for pause/unpause requests */
    check_for_pause_request();
    update_pause_screen();
}

void render()
{
    if( skip_frame )
    {
        skip_frame = No;
        return;
    }
    
//    clear_buffers();
//    set_render_target( &render_target );
    
    /* Clear the screen */
    clear_buffers();
    
    /* Enable 2D Rendering */
    enable_2d();
#if 1
    translate( -160.0f, 0 );
#endif
    /* Draw game stuff */
//    draw_background();
    transparent_blend(Yes);
    draw_func();
    transparent_blend(No);
    
    /* This should be drawn last besides debugging information */
    draw_boundaries();
    
//    set_colour( 0.0f, 0.0f, 0.0f, 1.0f );
    if( ingame_fade == 0 )	/* Fade out */
    {
        if( do_fade(No) )
            ingame_fade = -1;
    }
    if( ingame_fade == 1 )	/* Fade in */
    {
        if( do_fade(Yes) )
            ingame_fade = -1;
    }
    set_colour( 1.0f, 1.0f, 1.0f, 1.0f );
    
    transparent_blend(Yes);
    show_fps();
    transparent_blend(No);
    
    /* Disable 2D rendering */
    disable_2d();
    
//    set_render_target( NULL );
    
//    enable_2d();
//    draw_quad( render_target.tex.handle, 0, 0, 320, 240 );
//    disable_2d();
    
    /* Flip doublebuffer */
    swap();
    
    /* Block until vertical blank */
  //  block_until_vertical_blank();
}

int update()
{
    int quit = No; /* Change this to 'Yes' when it's quittin' time */
    
    /* Poll input devices */
    poll_input_devices( &input );
    
    /* Update the game */
    update_func();
    
    /* Update streaming BGM */
//    wavestream_update( &bgm[0] );
//    handle_music_fade();
    
    /* If we are quitting, set the quit flag */
    quit = input.escape;
    
    /* Taking a screenshot, are we? */
    if( input.screenshot && input.timestamp <= 1 )
        take_screenshot();
    
    return quit;
}


