#pragma once

/*
 * Misc functions used to help the iOS specific stuff
 * interface with C code.
 */

#ifdef __OBJC__
FILE* get_file_ptr( NSString* string, char* c ); /* Obj-C only */
#endif

FILE* get_file_ptr_c( char* fname, char* ext, char* c );
const char* get_file_path( char* fname );
uint64_t getTickCount(void);
const char* get_documents_path(void);
FILE* get_file_ptr_from_doc_dir_c( char* fname, char* c );
void get_dimensions( int* width, int* height );