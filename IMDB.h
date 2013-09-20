#ifndef IMDB_H
#define IMDB_H

/*=============================================================================
 *
 *  Program:   IMDB.h
 *
 *  Version:   1.0 (02.11.95) 
 *
 *  Purpose:   serves as a link between the GUI and Database functions
 *             to enable easy portation of the InternetMovieDatabase
 *             on different platforms, such as (Amiga, OS/2, X-Windows)
 *
 *  Author:    Andre Bernhardt <ab@imdb.com>
 *
 *  Copyright: (w) Andre Bernhardt 1995 - 2001
 *             (c) Internet MovieDatabase Limited 2001
 *
 *       This file is part of the Internet MovieDatabase project.
 *
 *  The  MovieDatabase  FAQ contains more information on the whole project.
 *  For   a   copy   send  an  e-mail   with  the  subject  "HELP  FAQ"  to
 *  <mail-server@imdb.com>.
 *
 *  Permission  is  granted  to make and distribute verbatim copies of this
 *  package  provided  the  copyright notice and this permission notice are
 *  preserved  on  all  copies  and the package is distributed in unaltered
 *  archive  form only.  It is not allowed to modify the source code and/or
 *  redistribute  modified  copies  of  it  and/or  the executables without
 *  written permission of the author.
 *
 *  If  you need to make a change to the source-code in order to be able to
 *  use the package, you have to notify the author.
 *
 *  No guarantee of any kind is given that the programs and scripts in this
 *  package  are  100%  reliable.  You are using this material at your  own
 *  risk.   The  author  cannot be made responsible for any damage which is
 *  caused by using these programs.
 *
 *  This  package  is  freely  distributable,  but still copyright by  IMDb
 *  Ltd.
 *
 *  None  of  the programs or scripts nor the source code (nor parts of it)
 *  may  be  included  or  used  in  commercial  programs unless by written
 *  permission from the author.
 *
 *=============================================================================
 *
 *  History: (for a detailed history see separate history file)
 *  0.1  11.06.95 first steps...
 *
 *=============================================================================
 *
 *  Compiling the code on other platforms:
 *
 *  The library- and tools-functions do not use system-specific functions/
 *  includes. However if you have any difficulties compiling the package
 *  please contact me.
 *
 *
 *=============================================================================
 * General Header File Information
 *-----------------------------------------------------------------------------
 *
 * All macro and structure definitions follow these rules:
 *
 * Name                       Meaning
 * 
 * IMDBFunctionName           this is a function or procedure 
 * 
 * IMDB_DEFINE                a predefined constant or whatever
 * 
 * IMDB_Type_Definition       structs & typedefs
 * 
 * imdb                       variable
 * 
 * imdb_macro                 a convenience macro-definition for access to
 *                            elements of 'imdb'-variable
 * 
 * 
 * You may only use those objects that are defined in this include file.
 * 
 *=============================================================================
 */

/*-----------------------------------------------------------------------------
 * Some includes that are used everywhere
 *-----------------------------------------------------------------------------
 */

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*-----------------------------------------------------------------------------
 * Some typedefs & defines to make our life easier
 *-----------------------------------------------------------------------------
 */

#ifdef SYS_AMIGA

 #include <exec/types.h>

#else

 typedef           long  LONG;      /* signed 32-bit quantity */
 typedef unsigned  long  ULONG;     /* unsigned 32-bit quantity */
 typedef           short WORD;      /* signed 16-bit quantity */
 typedef unsigned  short UWORD;     /* unsigned 16-bit quantity */
 #if __STDC__
 typedef signed    char  BYTE;      /* signed 8-bit quantity */
 #else
 typedef           char  BYTE;      /* signed 8-bit quantity */
 #endif
 typedef unsigned  char  UBYTE;     /* unsigned 8-bit quantity */
 typedef           short BOOL;
 typedef           void *APTR;      /* 32-bit untyped pointer */

 #ifndef FALSE
 #define FALSE     0
 #define TRUE      1
 #endif

 #ifndef NULL
 #define NULL      0L
 #endif

#endif

#define IMDB_IO_ERROR     -1        /* General IO-Error */

/*-----------------------------------------------------------------------------
 * Defines for Error-Handling
 *-----------------------------------------------------------------------------
 */

#define IMDB_PENALTY_OK              0 /* No error */
#define IMDB_PENALTY_HARMLESS        5 /* A harmless error that may be ignored */
#define IMDB_PENALTY_SERIOUS        10 /* A more serious error that should be reported to the user */
#define IMDB_PENALTY_REPORT         20 /* This Error is probably caused by a bug in the code. */
#define IMDB_PENALTY_CANCEL         40 /* No error, but the user has interrupted the command */

#define IMDB_FOUND                   0
#define IMDB_NO_ERROR                0
#define IMDB_NOTFOUND                1
#define IMDBE_NO_ERROR               0
#define IMDBE_NOTFOUND               1
#define IMDBE_NOT_UNIQUE             2 /* The entry is not specific enough to find a unique matching entry */
#define IMDBE_INTERRUPT              3 /* Function has been interrupted */

/* I-O-Errors */
#define IMDBE_FILE_OPEN             80
#define IMDBE_FILE_POSITION         81
#define IMDBE_FILE_READ             82
#define IMDBE_FILE_WRITE            83
#define IMDBE_FILE_APPEND           84
#define IMDBE_FILE_EXIST            85 /* File does not exist */
#define IMDBE_FILE_EOF              86 /* End of file */

/* System-Errors */
#define IMDBE_MEMORY                100

/*-----------------------------------------------------------------------------
 * This struct contains information on an error that has occured
 *-----------------------------------------------------------------------------
 */

typedef struct
 {
  LONG  status;                  /* Error-penalty, 0 == No error */
  LONG  routine;                 /* Where did it happen? (Routine-Code, Line-Number) */
  LONG  error;                   /* Error-number */
  char *text;                    /* Additional Text, e.g. Filename or NULL */
 } IMDB_Error;

/*-----------------------------------------------------------------------------
 * Functions for Error-handling (These functions are part of the library)
 *-----------------------------------------------------------------------------
 */

#ifndef IMDB_RESOURCES_C

/* Procedure:  IMDBSetError
 * Purpose:    Set the IMDB_Error-struct
 * Comment:    This function will allocate Memory for Error-Text and copy
 *             the textstring. This memory will be freed when calling 
 *             IMDBReSetError, so please make sure to call this function after 
 *             error-handling
 * Parameters: pointer to IMDB_Error
 *             penalty code
 *             routine 
 *             error
 *             text
 * Returns:    nothing
 */
extern void IMDBSetError (IMDB_Error *error, LONG penalty, LONG routine, LONG number, char *text);

/* Procedure:  IMDBResetError
 * Purpose:    Reset the IMDB_Error-struct
 * Comment:    This function will free memory allocated by IMDBSetError
 * Parameters: pointer to IMDB_Error
 * Returns:    nothing
 */
extern void IMDBResetError (IMDB_Error *error);

#endif



/*-----------------------------------------------------------------------------
 * General Information on Memory Management
 *-----------------------------------------------------------------------------
 * 
 * Memory is supposed to be allocated via these functions. 
 * These functions will be enhanced in the future by automatically using 
 * memory pools to prevent fragmentation of memory
 *-----------------------------------------------------------------------------
 */

#ifndef IMDB_RESOURCES_C

/* Procedure:  IMDBAllocMemory
 * Purpose:    Allocate Memory
 * Comment:    -
 * Parameters: size
 * Returns:    Pointer to memory or NULL
 */
extern APTR IMDBAllocMemory (LONG size);

/* Procedure:  IMDBFreeMemory
 * Purpose:    Free Memory
 * Comment:    -
 * Parameters: pointer to memory
 * Returns:    nothing
 */
extern void IMDBFreeMemory  (APTR mem);

#endif


/*-----------------------------------------------------------------------------
 * General Information on Files
 *-----------------------------------------------------------------------------
 * 
 *-----------------------------------------------------------------------------
 */

/* mode */
#define IMDBV_FILE_READ        0       /* Open file to read */
#define IMDBV_FILE_WRITE       1       /* Open file to write */
#define IMDBV_FILE_APPEND      2       /* Open file to append more data */
                                       /* max 3 */

#define IMDBV_FILE_GETSIZE     (1<<4)  /* Get size of File */

/*-----------------------------------------------------------------------------
 * Functions for Filehandling (These functions are part of the library)
 *-----------------------------------------------------------------------------
 */

#ifndef IMDB_RESOURCES_C

/* Procedure:  IMDBExistFile
 * Purpose:    checks for existance
 * Comment:
 * Parameters: fname    filename
 * Returns:    TRUE, if file does exist
 */
extern BOOL IMDBExistFile (char *fname);

#endif

/*-----------------------------------------------------------------------------
 * General Information on File-Buffering
 *-----------------------------------------------------------------------------
 * 
 * These functions will provide a maximum access-speed by allocating memory 
 * for file buffering.
 *  
 *-----------------------------------------------------------------------------
 */

/* special values */
#define IMDBV_SIZE_FILE       -1        /* use size of file */
#define IMDBV_SIZE_BUFFER     -2        /* use size of buffer */
#define IMDBV_SIZE_DEFAULT     1024     /* default size */

/*-----------------------------------------------------------------------------
 * This struct contains any information on a buffered file.
 * Do not access directly!
 *-----------------------------------------------------------------------------
 */

typedef struct
 {
  IMDB_Error  error;             /* Error-Status */
  LONG  filesize;                /* total size of file in bytes */
  LONG  filepos;                 /* actual position in file */
  char *fname;                   /* filename */
  LONG  mode;                    /* mode: read, write */
  FILE *stream;                  /* file-stream */
  LONG  buffersize;              /* actual size of buffer in bytes */
  LONG  bufferpos;               /* actual position in buffer */
  LONG  nb_bytes_in_buffer;      /* Number of bytes in buffer */
  char *buffer;                  /* buffer  */
 } IMDB_Buffer;

/*-----------------------------------------------------------------------------
 * Macros (read only)
 * use these to access file buffer
 *-----------------------------------------------------------------------------
 */

#define imdb_buffer_error(buffer)      (buffer->error)
#define imdb_buffer_filesize(buffer)   (buffer->filesize)
#define imdb_buffer_buffersize(buffer) (buffer->buffersize)
#define imdb_buffer_filepos(buffer)        (buffer->filepos) 

/*-----------------------------------------------------------------------------
 * Functions for File-buffering (These functions are part of the library)
 *-----------------------------------------------------------------------------
 */

#ifndef IMDB_RESOURCES_C

/* Procedure:  IMDBOpenBuffer
 * Purpose:    open file and allocate buffer, but do not fill buffer
 * Comment:    If you want to buffer a file completely or not at all, you have
 * Parameters: fname filename
 *             mode  IMDB_FILE_READ or IMDB_FILE_WRITE
 *             size  of buffer (see comment)
 * Returns:    pointer to IMDB_Buffer struct
 */
extern IMDB_Buffer *IMDBOpenBuffer (char *fname, LONG flags, LONG size);

/* Procedure:  IMDBCloseBuffer
 * Purpose:    Close buffer
 * Comment:    
 * Parameters: buffer  pointer to buffer struct
 * Returns:    error-code
 */
extern LONG IMDBCloseBuffer (IMDB_Buffer *p_buffer);

/* Procedure:  IMDBPositionBuffer
 * Purpose:    Position in a buffered file
 * Comment:    
 *             Buffer needs to be in IMDB_FILE_READ mode
 * Parameters: file    pointer to file
 *             pos     position where to go to
 * Returns:    error-code
 */
extern LONG IMDBPositionBuffer (IMDB_Buffer *p_buffer, LONG pos);

/* Procedure:  IMDBReadBuffer
 * Purpose:    Read Buffer
 * Comment:    mem
 * Parameters: buffer  pointer to buffer
 *             mem     pointer that will be changed to the location of
 *                     the data. Do not allocate or free!!
 *             size    number of bytes to read from file
 * Returns:    number of bytes read or IMDB_IO_ERROR
 */
extern LONG IMDBReadBuffer (IMDB_Buffer *p_buffer, APTR p_mem, LONG size);

/* Procedure:  IMDBReadBufferLine
 * Purpose:    Read a line of data from buffer
 * Comment:    
 *             Buffer needs to be in IMDB_FILE_READ mode
 * Parameters: file    pointer to file
 *             mem     pointer to memory where to store the data
 *             max_size maximum nb of bytes
 * Returns:    0 or IMDB_ERROR
 */
extern LONG IMDBReadBufferLine (IMDB_Buffer *p_buffer, APTR p_mem, LONG max_size);

/* Procedure:  IMDBWriteBuffer
 * Purpose:    write some data to buffer
 * Comment:    Note that size has to be smaller than the buffersize
 *             Buffer needs to be in IMDB_FILE_WRITE mode
 * Parameters: buffer  pointer to buffer
 *             data    pointer to data
 *             size    number of bytes to write
 * Returns:    error-code
 */
extern LONG IMDBWriteBuffer (IMDB_Buffer *p_buffer, APTR p_mem, LONG size);

#endif


#endif
