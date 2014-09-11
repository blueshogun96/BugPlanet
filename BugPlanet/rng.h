//
//  rng.h
//  BugPlanet
//
//  Created by admin on 12/4/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#ifndef BugPlanet_rng_h
#define BugPlanet_rng_h

/* Random number generator structure */
struct rng_t
{
    void (*setseed)(int);  /* Sets the random number generator seed */
    void (*reseed)();      /* Resets the seed back to it's prior state */
    int (*random)(int);    /* The function pointer to the actual algorithm */
};

/* Pre-defined random number generators */
int rand1( int lim );
int rand2( int lim );
int rand3( int lim );

#define RNG_DEFAULT1 rand1
#define RNG_DEFAULT2 rand2
#define RNG_DEFAULT3 rand3

/* Initialization function */
void rng_init( struct rng_t* rng, int (*rng_algorithm)(int) );

#endif
