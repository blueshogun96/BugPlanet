//
//  input.h
//  BugPlanet
//
//  Created by admin on 12/1/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#ifndef BugPlanet_input_h
#define BugPlanet_input_h

/* Global input structure */
struct input_t
{
    int escape;                 /* Does the user want to quit? */
    int up, down, left, right;  /* Basic movement */
    int firing;                 /* Is the user shooting? */
    int bomb;                   /* Is the user releasing a bomb? */
    int select;                 /* Did the user select something? */
    int pause;                  /* Pause request? */
    int screenshot;             /* Taking a screenshot? */
    int toggle_fs;              /* Did the user press the fullscreen toggle button */
    int timestamp;              /* Timestamp counter */
    int touch;                  /* Was the screen touched? */
    int x, y;                   /* Touch position */
    int lx, ly;                 /* Last touch position */
};

int enable_joystick();
void disable_joystick();
void poll_input_devices( struct input_t* inp );

void touch_func(int unused1, int is_down , int x, int y);
void move_func(int x, int y);

#endif
