//
//  menu.c
//  BugPlanet
//
//  Created by admin on 12/22/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#include "platform.h"
#include "ogldrv.h"
#include "menu.h"
#include "linkedlist.h"
#include "vmath.h"
#include "texfont.h"
#include "input.h"
#include "vmath.h"
#include "game.h"
#include "osx_util.h"
#include "ios_misc.h"
#include "rng.h"

/* Score data structure (for high scores) */
struct score_data_t
{
    char initials[4];   /* User's initials */
    int score;          /* User's final score */
    int stage;          /* User's highest stage */
    int rank;           /* User's overall rank */
};

struct sparkle_t
{
    float x, y; /* Sparkle position */
    float a;    /* Rotation angle */
    float s;    /* Rotation speed */
    float r;    /* Rotation */
};

struct texture_t* mtex[16];
struct node_t* sparkles = NULL;

extern struct font_t font1;
extern struct input_t input;
extern struct game_settings_t settings;
extern struct game_stats_t game_stats;
extern struct user_t user;
extern struct rng_t  rng;

extern void (*draw_func)(void);
extern void (*update_func)(void);

extern void set_next_track( char* track );
extern char* get_track_name();

int settings_loaded = No;
int scores_loaded = No;
struct score_data_t high_scores[10];

float pause_screen_alpha = 127.0f;
int unpausing = No;
extern int ingame;

extern void get_tex_coords( struct texture_t* t, struct Rect* r, float* tc );
extern void scale_tex_coords( float sx, float sy, float* tc );


/*
 * TODO: Seperate skills based on difficulty (3 different score data sets)
 */
void reset_high_scores()
{
    FILE* fp;
    int i = 9;
    
    ZeroMemory( high_scores, sizeof( struct score_data_t ) * 10 );
    
    while( i > -1 )
    {
        int j = abs(i-9);
        
        //memset( high_scores[i].initials, c, sizeof( char ) * 4 );
        /*high_scores[i].initials[0] = 'S';
        high_scores[i].initials[1] = '3';
        high_scores[i].initials[2] = 'D';
        high_scores[i].initials[3] = '\0';*/
        sprintf( high_scores[i].initials, "#%d ", i+1 );
        high_scores[i].initials[3] = '\0';
        high_scores[i].score = 100*(j*100);
        high_scores[i].stage = j+1;
        high_scores[i].rank = get_rank( high_scores[i].score );
        i--;
    }
    
    fp = get_file_ptr_from_doc_dir_c( "scores.dat", "wb" ); // fopen( "scores.dat", "wb" );
    if(fp)
    {
        fwrite( high_scores, 1, sizeof( struct score_data_t ) * 10, fp );
        fclose(fp);
    }
    else
        printf( "reset_high_scores(): Could not create new score data!\n" );
}

void load_high_scores()
{
    if( !scores_loaded )
    {
        FILE* fp = get_file_ptr_from_doc_dir_c( "scores.dat", "rb" ); //fopen( "scores.dat", "rb" );
    
        if(fp)
        {
            fread( high_scores, 1, sizeof( struct score_data_t ) * 10, fp );
            fclose(fp);
            scores_loaded = Yes;
        }
        else
        {
            printf( "load_high_scores(): Could not load score data. Attempting to reset scores...\n" );
            reset_high_scores();
        }
    }
}

void score_delete_func( void* ptr )
{
    if( ptr )
    {
        free( ptr );
        ptr = NULL;
    }
}

void save_high_scores()
{
    struct node_t* scores = NULL;
    FILE* fp = NULL;
    int i = 0;
    
    /* Load the existing high score data, or reset the scores if they don't. */
    load_high_scores(); /* TODO: Reset the scores_loaded flag? */
    
    /* Add the scores to the list */
    while( i < 10 )
    {
        struct score_data_t* sd = malloc( sizeof( struct score_data_t ) );
        memcpy( sd, &high_scores[i], sizeof( struct score_data_t ) );
        list_add_end( &scores, sd );
        
        i++;
    }
    
    /* Check the user's score for a qualifiying score */
    i = 0;
    while( i < 10 )
    {
        /* If we do have a qualifying score, add it to the list above the next to highest
           score and end the loop. */
        if( user.score > high_scores[i].score )
        {
            struct score_data_t* sc = malloc( sizeof( struct score_data_t ) );
            sc->score = user.score;
            sc->rank = get_rank( user.score );
            /*sc->initials[0] = 'Z';
            sc->initials[1] = ' ';
            sc->initials[2] = ' ';
            sc->initials[3] = '\0';*/
            sprintf( sc->initials, "#%d ", i+1 );
            sc->initials[3] = '\0';
            sc->stage = game_stats.stage;
            
            list_add_at( &scores, sc, i+1 );
            
            break;
        }
        
        i++;
    }
    
    /* Properly set the rankings for each score */
    i = 0;
    while( i < 10 )
    {
        i++;
    }
    
    /* Now, save the first 10 scores to elliminate the outranked score */
    i = 0;
    while( i < 10 )
    {
        struct score_data_t* sd = list_get_node_data( &scores, i+1 );
        sprintf( sd->initials, "#%d ", i+1 );
        sd->initials[3] = '\0';
        memcpy( &high_scores[i], sd, sizeof( struct score_data_t ) );
        i++;
    }
    
    /* Save the scores to disk */
    fp = get_file_ptr_from_doc_dir_c( "scores.dat", "wb" ); //fopen( "scores.dat", "wb" );
    if(fp)
    {
        fwrite( high_scores, 1, sizeof( struct score_data_t ) * 10, fp );
        fclose(fp);
    }
    else
        printf( "save_high_scores(): Could not save high score data!\n" );
    
    /* Delete the temporary linked list */
    set_deletion_callback( score_delete_func );
    list_clear( &scores );
}

void save_game_settings()
{
    FILE* fp = get_file_ptr_from_doc_dir_c( "settings.dat", "wb" ); // fopen( "settings.dat", "wb" );
    if( fp )
    {
        fwrite( &settings, 1, sizeof( struct game_settings_t ), fp );
        fclose(fp);
    }
    else
        printf( "save_game_settings(): Could not create save data!\n" );
    
    settings_loaded = No;
}

void load_game_settings()
{
    if( !settings_loaded )
    {
        FILE* fp = get_file_ptr_from_doc_dir_c( "settings.dat", "rb" ); //fopen( "settings.dat", "rb" );
        if( fp )
        {
            fread( &settings, 1, sizeof( struct game_settings_t ), fp );
            fclose(fp);
        }
        else
            printf( "load_game_settings(): Could not load save data!" );
        
        settings_loaded = Yes;
    }
}

void toggle_fs_check()
{
    if( input.toggle_fs && input.timestamp <= 1 )
    {
        toggle_fullscreen();
    }
}

void delete_sparkle_func( void* p );

void add_sparkle( float x, float y )
{
    struct sparkle_t* s = malloc( sizeof( struct sparkle_t ) );
    
    s->x = ((float)rng.random(320))+160.0f-32.0f;
    s->y = ((float)rng.random(480))-32.0f;
    s->s = ((float) rng.random(628))/100.0f;
    s->r = 0;
    
    list_add_end( &sparkles, s );
}

void draw_sparkles()
{
    /*struct node_t* n = sparkles;
    static int timer = 0;
    
    while( n != NULL )
    {
        struct sparkle_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            draw_quad( mtex[14]->handle, s->x, s->y, 64, 64 );
            draw_quad( mtex[15]->handle, s->x, s->y, 64, 64 );
        }
    }
    
    if( ++timer > 30 )
    {
        add_sparkle( (float) rng.random(320), -32 );
        timer = 0;
    }*/
}

void update_sparkles()
{
    /*struct node_t* n = sparkles;
    
    while( n != NULL )
    {
        struct sparkle_t* s = n->data;
        n = n->next;
        
        if( s != NULL )
        {
            s->y += s->v;
            if( s->y > 480+32 )
            {
                set_deletion_callback( delete_sparkle_func );
                list_delete( &sparkles, s );
            }
        }
    }*/
}

void delete_sparkle_func( void* p )
{
    if(p)
    {
        free(p);
    }
}

void delete_sparkles()
{
    set_deletion_callback( delete_sparkle_func );
    list_clear( &sparkles );
}

int init_menu_data()
{
    int index = 0;
    
    /* Load textures, soundfx, etc. */
    mtex[index++] = create_texture( "sfont2.tga" );
    mtex[index++] = create_texture( "flowers.tga" );
    mtex[index++] = create_texture( "options.tga" );
    mtex[index++] = create_texture( "shogun3d.tga" );
    mtex[index++] = create_texture( "title.tga" );
    mtex[index++] = create_texture( "difficulty.tga" );
    mtex[index++] = create_texture( "sship.tga" );
    mtex[index++] = create_texture( "initials.tga" );
    mtex[index++] = create_texture( "postgame.tga" );
    mtex[index++] = create_texture( "rankings.tga" );
    mtex[index++] = create_texture( "credits.tga" );
    mtex[index++] = create_texture( "icon1.tga" );
    mtex[index++] = create_texture( "icon2.tga" );
    mtex[index++] = create_texture( "paused.tga" );
    mtex[index++] = create_texture( "flare.tga" );
    mtex[index++] = create_texture( "sparkle.tga" );
    
    return 1;
}

void uninit_menu_data()
{
    int index = 0;
    
    while( index < 16 )
    {
        delete_texture(mtex[index]);
        mtex[index++] = NULL;
    }
    
    delete_sparkles();
}

#if 0
void draw_intro()
{
    /* Draw the intro screen */
    draw_quad( mtex[3]->handle, 0.0f, 0.0f, 640.0f, 480.0f );
}

void update_intro()
{
    
}
#endif

void draw_option_menu()
{
    int width, height;
    get_dimensions( &height, &width );
    
    char* shoot_colours[] = { "Default", "Red", "Green", "Blue", "Pink", "Violet" };
    char* yes_or_no[] = { "No", "Yes" };
    char string[16];
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw sparkles */
    draw_sparkles();
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[2]->handle, 320.0f-(mtex[2]->width/2.0f)+2, 60.0f+2, mtex[2]->width, mtex[2]->height );
    set_colour( 1, 1, 1, 1.0f );
    draw_quad( mtex[2]->handle, 320.0f-(mtex[2]->width/2.0f), 60.0f, mtex[2]->width, mtex[2]->height );
    
    /* Draw menu options */
    draw_shadowed_text( &font1, 220.0f, 200.0f, "Vibration" );
    /*draw_shadowed_text( &font1, 220.0f, 230.0f, "BGM Volume" );
    draw_shadowed_text( &font1, 220.0f, 260.0f, "SFX Volume" );*/
    draw_shadowed_text( &font1, 220.0f, 230.0f, "Speed Control" );
    draw_shadowed_text( &font1, 220.0f, 260.0f, "Shot Colour" );
    draw_shadowed_text( &font1, 220.0f, 350.0f, "Back" );

    /* TODO: Create functional sliders */
    /*draw_line( 350.0f, 205.5f, 450.0f, 205.5f );
    draw_quad( 0, 398.0f, 200.0f, 4.0f, 11.0f );*/
    /*draw_line( 350.0f, 235.5f, 450.0f, 235.5f );
    draw_quad( 0, 398.0f, 230.0f, 4.0f, 11.0f );
    draw_line( 350.0f, 265.5f, 450.0f, 265.5f );
    draw_quad( 0, 398.0f, 260.0f, 4.0f, 11.0f );*/
    
    draw_shadowed_text( &font1, 350.0f, 200.0f, yes_or_no[settings.vibrate] );
    draw_shadowed_text( &font1, 350.0f, 230.0f, yes_or_no[settings.speed_drop] );
    draw_shadowed_text( &font1, 350.0f, 260.0f, shoot_colours[settings.shoot_colour] );
}

void update_option_menu()
{
    struct Rect r1 = { 220.0f, 200.0f, 220.0f+(8.0f*9.0f), 200.0f+11.0f };
    struct Rect r2 = { 220.0f, 230.0f, 220.0f+(8.0f*9.0f), 230.0f+11.0f };
    struct Rect r3 = { 220.0f, 260.0f, 220.0f+(8.0f*9.0f), 260.0f+11.0f };
    struct Rect r4 = { 220.0f, 290.0f, 220.0f+(8.0f*9.0f), 290.0f+11.0f };
    struct Rect r5 = { 220.0f, 320.0f, 220.0f+(8.0f*9.0f), 320.0f+11.0f };
    struct Rect r6 = { 220.0f, 350.0f, 220.0f+(8.0f*9.0f), 350.0f+11.0f };
   
    /* Update sparkles */
    update_sparkles();
    
    /* Check for menu element selections */
    if( input.touch && input.timestamp < 2 )
    {
        /* Vibration (haptic) */
        if( point_in_rect( &r1, input.x, input.y ) )
        {
            settings.vibrate = !settings.vibrate;
        }
        
        /* Speed Control */
        if( point_in_rect( &r2, input.x, input.y ) )
        {
            settings.speed_drop = !settings.speed_drop;
        }
        
        /* Shot colour */
        if( point_in_rect( &r3, input.x, input.y ) )
        {
            settings.shoot_colour++;
            if( settings.shoot_colour > 5 )
                settings.shoot_colour = 0;
        }
        
        /* Back */
        if( point_in_rect( &r6, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_menu;
            update_func = update_menu;
            save_game_settings();
            play_snd( 5, No );
        }
    }

    toggle_fs_check();
}

void draw_pregame_menu1();
void update_pregame_menu1();

void draw_pregame_menu2()
{
    int width, height;
    get_dimensions( &height, &width );
    
    char* ship_names[] = { "Darius Hero >", "< Elias Prophet" };
    float t[8] = { 0.25f, 0.75f, 0.75f, 0.75f, 0.75f, 0.25f, 0.25f, 0.25f };
    static int flash = 0;
    static int c = 1;
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[6]->handle, 320.0f-(mtex[6]->width/2.0f)+2.0f, 60.0f+2.0f, mtex[6]->width, mtex[6]->height );
    set_colour( 1, 1, 1, 1 );
    draw_quad( mtex[6]->handle, 320.0f-(mtex[6]->width/2.0f), 60.0f, mtex[6]->width, mtex[6]->height );
    
    /* Put a picture of the ship in the center */
    draw_quad2( mtex[11+settings.ship_type]->handle, t, 320.0f-32.0f, 150.0f, 64.0f, 64.0f );
    
    /* Flashy red and yellow box! */
    set_colour( 1, c, 0, 1 );
    draw_line( 288.0f, 150.0f, 352.0f, 150.0f );
    draw_line( 352.0f, 150.0f, 352.0f, 214.0f );
    draw_line( 352.0f, 214.0f, 288.0f, 214.0f );
    draw_line( 288.0f, 214.0f, 288.0f, 150.0f );
    
    /* Draw menu options */
    draw_shadowed_text( &font1, 230.0f, 250.0f, ship_names[settings.ship_type] );
    draw_shadowed_text( &font1, 230.0f, 350.0f, "Start!" );
    draw_shadowed_text( &font1, 350.0f, 350.0f, "Back" );
    
    if( ++flash == 5 )
    {
        c = !c;
        flash = 0;
    }
}

void update_pregame_menu2()
{
    struct Rect r1 = { 230.0f, 250.0f, 230.0f+(8.0f*9.0f), 250.0f+11.0f };
    struct Rect r1_1 = { 320.0f-32.0f, 150.0f, (320.0f-32.0f)+64.0f, 150.0f+64.0f };
    struct Rect r2 = { 230.0f, 350.0f, 230.0f+(8.0f*9.0f), 350.0f+11.0f };
    struct Rect r3 = { 350.0f, 350.0f, 350.0f+(8.0f*9.0f), 350.0f+11.0f };
    
    /* Check for menu element selections */
    if( input.touch && input.timestamp <= 1 )
    {
        /* Select your ship */
        if( point_in_rect( &r1, input.x, input.y ) || point_in_rect( &r1_1, input.x, input.y ) )
            settings.ship_type = !settings.ship_type;
        
        /* Now, let's go in-game! */
        if( point_in_rect( &r2, input.x, input.y ) )
        {
            fade_in();
            reset_game();
            draw_func = draw_game;
            update_func = update_game;
            set_next_track( "cyrf_cave_of_bone.wav" );
            play_snd( 5, No );
            reset_time();
            ingame = Yes;
        }
        
        /* Go back */
        if( point_in_rect( &r3, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_pregame_menu1;
            update_func = update_pregame_menu1;
            play_snd( 5, No );
        }
    }
    
}


void draw_pregame_menu1()
{
    int width, height;
    get_dimensions( &height, &width );
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[5]->handle, 320.0f-(mtex[5]->width/2.0f)+2.0f, 60.0f+2.0f, mtex[5]->width, mtex[5]->height );
    set_colour( 1, 1, 1, 1 );
    draw_quad( mtex[5]->handle, 320.0f-(mtex[5]->width/2.0f), 60.0f, mtex[5]->width, mtex[5]->height );
    
    /* Draw menu options */
    draw_shadowed_text( &font1, 280.0f, 250.0f, "Easy" );
    draw_shadowed_text( &font1, 280.0f, 300.0f, "Normal" );
    draw_shadowed_text( &font1, 280.0f, 350.0f, "Hard" );
    draw_shadowed_text( &font1, 280.0f, 400.0f, "Back" );
}

void update_pregame_menu1()
{
    struct Rect r1 = { 280.0f, 250.0f, 280.0f+(8.0f*9.0f), 250.0f+11.0f };
    struct Rect r2 = { 280.0f, 300.0f, 280.0f+(8.0f*9.0f), 300.0f+11.0f };
    struct Rect r3 = { 280.0f, 350.0f, 280.0f+(8.0f*9.0f), 350.0f+11.0f };
    struct Rect r4 = { 280.0f, 400.0f, 280.0f+(8.0f*9.0f), 400.0f+11.0f };
    
    /* Check for menu element selections */
    if( input.touch && input.timestamp <= 1 )
    {
        /* Go to the next pregame menu and choose your ship */
        if( point_in_rect( &r1, input.x, input.y ) ||
           point_in_rect( &r2, input.x, input.y ) ||
           point_in_rect( &r3, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_pregame_menu2;
            update_func = update_pregame_menu2;
            play_snd( 5, No );
        }
        
        /* Go back */
        if( point_in_rect( &r4, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_menu;
            update_func = update_menu;
            set_next_track( "transparence.wav" );
            play_snd( 5, No );
        }
        
        /* Set the difficulty level once one has been selected. */
        if( point_in_rect( &r1, input.x, input.y ) )
            settings.difficulty = 1;
        
        if( point_in_rect( &r2, input.x, input.y ) )
            settings.difficulty = 2;
        
        if( point_in_rect( &r3, input.x, input.y ) )
            settings.difficulty = 3;
    }
    
}

int get_rank( int score )
{
    int base = 30000;
    
    /* D-Rank */
    if( score >= base && score < base*2 ) return 1;
    /* C-Rank */
    if( score >= base*2 && score < base*3 ) return 2;
    /* B-Rank */
    if( score >= base*3 && score < base*4 ) return 3;
    /* A-Rank */
    if( score >= base*4 && score < base*5 ) return 4;
    /* S-Rank */
    if( score >= base*5 && score < base*10 ) return 5;
    /* Z-Rank */
    if( score >= base*10 ) return 6;
    
    /* E-Rank (you suck) */
    return 0;
}

void draw_high_score_menu();
void update_high_score_menu();

void draw_post_game_menu()
{
    char string[64];
    char ranks[] = {'E','D','C','B','A','S','Z'};
    int rank = get_rank(user.score);
    int width, height;
    get_dimensions( &height, &width );
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[8]->handle, 320.0f-(mtex[8]->width/2.0f)+2.0f, 60.0f+2.0f, mtex[8]->width, mtex[8]->height );
    set_colour( 1, 1, 1, 1 );
    draw_quad( mtex[8]->handle, 320.0f-(mtex[8]->width/2.0f), 60.0f, mtex[8]->width, mtex[8]->height );
    
    /* Show user's post game status */
    sprintf( string, "Stage: %d", game_stats.stage );
    draw_shadowed_text( &font1, 240, 200, string );
    sprintf( string, "Bugs:  %d", user.bugs );
    draw_shadowed_text( &font1, 240, 220, string );
    sprintf( string, "Bombs: %d x 1000", user.bombs );
    draw_shadowed_text( &font1, 240, 240, string );
    sprintf( string, "Score: %d", user.score );
    draw_shadowed_text( &font1, 240, 260, string );
    sprintf( string, "Rank:  %c", ranks[rank] );
    draw_shadowed_text( &font1, 240, 280, string );
    
    draw_shadowed_text( &font1, 240, 320, "Continue" );
}

void update_post_game_menu()
{
    struct Rect r1 = { 240.0f, 320.0f, 240.0f+(8.0f*9.0f), 320.0f+11.0f };
    
    /* Check for menu element selections */
    if( input.touch && input.timestamp <= 1 )
    {
        /* TODO: Go to the high score menu */
        if( point_in_rect( &r1, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_high_score_menu;
            update_func = update_high_score_menu;
        //    set_next_track( "silence.wav" );
            play_snd( 5, No );
        }
        
    }
}

void drop_to_post_game_menu()
{
    fade_in();
    draw_func = draw_post_game_menu;
    update_func = update_post_game_menu;
    user.score += user.bombs * 1000;
    save_high_scores();
    set_next_track( "cyrf_waiting_room.wav" );
    //play_snd( 5, No );
}

void draw_high_score_menu()
{
    char string[128];
    char ranks[] = {'E','D','C','B','A','S','Z'};
    int i = 0;
    int width, height;
    get_dimensions( &height, &width );
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[9]->handle, 320.0f-(mtex[9]->width/2.0f)+2.0f, 60.0f+2.0f, mtex[9]->width, mtex[9]->height );
    set_colour( 1, 1, 1, 1 );
    draw_quad( mtex[9]->handle, 320.0f-(mtex[9]->width/2.0f), 60.0f, mtex[9]->width, mtex[9]->height );
    
    /* Show high scores */
    while( i < 10 )
    {
        sprintf( string, "%s Score: %6.6d St: %2.2d Rank: %c", high_scores[i].initials, high_scores[i].score,
            high_scores[i].stage, ranks[high_scores[i].rank] );
        draw_shadowed_text( &font1, 170.0f, 180.0f+(15.0f*i), string ); i++;
    }
    
    draw_shadowed_text( &font1, 290.0f, 360.0f, "Back" );
}

void update_high_score_menu()
{
    struct Rect r1 = { 290.0f, 360.0f, 290.0f+(8.0f*9.0f), 360.0f+11.0f };
    
    /* Check for menu element selections */
    if( input.touch )
    {
        /* Return to the main menu */
        if( point_in_rect( &r1, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_menu;
            update_func = update_menu;
            play_snd( 5, No );
            scores_loaded = No;
            
            if( strcmp( "transparence.wav", get_track_name() ) )
               set_next_track( "transparence.wav" );
        }
    }
}

void draw_credits()
{
    int width, height;
    get_dimensions( &height, &width );
    
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    get_tex_coords( mtex[1], &r, tex );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[10]->handle, 320.0f-(mtex[2]->width/2.0f)+2, 60.0f+2, mtex[2]->width, mtex[2]->height );
    set_colour( 1, 1, 1, 1.0f );
    draw_quad( mtex[10]->handle, 320.0f-(mtex[2]->width/2.0f), 60.0f, mtex[2]->width, mtex[2]->height );
    
    /* Show credits */
    draw_shadowed_text( &font1, 180.0f, 200.0f, "Program, Sound and Game Design:" );
    draw_shadowed_text( &font1, 180.0f, 220.0f, "Brandon Scott Fleming" );
    
    draw_shadowed_text( &font1, 180.0f, 250.0f, "Artwork:" );
    draw_shadowed_text( &font1, 180.0f, 270.0f, "Ari Feldman" );
    draw_shadowed_text( &font1, 180.0f, 290.0f, "Brandon Scott Fleming" );
    draw_shadowed_text( &font1, 180.0f, 310.0f, "The Lost Garden" );
    
    draw_shadowed_text( &font1, 180.0f, 340.0f, "Music:" );
    draw_shadowed_text( &font1, 180.0f, 360.0f, "Cyber Rainforce" );
    draw_shadowed_text( &font1, 180.0f, 380.0f, "Musicmaterial.jpn.org" );
    
    /* Show copyright string */
    draw_shadowed_text( &font1, 175.0f, height-30, "(C) 2012-13 Shogun3D (TM)" );
}

void update_credits()
{
    /* Check for menu element selections */
    if( input.touch && input.timestamp <= 1 )
    {
        /* Return to the main menu */
        fade_in();
        draw_func = draw_menu;
        update_func = update_menu;
        play_snd( 5, No );
    }

}

void draw_ingame_pause_menu()
{
    int width, height;
    get_dimensions( &height, &width );
    
    /* Darken the screen a bit */
    set_colour( 0.0f, 0.0f, 0.0f, pause_screen_alpha/255.0f );
    draw_quad( 0, 160.0f, 0.0f, 320.0f, (float)height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[13]->handle, 320.0f-(mtex[2]->width/2.0f)+2, 60.0f+2, mtex[2]->width, mtex[2]->height );
    set_colour( 1, 1, 1, 1.0f );
    draw_quad( mtex[13]->handle, 320.0f-(mtex[2]->width/2.0f), 60.0f, mtex[2]->width, mtex[2]->height );
}

int update_ingame_pause_menu()
{
    extern int paused;
    
    /* Fade out the pause screen */
    if( unpausing )
    {
        pause_screen_alpha -= 5.0f;
        if( pause_screen_alpha <= 0.0f )
        {
            paused = No;
            return 1;
        }
    }
    
    return 0;
}

void draw_menu()
{
    /* Draw the title background */
    float tex[8];
    struct Rect r = { 0, 0, 320, 480 };
    int width, height;
    get_tex_coords( mtex[1], &r, tex );
    get_dimensions( &height, &width );
    draw_quad2( mtex[1]->handle, tex, 160.0f, 0.0f, 320.0f, (float) height );
    
    /* Draw the title */
    set_colour( 0, 0, 0, 1.0f );
    draw_quad( mtex[4]->handle, 320.0f-(mtex[4]->width/2.0f)+2.0f, 60.0f+2.0f, mtex[4]->width, mtex[4]->height );
    set_colour( 1, 1, 1, 1 );
    draw_quad( mtex[4]->handle, 320.0f-(mtex[4]->width/2.0f), 60.0f, mtex[4]->width, mtex[4]->height );
    
    /* Draw sparkles */
    draw_sparkles();
    
    /* Draw menu options */
    draw_shadowed_text( &font1, 280.0f, 240.0f, "New Game" );
    draw_shadowed_text( &font1, 280.0f, 280.0f, "Options" );
    draw_shadowed_text( &font1, 280.0f, 320.0f, "Rankings" );
    draw_shadowed_text( &font1, 280.0f, 360.0f, "Credits" );
    
    /* Draw copyright string */
    draw_shadowed_text( &font1, 175.0f, (float)(height-30), "(C) 2012-13 Shogun3D (TM)" );
}

void update_menu()
{
    struct Rect r1 = { 280.0f, 240.0f, 280.0f+(8.0f*9.0f), 240.0f+11.0f };
    struct Rect r2 = { 280.0f, 280.0f, 280.0f+(8.0f*9.0f), 280.0f+11.0f };
    struct Rect r3 = { 280.0f, 320.0f, 280.0f+(8.0f*9.0f), 320.0f+11.0f };
    struct Rect r4 = { 280.0f, 360.0f, 280.0f+(8.0f*9.0f), 360.0f+11.0f };
    
    /* Update sparkles */
    update_sparkles();
    
    /* Check for menu element selections */
    if( input.touch && input.timestamp <= 1 )
    {
        /* New Game */
        if( point_in_rect( &r1, input.x, input.y ) )
        {
            fade_in();
            reset_game();
            draw_func = draw_pregame_menu1;
            update_func = update_pregame_menu1;
            set_next_track( "cyrf_crashed_dimension.wav" );
            play_snd( 5, No );
        }
        
        /* Options */
        if( point_in_rect( &r2, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_option_menu;
            update_func = update_option_menu;
            
            /* Load saved settings, if we haven't already */
            load_game_settings();
    
            play_snd( 5, No );
        }
        
        /* Rankings */
        if( point_in_rect( &r3, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_high_score_menu;
            update_func = update_high_score_menu;
            
            /* Load high scores if we haven't done so already */
            load_high_scores();
            
            play_snd( 5, No );
        }
        
        /* Credits */
        if( point_in_rect( &r4, input.x, input.y ) )
        {
            fade_in();
            draw_func = draw_credits;
            update_func = update_credits;
            
            play_snd( 5, No );
        }
    }
    
    toggle_fs_check();
}
