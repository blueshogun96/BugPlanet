//
//  osx_util.h
//  BugPlanet
//
//  Created by admin on 1/25/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#ifndef BugPlanet_osx_util_h
#define BugPlanet_osx_util_h

float get_frame_duration();
float get_frames_per_second();

void reset_time();
float update_time();
float get_elapsed_time();
float get_delta_time();
float get_current_time();
void vibrate();

#endif
