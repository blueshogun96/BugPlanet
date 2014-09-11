//
//  game.h
//  BugPlanet
//
//  Created by admin on 12/1/12.
//  Copyright (c) 2012 Shogun3D. All rights reserved.
//

#ifndef BugPlanet_game_h
#define BugPlanet_game_h

/* Game stats structure */
struct game_stats_t
{
    double enemies;         /* Number of enemies unleashed this session */
    double shots_fired;     /* How many shoots did the user fire */
    double shots_missed;    /* How many shoots went out of bounds? */
    int stage;              /* What stage is this? */
    int difficulty;         /* Difficuty level */
};

/* Game settings */
struct game_settings_t
{
    float brightness;   /* Brightness control */
    float bgm_volume;   /* Background music volume */
    float sfx_volume;   /* SoundFX volume */
    int speed_drop;     /* Half game speed when the screen has too many enemy shoots */
    int shoot_colour;   /* Enemy shoot colour */
    int difficulty;     /* Game difficulty setting */
    int ship_type;      /* Preffered ship type */
    int vibrate;        /* Enable vibration? */
};

/* Difficulty settings */
struct diff_settings_t
{
    int e_shoot_timer;
    int e_shoot_timer_min;
    int e_spawn_timer;
    int e_spawn_timer_max;
    int e_spawn_timer_min;
    float e_shoot_speed;
    float e_speed;
    int e_large_odds;
};

/* User structure */
struct user_t
{
    float x, y;     /* 2D Position */
    int score;      /* Score */
    double kills;   /* Number of kills this session */
    int a_delay;    /* Animation delay */
    int s_delay;    /* Shoot delay */
    int r_delay;    /* Respawn delay */
    int i_delay;    /* Invincibility delay */
    int ships;      /* Ships remaining */
    int frame;      /* Animation frame */
    int type;       /* The ship's "type" */
    int bombs;      /* Bomb count */
    float speed;    /* Ship speed */
    float collision_rect_size;  /* Size of the ship's collision center point */
    int bugs;       /* The number of bugs the user managed to kill */
};

/* Enemy sprite structure */
struct enemy_t
{
    float x, y;     /* 2D Position */
    float w, h;     /* Size dimensions */
    float dx, dy;   /* Destination (for big bugs) */
    float rot;      /* Rotation angle */
    int energy;     /* Enemy's strength */
    int is_large;   /* Is this a large enemy? */
    int type;       /* Enemy type */
    int score_value;/* How much is this enemy worth? */
    int a_delay;    /* Animation delay */
    int s_delay;    /* Shoot delay */
    float v[10];    /* 10 general purpose variables (v8 and v9 are reserved) */
    int tex;        /* Sprite texture to use */
    struct spline_t* spline; /* Spline (copy of real spline) */
    int flashing;   /* Is this enemy flashing? */
    int id;         /* Enemy's id number */
};

/* User shoot structure */
struct user_shoot_t
{
    float x, y;     /* 2D Position */
    int type;       /* Shoot type */
    float velocity; /* Shoot velocity */
    int was_used;   /* Did this shoot hit anything? */
};

/* Enemy shoot structure */
struct enemy_shoot_t
{
    float x, y;     /* 2D Position */
    int type;       /* Shoot type */
    float vx, vy;   /* Shoot velocity */
    float angle;    /* Angle to the user's position */
    float speed;    /* Speed of this shoot */
    int owner;      /* Owner of this shoot */
    int nullified;  /* Was the owner of this shoot killed? */
};

/* Explosion structure */
struct explosion_t
{
    float x, y;     /* 2D Position */
    int big;        /* Big or little explosion? */
    int max_frame;  /* Maximum number of animation frames */
    int frame;      /* Current animation frame */
    int anim_speed; /* Animation speed */
    int timer;      /* Timer for each animation sequence */
};

/* Smoke puff structure */
struct smoke_t
{
    float x, y;     /* 2D Position */
    float rot;      /* Rotation angle */
    int max_frame;  /* Maximum number of animation frames */
    int frame;      /* Current animation frame */
    int anim_speed; /* Animation speed */
    int timer;      /* Timer for each animation sequence */
    float size;     /* Size of the smoke puff */
};

/* Foliage structure */
struct foliage_t
{
    float x, y;         /* 2D Position */
    unsigned int tex;   /* Texture handle */
};

/* Bubble structure */
struct bubble_t
{
    float x, y;     /* Position in 2D space */
    int frame;      /* Current animation frame */
    int anim_speed; /* Animation speed */
    float size;     /* Size of this bubble */
};

/* "Shooting Star" structure */
struct star_t
{
    float x, y;
    int frame;
    int frame_timer;
    float sine;
    int shot_timer;
};

/* Missle structure */
struct missle_t
{
    float x, y;     /* 2D Position */
    float vx, vy;   /* Velocity */
    float rot;      /* Rotation angle */
    int was_used;   /* Was this missile used? */
    int smoke_timer;/* Smoke puff timer */
};

/* Crystal structure */
struct crystal_t
{
    float x, y;     /* 2D Position */
    float vx, vy;   /* Velocity */
    float size;     /* Crystal size */
    int value;      /* Score value */
    int delay;      /* Delay until it starts coming towards you */
};

/* Bomb structure */
struct bomb_t
{
    float x, y;     /* Bomb's position */
    float start_y;  /* The initial starting point */
    float dist;     /* The total distance this bomb has traveled */
    float b_radius; /* Blast radius */
    int active;     /* Is this bomb active? */
};

/* Extran bomb structure */
struct extra_bomb_t
{
    float x, y;     /* Extra bomb's position */
    float vx, vy;   /* Extra bomb's velocity */
    int active;     /* Is this extra bomb active? */
};

/* Shockwave structure */
struct shockwave_t
{
    float x, y;     /* Shockwave's center point */
    float vx, vy;   /* Velocity */
    float size;     /* How big has it gotten? */
    float max_size; /* How big can this shockwave get? */
    float alpha;    /* Alpha level. Decrease once you reach max size. */
};

int init_game();
void uninit_game();
void reset_game();
void render();
int update();

void draw_game();
void update_game();

#endif
