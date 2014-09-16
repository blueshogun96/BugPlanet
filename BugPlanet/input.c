//
//  input.c
//  BugPlanet
//
//  Created by admin on 12/1/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#include "platform.h"
#include "input.h"

#if 0
#include <SDL.h>


/* 
 * Globals
 */
SDL_Event event;
SDL_Joystick* joystick = NULL;


/*
 * Joystick functions
 */
int enable_joystick()
{
    /* Initialize the joystick subsystem */
    if( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) < 0 )
        return 0;
    
    /* Check for at least 1 available joystick */
    if( SDL_NumJoysticks() == 0 )
        return 0;
    
    /* Enable joystick events */
    SDL_JoystickEventState( SDL_ENABLE );
    
    /* Get a handle to the first joystick */
    joystick = SDL_JoystickOpen(0);
    if( !joystick )
        return 0;
    
    return 1;
}

void disable_joystick()
{
    /* Uninitialize the joystick subsystem */
    SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}



/*
 * Input functions
 */
void on_key_down( SDL_keysym* key, struct input_t* inp )
{
    /* Check the keyboard for the specificly mapped keys */
    switch( key->sym )
    {
        case SDLK_ESCAPE:   inp->escape = Yes; break;
        case SDLK_LEFT:     inp->left = Yes; break;
        case SDLK_RIGHT:    inp->right = Yes; break;
        case SDLK_UP:       inp->up = Yes;  break;
        case SDLK_DOWN:     inp->down = Yes; break;
        case SDLK_RETURN:   inp->select = Yes; break;
        case SDLK_z:        inp->firing = Yes; break;
        case SDLK_x:        inp->bomb = Yes; break;
        case SDLK_f:        inp->toggle_fs = Yes; break;
        case SDLK_p:        inp->pause = Yes; break;
        case SDLK_s:        inp->screenshot = Yes; break;
        default: break;
    }
}

void on_key_up( SDL_keysym* key, struct input_t* inp )
{
    /* Check the keyboard for the specificly mapped keys */
    switch( key->sym )
    {
        case SDLK_ESCAPE:   inp->escape = No; break;
        case SDLK_LEFT:     inp->left = No; break;
        case SDLK_RIGHT:    inp->right = No; break;
        case SDLK_UP:       inp->up = No;  break;
        case SDLK_DOWN:     inp->down = No; break;
        case SDLK_RETURN:   inp->select = No; break;
        case SDLK_z:        inp->firing = No; break;
        case SDLK_x:        inp->bomb = No; break;
        case SDLK_f:        inp->toggle_fs = No; break;
        case SDLK_p:        inp->pause = No; break;
        case SDLK_s:        inp->screenshot = No; break;
        default: break;
    }
}

void on_touch_down( SDL_MouseButtonEvent* mouse, struct input_t* inp )
{
    /* Use the primary mouse button click position to simulate touches */
    if( mouse->button == 1 )
    {
        inp->x = mouse->x;
        inp->y = mouse->y;
        inp->touch = Yes;
    }
}

void on_touch_up( SDL_MouseButtonEvent* mouse, struct input_t* inp )
{
    /* Release touch position */
    if( mouse->button == 1 )
    {
        inp->touch = No;
    }
}

void on_joy_axis_move( SDL_JoyAxisEvent* jaxis, struct input_t* inp )
{
    const int deadzone = 8000;
    
    /* Left/right movement */
    if( jaxis->axis == 0 )
    {
        if( jaxis->value < -deadzone )
            inp->left = Yes;
        else
            inp->left = No;
        
        if( jaxis->value > deadzone )
            inp->right = Yes;
        else
            inp->right = No;
    }
    
    /* Up/down movement */
    if( jaxis->axis == 1 )
    {
        if( jaxis->value < -deadzone )
            inp->up = Yes;
        else
            inp->up = No;
        
        if( jaxis->value > deadzone )
            inp->down = Yes;
        else
            inp->down = No;
    }
    
#if 0
    if( jaxis->value < -deadzone )
    {
        /* Left movement */
        if( jaxis->axis == 0 )
        {
            inp->left = Yes;
        }
        
        /* Down movement */
        if( jaxis->axis == 1 )
        {
            inp->down = Yes;
        }
    }
    
    if( jaxis->value < deadzone )
    {
        /* Up movement */
        if( jaxis->axis == 0 )
        {
            inp->right = Yes;
        }
        
        /* Up movement */
        if( jaxis->axis == 1 )
        {
            inp->up = Yes;
        }
    }
#endif
}

void on_joy_button_down( SDL_JoyButtonEvent* button, struct input_t* inp )
{
    switch( button->button )
    {
        case '\v':
        case '\r': inp->firing = Yes; break;
        case '\f':
        case 0xe: inp->bomb = Yes; break;
        case 4: inp->pause = Yes; break;
        default: break;
    }
}

void on_joy_button_up( SDL_JoyButtonEvent* button, struct input_t* inp )
{
    switch( button->button )
    {
        case '\v':
        case '\r': inp->firing = No; break;
        case '\f':
        case 0xe: inp->bomb = No; break;
        case 4: inp->pause = No; break;
        default: break;
    }
}

void poll_input_devices( struct input_t* inp )
{
    static int update_ts = No;
    
    /* Clear input structure */
//    ZeroMemory( inp, sizeof( struct input_t ) );
    
    /* Poll input devices */
    while( SDL_PollEvent( &event ) )
    {
        switch( event.type )
        {
            case SDL_KEYDOWN:
                on_key_down( &event.key.keysym, inp );
                update_ts = Yes;
                break;
            case SDL_KEYUP:
                on_key_up( &event.key.keysym, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                on_touch_down( &event.button, inp );
                update_ts = Yes;
                break;
            case SDL_MOUSEBUTTONUP:
                on_touch_up( &event.button, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_JOYBUTTONDOWN:
                on_joy_button_down( &event.jbutton, inp );
                update_ts = Yes;
                break;
            case SDL_JOYBUTTONUP:
                on_joy_button_up( &event.jbutton, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_JOYAXISMOTION:
                on_joy_axis_move( &event.jaxis, inp );
                break;
            case SDL_QUIT:
                inp->escape = Yes;
                break;
        }
    }
    
    /* Update timestamp */
    if( update_ts )
        inp->timestamp++;
    else
        inp->timestamp = 0;
}

#endif

struct touch_t
{
    int x, y;
    int lx, ly;
    int is_down;
    int moved;
    int multitouch;
};

/* Touch data */
struct touch_t touch;


int enable_joystick()
{
    return 0;
}

void disable_joystick()
{
    
}

void poll_input_devices( struct input_t* inp )
{
    static int update_ts = No;
    
    /* Clear input structure */
    //    ZeroMemory( inp, sizeof( struct input_t ) );
    
    /* Poll input devices */
    /*while( SDL_PollEvent( &event ) )
    {
        switch( event.type )
        {
            case SDL_KEYDOWN:
                on_key_down( &event.key.keysym, inp );
                update_ts = Yes;
                break;
            case SDL_KEYUP:
                on_key_up( &event.key.keysym, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_MOUSEBUTTONDOWN:
                on_touch_down( &event.button, inp );
                update_ts = Yes;
                break;
            case SDL_MOUSEBUTTONUP:
                on_touch_up( &event.button, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_JOYBUTTONDOWN:
                on_joy_button_down( &event.jbutton, inp );
                update_ts = Yes;
                break;
            case SDL_JOYBUTTONUP:
                on_joy_button_up( &event.jbutton, inp );
                update_ts = No;
                inp->timestamp = 0;
                break;
            case SDL_JOYAXISMOTION:
                on_joy_axis_move( &event.jaxis, inp );
                break;
            case SDL_QUIT:
                inp->escape = Yes;
                break;
        }
    }*/
    extern int reduce_speed;
    extern int swidth, sheight;
    
    if( reduce_speed )
        return;
    
    /* Always shooting */
    inp->firing = Yes;
    
    if( touch.is_down )
    {
        inp->touch = Yes;
        inp->x = touch.x;
        inp->y = touch.y;
        inp->lx = touch.moved ? touch.lx : touch.x;
        inp->ly = touch.moved ? touch.ly : touch.y;
        inp->bomb = touch.multitouch;
        update_ts = Yes;
        
        if( touch.x > 160+320-50 && touch.y < 80 && !touch.moved )
            inp->pause = Yes;
        if( touch.x > 160 && touch.x < 210 && touch.y > (sheight-120) && touch.y < (sheight-120)+50 && !touch.moved )
            inp->bomb = Yes;
        
        touch.moved = No;
    }
    else
    {
        inp->pause = No;
        inp->bomb = No;
        inp->touch = No;
        update_ts = No;
        inp->timestamp = 0;
    }
    
    /* Update timestamp */
    if( update_ts )
        inp->timestamp++;
    else
        inp->timestamp = 0;
}

void touch_func(int unused1, int is_down , int x, int y)
{
    if( unused1 > 1 && touch.is_down )
        return;
    
    touch.is_down = is_down;
    touch.x = x;
    touch.y = y;
    touch.lx = x;
    touch.ly = y;
    touch.moved = No;
    touch.multitouch = ( unused1 > 1 ) ? Yes : No;
}

void move_func(int x, int y)
{
    extern int reduce_speed;
    
    if( reduce_speed == Yes )
        return;
    
    touch.moved = Yes;
    touch.lx = touch.x;
    touch.ly = touch.y;
    touch.x = x;
    touch.y = y;
}
