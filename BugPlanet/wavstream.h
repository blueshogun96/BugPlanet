#pragma once


#define NUM_BUFFERS	3
#define BUFFER_SIZE	4096	// 4kb

/* Wave stream structure */
struct wave_stream
{
	ALuint source;
	ALuint buffers[NUM_BUFFERS];
	ALuint frequency;
	ALenum format;
	ALfloat volume;
	ALuint wave_format;
	int channels;
	int bits;
	ALboolean is_playing;
	unsigned char* buf;
	unsigned char* buf2;
	FILE* file;
	void* thread;
};


/* Wavestream API functions */
int		wavestream_open( char* filename, struct wave_stream* w );
void	wavestream_close( struct wave_stream* w );
void	wavestream_play( struct wave_stream* w );
void	wavestream_stop( struct wave_stream* w );
void    wavestream_pause( struct wave_stream* w );
void    wavestream_reset( struct wave_stream* w );
void	wavestream_set_volume( struct wave_stream* w, float gain );
int		wavestream_update( struct wave_stream* w );