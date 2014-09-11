#include "platform.h"
#include "aldrv.h"
#include "wavstatic.h"



ALuint wavstatic_open( char* filename )
{
	FILE* file = NULL;
	char* buffer = NULL;
	ALuint channels;
	ALuint frequency;
	ALuint format;
	ALuint bits;
	ALuint albuffer;
	long filesize = 0;
	int ret;

	/* Does the wav file exist? */
	file = fopen( filename, "rb" );
	if( !file )
		return 0;

	/* Get the file size */
	fseek( file, 0, SEEK_END );
	filesize = ftell(file);
	fseek( file, 0, SEEK_SET );

	/* Create buffers and sources */
	alGenBuffers( 1, &albuffer );
	if( alGetError() != AL_NO_ERROR )
	{
		fclose(file);
		free(buffer);
		alDeleteBuffers( 1, &albuffer );
		return 0;
	}

	/* Allocate this buffer to read bytes from as we go. */
	/* Should check for the strings "RIFF" and "WAVE" in the header, but I don't feel like
	   it at the moment, so... */
	buffer = malloc( filesize );
	fread( buffer, 1, 12, file );

	/* Check the wav file chunk header for the format information. If it's not there, forget it */
	fread( buffer, 1, 8, file );
	if( buffer[0] != 'f' || buffer[1] != 'm' || buffer[2] != 't' || buffer[3] != ' ' )
	{
		fclose(file);
		free(buffer);
		alDeleteBuffers( 1, &albuffer );
		return 0;
	}

	/* Read the wav format type. 1 == PCM.  If it's not, then we either
	   exit, or have to decode the format ourselves */
	fread( buffer, 1, 2, file );
	if( buffer[1] != 0 || buffer[0] != 1 )
	{
		fclose(file);
		free(buffer);
		alDeleteBuffers( 1, &albuffer );
		return 0;
	}

	/* Get the channel count */
	fread( buffer, 1, 2, file );
	channels = buffer[1]<<8;
	channels |= buffer[0];

	/* Get the sample frequency */
	fread( buffer, 1, 4, file );
	frequency  = buffer[3]<<24;
    frequency |= buffer[2]<<16;
    frequency |= buffer[1]<<8;
    frequency |= buffer[0];

	/* Ignore block size and bytes per second */
	fread( buffer, 1, 6, file );

	/* Get the bit depth */
	fread( buffer, 1, 2, file );
	bits = buffer[1]<<8;
	bits |= buffer[0];

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
	fread( buffer, 1, 8, file );
	if( buffer[0] != 'd' || buffer[1] != 'a' || buffer[2] != 't' || buffer[3] != 'a' )
	{
		fclose(file);
		free(buffer);
		alDeleteBuffers( 1, &albuffer );
		return 0;
	}

	/* Now read in the data section of this wav file */
	ret = fread( buffer, 1, filesize, file );
	alBufferData( albuffer, format, buffer, ret, frequency );
	if( alGetError() != AL_NO_ERROR )
	{
		fclose(file);
		free(buffer);
		alDeleteBuffers( 1, &albuffer );
		return 0;
	}

	/* That's all */
	fclose(file);
	free(buffer);

	return albuffer;
}