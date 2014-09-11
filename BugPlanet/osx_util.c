//
//  osx_util.c
//  BugPlanet
//
//  Created by admin on 1/25/13.
//  Copyright (c) 2013 Shogun3D. All rights reserved.
//

#include "platform.h"
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>


float get_frame_duration()
{
    // Compute Frame Duration
    static CFAbsoluteTime sPreviousTime = 0;
    const CFAbsoluteTime newTime = CFAbsoluteTimeGetCurrent();
    const CFAbsoluteTime deltaTime = newTime - sPreviousTime;
    sPreviousTime = newTime;
    float frameDuration = deltaTime;
    
    // keep frameDuration in [0.01 ; 0.5] seconds
    if (frameDuration > 0.5f)
    {
        frameDuration = 0.5f;
    }
    else if (frameDuration < 0.01f)
    {
        frameDuration = 0.01f;
    }
    
    return frameDuration;
}

float get_frames_per_second()
{
    static float frameCount = 0;
    static float framesPerSecond = 0;
    static CFAbsoluteTime sPreviousTime = 0;
    const CFAbsoluteTime newTime = CFAbsoluteTimeGetCurrent();
    const CFAbsoluteTime deltaTime = newTime - sPreviousTime;
    frameCount++;
    
    if( deltaTime > 1.0f )
    {
        sPreviousTime = newTime;
        framesPerSecond = frameCount;
        frameCount = 0;
    }
    
    return framesPerSecond;
}


float startTime = 0;    /* The starting timer value */
float previousTime = 0; /* The previous timer value */
float deltaTime = 0;    /* The amount of time that has passed since the last call */
float elapsedTime = 0;  /* The amount of time that has passed overall */

void reset_time()
{
    /* Reset timer variables */
    startTime = CFAbsoluteTimeGetCurrent();
    previousTime = CFAbsoluteTimeGetCurrent();
    elapsedTime = 0;
}

float update_time()
{
    /* Get the current time */
    float currentTime = CFAbsoluteTimeGetCurrent();
    
    /* Update the elapsed and delta time variables */
    elapsedTime = currentTime - startTime;
    deltaTime = currentTime - previousTime;
    
    /* Update the previous time variable */
    previousTime = currentTime;
}

float get_elapsed_time()
{
    return elapsedTime;
}

float get_delta_time()
{
    return deltaTime;
}

float get_current_time()
{
    return CFAbsoluteTimeGetCurrent();
}

int set_current_path_to_resource_directory()
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
        return 0;
    }
    CFRelease(resourcesURL);
    
    chdir(path);
    printf( "Current directory changed to: %s\n", path );
    
    return 1;
}

char* get_file_dir( char* fname, char* fext )
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
    {
        // error!
        return 0;
    }
    CFRelease(resourcesURL);
    
    return path;
}

void vibrate()
{
    AudioServicesPlaySystemSound( kSystemSoundID_Vibrate );
}
