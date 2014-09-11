#include "platform.h"
#include "aldrv.h"
#include "wavstream.h"
//#include "adpcm.h"


//struct adpcm_state adpcm;

/* Opens a .wav file for streaming */
int		wavestream_open( char* filename, struct wave_stream* w )
{
	int ret;

	/* Sanity check */
	if(!w)
		return 0;

	/* Set this wavestream to not playing */
	w->is_playing = AL_FALSE;

	/* Does the wav file exist? */
	w->file = fopen( filename, "rb" );
	if( !w->file )
		return 0;

	/* Create buffers and sources */
	alGenBuffers( NUM_BUFFERS, w->buffers );
	alGenSources( 1, &w->source );

	/* Allocate this buffer to read bytes from as we go. */
	/* Should check for the strings "RIFF" and "WAVE" in the header, but I don't feel like
	   it at the moment, so... */
	w->buf = malloc( BUFFER_SIZE );
	fread( w->buf, 1, 12, w->file );

	/* Nullify secondary buffer just in case */
	w->buf2 = NULL;

	/* Check the wav file chunk header for the format information. If it's not there, forget it */
	fread( w->buf, 1, 8, w->file );
	if( w->buf[0] != 'f' || w->buf[1] != 'm' || w->buf[2] != 't' || w->buf[3] != ' ' )
	{
		fclose(w->file);
		alDeleteSources( 1, &w->source );
		alDeleteBuffers( NUM_BUFFERS, w->buffers );
		return 0;
	}

	/* Read the wav format type. 1 == PCM.  If it's not, then we either
	   exit, or have to decode the format ourselves */
	fread( w->buf, 1, 2, w->file );
	w->wave_format = w->buf[1]<<8;
	w->wave_format |= w->buf[0];

	if( w->wave_format != 0x0001 && w->wave_format != 0x0011 )
	{
		fclose(w->file);
		alDeleteSources( 1, &w->source );
		alDeleteBuffers( NUM_BUFFERS, w->buffers );
		return 0;
	}

	/* If this is a Intel ADPCM encoded wave file, allocate a secondary
	   buffer to decode data into. */
	/*if( w->wave_format == 0x0011 )
	{
		w->buf2 = malloc( BUFFER_SIZE * 8 );
		memset( &adpcm, 0, sizeof( struct adpcm_state ) );
	}*/
    
	/* Get the channel count */
	fread( w->buf, 1, 2, w->file );
	w->channels = w->buf[1]<<8;
	w->channels |= w->buf[0];

	/* Get the sample frequency */
	fread( w->buf, 1, 4, w->file );
	w->frequency  = w->buf[3]<<24;
    w->frequency |= w->buf[2]<<16;
    w->frequency |= w->buf[1]<<8;
    w->frequency |= w->buf[0];

	/* Ignore block size and bytes per second */
	fread( w->buf, 1, 6, w->file );

	/* Get the bit depth */
	fread( w->buf, 1, 2, w->file );
	w->bits = w->buf[1]<<8;
	w->bits |= w->buf[0];

	if( w->bits == 8 )
	{
		if( w->channels == 1 )
			w->format = AL_FORMAT_MONO8;
		else if( w->channels == 2 )
			w->format = AL_FORMAT_STEREO8;
	}
	else if( w->bits == 16 || w->bits == 4 ) /* Convert 4->16 bits */
	{
		if( w->channels == 1 )
			w->format = AL_FORMAT_MONO16;
		else if( w->channels == 2 )
			w->format = AL_FORMAT_STEREO16;
	}

	/* Read the data chunk which will hold the decoded sample data */
	fread( w->buf, 1, 8, w->file );
	/*if( w->buf[0] != 'd' || w->buf[1] != 'a' || w->buf[2] != 't' || w->buf[3] != 'a' )
	{
		BYTE b1 = w->buf[0];
		BYTE b2 = w->buf[1];
		BYTE b3 = w->buf[2];
		BYTE b4 = w->buf[3];
		BYTE b5 = w->buf[4];
		BYTE b6 = w->buf[5];
		BYTE b7 = w->buf[6];
		BYTE b8 = w->buf[7];

		fclose(w->file);
		alDeleteSources( 1, &w->source );
		alDeleteBuffers( NUM_BUFFERS, w->buffers );
		return 0;
	}*/

	/* Fill the buffer with the amount of bytes per buffer for OpenAL to use */
	ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
    alBufferData( w->buffers[0], w->format, w->buf, ret, w->frequency );
    ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
    alBufferData( w->buffers[1], w->format, w->buf, ret, w->frequency );
    ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
    alBufferData( w->buffers[2], w->format, w->buf, ret, w->frequency );


	if( alGetError() != AL_NO_ERROR )
	{
		fclose(w->file);
		alDeleteSources( 1, &w->source );
		alDeleteBuffers( NUM_BUFFERS, w->buffers );
		return 0;
	}

	w->volume = 1.0f;

	/* Queue the buffers onto the source and start playback */

	return 1;
}

void	wavestream_close( struct wave_stream* w )
{
	ALint val = 0;

    /* Sanity check */
    if( !w )
        return;
    
	/* Free any used data and wait until the buffers are 
	   finished before deleting */
	if( w->file )
	{
		fclose( w->file );
		w->file = NULL;
	}

	if( w->buf2 )
	{
		free( w->buf2 );
		w->buf2 = NULL;
	}
	if( w->buf )
	{
		free( w->buf );
		w->buf = NULL;
	}
	
    /* Occasionally hangs on iOS, but not on other platforms... */
	/*do
	{
		alGetSourcei( w->source, AL_SOURCE_STATE, &val );
	} while( val == AL_PLAYING );*/

	alDeleteSources( 1, &w->source );
	alDeleteBuffers( NUM_BUFFERS, w->buffers );
}

void	wavestream_play( struct wave_stream* w )
{
	/* is this stream already playing? */
	if( w->is_playing )
		return;

	w->is_playing = AL_TRUE;

	/* Foamy the squirrel voice: PLAAAAAAAAAY! */
	wavestream_set_volume( w, w->volume );
	alSourceQueueBuffers( w->source, NUM_BUFFERS, w->buffers );
	alSourcePlay( w->source );
	if( alGetError() != AL_NO_ERROR )
		return;
}

int		wavestream_update( struct wave_stream* w )
{
	ALuint buffer;
	ALint val = 0;
	ALint ret;

	/* Don't update if the wavestream isn't open */
	if( !w->file )
		return 0;

	/* Don't call this if the stream isn't playing */
	if( !w->is_playing )
		return 0;

	/* Start playing this wav file.  Returns 0 when the .wav file is done playing */
	if( feof(w->file) )
	{
		char dummy[45];

		/* Reset the wav stream */
//		wavestream_stop(w);
		fseek( w->file, 0, SEEK_SET );
		fread( dummy, 1, 44, w->file );

#if 0
		alSourceUnqueueBuffers( w->source, 3, w->buffers );

		/* Fill the buffer with the amount of bytes per buffer for OpenAL to use */
		ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
		alBufferData( w->buffers[0], w->format, w->buf, ret, w->frequency );
		ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
		alBufferData( w->buffers[1], w->format, w->buf, ret, w->frequency );
		ret = fread( w->buf, 1, BUFFER_SIZE, w->file );
		alBufferData( w->buffers[2], w->format, w->buf, ret, w->frequency );
		if( alGetError() != AL_NO_ERROR )
		{
			fclose(w->file);
			alDeleteSources( 1, &w->source );
			alDeleteBuffers( NUM_BUFFERS, w->buffers );
			return 0;
		}

		w->is_playing = Yes;
#endif
	}
	//	return 0;

	/* Check if OpenAL is done with any of the queued buffers */
	alGetSourcei( w->source, AL_BUFFERS_PROCESSED, &val );
	if( val <= 0 )
		return 1;

	/* For each processed buffer... */
	while( val-- )
	{
		/* Red the next chunk of decoded data from the stream */
        ret = fread( w->buf, 1, BUFFER_SIZE, w->file );

        /* Pop the oldest queued buffer from the source, fill it with
            new data, then requeue it */
        alSourceUnqueueBuffers( w->source, 1, &buffer );
        alBufferData( buffer, w->format, w->buf, ret, w->frequency );
        alSourceQueueBuffers( w->source, 1, &buffer );
        if( alGetError() != AL_NO_ERROR )
        {
            /* TODO */
        }
	}

	/* Make sure that the source is still playing and restart it if necessary */
	alGetSourcei( w->source, AL_SOURCE_STATE, &val );
	if( val != AL_PLAYING )
		alSourcePlay( w->source );

	return 1;
}

void	wavestream_stop( struct wave_stream* w )
{
	ALint val = 0;

	/* Get buffer status */
	alGetSourcei( w->source, AL_SOURCE_STATE, &val );

	if( val == AL_PLAYING )
	{
		alSourceStop( w->source );
		w->is_playing = No;
	}
}

void	wavestream_resume( struct wave_stream* w )
{
	ALint val = 0;

	/* Get buffer status */
	alGetSourcei( w->source, AL_SOURCE_STATE, &val );

	if( val != AL_PLAYING )
	{
		alSourcePlay( w->source );
		w->is_playing = Yes;
	}
}

void	wavestream_pause( struct wave_stream* w )
{
    ALint val = 0;
    
	/* Get buffer status */
	alGetSourcei( w->source, AL_SOURCE_STATE, &val );
    
	if( val == AL_PLAYING )
	{
		alSourcePause( w->source );
		w->is_playing = No;
	}
}

void    wavestream_reset( struct wave_stream* w )
{
    char dummy[45];
    
    /* Reset the wav stream */
    fseek( w->file, 0, SEEK_SET );
    fread( dummy, 1, 44, w->file );
}

void	wavestream_set_volume( struct wave_stream* w, float gain )
{
	/* Set the wavestream's volume via AL_GAIN */
	/* 1.0 = max, 0.0 = min */

	w->volume = gain;
	alSourcef( w->source, AL_GAIN, gain );
}