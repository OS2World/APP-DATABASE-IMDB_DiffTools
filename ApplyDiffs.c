/*============================================================================
 *
 *  Program:      ApplyDiffs.c
 *
 *  Version:      2.5 (22.11.2001)
 *
 *  Purpose:      Applies diffs to a listfile
 *
 *                #define either SYS_AMIGA or SYS_UNIX (see below)
 *
 *                AMIGA-Commandline-Options:
 *
 *                   LISTDIR/A   path of the listfiles
 *                   DIFFDIR/A   path of the diffiles (diffiles must end with .list)
 *                   CHECKCRC/S  check crc of listfile before applying
 *                   FORCE/S     skip wrong/corrupted diffs, but apply all others
 *                   KEEP/S      keep a copy of the old list- and diffile even
 *                               if the diffs were applied successfully
 *                   NOSTATS/S   don't print statistics
 *                   QUIET/S     don't show progress
 *                   LOGFILE/N   name of logfile
 *
 *
 *                UNIX-Commandline-Options:
 *
 *                   <listdir>   path of the listfiles
 *                   <diffdir>   path of the diffiles (diffiles must end with .list)
 *                  optional:
 *                   -checkcrc   check crc of listfile before applying
 *                   -force      skip wrong/corrupted diffs, but apply all others
 *                   -keep       keep a copy of the old list- and diffile even
 *                               if the diffs were applied successfully
 *                   -nostats    don't print statistics
 *                   -quiet      don't show progress
 *                   -logfile    name of logfile
 *
 *
 *  Author:       Andre Bernhardt <ab@imdb.com>
 *
 *  Copyright:    (w) Andre Bernhardt 1995 - 2001
 *                (c) Internet MovieDatabase Limited 1996-2001
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
 *============================================================================
 */

/* some defines (specified by the Makefile) */
/*#define SYS_AMIGA */
/*#define SYS_UNIX  */
/*#define IMDB_DEBUG*/

/* 2.3 Packer & options -> Makefile */
#define IMDB_GZIP
#define IMDBV_FILE_PACKER_NAME      "gzip"
#define IMDBV_FILE_PACKER_EXT       ".gz"
#define IMDBV_FILE_PACKER_PACK      "-4"
#define IMDBV_FILE_PACKER_UNPACK    "-d"

/* ************************* */

#include "IMDB.h"

#ifdef SYS_AMIGA
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <Exec/Memory.h>
#endif /* SYS_AMIGA */

#ifdef SYS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef NEXT
#include <sys/dir.h>
#else
#include "dirent.h"
#endif /* NEXT */

#endif /* SYS_UNIX*/

#define VERSION "ApplyDiffs 2.5 (22.11.01)"
static const char version[] ="$VER: "VERSION;

/* Return values */
#define RET_OK              0
#define RET_WARNING        10
#define RET_ERROR          20

/* buffer sizes */
#define ADV_BUFFER_SIZE    512 * 1024
#define ADV_MAX_LINESIZE     8 * 1024

/* Information on a diff */
#define STATUS_OK       0  /* No error */
#define STATUS_UNKNOWN -1  /* unknown statuts (e.g. file is gzipped) */ /*2.3*/
#define STATUS_CRC      1  /* CRC-Checksum error */
#define STATUS_IO       2  /* IO-Error */
#define STATUS_VER      3  /* wrong file-diffile-combination */
#define STATUS_SYN      4  /* Syntax-Error in Diff-File */
#define STATUS_NEW      5  /* New file */

/* types */
#define DIFF_TYPE_UNKNOWN     0
#define DIFF_TYPE_ORIGINAL    1
#define DIFF_TYPE_STRIPPED    2

typedef struct DIFFINFO
 {
  struct DIFFINFO *next;
  char  fname_list[256];           /* filename */
  char  fname_diff[256];           /* filename */
  LONG  type;
  LONG  status;
  LONG  add;
  LONG  delete;
 } DiffInfo;

typedef struct
 {
  char *p_listdir;
  char *p_diffdir;
  LONG  f_checkcrc;
  LONG  f_force;
  LONG  f_keep;
  LONG  f_nostats;
  LONG  f_quiet;
  char *p_logfile;
 } AD_Commands;

  AD_Commands  ad_cmds  = {NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, NULL};

/******************************************************************************
 * Functions dealing with CRC-sum
 ******************************************************************************
 */

 /* global variable for tab */
 ULONG *pCrcTab = NULL;
 ULONG nCrc = 0xFFFFFFFFL;
 int nIndex;
 static char old_crc[16];
 static char new_crc[16];

/*-----------------------------------------------------------------------------
 * Procedure:   init_crc
 *
 * Purpose:     This function initializes the CRC-32 algorithm by setting a private
 *              pre  calculation  table  in the memory.  This table is used
 *              for all further CRC-32 calculations.
 *
 * Parameters:  none
 *
 * Returns:     Pointer to CRC-Table
 *-----------------------------------------------------------------------------
 */

u_long *InitCRC(void)
{
  u_long *p_crc_tab = NULL;
  int i;
  int j;

  u_char ib[8];
  u_char lb[32];


  /* allocate memory for table */
  if ((p_crc_tab = malloc(256*sizeof(u_long))))
   {
    /* initialize table with calculated bits */
    for (i=0; i<256; i++)
     {
       /* reset bits */
       for (j=0; j<8;j++)
	 ib[j]=(i>>(7-j))&1;
       for (j=0; j<32;j++)
	 lb[j]=0;

       /* calculate values */
       lb[31]=      ib[1]                              ^ib[7];
       lb[30]=ib[0]^ib[1]                        ^ib[6]^ib[7];
       lb[29]=ib[0]^ib[1]                  ^ib[5]^ib[6]^ib[7];
       lb[28]=ib[0]                  ^ib[4]^ib[5]^ib[6];
       lb[27]=      ib[1]      ^ib[3]^ib[4]^ib[5]      ^ib[7];
       lb[26]=ib[0]^ib[1]^ib[2]^ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[25]=ib[0]^ib[1]^ib[2]^ib[3]      ^ib[5]^ib[6];
       lb[24]=ib[0]      ^ib[2]      ^ib[4]^ib[5]      ^ib[7];
       lb[23]=                  ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[22]=            ib[2]^ib[3]      ^ib[5]^ib[6];
       lb[21]=            ib[2]      ^ib[4]^ib[5]      ^ib[7];
       lb[20]=                  ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[19]=      ib[1]^ib[2]^ib[3]      ^ib[5]^ib[6]^ib[7];
       lb[18]=ib[0]^ib[1]^ib[2]      ^ib[4]^ib[5]^ib[6];
       lb[17]=ib[0]^ib[1]      ^ib[3]^ib[4]^ib[5];
       lb[16]=ib[0]      ^ib[2]^ib[3]^ib[4];
       lb[15]=            ib[2]^ib[3]                  ^ib[7];
       lb[14]=      ib[1]^ib[2]                  ^ib[6];
       lb[13]=ib[0]^ib[1]                  ^ib[5];
       lb[12]=ib[0]                  ^ib[4];
       lb[11]=                  ib[3];
       lb[10]=            ib[2];
       lb[ 9]=                                          ib[7];
       lb[ 8]=      ib[1]                        ^ib[6]^ib[7];
       lb[ 7]=ib[0]                        ^ib[5]^ib[6];
       lb[ 6]=                        ib[4]^ib[5];
       lb[ 5]=      ib[1]      ^ib[3]^ib[4]            ^ib[7];
       lb[ 4]=ib[0]      ^ib[2]^ib[3]            ^ib[6];
       lb[ 3]=      ib[1]^ib[2]            ^ib[5];
       lb[ 2]=ib[0]^ib[1]            ^ib[4];
       lb[ 1]=ib[0]            ^ib[3];
       lb[ 0]=            ib[2];

       /* store value */
       p_crc_tab[i]=0;
       for (j=0; j<32;j++)
	 p_crc_tab[i]|=lb[j]<<(31-j);
     }
   }
  return (p_crc_tab);
}

/*-----------------------------------------------------------------------------
 * Procedure:   calc_crc
 *
 * Purpose:     calc crc of string. Automatically adds crc for '\n'
 *
 * Parameters:
 *
 * Returns:
 *-----------------------------------------------------------------------------
 */

void calc_crc (char *str, ULONG *nCrc)
 {
  while (*str)
   {
    nIndex = (int) ((*nCrc ^ *str++) & 0x000000FFL);
    *nCrc = ((*nCrc >> 8) & 0x00FFFFFFL) ^ pCrcTab[nIndex];
   }

  /* add '\n' */
  nIndex = (int) ((*nCrc ^ '\n') & 0x000000FFL);
  *nCrc = ((*nCrc >> 8) & 0x00FFFFFFL) ^ pCrcTab[nIndex];
 }

/******************************************************************************
 *
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:   checkfile_match
 *
 * Parameters:  listfile, diffile
 *
 * Comments:
 *-----------------------------------------------------------------------------
 */

int checkfile_match(char *listfile, char *diffile, BOOL flag_verbose, DiffInfo *diffinfo)
 {
  IMDB_Buffer *list_buffer = NULL;
  IMDB_Buffer *diff_buffer = NULL;
  char        *p_diff_line;
  char        *p_list_line;
  LONG         status      = STATUS_OK;

  /* open diff-file */
  if (NULL == (diff_buffer = IMDBOpenBuffer (diffile, IMDBV_FILE_READ , ADV_BUFFER_SIZE)))
   {
    diffinfo->status = STATUS_IO;
    if (flag_verbose)
     printf ("Error: Can't open Diff-File\n");
    return (RET_ERROR);
   }

  /* open old listfile */
  if (IMDBExistFile(listfile))
   {
    list_buffer = IMDBOpenBuffer (listfile, IMDBV_FILE_READ, ADV_BUFFER_SIZE);
   }

  if (DIFF_TYPE_ORIGINAL == diffinfo->type)
/****** Original Diff-File *********/
   {
    /* if this file does not exist, check if the diffs contains only data that */
    /* adds something -> new file */
    /* check if the diff-version matches the listfile */
    if (NULL == list_buffer)
     {/* new file ? */
      /* check if the diff-file contains no line starting with '<' */
      status = STATUS_NEW;
      while (0 == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
       {
        if (*p_diff_line == '<')
         {
          status = STATUS_VER;
          if (flag_verbose)
           printf ("Error: Missing Listfile\n");
          break;
         }
       }
      if ((flag_verbose) && (STATUS_NEW == status))
       printf ("New Listfile\n");
     }
    else
     {
      /* check if the diff-version matches the listfile */
      /* get the 2nd line in the diff-file and compare it with the 1st line in listfile */
      if ((0 == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
        &&(0 == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE)) /* 2nd line */
        &&(0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
       {
        if (0 != strcmp (p_diff_line+2, p_list_line))
         {
          status = STATUS_VER;
          if (flag_verbose)
           printf ("Error: Unsuitable Diff-File\n");
         }
        else
         if (flag_verbose)
          printf ("OK.\n");
       }
      else
       status = STATUS_IO;
     }
   }
/****** Stripped Diff-File *********/
  else
   {
    /* if the listfile does not exist, check if the diffs contains only data that */
    /* adds something -> new file */
    /* check if the diff-version matches the listfile */
    if (NULL == list_buffer)
     {/* new file ? */
      IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE);
      if (0 == strcmp(p_diff_line, "Apply on: ---"))
       {
        status = STATUS_NEW;
        if (flag_verbose)
        printf ("New Listfile\n");
       }
      else
       {
        status = STATUS_VER;
        if (flag_verbose)
         printf ("Error: Missing Listfile\n");
       }
     }
    else
     {
      /* check if the diff-version matches the listfile */
      /* get the 1st line in the diff-file and compare it with the 1st line in listfile */
      if ((0 == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
        &&(0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
       {
        if (0 != strcmp (p_diff_line+10, p_list_line))
         {
          status = STATUS_VER;
          if (flag_verbose)
           printf ("Error: Unsuitable Diff-File\n");
         }
        else
         if (flag_verbose)
          printf ("OK.\n");
       }
      else
       status = STATUS_IO;
     }
   }


  /* Close Buffer */
  IMDBCloseBuffer (diff_buffer);
  if (list_buffer)
   IMDBCloseBuffer (list_buffer);

  if (diffinfo)
   diffinfo->status = status;

  return (RET_OK);
 }

/*-----------------------------------------------------------------------------
 * Procedure:   checkfile_crc
 *
 * Parameters:  listfile
 *
 * Comments:
 *-----------------------------------------------------------------------------
 */

int checkfile_crc(char *listfile, BOOL flag_verbose)
 {
  IMDB_Buffer *list_buffer = NULL;
  char        *p_list_line;
  LONG         status      = STATUS_OK;
  LONG         progress    = 0;
  LONG         tprogress   = 0;

  /* Reset CRC */
  nCrc = 0xFFFFFFFFL;
  new_crc [0] = '\0';
  old_crc [0] = '\0';

  /* open old listfile */
  if ((list_buffer = IMDBOpenBuffer (listfile, IMDBV_FILE_READ|IMDBV_FILE_GETSIZE, ADV_BUFFER_SIZE))
    &&(0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
   {
    if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
     {
      strncpy (old_crc, p_list_line, 15);

      /* calculate CRC */
      while (0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE))
       {
        if ((flag_verbose) && (progress != (tprogress = (list_buffer->filepos*100/list_buffer->filesize))))
         {
          progress = tprogress;
          printf ("\b\b\b\b\b\b(%03i%%)", progress);
          fflush (stdout);
         }

        /* calculate CRC for next line */
        calc_crc (p_list_line, &nCrc);
       }

      /* compare CRC */
      sprintf(new_crc, "CRC: 0x%08X", nCrc);
      if (0 == strcmp (old_crc, new_crc))
       {
        if (flag_verbose)
         printf ("\b\b\b\b\b\b- CRC-Checksum O.K.\n");
       }
      else
       {
        if (flag_verbose)
         printf ("\b\b\b\b\b\b- CRC-Checksum Error\n");
        status = STATUS_CRC;
       }
     }

    /* Close Buffer */
    IMDBCloseBuffer (list_buffer);
   }

  return (status);
 }

/******************************************************************************
 *
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:  StrChangeSuffix
 *
 * Purpose:    replace suffix of a string by another string.
 *-----------------------------------------------------------------------------
 */

void StrChangeSuffix (char* p_str, char* p_suffix)
 {
  LONG len;


  len = strlen (p_str);
  while ((len>10) && (p_str[len-1]!='.'))
   len--;

  if (len > 10)
   p_str[len-1] = '\0';

  strcat (p_str, p_suffix);
 }

/*-----------------------------------------------------------------------------
 * Procedure:  GetPatch
 *
 * Purpose:    Parse a patchline
 *-----------------------------------------------------------------------------
 */

 struct TypPatch
  {
   char cmd;
   LONG i_start;
   LONG i_end;
   LONG o_start;
   LONG o_end;
  };

void GetPatch(struct TypPatch *patch, char *buffer)
 {
  char *c;
  char cmd;

  patch -> cmd     = '\0';
  patch -> i_start = 0;
  patch -> i_end   = 0;
  patch -> o_start = 0;
  patch -> o_end   = 0;

  if (buffer)
   {
    /* erster Teil */
    c = buffer;
    while (isdigit(*buffer)) buffer ++;
    cmd = *buffer;
    *buffer = '\0';
    patch -> i_start = patch -> i_end = strtol (c, NULL, 10);
    if (cmd == ',')
     {/* parse toline */
      buffer ++;
      c = buffer;
      while (isdigit(*buffer)) buffer++;
      cmd = *buffer;
      *buffer = '\0';
      patch -> i_end = strtol (c, NULL, 10);
     }

    patch -> cmd = cmd;

    /* 2ter Teil */
    buffer ++;
    c = buffer;
    while (isdigit(*buffer)) buffer ++;
    cmd = *buffer;
    *buffer = '\0';
    patch -> o_start = patch -> o_end = strtol (c, NULL, 10);
    if (cmd == ',')
     {/* parse toline */
      buffer ++;
      c = buffer;
      while (isdigit(*buffer)) buffer++;
      cmd = *buffer;
      *buffer = '\0';
      patch -> o_end = strtol (c, NULL, 10);
     }
    *buffer = cmd;

#ifdef IMDB_DEBUG
   printf ("Patch-Hunk: cmd:%i is:%i ie:%i os:%i oe:%i\n", patch->cmd, patch->i_start, patch->i_end, patch->o_start, patch->o_end);
#endif
   }
 }

/*-----------------------------------------------------------------------------
 * Procedure:   patchfile
 *
 * Parameters:  listfile, diffile
 *
 * Comments:
 *
 *              8,10c8,11 change lines 8-10 against 8-11
 *              13d13     delete line 13
 *              14a15     insert line after line 14
 *-----------------------------------------------------------------------------
 */

int patchfile_original(char *listfile, char *diffile, BOOL flag_keep, BOOL flag_verbose, DiffInfo *diffinfo)
 {
  static char     fname [256];
  struct TypPatch patch;
  IMDB_Buffer    *list_buffer = NULL;
  IMDB_Buffer    *diff_buffer = NULL;
  IMDB_Buffer    *out_buffer  = NULL;
  char           *p_list_line;
  char           *p_diff_line;
  LONG            list_line   = 1;
  LONG            out_line    = 0;
  LONG            l_add       = 0;
  LONG            l_delete    = 0;
  LONG            status      = STATUS_OK;
  LONG            progress    = 0;
  LONG            tprogress   = 0;
  LONG            i;
  LONG            len;

  strcpy (fname, listfile);
  StrChangeSuffix (fname, ".new");

  /* reset CRC */
  nCrc = 0xFFFFFFFFL;
  new_crc [0] = '\0';
  old_crc [0] = '\0';

  /* open diff-file */
  if (NULL == (diff_buffer = IMDBOpenBuffer (diffile, IMDBV_FILE_READ|IMDBV_FILE_GETSIZE, ADV_BUFFER_SIZE)))
   {
    diffinfo->status = STATUS_IO;
    return (RET_ERROR);
   }

  /* open old listfile */
  if (IMDBExistFile(listfile))
   {
    list_buffer = IMDBOpenBuffer (listfile, IMDBV_FILE_READ, ADV_BUFFER_SIZE);
   }
  else
   {
    /* Listfile does not exist. Maybe it's new? */
    if (STATUS_OK == diffinfo->status) /* d.h. wenn option NOCHECK benutzt wird */
     {
      /* check if the diff-file contains no line starting with '<' */
      diffinfo->status = STATUS_NEW;
      while ((STATUS_NEW == diffinfo->status) && (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE)))
       {
        if (*p_diff_line == '<')
         {
          status = diffinfo->status = STATUS_VER;
          if (flag_verbose)
           printf ("\b\b\b\b\b\b - Error: Missing Listfile\n");
         }
       }
      IMDBPositionBuffer (diff_buffer, 0);
     }

    if (STATUS_NEW != diffinfo->status)
     {
      if (STATUS_OK == diffinfo->status)
       diffinfo->status = STATUS_IO;
      IMDBCloseBuffer (diff_buffer);
      return (RET_WARNING);
     }
    if (flag_verbose)
     {
      printf ("\b\b\b\b\b\b(new file) (000%%)");
      fflush (stdout);
     }
   }

  /* open new listfile */
  if (NULL == (out_buffer =  IMDBOpenBuffer (fname, IMDBV_FILE_WRITE, ADV_BUFFER_SIZE)))
   {
    IMDBCloseBuffer (diff_buffer);
    IMDBCloseBuffer (list_buffer);
    if (diffinfo)
     diffinfo->status = STATUS_IO;
    return (RET_ERROR);
   }

  /*** now patch the file ***/
  /* get next line of diff */
  while ((STATUS_OK == status) && (!IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE)))
   {
    /* show progress */
    if ((flag_verbose) && (progress != (tprogress = (diff_buffer->filepos*100/diff_buffer->filesize))))
     {
      progress = tprogress;
      printf ("\b\b\b\b\b\b(%03i%%)", progress);
      fflush (stdout);
     }

    /* parse command */
    GetPatch(&patch, p_diff_line);

    /* Es gibt hier einen Sonderfall, naemlich eine Einfuegung gleich am Anfang */
    if ((0 == patch.i_start) && (patch.cmd == 'a'))
     {
      /* Und jetzt einfuegen */
      for (i = patch.o_start; i <= patch.o_end; i++)
       {
        if ((flag_verbose) && (progress != (tprogress = (diff_buffer->filepos*100/diff_buffer->filesize))))
         {
          progress = tprogress;
          printf ("\b\b\b\b\b\b(%03i%%)", progress);
          fflush (stdout);
         }
        if (IMDBE_FILE_READ == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
         {
          status = STATUS_IO;
          break;
         }
        len = strlen(p_diff_line);
        if ((IMDBWriteBuffer(out_buffer, p_diff_line+2, len-2))
          ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
         {
          status = STATUS_IO;
          break;
         }
        /* calc CRC */
        if (out_line)
         calc_crc (p_diff_line+2, &nCrc);
        else
         if (0 == strncmp (p_diff_line+2, "CRC: ", strlen("CRC: ")))
          strncpy (old_crc, p_diff_line+2, 15);
        out_line++;
        l_add++;
       }
      continue;
     }

    /* Sonderfall, dass es gar keine alte Liste gab */
    if (NULL == list_buffer)
     break;

    /* Error ??? */
    if ((patch.i_start < list_line) || (patch.i_start > patch.i_end)
      ||(patch.o_start < out_line)  || (patch.o_start > patch.o_end))
     {
      if (flag_verbose)
        printf ("\b\b\b\b\b\b - Confusion: cmd:%i is:%i ie:%i os:%i oe:%i\n", patch.cmd, patch.i_start, patch.i_end, patch.o_start, patch.o_end);
      status = STATUS_SYN;
      break;
     }

    /* Alles klar, wir suchen jetzt diese Zeile(n) im listfile */
    while (patch.i_start != list_line)
     {
      if (IMDBE_FILE_READ == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE))
       {
        status = STATUS_IO;
        break;
       }
      if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
        ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
       {
        status = STATUS_IO;
        break;
       }
      /* calc CRC */
      if (out_line)
       calc_crc (p_list_line, &nCrc);
      else
       if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
        strncpy (old_crc, p_list_line, 15);
      list_line++;
      out_line++;
     }

    /* Error */
    if (patch.i_start != list_line)
     {
      if (flag_verbose)
       printf ("\b\b\b\b\b\b - Error before reaching line number: %i\n", patch.i_start);
      status = STATUS_SYN;
      break;
     }
    else
     {
      /* O.K. jetzt sind wir an der richtigen Stelle */
      switch (patch.cmd)
       {
        case 'd': /* delete line */
         {
          for (i = patch.i_start; i <= patch.i_end; i++)
           {
            if ((IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
              ||(IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
             {
              status = STATUS_IO;
              break;
             }

            if (0 != strcmp (p_diff_line+2, p_list_line))
             {
              status = STATUS_VER;
              if (flag_verbose)
               printf ("\b\b\b\b\b\b - Error: Lines do not match (%i).\n", i);
              break;
             }
            list_line++;
            l_delete++;
           }
          break;
         }
        case 'a': /* add line */
         {
          /* Beim Anfuegen brauchen wir die Zeile spaeter */
          if(IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE))
           {
            status = STATUS_IO;
            break;
           }

          if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
            ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
           {
            status = STATUS_IO;
            break;
           }
          /* calc CRC */
          if (out_line)
           calc_crc (p_list_line, &nCrc);
          else
           if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
            strncpy (old_crc, p_list_line, 15);
          list_line++;
          out_line++;

          /* Und jetzt einfuegen */
          for (i = patch.o_start; i <= patch.o_end; i++)
           {
            if (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            len = strlen(p_diff_line);
            if ((IMDBWriteBuffer(out_buffer, p_diff_line+2, len-2))
              ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
             {
              status = STATUS_IO;
              break;
             }
            /* calc CRC */
            if (out_line)
             calc_crc (p_diff_line+2, &nCrc);
            else
             if (0 == strncmp (p_diff_line+2, "CRC: ", strlen("CRC: ")))
              strncpy (old_crc, p_diff_line+2, 15);
            out_line++;
            l_add++;
           }
          break;
         }
        case 'c': /* change line */
         {
          /* First delete those lines that are discarded */
          for (i = patch.i_start; i <= patch.i_end; i++)
           {
            if ((IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
              ||(IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
             {
              status = STATUS_IO;
              break;
             }
            if (0 != strcmp (p_diff_line+2, p_list_line))
             {
              status = STATUS_VER;
              if (flag_verbose)
               printf ("\b\b\b\b\b\b - Error: Lines do not match (%i).\n", i);
              break;
             }
            list_line++;
            l_delete++;
           }

          /* separator-line */
          if (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
           {
            status = STATUS_IO;
            break;
           }
          if (0 != strncmp (p_diff_line, "---", 3))
           {
            if ((flag_verbose) && (STATUS_OK == status))
             printf ("\b\b\b\b\b\b - Error: Can't find separator.\n");
            status = STATUS_VER;
            break;
           }

          /* now add new lines */
          for (i = patch.o_start; i <= patch.o_end; i++)
           {
            if (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            len = strlen(p_diff_line);
            if ((IMDBWriteBuffer(out_buffer, p_diff_line+2, len-2))
              ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
             {
              status = STATUS_IO;
              break;
             }
            /* calc CRC */
            if (out_line)
             calc_crc (p_diff_line+2, &nCrc);
            else
             if (0 == strncmp (p_diff_line+2, "CRC: ", strlen("CRC: ")))
              strncpy (old_crc, p_diff_line+2, 15);
            out_line++;
            l_add++;
           }
          break;
         }
        default:
         {
          if (flag_verbose)
           printf ("\b\b\b\b\b\b - Unknown command %i\n", patch.cmd);
          status = STATUS_SYN;
          break;
         }
       }
     }
   }

  /* Das restliche listfile kopieren falls nicht neue Liste */
  if (list_buffer)
   while ((STATUS_OK == status) && (!IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
    {
     if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
       ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
      {
       status = STATUS_IO;
       break;
      }
     /* calc CRC */
     if (out_line)
      calc_crc (p_list_line, &nCrc);
     else
      if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
       strncpy (old_crc, p_list_line, 15);
     list_line++;
     out_line++;
    }

  /* Close Buffer */
  IMDBCloseBuffer (out_buffer);
  IMDBCloseBuffer (diff_buffer);
  IMDBCloseBuffer (list_buffer);

  /* compare CRC */
  sprintf(new_crc, "CRC: 0x%08X", nCrc);
  if (((STATUS_OK == status) || (STATUS_NEW == status)) &&
      ((0 == strcmp (old_crc, new_crc)) || (ad_cmds.f_force == TRUE)))
   {
    if (flag_verbose)
     printf ("\b\b\b\b\b\b- CRC-Checksum O.K.\n");

    /* *.old file loeschen falls noch nicht geschehen und files umbenennen */
    strcpy (fname, listfile);
    StrChangeSuffix (fname, ".old");
    remove (fname);

    /* listfile umbenennen, bzw loeschen */
    if (flag_keep)
     rename (listfile, fname);
    else
     remove (listfile);

    /* neues Listfile umbenennen */
    strcpy (fname, listfile);
    StrChangeSuffix (fname, ".new");
    rename (fname, listfile);

#ifdef SYS_AMIGA
    /* Protection Bits richtig setzen */
    SetProtection (listfile, FIBF_EXECUTE);
#endif

    /* diffile loeschen */
    if (!flag_keep)
     remove (diffile);
   }
  else
   {
    if ((STATUS_OK == status) || (STATUS_NEW == status))
     { /* something went wrong! No changes */
      if (flag_verbose)
       printf ("\b\b\b\b\b\b- CRC-Checksum Error\n");
      status = STATUS_CRC;
     }
    remove (fname);
    l_add = 0;
    l_delete = 0;
   }

  /* remember DiffInfo */
  if (diffinfo)
   {
    if (STATUS_NEW != diffinfo->status)
     diffinfo->status = status;
    diffinfo->add = l_add;
    diffinfo->delete = l_delete;
   }

  if (status)
   return (RET_WARNING);
  else
   return (RET_OK);
 }

/*-----------------------------------------------------------------------------
 * Procedure:   patchfile
 *
 * Parameters:  listfile, diffile
 *
 * Comments:
 *
 *              8,10c8,11 change lines 8-10 against 8-11
 *              13d13     delete line 13
 *              14a15     insert line after line 14
 *-----------------------------------------------------------------------------
 */

int patchfile_stripped(char *listfile, char *diffile, BOOL flag_keep, BOOL flag_verbose, DiffInfo *diffinfo)
 {
  static char     fname [256];
  struct TypPatch patch;
  IMDB_Buffer    *list_buffer = NULL;
  IMDB_Buffer    *diff_buffer = NULL;
  IMDB_Buffer    *out_buffer  = NULL;
  char           *p_list_line;
  char           *p_diff_line;
  LONG            list_line   = 1;
  LONG            out_line    = 0;
  LONG            l_add       = 0;
  LONG            l_delete    = 0;
  LONG            status      = STATUS_OK;
  LONG            progress    = 0;
  LONG            tprogress   = 0;
  LONG            i;
  LONG            len;

  strcpy (fname, listfile);
  StrChangeSuffix (fname, ".new");

  /* reset CRC */
  nCrc = 0xFFFFFFFFL;
  new_crc [0] = '\0';
  old_crc [0] = '\0';

  /* open diff-file */
  if (NULL == (diff_buffer = IMDBOpenBuffer (diffile, IMDBV_FILE_READ|IMDBV_FILE_GETSIZE, ADV_BUFFER_SIZE)))
   {
    diffinfo->status = STATUS_IO;
    return (RET_ERROR);
   }

  /* open old listfile */
  if (IMDBExistFile(listfile))
   {
    /* check if diffs match */
    if ((list_buffer = IMDBOpenBuffer (listfile, IMDBV_FILE_READ, ADV_BUFFER_SIZE))
      &&(0 == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
      &&(0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
     {
      if (0 != strcmp (p_diff_line+10, p_list_line))
       {
        status = STATUS_VER;
        if (flag_verbose)
         printf ("Error: Unsuitable Diff-File\n");
       }
      IMDBPositionBuffer(list_buffer,0);
     }
    else
     status = STATUS_IO;
   }
  else
   {
    /* Listfile does not exist. Maybe it's new? */
    if (STATUS_OK == diffinfo->status) /* d.h. wenn option NOCHECK benutzt wird */
     {
      IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE);
      if (0 == strcmp(p_diff_line, "Apply on: ---"))
       {
        status = STATUS_NEW;
        if (flag_verbose)
        printf ("New Listfile\n");
       }
      else
       {
        status = STATUS_VER;
        if (flag_verbose)
         printf ("Error: Missing Listfile\n");
       }
     }
    else
     IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE); /* skip header */

    if (STATUS_NEW != diffinfo->status)
     {
      if (STATUS_OK == diffinfo->status)
       diffinfo->status = STATUS_IO;
      IMDBCloseBuffer (diff_buffer);
      return (RET_WARNING);
     }
    if (flag_verbose)
     {
      printf ("\b\b\b\b\b\b(new file) (000%%)");
      fflush (stdout);
     }
   }

  /* open new listfile */
  if (NULL == (out_buffer =  IMDBOpenBuffer (fname, IMDBV_FILE_WRITE, ADV_BUFFER_SIZE)))
   {
    IMDBCloseBuffer (diff_buffer);
    IMDBCloseBuffer (list_buffer);
    if (diffinfo)
     diffinfo->status = STATUS_IO;
    return (RET_ERROR);
   }

  /*** now patch the file ***/
  /* get next line of diff */
  while ((STATUS_OK == status) && (!IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE)))
   {
    /* show progress */
    if ((flag_verbose) && (progress != (tprogress = (diff_buffer->filepos*100/diff_buffer->filesize))))
     {
      progress = tprogress;
      printf ("\b\b\b\b\b\b(%03i%%)", progress);
      fflush (stdout);
     }

    /* parse command */
    GetPatch(&patch, p_diff_line);

    /* Es gibt hier einen Sonderfall, naemlich eine Einfuegung gleich am Anfang */
    if ((0 == patch.i_start) && (patch.cmd == 'a'))
     {
      /* Und jetzt einfuegen */
      for (i = patch.o_start; i <= patch.o_end; i++)
       {
        if ((flag_verbose) && (progress != (tprogress = (diff_buffer->filepos*100/diff_buffer->filesize))))
         {
          progress = tprogress;
          printf ("\b\b\b\b\b\b(%03i%%)", progress);
          fflush (stdout);
         }
        if (IMDBE_FILE_READ == IMDBReadBufferLine (diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
         {
          status = STATUS_IO;
          break;
         }
        len = strlen(p_diff_line);
        if ((IMDBWriteBuffer(out_buffer, p_diff_line, len))
          ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
         {
          status = STATUS_IO;
          break;
         }
        /* calc CRC */
        if (out_line)
         calc_crc (p_diff_line, &nCrc);
        else
         if (0 == strncmp (p_diff_line, "CRC: ", strlen("CRC: ")))
          strncpy (old_crc, p_diff_line, 15);
        out_line++;
        l_add++;
       }
      continue;
     }

    /* Sonderfall, dass es gar keine alte Liste gab */
    if (NULL == list_buffer)
     break;

    /* Error ??? */
    if ((patch.i_start < list_line) || (patch.i_start > patch.i_end)
      ||(patch.o_start < out_line)  || (patch.o_start > patch.o_end))
     {
      if (flag_verbose)
       printf ("\b\b\b\b\b\b - Confusion: cmd:%i is:%i ie:%i os:%s oe:%s\n", patch.cmd, patch.i_start, patch.i_end, patch.o_start, patch.o_end);
      status = STATUS_SYN;
      break;
     }

    /* Alles klar, wir suchen jetzt diese Zeile(n) im listfile */
    while (patch.i_start != list_line)
     {
      if (IMDBE_FILE_READ == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE))
       {
        status = STATUS_IO;
        break;
       }
      if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
        ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
       {
        status = STATUS_IO;
        break;
       }
      /* calc CRC */
      if (out_line)
       calc_crc (p_list_line, &nCrc);
      else
       if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
        strncpy (old_crc, p_list_line, 15);
      list_line++;
      out_line++;
     }

    /* Error */
    if (patch.i_start != list_line)
     {
      if (flag_verbose)
       printf ("\b\b\b\b\b\b - Error before reaching line number: %i\n", patch.i_start);
      status = STATUS_SYN;
      break;
     }
    else
     {
      /* O.K. jetzt sind wir an der richtigen Stelle */
      switch (patch.cmd)
       {
        case 'd': /* delete line */
         {
          for (i = patch.i_start; i <= patch.i_end; i++)
           {
            if (IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            list_line++;
            l_delete++;
           }
          break;
         }
        case 'a': /* add line */
         {
          /* Beim Anfuegen brauchen wir die Zeile spaeter */
          if(IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE))
           {
            status = STATUS_IO;
            break;
           }

          if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
            ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
           {
            status = STATUS_IO;
            break;
           }
          /* calc CRC */
          if (out_line)
           calc_crc (p_list_line, &nCrc);
          else
           if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
            strncpy (old_crc, p_list_line, 15);
          list_line++;
          out_line++;

          /* Und jetzt einfuegen */
          for (i = patch.o_start; i <= patch.o_end; i++)
           {
            if (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            len = strlen(p_diff_line);
            if ((IMDBWriteBuffer(out_buffer, p_diff_line, len))
              ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
             {
              status = STATUS_IO;
              break;
             }
            /* calc CRC */
            if (out_line)
             calc_crc (p_diff_line, &nCrc);
            else
             if (0 == strncmp (p_diff_line, "CRC: ", strlen("CRC: ")))
              strncpy (old_crc, p_diff_line, 15);
            out_line++;
            l_add++;
           }
          break;
         }
        case 'c': /* change line */
         {
          /* First delete those lines that are discarded */
          for (i = patch.i_start; i <= patch.i_end; i++)
           {
            if (IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            list_line++;
            l_delete++;
           }

          /* now add new lines */
          for (i = patch.o_start; i <= patch.o_end; i++)
           {
            if (IMDBReadBufferLine(diff_buffer, &p_diff_line, ADV_MAX_LINESIZE))
             {
              status = STATUS_IO;
              break;
             }
            len = strlen(p_diff_line);
            if ((IMDBWriteBuffer(out_buffer, p_diff_line, len))
              ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
             {
              status = STATUS_IO;
              break;
             }
            /* calc CRC */
            if (out_line)
             calc_crc (p_diff_line, &nCrc);
            else
             if (0 == strncmp (p_diff_line, "CRC: ", strlen("CRC: ")))
              strncpy (old_crc, p_diff_line, 15);
            out_line++;
            l_add++;
           }
          break;
         }
        default:
         {
          if (flag_verbose)
           printf ("\b\b\b\b\b\b - Unknown command %i\n", patch.cmd);
          status = STATUS_SYN;
          break;
         }
       }
     }
   }

  /* Das restliche listfile kopieren falls nicht neue Liste*/
  if (list_buffer)
   while ((STATUS_OK == status) && (!IMDBReadBufferLine(list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
    {
     if ((IMDBWriteBuffer(out_buffer, p_list_line, strlen(p_list_line)))
       ||(IMDBWriteBuffer(out_buffer, "\n", 1)))
      {
       status = STATUS_IO;
       break;
      }
     /* calc CRC */
     if (out_line)
      calc_crc (p_list_line, &nCrc);
     else
      if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
       strncpy (old_crc, p_list_line, 15);
     list_line++;
     out_line++;
    }

  /* Close Buffer */
  IMDBCloseBuffer (out_buffer);
  IMDBCloseBuffer (diff_buffer);
  IMDBCloseBuffer (list_buffer);

  /* compare CRC */
  sprintf(new_crc, "CRC: 0x%08X", nCrc);
  if (((STATUS_OK == status) || (STATUS_NEW == status)) &&
      ((0 == strcmp (old_crc, new_crc)) || (ad_cmds.f_force == TRUE)))
   {
    if (flag_verbose)
     printf ("\b\b\b\b\b\b- CRC-Checksum O.K.\n");

    /* *.old file loeschen falls noch nicht geschehen und files umbenennen */
    strcpy (fname, listfile);
    StrChangeSuffix (fname, ".old");
    remove (fname);

    /* listfile umbenennen, bzw loeschen */
    if (flag_keep)
     rename (listfile, fname);
    else
     remove (listfile);

    /* neues Listfile umbenennen */
    strcpy (fname, listfile);
    StrChangeSuffix (fname, ".new");
    rename (fname, listfile);

#ifdef SYS_AMIGA
    /* Protection Bits richtig setzen */
    SetProtection (listfile, FIBF_EXECUTE);
#endif

    /* diffile loeschen */
    if (!flag_keep)
     remove (diffile);
   }
  else
   {
    if ((STATUS_OK == status) || (STATUS_NEW == status))
     { /* something went wrong! No changes */
      if (flag_verbose)
       printf ("\b\b\b\b\b\b- CRC-Checksum Error\n");
      status = STATUS_CRC;
     }
    remove (fname);
    l_add = 0;
    l_delete = 0;
   }

  /* remember DiffInfo */
  if (diffinfo)
   {
    if (STATUS_NEW != diffinfo->status)
     diffinfo->status = status;
    diffinfo->add = l_add;
    diffinfo->delete = l_delete;
   }

  if (status)
   return (RET_WARNING);
  else
   return (RET_OK);
 }


/******************************************************************************
 *  Main - Procedure
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:   main
 *
 * Parameters:  nb_args, filename
 *
 * Returns:
 *
 * Comments:
 *-----------------------------------------------------------------------------
 */

int main(int argc, char *argv[])
 {
  DiffInfo    *diffinfo = NULL;
  DiffInfo    *t_diffinfo = NULL;
  DiffInfo    *a_diffinfo = NULL;

  int  ret_val        = RET_OK;
  int  ret            = RET_OK;

#ifdef SYS_AMIGA
  printf (VERSION" written by André Bernhardt; © 1996-2001 IMDb Ltd.\n");
#else
  printf (VERSION" written by André Bernhardt; (c) 1996-2001 IMDb Ltd.\n");
#endif

  /* Parse command line parameters */
#ifdef SYS_AMIGA
  {
   static const char Template[]    = "LISTDIR/A,DIFFDIR/A,CHECKCRC/S,FORCE/S,KEEP/S,NOSTATS/S,QUIET/S,LOGFILE/K";
   AD_Commands       cmdlineparams = {NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, NULL};
   struct RDArgs    *rda;
   LONG              len;
   char              c;

   rda = ReadArgs((char*) Template, (LONG *) &cmdlineparams, NULL);

   /* Get values */
   if (cmdlineparams.p_listdir)
    if (ad_cmds.p_listdir = IMDBAllocMemory (2+(len = strlen(cmdlineparams.p_listdir))))
     {
      strcpy(ad_cmds.p_listdir, cmdlineparams.p_listdir);
      c = ad_cmds.p_listdir[len-1];
      if ((c != ':') && (c != '/'))
       strcat (ad_cmds.p_listdir,"/");
     }

   if (cmdlineparams.p_diffdir)
    if (ad_cmds.p_diffdir = IMDBAllocMemory (2+(len = strlen(cmdlineparams.p_diffdir))))
     {
      strcpy(ad_cmds.p_diffdir, cmdlineparams.p_diffdir);
      c = ad_cmds.p_diffdir[len-1];
      if ((c != ':') && (c != '/'))
       strcat (ad_cmds.p_diffdir,"/");
     }

   ad_cmds.f_checkcrc = cmdlineparams.f_checkcrc;
   ad_cmds.f_force    = cmdlineparams.f_force   ;
   ad_cmds.f_keep     = cmdlineparams.f_keep    ;
   ad_cmds.f_nostats  = cmdlineparams.f_nostats ;
   ad_cmds.f_quiet    = cmdlineparams.f_quiet   ;

   if (cmdlineparams.p_logfile)
    if (ad_cmds.p_logfile = IMDBAllocMemory (1+ strlen(cmdlineparams.p_logfile)))
     strcpy(ad_cmds.p_logfile, cmdlineparams.p_logfile);

   /* Free ReadArgs parameters */
   if (NULL == rda)
    {
     printf ("Template: %s\n",Template);
     exit (RET_ERROR);
    }
   else
    FreeArgs(rda);
  }
#endif

#ifdef SYS_UNIX
  {
   static const char Template[] = "usage: ApplyDiffs <listpath> <diffpath> [-checkcrc][-force][-keep][-nostats][-quiet][-logfile <filename>]";
   LONG              i;

   if (argc <3)
    {
     puts (Template);
     exit (10);
    }

   /* list-path */
   if (ad_cmds.p_listdir = IMDBAllocMemory (2 + strlen(argv[1])))
    {
     strcpy(ad_cmds.p_listdir, argv[1]);
     if ('/' != ad_cmds.p_listdir[strlen(ad_cmds.p_listdir)-1])
      strcat (ad_cmds.p_listdir,"/");
    }

   /* diff-path */
   if (ad_cmds.p_diffdir = IMDBAllocMemory (2 + strlen(argv[2])))
    {
     strcpy(ad_cmds.p_diffdir, argv[2]);
     if ('/' != ad_cmds.p_diffdir[strlen(ad_cmds.p_diffdir)-1])
      strcat (ad_cmds.p_diffdir,"/");
    }


   /* Parse Command Line Parameters */
   for (i=3; i < argc; i++)
    {
     if (!strcmp(argv[i], "-checkcrc"))
      ad_cmds.f_checkcrc = TRUE;
     else
     if (!strcmp(argv[i], "-force"))
      ad_cmds.f_force    = TRUE;
     else
     if (!strcmp(argv[i], "-keep"))
      ad_cmds.f_keep     = TRUE;
     else
     if (!strcmp(argv[i], "-nostats"))
      ad_cmds.f_nostats  = TRUE;
     else
     if (!strcmp(argv[i], "-quiet"))
      ad_cmds.f_quiet    = TRUE;
     else
     if (!strcmp(argv[i], "-logfile"))
      {
       if (ad_cmds.p_logfile = IMDBAllocMemory (2 + strlen(argv[++i])))
        strcpy(ad_cmds.p_logfile, argv[i]);
      }
     else
      {
       puts (Template);
       exit (10);
      }
    }

   if ((NULL == ad_cmds.p_listdir) || (NULL == ad_cmds.p_diffdir))
    {
     puts (Template);
     exit (10);
    }
  }
#endif

  /* Check Syntax */
  if (0 == strcmp(ad_cmds.p_listdir, ad_cmds.p_diffdir))
   {
    printf("Error: Lists- and Diffs-Directory must be different!\n");
    exit (RET_ERROR);
   }

#ifdef IMDB_DEBUG
  printf ("Listdir: %s\nDiffdir: %s\ncheckcrc %i\nforce %i\nkeep %i\nnostats %i\nquiet %i\n", ad_cmds.p_listdir, ad_cmds.p_diffdir, ad_cmds.f_checkcrc, ad_cmds.f_force, ad_cmds.f_keep, ad_cmds.f_nostats, ad_cmds.f_quiet);
  if (ad_cmds.p_logfile)
   printf ("Logfile: %s \n", ad_cmds.p_logfile);
  else
   printf ("Logfile: -none-\n");
#endif

  /* Create CRC-Table */
  if (!(pCrcTab = InitCRC()))
   {
    printf("Can't Create CRC-Table!\n");
    exit (RET_ERROR);
   }

  /* get all filenames in the diff-files-directory */
#ifdef SYS_AMIGA
  {
   BPTR                  lock;
   struct FileInfoBlock *fib;

   if (fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC))
    {
     if (lock = Lock(ad_cmds.p_diffdir,SHARED_LOCK))
      {
       if ((Examine(lock,fib))&&(fib->fib_DirEntryType > 0))
        {
         while ((ExNext(lock,fib)) && (ret_val != RET_ERROR))
          {
           if ((fib->fib_DirEntryType <= 0) && (strlen(fib->fib_FileName) > 5)
            && ((0 == strnicmp(&fib->fib_FileName[strlen(fib->fib_FileName)-5], ".list", 5))
              ||(0 == strnicmp(&fib->fib_FileName[strlen(fib->fib_FileName)-5], ".diff", 5))))
            {
             /* Create and initialize DiffInfo */
             if (a_diffinfo = IMDBAllocMemory (sizeof (DiffInfo)))
              {
               a_diffinfo->next = NULL;
               strcpy (a_diffinfo->fname_list, fib->fib_FileName);
               strcpy (a_diffinfo->fname_diff, fib->fib_FileName);
               if (0 == strnicmp(&a_diffinfo->fname_list[strlen(a_diffinfo->fname_list)-5], ".list", 5))
                a_diffinfo->type = DIFF_TYPE_ORIGINAL;
               else
                {
                 a_diffinfo->type = DIFF_TYPE_STRIPPED;
                 a_diffinfo->fname_list[strlen(a_diffinfo->fname_list)-4] = '\0';
                 strcat(a_diffinfo->fname_list, "list");
                }
               a_diffinfo->status = STATUS_OK;
               a_diffinfo->add = 0;
               a_diffinfo->delete = 0;

              }
             else
              {
               printf("Can't allocate memory for DiffInfo\n");
               ret_val = RET_ERROR;
              }
             if (diffinfo)
              {
               t_diffinfo->next = a_diffinfo;
               if (a_diffinfo)
                t_diffinfo = a_diffinfo;
              }
             else
              diffinfo = t_diffinfo = a_diffinfo;
            }
          }
        }
       else
        {
         printf("%s is no directory!\n", ad_cmds.p_diffdir);
         ret_val = RET_ERROR;
        }
       UnLock(lock);
      }
     else
      {
       printf("Can't get shared lock on %s\n", ad_cmds.p_diffdir);
       ret_val = RET_ERROR;
      }
     FreeMem(fib,sizeof(struct FileInfoBlock));
    }
   else
    {
     printf("Can't allocate memory for FileInfoBlock\n");
     ret_val = RET_ERROR;
    }
  }
#endif

#ifdef SYS_UNIX
  {
   struct stat stbuf;

   if (-1 == stat(ad_cmds.p_diffdir, &stbuf))
    {
     printf("Can't access %s\n", ad_cmds.p_diffdir);
     ret_val = RET_ERROR;
    }
   else
   if (S_IFDIR != (stbuf.st_mode & S_IFMT))
    {
     printf("%s is no directory.\n", ad_cmds.p_diffdir);
     ret_val = RET_ERROR;
    }
   else
    {
#ifdef NEXT
     struct direct *dp;
#else
     struct dirent *dp;
#endif /* NEXT */
     DIR *dfd;

     if (NULL == (dfd = opendir(ad_cmds.p_diffdir)))
      {
       printf("Can't open %s\n", ad_cmds.p_diffdir);
       ret_val = RET_ERROR;
      }
     else
      {
       while (dp = readdir(dfd))
        {
         if ((strlen (dp->d_name) <=8 ) || ((0 != strncmp(&dp->d_name[strlen(dp->d_name)-5], ".list", 5))
                                          &&(0 != strncmp(&dp->d_name[strlen(dp->d_name)-5], ".diff", 5))))
          continue;

         /* Create and initialize DiffInfo */
         if (a_diffinfo = IMDBAllocMemory (sizeof (DiffInfo)))
          {
           a_diffinfo->next = NULL;
           strcpy (a_diffinfo->fname_list, dp->d_name);
           strcpy (a_diffinfo->fname_diff, dp->d_name);
#ifdef IMDB_DEBUG
printf ("-> %s\n", a_diffinfo->fname_list);
#endif
           if (0 == strncmp(&a_diffinfo->fname_list[strlen(a_diffinfo->fname_list)-5], ".list", 5))
            a_diffinfo->type = DIFF_TYPE_ORIGINAL;
           else
            {
             a_diffinfo->type = DIFF_TYPE_STRIPPED;
             a_diffinfo->fname_list[strlen(a_diffinfo->fname_list)-4] = '\0';
             strcat(a_diffinfo->fname_list, "list");
            }
           a_diffinfo->status = STATUS_OK;
           a_diffinfo->add = 0;
           a_diffinfo->delete = 0;
          }
         else
          {
           printf("Can't allocate memory for DiffInfo\n");
           ret_val = RET_ERROR;
          }
         if (diffinfo)
          {
           t_diffinfo->next = a_diffinfo;
           if (a_diffinfo)
            t_diffinfo = a_diffinfo;
          }
         else
          diffinfo = t_diffinfo = a_diffinfo;
        }
       closedir(dfd);
      }
    }
  }
#endif

  /* sort filenames (= sort Diffinfo) */
  {
   char t_fname[256];
   LONG t_type;

   t_diffinfo = diffinfo;
   while (t_diffinfo)
    {
     a_diffinfo = t_diffinfo->next;
     while (a_diffinfo)
      {
       if (strcmp(t_diffinfo->fname_list, a_diffinfo->fname_list) > 0)
        {
         strcpy(t_fname               , t_diffinfo->fname_list);
         strcpy(t_diffinfo->fname_list, a_diffinfo->fname_list);
         strcpy(a_diffinfo->fname_list, t_fname);
         strcpy(t_fname               , t_diffinfo->fname_diff);
         strcpy(t_diffinfo->fname_diff, a_diffinfo->fname_diff);
         strcpy(a_diffinfo->fname_diff, t_fname);
/* 2.3 Bugfix: bei gemischten original/gestrippten Diffs trat Fehler auf */
         t_type           = t_diffinfo->type;
         t_diffinfo->type = a_diffinfo->type;
         a_diffinfo->type = t_type;
        }
       a_diffinfo = a_diffinfo->next;
      }
     t_diffinfo = t_diffinfo->next;
    }
  }

  printf ("\n");

  /* check if the correct diffs are applied and check CRC-sum of the listfiles if desired */
  /* Sonderfall: Neues File wird eingefuehrt */
  /* 2.3 */ /* Erst wird getestet ob das Listfile gepackt ist. In diesem Fall wird erst mal */
  /* auf einen Test verzichtet */
  if (RET_OK == ret_val)
   {
    t_diffinfo = diffinfo;

    while ((t_diffinfo) && (RET_ERROR != ret_val))
     {
      char listname[256];
      char diffname[256];

      strcpy (listname, ad_cmds.p_listdir);
      strncat(listname, t_diffinfo->fname_list, 255-strlen(listname));
      strcpy (diffname, ad_cmds.p_diffdir);
      strncat(diffname, t_diffinfo->fname_diff, 255-strlen(diffname));

#ifdef IMDB_GZIP
      /* 2.3 File gzipped? */
      if (FALSE == IMDBExistFile(listname))
       {
        char t_listname[256];
        strcpy (t_listname, listname);
        strcat (t_listname, IMDBV_FILE_PACKER_EXT);
        if (IMDBExistFile(t_listname))
         {
          if (!ad_cmds.f_quiet)
           printf ("Skipping File %s - File is Compressed\n", t_diffinfo->fname_list);
          t_diffinfo->status = STATUS_UNKNOWN;
          t_diffinfo = t_diffinfo->next;
          continue; /* try next file */
         }
       }
#endif

      /* Test if listfile and diffile match */
      if (!ad_cmds.f_force)
       {
        if (!ad_cmds.f_quiet)
         {
          printf ("Test File %s - ", t_diffinfo->fname_list);
          fflush (stdout);
         }
        if (ret = checkfile_match (listname, diffname, !ad_cmds.f_quiet, t_diffinfo))
         ret_val = ret;
       }

      /* Check CRC */
      if (ad_cmds.f_checkcrc)
       {
        if (!ad_cmds.f_quiet)
         {
          printf ("Check CRC of File %s (000%%)", t_diffinfo->fname_list);
          fflush (stdout);
         }

        if (ret = checkfile_crc (listname, !ad_cmds.f_quiet))
         ret_val = ret;
       }

      t_diffinfo = t_diffinfo->next;
     }
    if (!ad_cmds.f_quiet)
     printf ("\n");
   }

  /* check if anything unusual happened */
  t_diffinfo = diffinfo;
  if (RET_OK == ret_val)
   while (t_diffinfo)
    {
     if ((STATUS_OK != t_diffinfo->status) && (STATUS_NEW != t_diffinfo->status) && (STATUS_UNKNOWN != t_diffinfo->status))
      ret_val = RET_WARNING;
     t_diffinfo = t_diffinfo->next;
    }


  /* Now apply all diffs */
  /* Sonderfall: Neues File wird eingefuehrt */
  if (RET_OK == ret_val)
   {
    t_diffinfo = diffinfo;

    while ((t_diffinfo) && (RET_ERROR != ret_val))
     {
      char listname[256];
      char diffname[256];
#ifdef IMDB_GZIP
      BOOL  f_gzip = FALSE;
      t_diffinfo->status = STATUS_OK;
#endif

      strcpy (listname, ad_cmds.p_listdir);
      strncat(listname, t_diffinfo->fname_list, 255-strlen(listname));
      strcpy (diffname, ad_cmds.p_diffdir);
      strncat(diffname, t_diffinfo->fname_diff, 255-strlen(diffname));

#ifdef IMDB_GZIP
/* 2.3 unpack file if necessary */
      if (FALSE == IMDBExistFile(listname))
       {
        char t_listname[256];
        strcpy (t_listname, listname);
        strcat (t_listname, IMDBV_FILE_PACKER_EXT);
        if (IMDBExistFile(t_listname))
         {
          char command [255];
          f_gzip = TRUE;
          if (!ad_cmds.f_quiet)
           {
            printf ("Uncompressing File %s - ", t_diffinfo->fname_list);
            fflush (stdout);
           }
          sprintf (command, IMDBV_FILE_PACKER_NAME " " IMDBV_FILE_PACKER_UNPACK " %s", t_listname);
          if (system(command))
           {/* Entpacken hat leider nicht geklappt */
            t_diffinfo->status = STATUS_IO;
            ret_val = RET_WARNING;
            printf ("failed!\n");
            t_diffinfo = t_diffinfo->next;
            continue;
           }
          else
            printf ("OK\n");
         }
       }
#endif

      if (!ad_cmds.f_quiet)
       {
        printf ("Apply diffs on file %s (000%%)", t_diffinfo->fname_list);
        fflush (stdout);
       }

      switch (t_diffinfo->type)
       {
        case DIFF_TYPE_ORIGINAL:
         if (ret = patchfile_original (listname, diffname, ad_cmds.f_keep, !ad_cmds.f_quiet, t_diffinfo))
          ret_val = ret;
         break;
        case DIFF_TYPE_STRIPPED:
         if (ret = patchfile_stripped (listname, diffname, ad_cmds.f_keep, !ad_cmds.f_quiet, t_diffinfo))
          ret_val = ret;
         break;
        default:
         puts ("Serious Error!");
       }

#ifdef IMDB_GZIP
/* 2.3 pack file if it was packed before */
      if (f_gzip)
       {
        char command [255];
        if (!ad_cmds.f_quiet)
         {
          printf ("Compressing File %s - ", t_diffinfo->fname_list);
          fflush (stdout);
         }
        sprintf (command, IMDBV_FILE_PACKER_NAME " " IMDBV_FILE_PACKER_PACK " %s", listname);
        if (system(command))
         {/* Packen hat leider nicht geklappt */
          t_diffinfo->status = STATUS_IO;
          ret_val = RET_WARNING;
          printf ("failed!\n");
         }
        else
         printf ("OK\n");
       }
#endif

      if (!ad_cmds.f_quiet)
       {
        printf ("Status: ");
        switch (t_diffinfo->status)
         {
          case STATUS_OK:
           printf ("OK\n");
           break;
          case STATUS_CRC:
           printf ("CRC-Error\n");
           break;
          case STATUS_IO:
           printf ("IO-Error\n");
           break;
          case STATUS_VER:
           printf ("Wrong Diffs\n");
           break;
          case STATUS_SYN:
           printf ("Syntax Error\n");
           break;
          case STATUS_NEW:
           printf ("New File\n");
           break;
          default:
           printf ("%i", diffinfo->status);
           break;
         }
        printf ("\n");
       }

      t_diffinfo = t_diffinfo->next;
     }
    if (!ad_cmds.f_quiet)
     printf ("\n");
   }


  /* Free CRC-Tab */
  IMDBFreeMemory(pCrcTab);

  /* Print statistic */
  if (!ad_cmds.f_nostats)
   {
    FILE   *p_file = NULL;
    time_t timeval;

    if (ad_cmds.p_logfile)
     p_file = fopen(ad_cmds.p_logfile, "ab");

    if ((timeval = time (NULL)) != -1)
     {
      printf ("ApplyDiff Statistics - %s", ctime (&timeval));
      if (p_file) fprintf (p_file,"\nApplyDiff Statistics - %s", ctime (&timeval));
     }
    else
     {
      printf ("ApplyDiff Statistics\n");
      if (p_file) fprintf (p_file,"\nApplyDiff Statistics\n");
     }
    printf ("\n            Lines    Lines\n");
    if (p_file) fprintf (p_file,"\n            Lines    Lines\n");
    printf ("Status      Removed  Added Listfile\n");
    if (p_file) fprintf (p_file,"Status      Removed  Added Listfile\n");
    printf ("------------------------------------------------------------\n");
    if (p_file) fprintf (p_file,"------------------------------------------------------------\n");
    while (diffinfo)
     {
      switch (diffinfo->status)
       {
        case STATUS_OK:
         printf ("OK          ");
         if (p_file) fprintf (p_file,"OK          ");
         break;
        case STATUS_UNKNOWN:
         printf ("N/A         ");
         if (p_file) fprintf (p_file,"N/A         ");
         break;
        case STATUS_CRC:
         printf ("CRC-Error   ");
         if (p_file) fprintf (p_file,"CRC-Error   ");
         break;
        case STATUS_IO:
         printf ("IO-Error    ");
         if (p_file) fprintf (p_file,"IO-Error    ");
         break;
        case STATUS_VER:
         printf ("Wrong Diffs ");
         if (p_file) fprintf (p_file,"Wrong Diffs ");
         break;
        case STATUS_SYN:
         printf ("Syntax Error");
         if (p_file) fprintf (p_file,"Syntax Error");
         break;
        case STATUS_NEW:
         printf ("New File    ");
         if (p_file) fprintf (p_file,"New File    ");
         break;
        default:
         printf ("%i", diffinfo->status);
         if (p_file) fprintf (p_file,"%i", diffinfo->status);
         break;
       }
      printf (" %6li %6li %s\n", diffinfo->delete, diffinfo->add, diffinfo->fname_list);
      if (p_file) fprintf (p_file," %6li %6li %s\n", diffinfo->delete, diffinfo->add, diffinfo->fname_list);
      t_diffinfo = diffinfo->next;
      IMDBFreeMemory (diffinfo);
      diffinfo = t_diffinfo;
     }

    if (p_file)
     fclose (p_file);
   }
  else
   {
    while (diffinfo)
     {
      t_diffinfo = diffinfo->next;
      IMDBFreeMemory (diffinfo);
      diffinfo = t_diffinfo;
     }
   }

  /* Free memory */
  if (ad_cmds.p_listdir) IMDBFreeMemory(ad_cmds.p_listdir);
  if (ad_cmds.p_diffdir) IMDBFreeMemory(ad_cmds.p_diffdir);
  if (ad_cmds.p_logfile) IMDBFreeMemory(ad_cmds.p_logfile);

  if (RET_OK != ret_val)
   printf ("\nWARNING: ApplyDiffs could not successfully apply all diffs.\n");

  exit (ret_val);
 }
