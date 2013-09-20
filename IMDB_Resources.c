#define IMDB_RESOURCES_C
#define IMDB_INTERNAL

/*=============================================================================
 *
 *  Program:   IMDB_Resources
 *
 *  Version:   1.0 (29.12.95) 
 *
 *  Purpose:   provides all Memory-Allocation and I-O functions
 *
 *  Author:    Andre Bernhardt <ab@imdb.com>
 *
 *  Copyright: (w) Andre Bernhardt 1995 - 2001
 *             (c) Internet MovieDatabase Limited 1996-2001
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
 *  0.1  11.06.95 First steps...
 *
 *=============================================================================
 *
 *
 *=============================================================================
 */

#include "IMDB.h"

#ifdef SYS_AMIGA
#include <clib/exec_protos.h>       
#include <Exec/Memory.h>
#endif /* SYS_AMIGA */

#ifdef SYS_UNIX
#ifndef NEXT
#include <unistd.h>

#ifndef memmove
#define memmove(s,t,n) memcpy(s,t,n)
#endif /* memmove */

#endif /* NEXT */
#endif /* SYS_UNIX */

/*-----------------------------------------------------------------------------
 * Procedure:   IMDBSetError
 *
 * Purpose:     Set the IMDB_Error-struct
 *
 * Parameters:  pointer to IMDB_Error
 *
 * Returns:     nothing
 *-----------------------------------------------------------------------------
 */

void IMDBSetError (IMDB_Error *error, LONG penalty, LONG routine, LONG number, char *text)
{
 error->status  = penalty;
 error->routine = routine;
 error->error   = number;
 error->text    = text;
};

/*-----------------------------------------------------------------------------
 * Procedure:   IMDBResetError
 *
 * Purpose:     Reset the IMDB_Error-struct
 *
 * Parameters:  pointer to IMDB_Error
 *
 * Returns:     nothing
 *-----------------------------------------------------------------------------
 */

void IMDBResetError (IMDB_Error *error)
{
 IMDBSetError (error, IMDB_PENALTY_OK, 0, IMDB_NO_ERROR, NULL);
};


/*-----------------------------------------------------------------------------
 * Procedure:   IMDBAllocMemory
 *
 * Purpose:     Allocate Memory
 *
 * Comment:     Side-effects on File-Buffer-functions
 *
 * Parameters:  size    number of bytes to be allocated
 *
 * Return:      Pointer to Memory, or NULL if failed
 *-----------------------------------------------------------------------------
 */

APTR IMDBAllocMemory (LONG size)
 {
  #ifdef SYS_AMIGA

  return (AllocVec(size, MEMF_ANY));

  #else

  return (malloc(size));

  #endif
 };


/*-----------------------------------------------------------------------------
 * Procedure:   IMDBFreeMemory
 *
 * Purpose:     Free Memory
 *
 * Parameters:  mem     pointer to memory
 *
 * Return:      nothing
 *-----------------------------------------------------------------------------
 */

void IMDBFreeMemory (APTR mem)
 {
  #ifdef SYS_AMIGA

  if (mem)
   FreeVec (mem);

  #else

  if (mem)
   free (mem);

  #endif
 };


/******************************************************************************
 *  File-Handling
 ******************************************************************************
 */

#define IMDB_BUFFER_SIZE 2048

/*-----------------------------------------------------------------------------
 * Procedure:  IMDBExistFile
 *
 * Purpose:    
 *
 * Comment:    
 *
 * Parameters: fname Filename
 *
 * Returns:    TRUE or FALSE
 *-----------------------------------------------------------------------------
 */

BOOL IMDBExistFile (char *fname)
 {
  FILE *fp;

  if (fp = fopen (fname, "r"))
   {
    fclose(fp);
    return (TRUE);
   }
  return (FALSE);
 }

/******************************************************************************
 *  Buffer-Handling
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:  IMDBOpenBuffer
 *
 * Purpose:    open buffer
 *
 * Comment:
 *
 * Parameters: fname    filename
 *             mode     IMDBV_FILE_READ, IMDBV_FILE_WRITE or IMDBV_FILE_APPEND
 *             size     of buffer
 * Returns:    pointer to file-info or NULL if failed
 *-----------------------------------------------------------------------------
 */

IMDB_Buffer *IMDBOpenBuffer (char *fname, LONG flags, LONG size)
 {
  IMDB_Buffer *p_buffer;
  char modestr[5];
  LONG mode;

  mode = (flags & 3);

  switch (mode)
   {
    case IMDBV_FILE_READ:
      strcpy (modestr,"rb");
      break;
    case IMDBV_FILE_WRITE:
      strcpy (modestr,"wb");
      break;
    case IMDBV_FILE_APPEND:
      strcpy (modestr,"ab");
      break;
    default:
     modestr[0] = '\0';
   }

  if (p_buffer = IMDBAllocMemory (sizeof (IMDB_Buffer)))
   {
    IMDBResetError(&p_buffer->error);
    p_buffer->filesize           = 0;
    p_buffer->filepos            = 0;
    if (p_buffer->fname = IMDBAllocMemory(strlen(fname)+1))
     strcpy (p_buffer->fname, fname);
    p_buffer->mode               = mode;
    p_buffer->stream             = fopen(p_buffer->fname, modestr);
/*    p_buffer->status             = IMDBE_NO_ERROR;*/
    p_buffer->buffersize         = size;
    p_buffer->bufferpos          = 0;
    p_buffer->nb_bytes_in_buffer = 0;
    p_buffer->buffer             = NULL;

    if (NULL == p_buffer->stream)
     {
      if (p_buffer->fname) IMDBFreeMemory(p_buffer->fname);
      IMDBFreeMemory(p_buffer);
      return (NULL);
     }
    else
     {
      if (NULL == (p_buffer->buffer = IMDBAllocMemory(p_buffer->buffersize+2)))
       {
        fclose(p_buffer->stream);
        if (p_buffer->fname) IMDBFreeMemory(p_buffer->fname);
        IMDBFreeMemory(p_buffer);
        return (NULL);
       }
      p_buffer->buffer[p_buffer->buffersize+0] = '\n';
      p_buffer->buffer[p_buffer->buffersize+1] = '\0';

      if ((IMDBV_FILE_READ == mode) && (flags & IMDBV_FILE_GETSIZE))
       { /* get size of file */       
        if(fseek (p_buffer->stream, 0, SEEK_END))
         IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_POSITION, p_buffer->fname);
        else 
         {
          p_buffer->filesize = ftell(p_buffer->stream);
          if(fseek (p_buffer->stream, 0, SEEK_SET))
           IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_POSITION, p_buffer->fname);
         }
       }
     }
   }

  return (p_buffer);
 }


/*-----------------------------------------------------------------------------
 * Procedure:  IMDBCloseBuffer
 *
 * Purpose:    Close Buffer
 *
 * Comment:    
 *
 * Parameters: file  pointer to file struct
 *
 * Returns:    nothing    
 *-----------------------------------------------------------------------------
 */

LONG IMDBCloseBuffer (IMDB_Buffer *p_buffer)
 {
  LONG error_code = IMDBE_NO_ERROR;

  if (NULL == p_buffer)
   return (1);

  /* Flush Buffer */
  if (IMDBV_FILE_WRITE == p_buffer->mode)
   if (p_buffer->nb_bytes_in_buffer != fwrite(p_buffer->buffer, 1, p_buffer->nb_bytes_in_buffer, p_buffer->stream))
    {
     IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_WRITE, p_buffer->fname);
     error_code = IMDBE_FILE_WRITE;
    }

  fclose(p_buffer->stream);
  if (p_buffer->fname) IMDBFreeMemory(p_buffer->fname);
  if (p_buffer->buffer) IMDBFreeMemory(p_buffer->buffer);
  IMDBFreeMemory(p_buffer);

  return (error_code);
 }


/*-----------------------------------------------------------------------------
 * Procedure:  IMDBPositionBufer
 *
 * Purpose:    Position in file
 *
 * Comment:    Buffer needs to be in IMDBV_FILE_READ mode
 *
 * Parameters: file    pointer to file
 *             pos     position where to go to
 *
 * Returns:    NULL or IMDB_IO_ERROR if failed
 *-----------------------------------------------------------------------------
 */

LONG IMDBPositionBuffer (IMDB_Buffer *p_buffer, LONG pos)
 {

  /*first check, if the position is already within the buffer */
  if ((pos < (p_buffer->filepos - p_buffer->bufferpos))
    ||(pos > (p_buffer->filepos - p_buffer->bufferpos + p_buffer->nb_bytes_in_buffer)))
   {/* ausserhalb des Buffers, also neu positionieren */
    if(fseek (p_buffer->stream, pos, SEEK_SET))
     {
      IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_POSITION, p_buffer->fname);
      return (IMDBE_FILE_POSITION);
     }
    p_buffer->bufferpos = 0;
    p_buffer->filepos = pos;
    p_buffer->nb_bytes_in_buffer = fread(p_buffer->buffer, 1, p_buffer->buffersize, p_buffer->stream);
    return (IMDBE_NO_ERROR);
   }
  else
   {/* position ist innerhalb des Buffers */
    p_buffer->bufferpos = p_buffer->bufferpos + (pos - p_buffer->filepos);
    p_buffer->filepos = pos;
    return (IMDBE_NO_ERROR);
   }

  p_buffer->filepos = pos;
  return (0);
 }

/*-----------------------------------------------------------------------------
 * Procedure:  IMDBReadBuffer
 *
 * Purpose:    Read data from file
 *
 * Comment:    
 *             Buffer needs to be in IMDB_FILE_READ mode
 *
 * Parameters: file    pointer to file
 *             mem     pointer to memory where to store the data
 *             size    number of bytes to read from file
 *
 * Returns:    number of bytes read or IMDB_IO_ERROR
 *-----------------------------------------------------------------------------
 */

LONG IMDBReadBuffer (IMDB_Buffer *p_buffer, char **p_mem, LONG size)
 {
/*  LONG streampos;*/
  LONG i;

/*
printf ("%i %i %i\n", p_buffer->nb_bytes_in_buffer, p_buffer->filepos, p_buffer->bufferpos);
*/

  if (p_buffer->nb_bytes_in_buffer - p_buffer->bufferpos - size >= 0)
   {/* O.K. Ist alles im Speicher */
    *p_mem = &p_buffer->buffer[p_buffer->bufferpos];
    p_buffer->bufferpos += size;
    p_buffer->filepos += size;   
    return (size);
   }

  /* Move the last bits of memory  */
  i = p_buffer->nb_bytes_in_buffer - p_buffer->bufferpos;
  if ((i<0) || (i>p_buffer->buffersize))
   {
#ifdef IMDB_DEBUG
    printf ("Fatal error in IMDBReadBuffer!!!\n Remaining bytes: %i\nBytes in Buffer %i\n", i, p_buffer->buffersize);
#endif
    return (0);
   }
  if (i > 0)
   memmove(p_buffer->buffer, &p_buffer->buffer[p_buffer->bufferpos], i);

  /* Load new segment in memory */
  p_buffer->nb_bytes_in_buffer = i + fread(&p_buffer->buffer[i], 1, p_buffer->buffersize - i, p_buffer->stream);
  *p_mem = p_buffer->buffer;

  if (p_buffer->nb_bytes_in_buffer > size)
   {
    p_buffer->bufferpos = size;
    p_buffer->filepos  += size;
    return (size);
   }
  else
   {
    p_buffer->bufferpos = p_buffer->nb_bytes_in_buffer;
    p_buffer->filepos  += p_buffer->nb_bytes_in_buffer;
    return (p_buffer->nb_bytes_in_buffer);
   }
 }

/*-----------------------------------------------------------------------------
 * Procedure:  IMDBReadBufferLine
 *
 * Purpose:    Read data from file
 *
 * Comment:    
 *             Buffer needs to be in IMDB_FILE_READ mode
 *
 * Parameters: file    pointer to file
 *             mem     pointer to memory where to store the data
 *             size    number of bytes to read from file
 *
 * Returns:    error_code
 *-----------------------------------------------------------------------------
 */

LONG IMDBReadBufferLine (IMDB_Buffer *p_buffer, char **p_mem, LONG size)
 {
/*  LONG streampos;*/
  LONG i;
  char c;

/*
printf ("%i %i %i\n", p_buffer->nb_bytes_in_buffer, p_buffer->filepos, p_buffer->bufferpos);
*/

  if (p_buffer->nb_bytes_in_buffer - p_buffer->bufferpos - size >= 0)
   {/* O.K. Ist alles im Speicher */
    *p_mem = &p_buffer->buffer[p_buffer->bufferpos];
    /* find EOL */
    i = 0;
    while ((c = p_buffer->buffer[p_buffer->bufferpos + i]) && ('\n' != c))
     i++;
    if (i >= size)
     return (IMDBE_FILE_READ);
    p_buffer->buffer[p_buffer->bufferpos + i] = '\0';
    /* correct position */
    p_buffer->bufferpos += (i + 1);
    p_buffer->filepos += (i + 1);   
    return (IMDBE_NO_ERROR);
   }

  /* Move the last bits of memory  */
  i = p_buffer->nb_bytes_in_buffer - p_buffer->bufferpos;
  if ((i<0) || (i>p_buffer->buffersize))
   {
#ifdef IMDB_DEBUG
    printf ("Fatal error in IMDBReadBufferLine!!!\n Remaining bytes: %i\nBytes in Buffer %i\n", i, p_buffer->buffersize);
#endif
    return (IMDBE_FILE_READ);
   }
  if (i > 0)
   memmove(p_buffer->buffer, &p_buffer->buffer[p_buffer->bufferpos], i);

  /* Load new segment in memory */
  p_buffer->nb_bytes_in_buffer = i + fread(&p_buffer->buffer[i], 1, p_buffer->buffersize - i, p_buffer->stream);
  *p_mem = p_buffer->buffer;

  /* end of file? */
  if (0 == p_buffer->nb_bytes_in_buffer)
   {
    *p_mem = NULL;
    return (IMDBE_FILE_EOF);
   }

  /* find EOL */
  i = 0;
  while ((c = p_buffer->buffer[i]) && ('\n' != c))
   i++;
  if ((i >= size) || (i >= p_buffer->nb_bytes_in_buffer))
   return (IMDBE_FILE_READ);
  p_buffer->buffer[i] = '\0';

  /* correct position */
  p_buffer->bufferpos = (i + 1);
  p_buffer->filepos  += (i + 1);

  return (IMDBE_NO_ERROR);
 }

/*-----------------------------------------------------------------------------
 * Procedure:  IMDBWriteBuffer
 *
 * Purpose:    write some data to file
 *
 * Comment:    
 *             Buffer needs to be in IMDB_FILE_WRITE or IMDB_FILE_APPEND mode
 *
 * Parameters: buffer  pointer to buffer
 *             data    pointer to data
 *             size    number of bytes to write
 *
 * Returns:    NULL if successful, IMDB_IO_ERROR otherwise
 *-----------------------------------------------------------------------------
 */

LONG IMDBWriteBuffer (IMDB_Buffer *p_buffer, APTR p_mem, LONG size)
 {
  if (size <= (p_buffer->buffersize - p_buffer->nb_bytes_in_buffer))
   {/* save in memory */ 
    memcpy(&p_buffer->buffer[p_buffer->nb_bytes_in_buffer], p_mem, size);
    p_buffer->nb_bytes_in_buffer += size;
    p_buffer->bufferpos = p_buffer->nb_bytes_in_buffer;
    p_buffer->filepos  += size;
    return (IMDBE_NO_ERROR);
   }

  /* Flush Buffer */
  if (p_buffer->nb_bytes_in_buffer != fwrite(p_buffer->buffer, 1, p_buffer->nb_bytes_in_buffer, p_buffer->stream))
   {
    IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_WRITE, p_buffer->fname);
    return (IMDBE_FILE_WRITE);
   }
  p_buffer->filepos = ftell(p_buffer->stream);
  p_buffer->bufferpos = 0;
  p_buffer->nb_bytes_in_buffer = 0;

  /* is buffer too small for this block? */
  if (size > p_buffer->buffersize)
   {/* save immediately */
    if (size != fwrite(p_mem, 1, size, p_buffer->stream))
     {
      IMDBSetError(&p_buffer->error, IMDB_PENALTY_HARMLESS, 0, IMDBE_FILE_WRITE, p_buffer->fname);
      return (IMDBE_FILE_WRITE);
     }
    p_buffer->filepos = ftell(p_buffer->stream);
   }
  else
   {/* save in memory */ 
    memcpy(p_buffer->buffer, p_mem, size);
    p_buffer->nb_bytes_in_buffer = size;
    p_buffer->bufferpos = size;
    p_buffer->filepos  += size;
    return (IMDBE_NO_ERROR);
   }

  return (IMDBE_NO_ERROR);
 }

