#include "platform.h"
#include "game.h"
#include "input.h"


int (*_init_func)();
void (*_display_func)(void);
void (*_touch_func)(int, int, int, int);
void (*_move_func)(int, int);
void (*_exit_func)();

void display_func()
{
    /* Update the game if the frame rate hasn't exceeded the limit */
    if( calculate_framerate() )
    {
        render();
        update();
    }
}

void pre_init()
{
    _init_func = init_game;
    _display_func = display_func;
    _exit_func = uninit_game;
    _touch_func = touch_func;
    _move_func = move_func;
}

