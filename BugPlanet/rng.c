//
//  rng.c
//  BugPlanet
//
//  Created by admin on 12/4/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#include <stdio.h>
#include "rng.h"


int master_seed = 0;    /* The master seed's value */
int slave_seed;


void setseed( int seed )
{
    /* Set the master seed value */
    master_seed = seed;
}

void reseed()
{
    /* Reset the slave seed to the master seed's value */
    slave_seed = master_seed;
}


/* http://www.daniweb.com/software-development/c/code/216329/construct-your-own-random-number-generator */

int rand1( int lim )
{
    if( slave_seed < 100000 )
        slave_seed += 100000;
    
    slave_seed = (slave_seed * 125) % 2796203;
    return ((slave_seed % lim) + 1);
}

int rand2( int lim )
{
    slave_seed = (slave_seed * 32719 + 3) % 32749;
    return ((slave_seed % lim) + 1);
}

int rand3( int lim )
{
    slave_seed = (((slave_seed * 214013L + 2531011L) >> 16) & 32767);
    return ((slave_seed % lim) + 1);
}

void rng_init( struct rng_t* rng, int (*rng_algorithm)(int) )
{
    if( rng )
    {
        rng->setseed = setseed;
        rng->reseed = reseed;
        rng->random = rng_algorithm;
    }
}
