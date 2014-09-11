#pragma warning (disable:4996)	// Depreciation warnings

#include "platform.h"
#include "aldrv.h"
#include "wavstream.h"



//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
#define USE_ALUT	0

struct au_header_t
{
	ALuint magic_number;
	ALuint data_offset;
	ALuint data_size;
	ALuint encoding;
	ALuint sample_rate;
	ALuint channels;
};

ALCdevice*	pDevice = NULL;
ALCcontext*	pContext = NULL;

struct Vector3 listener_position;
struct Vector3 listener_velocity;
struct Vector3 listener_at;
struct Vector3 listener_up;



//-----------------------------------------------------------------------------
// Name: EnableOpenAL
// Desc: Initializes OpenAL
//-----------------------------------------------------------------------------
int EnableOpenAL()
{
	// We'll use ALUT to initialize OpenAL for us for the time being,
	// then we can create contexts and devices of our own later for
	// greater control.
#if USE_ALUT
	alutInit( NULL, 0 );
#else
//	alutInitWithoutContext( NULL, 0 );

	pDevice = alcOpenDevice( NULL );
	if( !pDevice )
		return 0;

	pContext = alcCreateContext( pDevice, NULL );
	if( !pContext )
	{
		alcCloseDevice( pDevice );
		return 0;
	}

	if( !alcMakeContextCurrent( pContext ) )
		return 0;
#endif

	// Clear the error bit (why doesn't OpenAL do this for us?)
	alGetError();

	// Create a new OpenAL device (NULL for default, for now)
/*	pDevice = alcOpenDevice( NULL );
	if( !pDevice )
		return 0;*/
    
    /* Clear the listener attributes */
    ZeroMemory( &listener_position, sizeof( struct Vector3 ) );
    ZeroMemory( &listener_velocity, sizeof( struct Vector3 ) );
    ZeroMemory( &listener_up, sizeof( struct Vector3 ) );
    ZeroMemory( &listener_at, sizeof( struct Vector3 ) );
    listener_at.z = -1.0f;

	return 1;
}


//-----------------------------------------------------------------------------
// Name: DisableOpenAL
// Desc: Uninitializes OpenAL
//-----------------------------------------------------------------------------
void DisableOpenAL()
{
#if USE_ALUT
	alutExit();
#else
	if( pContext )
	{
		alcMakeContextCurrent( NULL );
		alcDestroyContext( pContext );
        
        pContext = NULL;
	}

	if( pDevice )
    {
		alcCloseDevice( pDevice );
        pDevice = NULL;
    }

//	alutExit();
#endif
}

//-----------------------------------------------------------------------------
// Name: get_al_error
// Desc: Returns an error code string if OpenAL recieved an error.  If there 
//		 was no error returned (AL_NO_ERROR), then NULL is returned.
//-----------------------------------------------------------------------------
char* get_al_error()
{
	ALuint error;
	ALchar* string;

	char* error_strings[] = 
	{
		NULL,
		"AL_INVALID_NAME",
		"AL_INVALID_ENUM",
		"AL_INVALID_VALUE",
		"AL_INVALID_OPERATION",
	};

	error = alGetError();

//	string = alutGetErrorString( error );

//	OutputDebugString( "get_al_error(): " );
//	OutputDebugString( string );
//	OutputDebugString( "\n" );

	return error_strings[error&0x000F];
}

//-----------------------------------------------------------------------------
// Name: create_sound_wav
// Desc: Creates a sound and source from a .wav file.
//		 TODO: Stop using ALUT for this.
//-----------------------------------------------------------------------------
int  create_sound_wav( char* filename, struct sound_t* sound )
{
	ALenum	format;
	ALsizei	size;
	ALvoid* data;
	ALsizei frequency;
	ALboolean loop = 0;
	ALfloat pos[] = { 0.0f, 0.0f, 0.0f };
	ALuint error = 0;
	char* error_string;
	FILE* fp;

	if( !sound )
		return 0;

#if USE_ALUT
	// Create a new sound buffer
	alGenBuffers( 1, &sound->buffer );

	if( alGetError() )
		return 0;

	// Open wav file from file
	alutLoadWAVFile( filename, &format, &data, &size, &frequency, &loop );
	alBufferData( sound->buffer, format, data, size, frequency );
	alutUnloadWAV( format, data, size, frequency );
#else
//	sound->buffer = alutCreateBufferFromFile( filename );
//	sound->buffer = wavstatic_open( filename );

	// Open the .wav file and read it contents
	fp = fopen( filename, "rb" );
	if(!fp)
		return 0;

	// Get the file size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Allocate a buffer to read in the .wav file
	data = malloc( size );
	if(!data)
	{
		fclose(fp);
		return 0;
	}

	// Read in the file data
	fread( data, size, 1, fp );

	// Create a new sound with OpenAL
	if( !create_sound_wav_from_memory( (ALubyte*) data, size, sound ) )
	{
		fclose(fp);
		free(data);
		return 0;
	}

	free(data);
	fclose(fp);

	return 1;
#endif

	// Create a new sound source
	alGenSources( 1, &sound->source );

//	if( alGetError() )
//		return 0;
	error_string = get_al_error();
	if( error_string )
		return 0;

	memset( &sound->position, 0, sizeof( struct Vector3 ) );
	memset( &sound->velocity, 0, sizeof( struct Vector3 ) );

	// Set the sound source's default attributes
	alSourcei( sound->source, AL_BUFFER, sound->buffer );
	alSourcef( sound->source, AL_PITCH,  1.0f );
	alSourcef( sound->source, AL_GAIN,	 1.0f );
	alSourcefv( sound->source, AL_POSITION, (float*) &sound->position );
	alSourcefv( sound->source, AL_VELOCITY, (float*) &sound->velocity );
	alSourcei( sound->source, AL_LOOPING, loop );

	if( (error = alGetError()) != AL_NO_ERROR )
		return 0;

	sound->volume = 1.0f;

	return 1;
}

//-----------------------------------------------------------------------------
// Name: create_sound_wav
// Desc: Creates a sound and source from a .wav file.
//		 TODO: Stop using ALUT for this.
//-----------------------------------------------------------------------------
int  create_sound_wav_from_memory( ALubyte* buffer, long size, struct sound_t* sound )
{
	ALenum	format;
	ALsizei	data_size;
    ALsizei channels;
	ALvoid* data;
	ALsizei frequency;
	ALboolean loop;
    int byte = 0;
    int bits = 0;
	char* error;

	if( !sound )
		return 0;

	if( !buffer )
		return 0;
    
	/* Create buffers and sources */
	alGenBuffers( 1, &sound->buffer );
	if( get_al_error() )
		return 0;
    
	alGenSources( 1, &sound->source );
	if( get_al_error() )
		return 0;
    
	/* Allocate this buffer to read bytes from as we go. */
	/* Should check for the strings "RIFF" and "WAVE" in the header, but I don't feel like
     it at the moment, so... */
	/*buf = malloc( BUFFER_SIZE );
	fread( buf, 1, 12, file );*/
    byte += 12;
    
	/* Check the wav file chunk header for the format information. If it's not there, forget it */
	//fread( buf, 1, 8, file );
	if( buffer[byte] != 'f' || buffer[byte+1] != 'm' || buffer[byte+2] != 't' || buffer[byte+3] != ' ' )
	{
        alDeleteSources( 1, &sound->source );
		alDeleteBuffers( 1, &sound->buffer );
		return 0;
	}
    byte += 8;
    
	/* Read the wav format type. 1 == PCM.  If it's not, then we either
     exit, or have to decode the format ourselves */
	//fread( buf, 1, 2, file );
	if( buffer[byte+1] != 0 || buffer[byte] != 1 )
	{
        alDeleteSources( 1, &sound->source );
		alDeleteBuffers( 1, &sound->buffer );
		return 0;
	}
    byte += 2;
    
	/* Get the channel count */
//	fread( buffer, 1, 2, file );
	channels = buffer[byte+1]<<8;
	channels |= buffer[byte];
    byte += 2;
    
	/* Get the sample frequency */
//	fread( buf, 1, 4, file );
	frequency  = buffer[byte+3]<<24;
    frequency |= buffer[byte+2]<<16;
    frequency |= buffer[byte+1]<<8;
    frequency |= buffer[byte+0];
    byte += 4;
    
	/* Ignore block size and bytes per second */
//	fread( buf, 1, 6, file );
    byte += 6;
    
	/* Get the bit depth */
//	fread( buf, 1, 2, file );
	bits = buffer[byte+1]<<8;
	bits |= buffer[byte];
    byte += 2;
    
	if( bits == 8 )
	{
		if( channels == 1 )
			format = AL_FORMAT_MONO8;
		else if( channels == 2 )
			format = AL_FORMAT_STEREO8;
	}
	else if( bits == 16 )
	{
		if( channels == 1 )
			format = AL_FORMAT_MONO16;
		else if( channels == 2 )
			format = AL_FORMAT_STEREO16;
	}
    
	/* Read the data chunk which will hold the decoded sample data */
//	fread( buffer, 1, 8, file );
	if( buffer[byte] != 'd' || buffer[byte+1] != 'a' || buffer[byte+2] != 't' || buffer[byte+3] != 'a' )
	{
		/* Some .wav files have an extra 2 bytes before the data chunk */
		/* TODO: What are they? */
		if( buffer[byte+2] == 'd' || buffer[byte+3] == 'a' )
		{
		//	byte += 2;
		}
		else
		{
			alDeleteSources( 1, &sound->source );
			alDeleteBuffers( 1, &sound->buffer );
			return 0;
		}
	}
    byte += 8;
    
    /* Get the size of the data chunk (should be from here till the end of the file) */
    data_size = size - byte;
    
    /* Copy the data chunk */
//    data = malloc( data_size );
//    memcpy( data, &buffer[byte], data_size );

	// Load wav file from memory
//	alBufferData( sound->buffer, format, data, data_size, frequency );
    alBufferData( sound->buffer, format, &buffer[byte], data_size, frequency );

	error = get_al_error();
    if( error )
		return 0;
//    free(data);
    
	memset( &sound->position, 0, sizeof( struct Vector3 ) );
	memset( &sound->velocity, 0, sizeof( struct Vector3 ) );

	// Set the sound source's default attributes
	alSourcei( sound->source, AL_BUFFER, sound->buffer );
	alSourcef( sound->source, AL_PITCH,  1.0f );
	alSourcef( sound->source, AL_GAIN,	 1.0f );
	alSourcefv( sound->source, AL_POSITION, (float*) &sound->position );
	alSourcefv( sound->source, AL_VELOCITY, (float*) &sound->velocity );
	alSourcei( sound->source, AL_LOOPING, 0 );
    
    sound->volume = 1.0f;
    sound->pitch = 1.0f;

	if( get_al_error() )
		return 0;

	return 1;
}

void endian_swap( unsigned int* v )
{
	*v = (*v>>24) | 
        ((*v<<8) & 0x00FF0000) |
        ((*v>>8) & 0x0000FF00) |
        (*v<<24);
}

void endian_swap16( unsigned short* v )
{
	*v = (*v&0xFF) | ((*v>>8)&0xFF);
}


int create_sound_au( char* filename, struct sound_t* sound )
{
	struct au_header_t au;
	struct _iobuf* fp;
	void* buffer = NULL;
	ALenum format = 0;
	ALuint frequency;
	int i = 0;

	/* Sanity check */
	if( !filename || !sound )
		return 0;

	/* Open the .au file */
	fp = fopen( filename, "rb" );
	if( !fp )
		return 0;

	/* Read in the header */
	fread( &au, 1, sizeof( struct au_header_t ), fp );

	/* Convert each field from big endian to little endian */
	endian_swap(&au.magic_number);
	endian_swap(&au.sample_rate);
	endian_swap(&au.channels);
	endian_swap(&au.data_offset);
	endian_swap(&au.data_size);
	endian_swap(&au.encoding);

	/* More sanity checks */
	/* Check for a valid .au header */
	if( au.magic_number != 0x2E736E64 )
	{
		/* Either this is not a real .au file, or it's a REALLY OLD
		   version of it. I don't plan to support the latter */
		fclose(fp);
		return 0;
	}

	/* Check the data size */
	if( au.data_size == 0xFFFFFFFF )
	{
		/* Data size unknown */
		fclose(fp);
		return 0;
	}

	/* Only 8-bit and 16-bit PCM data is supported right now */
	if( au.encoding != 2 && au.encoding != 3 )
	{
		fclose(fp);
		return 0;
	}

	/* Now we can get the file attributes and what not, then load the
	   actual sound byte. */
	frequency = au.sample_rate;

	if( au.encoding == 2 )
	{
		if( au.channels == 1 )
			format = AL_FORMAT_MONO8;
		else if( au.channels == 2 )
			format = AL_FORMAT_STEREO8;
	}
	else if( au.encoding == 3 )
	{
		if( au.channels == 1 )
			format = AL_FORMAT_MONO16;
		else if( au.channels == 2 )
			format = AL_FORMAT_STEREO16;
	}

	if( format == 0 )
	{
		fclose(fp);
		return 0;
	}

	/* Allocate enough memory to read in the sound data */
	buffer = malloc( au.data_size );

	/* Read in the sound buffer data */
	fseek( fp, au.data_offset, SEEK_SET );
	fread( buffer, 1, au.data_size, fp );

	/* Close the file pointer */
	fclose(fp);

	/* Swap the endianness on the entire buffer */
	/*while( i < (au.data_size/4) )
	{
		endian_swap( &((ALuint*)buffer)[i] );
		i++;
	}*/

	while( i < (au.data_size/2) )
	{
		endian_swap16( &((unsigned short*)buffer)[i] );
		i++;
	}

	/* Send the buffer data into an OpenAL buffer. */
	/* But before we can do that, we need to create the required OpenAL buffers */
	/* and sources */
	alGenBuffers( 1, &sound->buffer );

	if( get_al_error() )
	{
		free(buffer);
		return 0;
	}

	/* Load wav file from memory */
	alBufferData( sound->buffer, format, buffer, au.data_size, frequency );
	free(buffer);

	/* Create a new sound source */
	alGenSources( 1, &sound->source );

	if( get_al_error() )
	{
		alDeleteBuffers( 1, &sound->buffer );
		return 0;
	}

	/* Set the sound source's default attributes */
	alSourcei( sound->source, AL_BUFFER, sound->buffer );
	alSourcef( sound->source, AL_PITCH,  1.0f );
	alSourcef( sound->source, AL_GAIN,	 1.0f );
	alSourcefv( sound->source, AL_POSITION, (float*) &sound->position );
	alSourcefv( sound->source, AL_VELOCITY, (float*) &sound->velocity );
	alSourcei( sound->source, AL_LOOPING, 0 );

	if( get_al_error() )
	{
		alDeleteBuffers( 1, &sound->buffer );
		alDeleteSources( 1, &sound->source );
		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Name: delete_sound
// Desc: Delete's a sound from OpenAL
//-----------------------------------------------------------------------------
void delete_sound( struct sound_t* sound )
{
	alDeleteSources( 1, &sound->source );
	alDeleteBuffers( 1, &sound->buffer );
}

//-----------------------------------------------------------------------------
// Name: play_sound_effect
// Desc: Plays a sound effect at it's given position. Can be played while looping
//		 as well.
//-----------------------------------------------------------------------------
void play_sound_effect( struct sound_t* sound, int looping )
{
	alSourcei( sound->source, AL_LOOPING, looping );
	alSourcePlay( sound->source );
}

//-----------------------------------------------------------------------------
// Name: play_sound_effect_static
// Desc: Plays a sound effect that is not in 3D space. Can be played while looping
//		 as well.
//-----------------------------------------------------------------------------
void play_sound_effect_static( struct sound_t* sound, int looping )
{
	struct Vector3 pos = { 0.0f, 0.0f, 0.0f };
	struct Vector3 vel = { 0.0f, 0.0f, 0.0f };
	struct Vector3 at = { 0.0f, 0.0f, -1.0f };
	struct Vector3 up = { 0.0f, 1.0f, 0.0f };

	float orientation[6] = { at.x, at.y, at.z, up.z, up.y, up.z };
	
	alListenerfv( AL_VELOCITY, (float*) &vel );
	alListenerfv( AL_POSITION, (float*) &pos );
	alListenerfv( AL_ORIENTATION, orientation );

	alSourcei( sound->source, AL_LOOPING, looping );
	alSourcePlay( sound->source );

	set_listener_position( &listener_position );
	set_listener_velocity( &listener_velocity );
	set_listener_orientation( &listener_at, &listener_up );
}

//-----------------------------------------------------------------------------
// Name: stop_sound_effect
// Desc: Stops a sound effect from playing.
//-----------------------------------------------------------------------------
void stop_sound_effect( struct sound_t* sound )
{
	alSourceStop( sound->source );
}

//-----------------------------------------------------------------------------
// Name: sound_is_playing
// Desc: Determines if the sound effect is playing
//-----------------------------------------------------------------------------
int sound_is_playing( struct sound_t* sound )
{
	int is_playing;

	alGetSourcei( sound->source, AL_SOURCE_STATE, &is_playing );

	if( is_playing != AL_PLAYING )
		return 0;

	return 1;
}

//-----------------------------------------------------------------------------
// Name: pause_sound_effect
// Desc: Pauses a sound effect.
//-----------------------------------------------------------------------------
void pause_sound_effect( struct sound_t* sound )
{
	alSourcePause( sound->source );
}

//-----------------------------------------------------------------------------
// Name: set_sound_effect_frequency
// Desc: Sets the frequency of this sound buffer.
//-----------------------------------------------------------------------------
void set_sound_effect_pitch( struct sound_t* sound, float pitch )
{
	alSourcef( sound->source, AL_PITCH, pitch );
}

//-----------------------------------------------------------------------------
// Name: set_sound_effect_position
// Desc: Sets the position of this sound effect in 3D space.
//-----------------------------------------------------------------------------
void set_sound_effect_position( struct sound_t* sound, struct Vector3* position )
{
	alSourcefv( sound->source, AL_POSITION, (float*) position );
}

//-----------------------------------------------------------------------------
// Name: set_sound_effect_velocity
// Desc: Sets the velocity of this sound effect.
//-----------------------------------------------------------------------------
void set_sound_effect_velocity( struct sound_t* sound, struct Vector3* velocity )
{
	alSourcefv( sound->source, AL_VELOCITY, (float*) velocity );
}

//-----------------------------------------------------------------------------
// Name: set_sound_effect_volume
// Desc: Sets the volume of this sound effect.
//-----------------------------------------------------------------------------
void set_sound_effect_volume( struct sound_t* sound, float gain )
{
	sound->volume = gain;
	alSourcef( sound->source, AL_GAIN, gain );
}

//-----------------------------------------------------------------------------
// Name: set_listener_position
// Desc: 
//-----------------------------------------------------------------------------
void set_listener_position( struct Vector3* position )
{
	alListenerfv( AL_POSITION, (float*) &position );
	listener_position = *position;
}

//-----------------------------------------------------------------------------
// Name: set_listener_velocity
// Desc: 
//-----------------------------------------------------------------------------
void set_listener_velocity( struct Vector3* velocity )
{
	alListenerfv( AL_POSITION, (float*) &velocity );
	listener_velocity = *velocity;
}

//-----------------------------------------------------------------------------
// Name: set_listener_orientation
// Desc: 
//-----------------------------------------------------------------------------
void set_listener_orientation( struct Vector3* at, struct Vector3* up )
{
	float orientation[6] = { at->x, at->y, at->z, up->z, up->y, up->z };
	
	alListenerfv( AL_ORIENTATION, orientation );
	listener_at = *at;
	listener_up = *up;
}
