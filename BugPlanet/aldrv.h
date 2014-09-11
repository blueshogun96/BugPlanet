#pragma once

//--------------------------------------------------------------------------------------
// OpenAL includes
//--------------------------------------------------------------------------------------

#include <OpenAL/al.h>
#include <OpenAL/alc.h>
//#include <al\alut.h>

#if 0
#include "inc\\ogg\\ogg.h"
#include "inc\\vorbis\\codec.h"
#include "inc\\vorbis\\vorbisinc.h"
#include "inc\\vorbis\\vorbisfile.h"
#endif

//#define BUFFER_SIZE (4096 * 16)


//--------------------------------------------------------------------------------------
// Vector 3D (if not defined)
//--------------------------------------------------------------------------------------
struct Vector3
{
	float x, y, z;
};

//--------------------------------------------------------------------------------------
// OpenAL sound effect
//--------------------------------------------------------------------------------------
struct sound_t
{
	ALuint buffer;
	ALuint source;
	ALfloat volume;
	float  pitch;
	struct Vector3 position;
	struct Vector3 velocity;
};

//--------------------------------------------------------------------------------------
// OpenAL ogg vorbis player (streaming)
//--------------------------------------------------------------------------------------
#if 0
struct oggstream_t
{
	FILE*			file;
	OggVorbis_File	stream;
	vorbis_info*	info;
	vorbis_comment*	comment;
	ALuint			buffers[2];
	ALuint			source;
	ALenum			format;
};
#endif

//--------------------------------------------------------------------------------------
// OpenAL initialization/uninitialization
//--------------------------------------------------------------------------------------
int EnableOpenAL();
void DisableOpenAL();

//--------------------------------------------------------------------------------------
// Misc OpenAL stuff
//--------------------------------------------------------------------------------------
char* get_al_error();

//--------------------------------------------------------------------------------------
// sound effect functions
//--------------------------------------------------------------------------------------
int  create_sound_wav( char* filename, struct sound_t* sound );
int  create_sound_wav_from_memory( ALubyte* buffer, long size, struct sound_t* sound );
void delete_sound( struct sound_t* sound );
void play_sound_effect( struct sound_t* sound, int looping );
void play_sound_effect_static( struct sound_t* sound, int looping );
void stop_sound_effect( struct sound_t* sound );
void pause_sound_effect( struct sound_t* sound );
int  sound_is_playing( struct sound_t* sound );
void set_sound_effect_pitch( struct sound_t* sound, float pitch );
void set_sound_effect_volume( struct sound_t* sound, float gain );
void set_sound_effect_position( struct sound_t* sound, struct Vector3* position );
void set_sound_effect_velocity( struct sound_t* sound, struct Vector3* velocity );

//--------------------------------------------------------------------------------------
// ogg playback functions
//--------------------------------------------------------------------------------------
/*int  oggstream_open( struct oggstream_t* ogg, char* file );
void oggstream_release( struct oggstream_t* ogg );
void oggstream_display( struct oggstream_t* ogg );
int	 oggstream_playback( struct oggstream_t* ogg );
int	 oggstream_isplaying( struct oggstream_t* ogg );
int  oggstream_update( struct oggstream_t* ogg ); 
int  oggstream_stream( struct oggstream_t* ogg, int buf ); 
void oggstream_empty( struct oggstream_t* ogg ); 
void oggstream_check( struct oggstream_t* ogg );*/

//--------------------------------------------------------------------------------------
// listener functions
//--------------------------------------------------------------------------------------
void set_listener_position( struct Vector3* position );
void set_listener_velocity( struct Vector3* velocity );
void set_listener_orientation( struct Vector3* at, struct Vector3* up );