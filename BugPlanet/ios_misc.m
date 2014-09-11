#import <UIKit/UIKit.h>
#import "ios_misc.h"

#include <mach/mach.h>
#include <mach/mach_time.h>


FILE* get_file_ptr( NSString* string, char* c )
{
    /* Should be a valid path */
    
    FILE* fp = fopen( [string cStringUsingEncoding:1], c );
    
    if( fp == NULL )
        NSLog( [string stringByAppendingString:@" was not found!"] );
    
    return fp;
}

FILE* get_file_ptr_c( char* fname, char* ext, char* c )
{
    /* Convert C-strings to NSStrings */
    NSString* _fname = [[NSString alloc] initWithUTF8String:fname];
    NSString* _ext = [[NSString alloc] initWithUTF8String:ext];
    
    /* Get actual path from resource */
    NSString* path = [[NSBundle mainBundle] pathForResource:_fname ofType:_ext];
    
    FILE* fp = fopen( [path cStringUsingEncoding:1], c );
    if( fp == NULL )
        NSLog( [path stringByAppendingString:@" was not found!"] );
    
    return fp;
}

const char* get_file_path( char* fname )
{
    int len = strlen( fname );
    char temp[64];
    char ext[8];
//    char ret[64];
    int i = 0, j = 0;
    
    /* Clean out this temporary buffer first */
    memset( temp, 0, 64 );
    memset( ext, 0, 8 );
    
    /* Get the file name without extension */
    while( fname[i] != '.' )
    {
        temp[i] = fname[i];
        i++;
        
        if( i >= len )
            return NULL;
    }
    
    /* Get the extension */
    i++;
    while( fname[i] != 0 )
    {
        ext[j++] = fname[i];
        i++;
    }
    
    /* Convert C-strings to NSStrings */
    NSString* _fname = [[NSString alloc] initWithUTF8String:temp];
    NSString* _ext = [[NSString alloc] initWithUTF8String:ext];
    
    /* Get actual path from resource */
    NSString* path = [[NSBundle mainBundle] pathForResource:_fname ofType:_ext];
    
//    strcpy( ret, [path cStringUsingEncoding:1] ); // <- Causes SIGABRT!
    
//    return ret;
    return [path cStringUsingEncoding:1];
}

const char* get_documents_path(void)
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    
    return [documentsDirectory cStringUsingEncoding:1];
}

FILE* get_file_ptr_from_doc_dir_c( char* fname, char* c )
{
    /* Convert C-strings to NSStrings */
    NSString* _fname = [[NSString alloc] initWithUTF8String:fname];
    
    /* Get the path to the documents folder */
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    NSString* filename = [documentsDirectory stringByAppendingPathComponent:_fname];
    
    FILE* fp = fopen( [filename cStringUsingEncoding:1], c );
    if( fp == NULL )
        NSLog( [filename stringByAppendingString:@" was not found!"] );
    
    return fp; /* TODO */
}

/* http://stackoverflow.com/questions/741830/getting-the-time-elapsed-objective-c */

uint64_t getTickCount(void)
{
    static mach_timebase_info_data_t sTimebaseInfo;
    uint64_t machTime = mach_absolute_time();
    
    // Convert to nanoseconds - if this is the first time we've run, get the timebase.
    if (sTimebaseInfo.denom == 0 )
    {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    
    // Convert the mach time to milliseconds
    uint64_t millis = ((machTime / 1000000) * sTimebaseInfo.numer) / sTimebaseInfo.denom;
    return millis;
}

void get_dimensions( int* width, int* height )
{
    CGRect rect = [[UIScreen mainScreen] bounds];
    
#ifdef __IPHONE_3_0
    *height = rect.size.width;
    *width = rect.size.height;
#else
    *width = rect.size.width;
    *height = rect.size.height;
#endif
}